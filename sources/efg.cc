//#
//# FILE: extform.cc -- Implementation of extensive form data type
//#
//# $Id$
//#

class Node;
class Player;
class Infoset;
class Action;
class Outcome;
#include "garray.h"
#include "rational.h"
#ifdef __GNUG__
#define TEMPLATE template
#elif defined __BORLANDC__
class gArray<int>;
class gArray<double>;
class gArray<gRational>;
#define TEMPLATE
#pragma option -Jgd
#endif   // __GNUG__, __BORLANDC__

#include "glist.imp"

TEMPLATE class gList<Node *>;
TEMPLATE class gNode<Node *>;

#include "garray.imp"
#include "gblock.imp"

TEMPLATE class gArray<Player *>;
TEMPLATE class gBlock<Player *>;

TEMPLATE class gArray<Infoset *>;
TEMPLATE class gBlock<Infoset *>;

TEMPLATE class gArray<Node *>;
TEMPLATE class gBlock<Node *>;

TEMPLATE class gArray<Action *>;
TEMPLATE class gBlock<Action *>;

TEMPLATE class gArray<Outcome *>;
TEMPLATE class gBlock<Outcome *>;

#pragma -Jgx


#include "extform.h"
#include "player.h"
#include "infoset.h"
#include "node.h"
#include "outcome.h"
#include "gconvert.h"
#include <assert.h>

Player::~Player()
{
  while (infosets.Length())   delete infosets.Remove(1);
}

bool Player::IsInfosetDefined(const gString &s) const
{
  for (int i = 1; i <= infosets.Length(); i++)
    if (infosets[i]->name == s)   return true;
  return false;
}

Infoset *Player::GetInfoset(const gString &name) const
{
  for (int i = 1; i <= infosets.Length(); i++)
    if (infosets[i]->name == name)   return infosets[i];
  return 0;
}

//------------------------------------------------------------------------
//      BaseExtForm: Constructors, destructor, constructive operators
//------------------------------------------------------------------------

BaseExtForm::BaseExtForm(void) : title("UNTITLED"), chance(new Player(0))
{ }

BaseExtForm::~BaseExtForm()
{
  delete root;
  delete chance;
  while (players.Length())    delete players.Remove(1);
  while (outcomes.Length())   delete outcomes.Remove(1);
}

//------------------------------------------------------------------------
//               BaseExtForm: Title access and manipulation
//------------------------------------------------------------------------

void BaseExtForm::SetTitle(const gString &s)
{ title = s; }

const gString &BaseExtForm::GetTitle(void) const
{ return title; }

//------------------------------------------------------------------------
//                    BaseExtForm: Writing data files
//------------------------------------------------------------------------

void BaseExtForm::DisplayTree(gOutput &f, Node *n) const
{
  f << "{ " << n << ' ';
  for (int i = 1; i <= n->children.Length(); DisplayTree(f, n->children[i++]));
  f << "} ";
}

void BaseExtForm::DisplayTree(gOutput &f) const
{
  DisplayTree(f, root);
}

void BaseExtForm::WriteEfgFile(gOutput &f, Node *n) const
{
  if (n->children.Length() == 0)   {
    f << "t \"" << n->name << "\" \"";
    if (n->outcome)  {
      f << n->outcome->name << "\" ";
      n->outcome->PrintValues(f);
      f << '\n';
    }
    else
      f << "\"\n";
  }
  
  else if (n->infoset->player->number)   {
    f << "p \"" << n->name << "\" \"" << n->infoset->player->name << "\" ";
    f << '"' << n->infoset->name << "\" ";
    n->infoset->PrintActions(f);
    f << " \"";
    if (n->outcome)  {
      f << n->outcome->name << "\" ";
      n->outcome->PrintValues(f);
      f << '\n';
    }
    else
      f << "\"\n";
  }
  
  else   {    // chance node
    f << "c \"" << n->name << "\" ";
    f << '"' << n->infoset->name << "\" ";
    n->infoset->PrintActions(f);
    f << " \"";
    if (n->outcome)  {
      f << n->outcome->name << "\" ";
      n->outcome->PrintValues(f);
      f << '\n';
    }
    else
      f << "\"\n";
  }

  for (int i = 1; i <= n->children.Length(); i++)
    WriteEfgFile(f, n->children[i]);
}

void BaseExtForm::WriteEfgFile(gOutput &f) const
{
  f << "EFG 1 " << ((Type() == DOUBLE) ? 'D' : 'R');
  f << " \"" << title << "\" { ";
  for (int i = 1; i <= players.Length(); i++)
    f << '"' << players[i]->name << "\" ";
  f << "}\n\n";

  WriteEfgFile(f, root);
}


//------------------------------------------------------------------------
//                    ExtForm<T>: General data access
//------------------------------------------------------------------------

int BaseExtForm::NumPlayers(void) const
{ return players.Length(); }

int BaseExtForm::NumOutcomes(void) const
{ return outcomes.Length(); }

Node *BaseExtForm::RootNode(void) const
{ return root; }

bool BaseExtForm::IsSuccessor(const Node *n, const Node *from) const
{ return IsPredecessor(from, n); }

bool BaseExtForm::IsPredecessor(const Node *n, const Node *of) const
{ 
  while (n && n != of)    n = n->parent;

  return (n == of);
}

//------------------------------------------------------------------------
//                    ExtForm<T>: Operations on players
//------------------------------------------------------------------------

Player *BaseExtForm::GetChance(void) const
{
  return chance;
}

Player *BaseExtForm::GetPlayer(const gString &name) const
{
  for (int i = 1; i <= players.Length(); i++)
    if (players[i]->name == name)   return players[i];
  return 0;
}

Player *BaseExtForm::NewPlayer(void)
{
  Player *ret = new Player(players.Length() + 1);
  players.Append(ret);
  root->Resize(players.Length());
  return ret;
}

Infoset *BaseExtForm::AppendNode(Node *n, Player *p, int count)
{
  assert(n && p && count > 0);

  if (n->children.Length() == 0)   {
    n->infoset = CreateInfoset(p->infosets.Length() + 1, p, count);
    n->infoset->members.Append(n);
    p->infosets.Append(n->infoset);
    while (count--)
      n->children.Append(CreateNode(n));
  }

  return n->infoset;
}  

Infoset *BaseExtForm::AppendNode(Node *n, Infoset *s)
{
  assert(n && s);
  
  if (n->children.Length() == 0)   {
    n->infoset = s;
    s->members.Append(n);
    for (int i = 1; i <= s->actions.Length(); i++)
      n->children.Append(CreateNode(n));
  }

  return s;
}
  
Node *BaseExtForm::DeleteNode(Node *n, Node *keep)
{
  return n;
}

Infoset *BaseExtForm::InsertNode(Node *n, Player *p, int count)
{
  assert(n && p && count > 0);

  Node *m = CreateNode(n->parent);
  m->infoset = CreateInfoset(p->infosets.Length() + 1, p, count);
  p->infosets.Append(m->infoset);
  m->infoset->members.Append(m);
  if (n->parent)
    n->parent->children[n->parent->children.Find(n)] = m;
  else
    root = m;
  m->children.Append(n);
  while (--count)
    m->children.Append(CreateNode(m));

  return m->infoset;
}

Infoset *BaseExtForm::InsertNode(Node *n, Infoset *s)
{
  assert(n && s);
  
  Node *m = CreateNode(n->parent);
  m->infoset = s;
  s->members.Append(m);
  if (n->parent)
    n->parent->children[n->parent->children.Find(n)] = m;
  else
    root = m;
  m->children.Append(n);
  int count = s->actions.Length();
  while (--count)
    m->children.Append(CreateNode(m));
  
  return m->infoset;
}

Infoset *BaseExtForm::JoinInfoset(Infoset *s, Node *n)
{
  assert(n && s);

  if (!n->infoset)   return 0; 
  if (n->infoset == s)   return s;
  if (s->actions.Length() != n->children.Length())  return n->infoset;

  Infoset *t = n->infoset;
  Player *p = n->infoset->player;

  t->members.Remove(t->members.Find(n));
  if (t->members.Length() == 0)
    delete p->infosets.Remove(p->infosets.Find(t));
  s->members.Append(n);

  n->infoset = s;

  return s;
}

Infoset *BaseExtForm::LeaveInfoset(Node *n)
{
  assert(n);

  if (!n->infoset)   return 0;

  Infoset *s = n->infoset;
  if (s->members.Length() == 1)   return s;

  Player *p = s->player;
  s->members.Remove(s->members.Find(n));
  n->infoset = CreateInfoset(p->infosets.Length() + 1, p,
			     n->children.Length());
  n->infoset->members.Append(n);
  p->infosets.Append(n->infoset);
  for (int i = 1; i <= s->actions.Length(); i++)
    n->infoset->actions[i]->name = s->actions[i]->name;

  return n->infoset;
}

Infoset *BaseExtForm::MergeInfoset(Infoset *to, Infoset *from)
{
  assert(to && from);

  if (to == from ||
      to->actions.Length() != from->actions.Length())   return from;

  to->members += from->members;
  for (int i = 1; i <= from->members.Length(); i++)
    from->members[i]->infoset = to;
  delete from->player->infosets.Remove(from->player->infosets.Find(from));
  return to;
}

Infoset *BaseExtForm::SwitchPlayer(Infoset *s, Player *p)
{
  return s;
}

Infoset *BaseExtForm::SwitchPlayer(Node *n, Player *p)
{
  return n->infoset;
}

void BaseExtForm::CopySubtree(Node *src, Node *dest, Node *stop)
{
  if (src == stop)   return;

  if (src->children.Length())  {
    AppendNode(dest, src->infoset);
    for (int i = 1; i <= src->children.Length(); i++)
      CopySubtree(src->children[i], dest->children[i], stop);
  }

  dest->name = src->name;
  dest->outcome = src->outcome;
}

Node *BaseExtForm::CopyTree(Node *src, Node *dest)
{
  assert(src && dest);
  if (src == dest || dest->children.Length())   return src;

  CopySubtree(src, dest, dest);

  return dest;
}

Node *BaseExtForm::MoveTree(Node *src, Node *dest)
{
  assert(src && dest);
  if (src == dest || dest->children.Length() || IsPredecessor(src, dest))
    return src;

  Node *parent = src->parent;    // cannot be null, saves us some problems

  parent->children[parent->children.Find(src)] = dest;
  dest->parent->children[dest->parent->children.Find(dest)] = src;

  src->parent = dest->parent;
  dest->parent = parent;

  dest->name = "";
  dest->outcome = 0;
  
  return dest;
}

Node *BaseExtForm::DeleteTree(Node *n)
{
  return n;
}

Infoset *BaseExtForm::AppendAction(Infoset *s)
{
  assert(s);
  s->actions.Append(new Action(""));
  for (int i = 1; i <= s->members.Length(); i++)
    s->members[i]->children.Append(CreateNode(s->members[i]));
  return s;
}

Infoset *BaseExtForm::InsertAction(Infoset *s, Action *a)
{
  assert(a && s);
  for (int where = 1; where <= s->actions.Length() && s->actions[where] != a;
       where++);
  if (where > s->actions.Length())   return s;
  s->actions.Insert(new Action(""), where);
  for (int i = 1; i <= s->members.Length(); i++)
    s->members[i]->children.Insert(CreateNode(s->members[i]), where);

  return s;
}

Infoset *BaseExtForm::DeleteAction(Infoset *s, Action *a)
{
  return s;
}

//========================================================================

//---------------------------------------------------------------------------
//                    BaseBehavProfile member functions
//---------------------------------------------------------------------------

BaseBehavProfile::BaseBehavProfile(const BaseExtForm &EF, bool trunc)
  : E(&EF), truncated(trunc)  { }

BaseBehavProfile::~BaseBehavProfile()   { }

BaseBehavProfile &BaseBehavProfile::operator=(const BaseBehavProfile &p)
{
  E = p.E;
  truncated = p.truncated;
  return *this;
}

DataType BaseBehavProfile::Type(void) const
{
  return E->Type();
}

const gString &BaseBehavProfile::GetPlayerName(int p) const
{
  return E->PlayerList()[p]->GetName();
}

const gString &BaseBehavProfile::GetInfosetName(int p, int iset) const
{
  return E->PlayerList()[p]->InfosetList()[iset]->GetName();
}

const gString &BaseBehavProfile::GetActionName(int p, int iset, int act) const
{
  return E->PlayerList()[p]->InfosetList()[iset]->GetActionName(act);
}


