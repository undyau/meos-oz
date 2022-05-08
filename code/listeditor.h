﻿#pragma once

/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2022 Melin Software HB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Eksoppsvägen 16, SE-75646 UPPSALA, Sweden

************************************************************************/

class MetaList;
class MetaListPost;
class MetaListContainer;
class gdioutput;
class BaseInfo;
class ButtonInfo;
class oEvent;
enum EPostType;
class TabBase;

#include <vector>
#include "autocompletehandler.h"
#include "oListInfo.h"

class ListEditor : public AutoCompleteHandler {
private:
  enum SaveType {NotSaved, SavedInside, SavedFile};
  oEvent *oe;
  MetaList *currentList;
  void setCurrentList(MetaList *lst);
  int currentIndex;
  wstring savedFileName;
  bool dirtyExt;
  bool dirtyInt;
  SaveType lastSaved;
  const wchar_t *getIndexDescription(EPostType type);
  wstring lastShownExampleText;
  void showLine(gdioutput &gdi, const vector<MetaListPost> &line, int ix) const;
  int editList(gdioutput &gdi, int type, BaseInfo &data);
  void updateType(int iType, gdioutput &gdi);

  bool saveListPost(gdioutput &gdi, MetaListPost &mlp);

  ButtonInfo &addButton(gdioutput &gdi, const MetaListPost &mlp, int x, int y,
                       int lineIx, int ix) const;

  void editListPost(gdioutput &gdi, const MetaListPost &mlp, int id);
  
  void showExample(gdioutput &gdi, EPostType type = EPostType::lLastItem);

  void showExample(gdioutput &gdi, const MetaListPost &mlp);

  int readLeg(gdioutput &gdi, EPostType newType, bool checkError) const;

  void editListProp(gdioutput &gdi, bool newList);

  enum DirtyFlag {MakeDirty, ClearDirty, NoTouch};

  /// Check (and autosave) if there are unsaved changes in a dialog box
  void checkUnsaved(gdioutput &gdi);

  /// Check and ask if there are changes to save
  bool checkSave(gdioutput &gdi);

  // Enable or disable open button
  void enableOpen(gdioutput &gdi);

  void makeDirty(gdioutput &gdi, DirtyFlag inside, DirtyFlag outside);

  void updateAlign(gdioutput &gdi, int val);

  TabBase *origin = nullptr;
  void show(gdioutput &gdi);

  int xpUseLeg;
  int ypUseLeg;

  bool legStageTypeIndex(gdioutput &gdi, EPostType type, int leg);


public:
  ListEditor(oEvent *oe);
  virtual ~ListEditor();

  void load(const MetaListContainer &mlc, int index);
  void show(TabBase *dst, gdioutput &gdi);
  bool isShown(TabBase *tab) const { return origin == tab; }
  MetaList *getCurrentList() const {return currentList;};
  void handleAutoComplete(gdioutput &gdi, AutoCompleteInfo &info) final;

  friend int editListCB(gdioutput*, int, void *);
};
