//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Computation of quantal response equilibrium correspondence for
// normal form games.
//
// This file is part of Gambit
// Copyright (c) 2002, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#include <unistd.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <libgambit/libgambit.h>
#include <libgambit/sqmatrix.h>

//=========================================================================
//             QRE Correspondence Computation via Homotopy
//=========================================================================

//
// The following code implements a homotopy approach to computing
// the logistic QRE correspondence.  This implementation is a basic
// Euler-Newton approach with adaptive step size, based upon the
// ideas and codes presented in Allgower and Georg's
// _Numerical Continuation Methods_.
//

inline double sqr(double x) { return x*x; }

static void Givens(Gambit::Matrix<double> &b, Gambit::Matrix<double> &q,
		   double &c1, double &c2, int l1, int l2, int l3)
{
  if (fabs(c1) + fabs(c2) == 0.0) {
    return;
  }

  double sn;
  if (fabs(c2) >= fabs(c1)) {
    sn = sqrt(1.0 + sqr(c1/c2)) * fabs(c2);
  }
  else {
    sn = sqrt(1.0 + sqr(c2/c1)) * fabs(c1);
  }
  double s1 = c1/sn;
  double s2 = c2/sn;

  for (int k = 1; k <= q.NumColumns(); k++) {
    double sv1 = q(l1, k);
    double sv2 = q(l2, k);
    q(l1, k) = s1 * sv1 + s2 * sv2;
    q(l2, k) = -s2 * sv1 + s1 * sv2;
  }

  for (int k = l3; k <= b.NumColumns(); k++) {
    double sv1 = b(l1, k);
    double sv2 = b(l2, k);
    b(l1, k) = s1 * sv1 + s2 * sv2;
    b(l2, k) = -s2 * sv1 + s1 * sv2;
  }

  c1 = sn;
  c2 = 0.0;
}

static void QRDecomp(Gambit::Matrix<double> &b, Gambit::Matrix<double> &q)
{
  q.MakeIdent();
  for (int m = 1; m <= b.NumColumns(); m++) {
    for (int k = m + 1; k <= b.NumRows(); k++) {
      Givens(b, q, b(m, m), b(k, m), m, k, m + 1);
    }
  }
}

static void NewtonStep(Gambit::Matrix<double> &q, Gambit::Matrix<double> &b,
		       gbtVector<double> &u, gbtVector<double> &y,
		       double &d)
{
  for (int k = 1; k <= b.NumColumns(); k++) {
    for (int l = 1; l <= k - 1; l++) {
      y[k] -= b(l, k) * y[l];
    }
    y[k] /= b(k, k);
  }

  d = 0.0;
  for (int k = 1; k <= b.NumRows(); k++) {
    double s = 0.0;
    for (int l = 1; l <= b.NumColumns(); l++) {
      s += q(l, k) * y[l];
    }
    u[k] -= s;
    d += s * s;
  }
  d = sqrt(d);
}


void QreLHS(const gbtNfgSupport &p_support, 
	    const gbtVector<double> &p_point,
	    const gbtArray<bool> &p_isLog,
	    gbtVector<double> &p_lhs)
{
  Gambit::MixedStrategyProfile<double> profile(p_support), logprofile(p_support);
  for (int i = 1; i <= profile.Length(); i++) {
    if (p_isLog[i]) {
      profile[i] = exp(p_point[i]);
      logprofile[i] = p_point[i];
    }
    else {
      profile[i] = p_point[i];
      logprofile[i] = log(p_point[i]);
    }
  }
  double lambda = p_point[p_point.Length()];
  
  p_lhs = 0.0;

  int rowno = 0;
  for (int pl = 1; pl <= p_support.GetGame()->NumPlayers(); pl++) {
    Gambit::GamePlayer player = p_support.GetGame()->GetPlayer(pl);
    for (int st = 1; st <= player->NumStrategies(); st++) {
      rowno++;
      if (st == 1) {
	// should be st==lead: sum-to-one equation
	p_lhs[rowno] = -1.0;
	for (int j = 1; j <= player->NumStrategies(); j++) {
	  p_lhs[rowno] += profile(pl, j);
	}
      }
      else {
	p_lhs[rowno] = (logprofile(pl, st) - logprofile(pl, 1) -
			lambda * (profile.GetPayoff(pl, pl, st) -
				  profile.GetPayoff(pl, pl, 1)));

      }
    }
  }
}

void QreJacobian(const gbtNfgSupport &p_support,
		 const gbtVector<double> &p_point,
		 const gbtArray<bool> &p_isLog,
		 Gambit::Matrix<double> &p_matrix)
{
  Gambit::MixedStrategyProfile<double> profile(p_support), logprofile(p_support);
  for (int i = 1; i <= profile.Length(); i++) {
    if (p_isLog[i]) {
      profile[i] = exp(p_point[i]);
      logprofile[i] = p_point[i];
    }
    else {
      profile[i] = p_point[i];
      logprofile[i] = log(p_point[i]);
    }
  }
  double lambda = p_point[p_point.Length()];

  p_matrix = 0.0;

  int rowno = 0;
  for (int i = 1; i <= p_support.GetGame()->NumPlayers(); i++) {
    Gambit::GamePlayer player = p_support.GetGame()->GetPlayer(i);

    for (int j = 1; j <= player->NumStrategies(); j++) {
      rowno++;
      if (j == 1) {
	// Should be j == lead: sum-to-one equation
	
	int colno = 0;
	for (int ell = 1; ell <= p_support.GetGame()->NumPlayers(); ell++) {
	  Gambit::GamePlayer player2 = p_support.GetGame()->GetPlayer(ell);

	  for (int m = 1; m <= player2->NumStrategies(); m++) {
	    colno++;
	    
	    if (i == ell) {
	      p_matrix(colno, rowno) = (p_isLog[colno]) ? profile(ell, m) : 1.0;
	    }
	    else {
	      p_matrix(colno, rowno) = 0.0;
	    }
	  }
	}
	
	// Derivative wrt lambda is zero
	p_matrix(p_matrix.NumRows(), rowno) = 0.0;
      }
      else {
	// This is a ratio equation

	int colno = 0;
	for (int ell = 1; ell <= p_support.GetGame()->NumPlayers(); ell++) {
	  Gambit::GamePlayer player2 = p_support.GetGame()->GetPlayer(ell);

	  for (int m = 1; m <= player2->NumStrategies(); m++) {
	    colno++;

	    if (i == ell) {
	      if (m == 1) {
		// should be m==lead
		p_matrix(colno, rowno) = (p_isLog[colno]) ? -1.0 : -1.0/profile(ell, m);
	      }
	      else if (m == j) {
		p_matrix(colno, rowno) = (p_isLog[colno]) ? 1.0 : 1.0/profile(ell, m);
	      }
	      else {
		p_matrix(colno, rowno) = 0.0;
	      }
	    }
	    else {
	      // 1 == lead
	      if (p_isLog[colno]) {
		p_matrix(colno, rowno) =
		  -lambda * profile(ell, m) * logprofile(ell, m) *
		  (profile.GetPayoff(i, i, j, ell, m) -
		   profile.GetPayoff(i, i, 1, ell, m));
	      }
	      else {
		p_matrix(colno, rowno) =
		  -lambda * (profile.GetPayoff(i, i, j, ell, m) -
			     profile.GetPayoff(i, i, 1, ell, m));
	      }
	    }
	  }

	}
	
	// column wrt lambda
	// 1 == lead
	p_matrix(p_matrix.NumRows(), rowno) =
	  profile.GetPayoff(i, i, 1) - profile.GetPayoff(i, i, j);
      }
    }
  }
}

//
// For maximum likelihood estimation
//
bool g_maxLike = false;
gbtArray<double> g_obsProbs;

double LogLike(const gbtArray<double> &p_point)
{
  double ret = 0.0;
  
  for (int i = 1; i <= g_obsProbs.Length(); i++) {
    ret += g_obsProbs[i] * log(p_point[i]);
  }

  return ret;
}

double DiffLogLike(const gbtArray<double> &p_point,
		   const gbtArray<bool> &p_isLog,
		   const gbtArray<double> &p_tangent)
{
  double ret = 0.0;

  for (int i = 1; i <= g_obsProbs.Length(); i++) {
    if (p_isLog[i]) {
      ret += g_obsProbs[i] * p_tangent[i];
    }
    else {
      ret += g_obsProbs[i] * p_tangent[i] / p_point[i];
    }
  }

  return ret;
}


int g_numDecimals = 6;

void PrintProfile(std::ostream &p_stream,
		  const gbtNfgSupport &p_support, const gbtVector<double> &x,
		  const gbtArray<bool> &p_isLog,
		  bool p_terminal = false)
{
  p_stream.setf(std::ios::fixed);
  // By convention, we output lambda first
  if (!p_terminal) {
    p_stream << std::setprecision(g_numDecimals) << x[x.Length()];
  }
  else {
    p_stream << "NE";
  }
  p_stream.unsetf(std::ios::fixed);

  for (int i = 1; i <  x.Length(); i++) {
    if (p_isLog[i]) {
      p_stream << "," << std::setprecision(g_numDecimals) << exp(x[i]);
    }
    else {
      p_stream << "," << std::setprecision(g_numDecimals) << x[i];
    }
  }

  if (g_maxLike) {
    Gambit::MixedStrategyProfile<double> profile(p_support);
    for (int i = 1; i <= profile.Length(); i++) {
      if (p_isLog[i]) {
	profile[i] = exp(x[i]);
      }
      else {
	profile[i] = x[i];
      }
    }

    p_stream.setf(std::ios::fixed);
    p_stream << "," << std::setprecision(g_numDecimals) << LogLike(profile);
    p_stream.unsetf(std::ios::fixed);
  }

  p_stream << std::endl;
}

//
// TracePath does the real work of tracing a branch of the correspondence
//
// Strategy:
// This is the standard simple PC continuation method outlined in
// Allgower & Georg, _Numerical Continuation Methods_.
//
// The only novelty is the handling of the representation of the
// probabilities.  We deal with a correspondence in which probabilities
// often tend to zero exponentially in the lambda parameter.  However,
// negative probabilities make no sense, and cause the defining equations
// to be ill-defined.  This suggests that representing the probabilities
// as logarithms is indicated.  However, experience is that this does
// not work well when lambda is small, as the change in the probabilities
// as lambda changes in this region is closer to linear than exponential.
//
// The compromise is this: we represent probabilities below a certain
// cutoff (here set to .001) as logarithms, and probabilities above that
// as the actual probability.  Thus, we can take advantage of the
// exponential decay of small probabilities.
//


double g_maxDecel = 1.1;
double g_hStart = .03;
bool g_fullGraph = true;

static void TracePath(const Gambit::MixedStrategyProfile<double> &p_start,
		      double p_startLambda, double p_maxLambda, double p_omega)	
{
  const double c_tol = 1.0e-4;     // tolerance for corrector iteration
  const double c_maxDist = 0.4;    // maximal distance to curve
  const double c_maxContr = 0.6;   // maximal contraction rate in corrector
  const double c_eta = 0.1;        // perturbation to avoid cancellation
                                   // in calculating contraction rate
  double h = g_hStart;             // initial stepsize
  const double c_hmin = 1.0e-5;    // minimal stepsize

  bool newton = false;          // using Newton steplength (for MLE)
  bool restarting = false;      // flag for first restart step after MLE
  gbtArray<bool> isLog(p_start.Length());
  for (int i = 1; i <= p_start.Length(); i++) {
    isLog[i] = (p_start[i] < .001);
  }

  // When doing MLE finding, we push the data from the original path-following
  // here, and resume once we've found the local extremum.
  gbtVector<double> pushX(p_start.Length() + 1);
  double pushH = h;
  gbtArray<bool> pushLog(p_start.Length());

  gbtVector<double> x(p_start.Length() + 1), u(p_start.Length() + 1);
  for (int i = 1; i <= p_start.Length(); i++) {
    if (isLog[i]) {
      x[i] = log(p_start[i]);
    }
    else {
      x[i] = p_start[i];
    }
  }
  x[x.Length()] = p_startLambda;

  if (g_fullGraph) {
    PrintProfile(std::cout, p_start.GetSupport(), x, isLog);
  }

  gbtVector<double> t(p_start.Length() + 1);
  gbtVector<double> y(p_start.Length());

  Gambit::Matrix<double> b(p_start.Length() + 1, p_start.Length());
  Gambit::SquareMatrix<double> q(p_start.Length() + 1);
  QreJacobian(p_start.GetSupport(), x, isLog, b);
  QRDecomp(b, q);
  q.GetRow(q.NumRows(), t);
  
  int niters = 0;

  while (x[x.Length()] >= 0.0 && x[x.Length()] < p_maxLambda) {
    bool accept = true;

    if (fabs(h) <= c_hmin) {
      // Stop.  If this occurs because we are in MLE-finding mode,
      // resume tracing the original branch
      if (newton) {
	//printf("popping! %f\n", pushH);
	x = pushX;
	h = pushH;
	isLog = pushLog;
	QreJacobian(p_start.GetSupport(), x, isLog, b);
	QRDecomp(b, q);
	q.GetRow(q.NumRows(), t);
	newton = false;
	restarting = true;
       	continue;
      }
      else {
	// We're really done.
	return;
      }
    }

    // Predictor step
    for (int k = 1; k <= x.Length(); k++) {
      u[k] = x[k] + h * p_omega * t[k];
    }

    double decel = 1.0 / g_maxDecel;  // initialize deceleration factor
    QreJacobian(p_start.GetSupport(), u, isLog, b);
    QRDecomp(b, q);

    int iter = 1;
    double disto = 0.0;
    while (true) {
      double dist;

      QreLHS(p_start.GetSupport(), u, isLog, y);
      NewtonStep(q, b, u, y, dist); 
      if (dist >= c_maxDist) {
	accept = false;
	break;
      }
      
      decel = gmax(decel, sqrt(dist / c_maxDist) * g_maxDecel);
      if (iter >= 2) {
	double contr = dist / (disto + c_tol * c_eta);
	if (contr > c_maxContr) {
	  accept = false;
	  break;
	}
	decel = gmax(decel, sqrt(contr / c_maxContr) * g_maxDecel);
      }

      if (dist <= c_tol) {
	// Success; break out of iteration
	break;
      }
      disto = dist;
      iter++;
    }

    if (!accept) {
      h /= g_maxDecel;   // PC not accepted; change stepsize and retry
      if (fabs(h) <= c_hmin) {
	// Stop.  If this occurs because we are in MLE-finding mode,
	// resume tracing the original branch
	if (newton) {
	  //printf("popping! %f\n", pushH);
	  x = pushX;
	  h = pushH;
	  isLog = pushLog;
	  newton = false;
	  restarting = true;
	  QreJacobian(p_start.GetSupport(), x, isLog, b);
	  QRDecomp(b, q);
	  q.GetRow(q.NumRows(), t);
	  continue;
	}
	else {
	  // We're really done.
	  return;
	}
      }

      continue;
    }

    // Determine new stepsize
    if (decel > g_maxDecel) {
      decel = g_maxDecel;
    }

    if (g_maxLike) {
      // Currently, 't' is the tangent at 'x'.  We also need the
      // tangent at 'u'.
      gbtVector<double> newT(t);
      q.GetRow(q.NumRows(), newT); 

      if (!restarting && 
	  DiffLogLike(x, isLog, t) * DiffLogLike(u, isLog, newT) < 0.0) {
	// Store the current state, to resume later
	pushX = x;
	pushH = h;
	pushLog = isLog;
	newton = true;
	//printf("entering newton mode\n");
      }
    }

    if (newton) {
      // Newton-type steplength adaptation, secant method
      gbtVector<double> newT(t);
      q.GetRow(q.NumRows(), newT); 

      h *= -DiffLogLike(u, isLog, newT) / (DiffLogLike(u, isLog, newT) - 
					   DiffLogLike(x, isLog, t));
    }
    else {
      // Standard steplength adaptation
      h = fabs(h / decel);
    }

    restarting = false;

    // PC step was successful; update and iterate
    x = u;

    if (g_fullGraph) {
      PrintProfile(std::cout, p_start.GetSupport(), x, isLog);
    }
    

    // Update isLog: any strategy below 10^-10 should switch to log rep
    bool recompute = false;

    for (int i = 1; i < x.Length(); i++) {
      if (!isLog[i] && x[i] < .001) {
	x[i] = log(x[i]);
	isLog[i] = true;
	recompute = true;
      }
      else if (isLog[i] && exp(x[i]) > .001) {
	x[i] = exp(x[i]);
	isLog[i] = false;
	recompute = true;
      }
    }

    if (recompute) {
      // If we switch representations, make sure to get the new Jacobian
      QreJacobian(p_start.GetSupport(), x, isLog, b);
      QRDecomp(b, q);
    }

    gbtVector<double> newT(t);
    q.GetRow(q.NumRows(), newT);  // new tangent
    if (t * newT < 0.0) {
      //printf("Bifurcation! at %f\n", x[x.Length()]);
      // Bifurcation detected; for now, just "jump over" and continue,
      // taking into account the change in orientation of the curve.
      // Someday, we need to do more here! :)
      p_omega = -p_omega;
    }
    t = newT;

  }

  if (!g_fullGraph) {
    PrintProfile(std::cout, p_start.GetSupport(), x, isLog, true);
  }
}

void PrintBanner(std::ostream &p_stream)
{
  p_stream << "Compute a branch of the logit equilibrium correspondence\n";
  p_stream << "Gambit version " VERSION ", Copyright (C) 2005, The Gambit Project\n";
  p_stream << "This is free software, distributed under the GNU GPL\n\n";
}

void PrintHelp(char *progname)
{
  PrintBanner(std::cerr);
  std::cerr << "Usage: " << progname << " [OPTIONS]\n";
  std::cerr << "Accepts strategic game on standard input.\n";

  std::cerr << "Options:\n";
  std::cerr << "  -d DECIMALS      show equilibria as floating point with DECIMALS digits\n";
  std::cerr << "  -s STEP          initial stepsize (default is .03)\n";
  std::cerr << "  -a ACCEL         maximum acceleration (default is 1.1)\n";
  std::cerr << "  -m MAXLAMBDA     stop when reaching MAXLAMBDA (default is 1000000)\n";
  std::cerr << "  -h               print this help message\n";
  std::cerr << "  -q               quiet mode (suppresses banner)\n";
  std::cerr << "  -e               print only the terminal equilibrium\n";
  std::cerr << "                   (default is to print the entire branch)\n";
  exit(1);
}

//
// Read in a comma-separated values list of observed data values
//
bool ReadProfile(std::istream &p_stream, gbtArray<double> &p_profile)
{
  for (int i = 1; i <= p_profile.Length(); i++) {
    if (p_stream.eof() || p_stream.bad()) {
      return false;
    }

    p_stream >> p_profile[i];
    if (i < p_profile.Length()) {
      char comma;
      p_stream >> comma;
    }
  }
  // Read in the rest of the line and discard
  std::string foo;
  std::getline(p_stream, foo);
  return true;
}

int main(int argc, char *argv[])
{
  opterr = 0;

  bool quiet = false;
  double maxLambda = 1000000.0;
  std::string mleFile = "";

  int c;
  while ((c = getopt(argc, argv, "d:s:a:m:qehL:")) != -1) {
    switch (c) {
    case 'q':
      quiet = true;
      break;
    case 'd':
      g_numDecimals = atoi(optarg);
      break;
    case 's':
      g_hStart = atof(optarg);
      break;
    case 'a':
      g_maxDecel = atof(optarg);
      break;
    case 'm':
      maxLambda = atof(optarg);
      break;
    case 'e':
      g_fullGraph = false;
      break;
    case 'h':
      PrintHelp(argv[0]);
      break;
    case 'L':
      mleFile = optarg;
      break;
    case '?':
      if (isprint(optopt)) {
	std::cerr << argv[0] << ": Unknown option `-" << ((char) optopt) << "'.\n";
      }
      else {
	std::cerr << argv[0] << ": Unknown option character `\\x" << optopt << "`.\n";
      }
      return 1;
    default:
      abort();
    }
  }

  if (!quiet) {
    PrintBanner(std::cerr);
  }


  Gambit::Game nfg;

  try {
    nfg = Gambit::ReadNfg(std::cin);
  }
  catch (...) {
    return 1;
  }

  if (mleFile != "") {
    g_obsProbs = gbtArray<double>(nfg->MixedProfileLength());
    std::ifstream mleData(mleFile.c_str());
    ReadProfile(mleData, g_obsProbs);
    g_maxLike = true;
  }
  

  Gambit::MixedStrategyProfile<double> start(nfg);

  try {
    TracePath(start, 0.0, maxLambda, 1.0);
    return 0;
  }
  catch (...) {
    return 1;
  }
}

