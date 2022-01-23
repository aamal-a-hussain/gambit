//
// This file is part of Gambit
// Copyright (c) 1994-2022, The Gambit Project (http://www.gambit-project.org)
//
// FILE: src/gui/nfgprofile.h
// Normal form mixed profile window
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

#ifndef NFGPROFILE_H
#define NFGPROFILE_H

#include "wx/sheet/sheet.h"
#include "gamedoc.h"

class gbtMixedProfileList : public wxSheet, public gbtGameView {
private:
  int m_showProbs, m_showPayoff;

  // Overriding wxSheet members for data access
  wxString GetCellValue(const wxSheetCoords &);
  wxSheetCellAttr GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const;
  
  // Overriding wxSheet members to disable selection behavior
  bool SelectRow(int, bool = false, bool = false)
    { return false; }
  bool SelectRows(int, int, bool = false, bool = false)
    { return false; }
  bool SelectCol(int, bool = false, bool = false)
    { return false; }
  bool SelectCols(int, int, bool = false, bool = false)
    { return false; }
  bool SelectCell(const wxSheetCoords&, bool = false, bool = false)
    { return false; }
  bool SelectBlock(const wxSheetBlock&, bool = false, bool = false)
    { return false; }
  bool SelectAll(bool = false) { return false; }

  // Overriding wxSheet member to suppress drawing of cursor
  void DrawCursorCellHighlight(wxDC&, const wxSheetCellAttr &) { }

  // Event handlers
  void OnLabelClick(wxSheetEvent &);
  void OnCellClick(wxSheetEvent &);

  // Overriding gbtGameView members
  void OnUpdate(void);

  // Which profile index corresponds to a sheet row
  int RowToProfile(int row) const
  { return row / (m_showProbs + m_showPayoff) + 1; }
  bool IsProbabilityRow(int row) const
  { return (m_showPayoff == 0 || row % 2 == 0); }
  bool IsPayoffRow(int row) const
  { return (m_showProbs == 0 || row % 2 == 1); }


public:
  gbtMixedProfileList(wxWindow *p_parent, gbtGameDocument *p_doc);
  virtual ~gbtMixedProfileList();
};

#endif  // NFGPROFILE_H
