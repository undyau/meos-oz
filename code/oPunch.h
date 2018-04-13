// oPunch.h: interface for the oPunch class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPUNCH_H__67B23AF5_5783_4A6A_BB2E_E522B9283A42__INCLUDED_)
#define AFX_OPUNCH_H__67B23AF5_5783_4A6A_BB2E_E522B9283A42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "oBase.h"

/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2017 Melin Software HB

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
    Eksoppsv�gen 16, SE-75646 UPPSALA, Sweden

************************************************************************/

class oEvent;

class oPunch : public oBase
{
protected:

  int Type;
  int Time;
  bool isUsed; //Is used in the course...

  // Index into course (-1 if unused)
  int tRogainingIndex;
  // Number of rogaining points given
  int tRogainingPoints;

  //Adjustment of this punch, loaded from control
  int tTimeAdjust;

  volatile int tCardIndex; // Index into card
  int tIndex; // Control match index in course
  int tMatchControlId;
  bool hasBeenPlayed;

  /** Get internal data buffers for DI */
  oDataContainer &getDataBuffers(pvoid &data, pvoid &olddata, pvectorstr &strData) const;
  int getDISize() const {return -1;}

  void changedObject();
public:

  virtual int getControlId() {return tMatchControlId;}

  bool isUsedInCourse() const {return isUsed;}
  void remove();
  bool canRemove() const;

  wstring getInfo() const;

  bool isStart() const {return Type==PunchStart;}
  bool isStart(int startType) const {return Type==PunchStart || Type == startType;}
  bool isFinish() const {return Type==PunchFinish;}
  bool isFinish(int finishType) const {return Type==PunchFinish || Type == finishType;}
  bool isCheck() const {return Type==PunchCheck;}
  int getControlNumber() const {return Type>=30 ? Type : 0;}
  const wstring &getType() const;
  static const wstring &getType(int t);
  int getTypeCode() const {return Type;}
  wstring getString() const ;
  wstring getSimpleString() const;

  wstring getTime() const;
  int getAdjustedTime() const;
  void setTime(const wstring &t);
  virtual void setTimeInt(int newTime, bool databaseUpdate);

  void setTimeAdjust(int t) {tTimeAdjust=t;}
  void adjustTimeAdjust(int t) {tTimeAdjust+=t;}

  wstring getRunningTime(int startTime) const;

  enum SpecialPunch {PunchStart=1, PunchFinish=2, PunchCheck=3};
  void decodeString(const string &s);
  string codeString() const;
  oPunch(oEvent *poe);
  virtual ~oPunch();

  friend class oCard;
  friend class oRunner;
  friend class oTeam;
  friend class oEvent;
};

typedef oPunch * pPunch;

#endif // !defined(AFX_OPUNCH_H__67B23AF5_5783_4A6A_BB2E_E522B9283A42__INCLUDED_)
