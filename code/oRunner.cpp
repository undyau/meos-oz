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

// oRunner.cpp: implementation of the oRunner class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "oRunner.h"

#include "oEvent.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "table.h"
#include "meos_util.h"
#include "oFreeImport.h"
#include <cassert>
#include "localizer.h"
#include "SportIdent.h"
#include <cmath>
#include "intkeymapimpl.hpp"
#include "runnerdb.h"
#include "meosexception.h"
#include <algorithm>
#include "oExtendedEvent.h"
#include "socket.h"
#include "MeOSFeatures.h"
#include "oListInfo.h"
#include "qualification_final.h"

oRunner::RaceIdFormatter oRunner::raceIdFormatter;

const wstring &oRunner::RaceIdFormatter::formatData(const oBase *ob) const {
  return itow(dynamic_cast<const oRunner &>(*ob).getRaceIdentifier());
}

wstring oRunner::RaceIdFormatter::setData(oBase *ob, const wstring &input) const {
  int rid = _wtoi(input.c_str());
  if (input == L"0")
    ob->getDI().setInt("RaceId", 0);
  else if (rid>0 && rid != dynamic_cast<oRunner *>(ob)->getRaceIdentifier())
    ob->getDI().setInt("RaceId", rid);
  return formatData(ob);
}

int oRunner::RaceIdFormatter::addTableColumn(Table *table, const string &description, int minWidth) const {
  return table->addColumn(description, max(minWidth, 90), true, true);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
oAbstractRunner::oAbstractRunner(oEvent *poe, bool loading):oBase(poe)
{
  Class=0;
  Club=0;
  startTime = 0;
  tStartTime = 0;

  FinishTime = 0;
  tStatus = status = StatusUnknown;

  inputPoints = 0;
  if (loading || !oe->hasPrevStage())
    inputStatus = StatusOK;
  else
    inputStatus = StatusNotCompetiting;
  
  inputTime = 0;
  inputPlace = 0;

  tTimeAdjustment = 0;
  tPointAdjustment = 0;
  tAdjustDataRevision = -1;
}

wstring oAbstractRunner::getInfo() const
{
  return getName();
}

void oAbstractRunner::setFinishTimeS(const wstring &t)
{
  setFinishTime(oe->getRelativeTime(t));
}

void oAbstractRunner::setStartTimeS(const wstring &t)
{
  setStartTime(oe->getRelativeTime(t), true, false);
}

oRunner::oRunner(oEvent *poe):oAbstractRunner(poe, false)
{
  isTemporaryObject = false;
  Id=oe->getFreeRunnerId();
  Course=0;
  StartNo=0;
  CardNo=0;

  tInTeam=0;
  tLeg=0;
  tLegEquClass = 0;
  tNeedNoCard=false;
  tUseStartPunch=true;
  getDI().initData();
  correctionNeeded = false;

  tDuplicateLeg=0;
  tParentRunner=0;

  Card=0;
  cPriority=0;

  tCachedRunningTime = 0;
  tSplitRevision = -1;

  tRogainingPoints = 0;
  tRogainingOvertime = 0;
  tReduction = 0;
  tRogainingPointsGross = 0;
  tAdaptedCourse = 0;
  tAdaptedCourseRevision = -1;

  tShortenDataRevision = -1;
  tNumShortening = 0;
}

oRunner::oRunner(oEvent *poe, int id):oAbstractRunner(poe, true)
{
  isTemporaryObject = false;
  Id=id;
  oe->qFreeRunnerId = max(id, oe->qFreeRunnerId);
  Course=0;
  StartNo=0;
  CardNo=0;

  tInTeam=0;
  tLeg=0;
  tLegEquClass = 0;
  tNeedNoCard=false;
  tUseStartPunch=true;
  getDI().initData();
  correctionNeeded = false;

  tDuplicateLeg=0;
  tParentRunner=0;

  Card=0;
  cPriority=0;
  tCachedRunningTime = 0;
  tSplitRevision = -1;

  tRogainingPoints = 0;
  tRogainingOvertime = 0;
  tReduction = 0;
  tRogainingPointsGross = 0;
  tAdaptedCourse = 0;
  tAdaptedCourseRevision = -1;
}

oRunner::~oRunner()
{
  if (tInTeam){
    for(unsigned i=0;i<tInTeam->Runners.size(); i++)
      if (tInTeam->Runners[i] && tInTeam->Runners[i]==this)
        tInTeam->Runners[i]=0;

    tInTeam=0;
  }

  for (size_t k=0;k<multiRunner.size(); k++) {
    if (multiRunner[k])
      multiRunner[k]->tParentRunner=0;
  }

  if (tParentRunner) {
    for (size_t k=0;k<tParentRunner->multiRunner.size(); k++)
      if (tParentRunner->multiRunner[k] == this)
        tParentRunner->multiRunner[k]=0;
  }

  delete tAdaptedCourse;
  tAdaptedCourse = 0;
}

bool oRunner::Write(xmlparser &xml)
{
  if (Removed) return true;

  xml.startTag("Runner");
  xml.write("Id", Id);
  xml.write("Updated", Modified.getStamp());
  xml.write("Name", sName);
  xml.write("Start", startTime);
  xml.write("Finish", FinishTime);
  xml.write("Status", status);
  xml.write("CardNo", CardNo);
  xml.write("StartNo", StartNo);

  xml.write("InputPoint", inputPoints);
  if (inputStatus != StatusOK)
    xml.write("InputStatus", itos(inputStatus)); //Force write of 0
  xml.write("InputTime", inputTime);
  xml.write("InputPlace", inputPlace);

  if (Club) xml.write("Club", Club->Id);
  if (Class) xml.write("Class", Class->Id);
  if (Course) xml.write("Course", Course->Id);

  if (multiRunner.size()>0)
    xml.write("MultiR", codeMultiR());

  if (Card) {
    assert(Card->tOwner==this);
    Card->Write(xml);
  }
  getDI().write(xml);

  xml.endTag();

  for (size_t k=0;k<multiRunner.size();k++)
    if (multiRunner[k])
      multiRunner[k]->Write(xml);

  return true;
}

void oRunner::Set(const xmlobject &xo)
{
  xmlList xl;
  xo.getObjects(xl);
  xmlList::const_iterator it;

  for(it=xl.begin(); it != xl.end(); ++it){
    if (it->is("Id")){
      Id = it->getInt();
    }
    else if (it->is("Name")){
      sName = it->getw();
      getRealName(sName, tRealName);
    }
    else if (it->is("Start")){
      tStartTime = startTime = it->getInt();
    }
    else if (it->is("Finish")){
      FinishTime = it->getInt();
    }
    else if (it->is("Status")){
      unsigned rawStat = it->getInt();
      tStatus = status = RunnerStatus(rawStat<100u ? rawStat : 0);
    }
    else if (it->is("CardNo")){
      CardNo=it->getInt();
    }
    else if (it->is("StartNo") || it->is("OrderId"))
      StartNo=it->getInt();
    else if (it->is("Club"))
      Club=oe->getClub(it->getInt());
    else if (it->is("Class"))
      Class=oe->getClass(it->getInt());
    else if (it->is("Course"))
      Course=oe->getCourse(it->getInt());
    else if (it->is("Card")){
      Card=oe->allocateCard(this);
      Card->Set(*it);
      assert(Card->getId()!=0);
    }
    else if (it->is("oData"))
      getDI().set(*it);
    else if (it->is("Updated"))
      Modified.setStamp(it->getRaw());
    else if (it->is("MultiR"))
      decodeMultiR(it->getRaw());
    else if (it->is("InputTime")) {
      inputTime = it->getInt();
    }
    else if (it->is("InputStatus")) {
      unsigned rawStat = it->getInt();
      inputStatus = RunnerStatus(rawStat<100u ? rawStat : 0);
    }
    else if (it->is("InputPoint")) {
      inputPoints = it->getInt();
    }
    else if (it->is("InputPlace")) {
      inputPlace = it->getInt();
    }
  }
}

int oAbstractRunner::getBirthAge() const {
  return 0;
}

int oRunner::getBirthAge() const {
  int y = getBirthYear();
  if (y > 0)
    return getThisYear() - y;
  return 0;
}

void oAbstractRunner::addClassDefaultFee(bool resetFees) {
  if (Class) {
    oDataInterface di = getDI();

    if (isVacant()) {
      di.setInt("Fee", 0);
      di.setInt("EntryDate", 0);
      di.setInt("EntryTime", 0);
      di.setInt("Paid", 0);
      if (typeid(*this)==typeid(oRunner))
        di.setInt("CardFee", 0);
      return;
    }
    wstring date = getEntryDate();
    int currentFee = di.getInt("Fee");

    pTeam t = getTeam();
    if (t && t != this) {
      // Thus us a runner in a team
      // Check if the team has a fee.
      // Don't assign personal fee if so.
      if (t->getDCI().getInt("Fee") > 0)
        return;
    }

    if ((currentFee == 0 && !hasFlag(FlagFeeSpecified)) || resetFees) {
      int age = getBirthAge();
      int fee = Class->getEntryFee(date, age);
      di.setInt("Fee", fee);
    }
  }
}

// Get entry date of runner (or its team)
wstring oRunner::getEntryDate(bool useTeamEntryDate) const {
  if (useTeamEntryDate && tInTeam) {
    wstring date = tInTeam->getEntryDate(false);
    if (!date.empty())
      return date;
  }
  oDataConstInterface dci = getDCI();
  int date = dci.getInt("EntryDate");
  if (date == 0) {
    auto di = (const_cast<oRunner *>(this)->getDI());
    di.setDate("EntryDate", getLocalDate());
    di.setInt("EntryTime", convertAbsoluteTimeHMS(getLocalTimeOnly(), -1));

  }
  return dci.getDate("EntryDate");
}

string oRunner::codeMultiR() const
{
  char bf[32];
  string r;

  for (size_t k=0;k<multiRunner.size() && multiRunner[k];k++) {
    if (!r.empty())
      r+=":";
    sprintf_s(bf, "%d", multiRunner[k]->getId());
    r+=bf;
  }
  return r;
}

void oRunner::decodeMultiR(const string &r)
{
  vector<string> sv;
  split(r, ":", sv);
  multiRunnerId.clear();

  for (size_t k=0;k<sv.size();k++) {
    int d = atoi(sv[k].c_str());
    if (d>0)
      multiRunnerId.push_back(d);
  }
  multiRunnerId.push_back(0); // Mark as containing something
}

void oAbstractRunner::setClassId(int id, bool isManualUpdate) {
  pClass pc=Class;
  Class=id ? oe->getClass(id):0;

  if (Class!=pc) {
    apply(false, 0, false);
    if (Class) {
      Class->clearCache(true);
    }
    if (pc) {
      pc->clearCache(true);
      if (isManualUpdate) {
        setFlag(FlagUpdateClass, true);
        // Update heat data
        int heat = pc->getDCI().getInt("Heat");
        if (heat != 0)
          getDI().setInt("Heat", heat);
      }
    }
    updateChanged();
  }
}

// Update all classes (for multirunner)
void oRunner::setClassId(int id, bool isManualUpdate)
{
  if (Class && Class->getQualificationFinal() && isManualUpdate) {
    int heat = Class->getQualificationFinal()->getHeatFromClass(id, Class->getId());
    if (heat >= 0) {
      int oldHeat = getDI().getInt("Heat");

      if (heat != oldHeat) {
        pClass oldHeatClass = getClassRef(true);
        getDI().setInt("Heat", heat);
        pClass newHeatClass = getClassRef(true);
        oldHeatClass->clearCache(true);
        newHeatClass->clearCache(true);
        tSplitRevision = 0;
      }
    }
    return;
  }

  if (tParentRunner) { 
    assert(!isManualUpdate); // Do not support! This may be destroyed if calling tParentRunner->setClass
    return;
  }
  else {
    pClass pc = Class;
    pClass nPc = id>0 ? oe->getClass(id):0;

    if (pc && pc->isSingleRunnerMultiStage() && nPc!=pc && tInTeam) {
      if (!isTemporaryObject) {
        oe->autoRemoveTeam(this);

        if (nPc) {
          int newNR = max(nPc->getNumMultiRunners(0), 1);
          for (size_t k = newNR - 1; k<multiRunner.size(); k++) {
            if (multiRunner[k]) {
              assert(multiRunner[k]->tParentRunner == this);
              multiRunner[k]->tParentRunner = 0;
              vector<int> toRemove;
              toRemove.push_back(multiRunner[k]->Id);
              oe->removeRunner(toRemove);
            }
          }
          multiRunner.resize(newNR-1);
        }
      }
    }

    Class = nPc;

    if (Class != 0 && Class != pc && tInTeam==0 &&
                      Class->isSingleRunnerMultiStage()) {
      if (!isTemporaryObject) {
        pTeam t = oe->addTeam(getName(), getClubId(), getClassId(false));
        t->setStartNo(StartNo, false);
        t->setRunner(0, this, true);
      }
    }

    apply(false, 0, false); //We may get old class back from team.

    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k] && Class!=multiRunner[k]->Class) {
        multiRunner[k]->Class=Class;
        multiRunner[k]->updateChanged();
      }
    }

    if (Class!=pc && !isTemporaryObject) {
      if (Class) {
        Class->clearCache(true);
      }
      if (pc) {
        pc->clearCache(true);
      }
      tSplitRevision = 0;
      updateChanged();
      if (isManualUpdate && pc) {
        setFlag(FlagUpdateClass, true);
        // Update heat data
        int heat = pc->getDCI().getInt("Heat");
        if (heat != 0)
          getDI().setInt("Heat", heat);

      }
    }
  }
}

void oRunner::setCourseId(int id)
{
  pCourse pc=Course;

  if (id>0)
    Course=oe->getCourse(id);
  else
    Course=0;

  if (Course!=pc) {
    updateChanged();
    if (Class)
      Class->clearSplitAnalysis();
    tSplitRevision = 0;
  }
}

bool oAbstractRunner::setStartTime(int t, bool updateSource, bool tmpOnly, bool recalculate) {
  assert(!(updateSource && tmpOnly));
  if (tmpOnly) {
    tmpStore.startTime = t;
    return false;
  }

  int tOST=tStartTime;
  if (t>0)
    tStartTime=t;
  else tStartTime=0;

  if (updateSource) {
    int OST=startTime;
    startTime = tStartTime;

    if (OST!=startTime) {
      updateChanged();
    }
  }

  if (tOST != tStartTime) {
    changedObject();
    if (Class) {
      Class->clearCache(false);
    }
  }

  if (tOST<tStartTime && Class && recalculate)
    oe->reCalculateLeaderTimes(Class->getId());

  return tOST != tStartTime;
}

void oAbstractRunner::setFinishTime(int t)
{
  int OFT=FinishTime;

  if (t>tStartTime)
    FinishTime=t;
  else //Beeb
    FinishTime=0;

  if (OFT != FinishTime) {
    updateChanged();
    if (Class) {
      Class->clearCache(false);
    }
  }

  if (OFT>FinishTime && Class)
    oe->reCalculateLeaderTimes(Class->getId());
}

void oRunner::setFinishTime(int t)
{
  bool update=false;
  if (Class && (getTimeAfter(tDuplicateLeg)==0 || getTimeAfter()==0))
    update=true;

  oAbstractRunner::setFinishTime(t);

  tSplitRevision = 0;

  if (update && t!=FinishTime)
    oe->reCalculateLeaderTimes(Class->getId());
}

const wstring &oAbstractRunner::getStartTimeS() const {
  if (tStartTime>0)
    return oe->getAbsTime(tStartTime);
  else if (Class && Class->hasFreeStart())
    return _EmptyWString;
  else  
    return makeDash(L"-");
}

const wstring &oAbstractRunner::getStartTimeCompact() const {
  if (tStartTime>0) {
    if (oe->useStartSeconds())
      return oe->getAbsTime(tStartTime);
    else
      return oe->getAbsTimeHM(tStartTime);
  }
  else if (Class && Class->hasFreeStart())
    return _EmptyWString;
  else 
    return makeDash(L"-");
}

const wstring &oAbstractRunner::getFinishTimeS() const
{
  if (FinishTime>0)
    return oe->getAbsTime(FinishTime);
  else return makeDash(L"-");
}

int oAbstractRunner::getRunningTime() const {
  int rt = max(FinishTime-tStartTime, 0);
  if (rt > 0)
    return getTimeAdjustment() + rt;
  else
    return 0;
}

const wstring &oAbstractRunner::getRunningTimeS() const
{
  return formatTime(getRunningTime());
}

const wstring &oAbstractRunner::getTotalRunningTimeS() const
{
  return formatTime(getTotalRunningTime());
}

int oAbstractRunner::getTotalRunningTime() const {
  int t = getRunningTime();
  if (t > 0 && inputTime>=0)
    return t + inputTime;
  else
    return 0;
}

int oRunner::getTotalRunningTime() const {
  return getTotalRunningTime(getFinishTime(), true);
}


const wstring &oAbstractRunner::getStatusS() const
{
  return oEvent::formatStatus(tStatus);
}

const wstring &oAbstractRunner::getTotalStatusS() const
{
  return oEvent::formatStatus(getTotalStatus());
}

/*
 - Inactive		: Has not yet started
   - DidNotStart	: Did Not Start (in this race)
   - Active		: Currently on course
   - Finished		: Finished but not validated
   - OK			: Finished and validated
   - MisPunch		: Missing Punch
   - DidNotFinish	: Did Not Finish
   - Disqualified	: Disqualified
   - NotCompeting	: Not Competing (running outside the competition)
   - SportWithdr	: Sporting Withdrawal (e.g. helping injured)
   - OverTime 	: Overtime, i.e. did not finish within max time
   - Moved		: Moved to another class
   - MovedUp		: Moved to a "better" class, in case of entry
            restrictions
   - Cancelled
*/
const wchar_t *formatIOFStatus(RunnerStatus s) {
  switch(s) {
  case StatusOK:
    return L"OK";
  case StatusDNS:
    return L"DidNotStart";
  case StatusCANCEL:
    return L"Cancelled";
  case StatusMP:
    return L"MisPunch";
  case StatusDNF:
    return L"DidNotFinish";
  case StatusDQ:
    return L"Disqualified";
  case StatusMAX:
    return L"OverTime";
  }
  return L"Inactive";
}

wstring oAbstractRunner::getIOFStatusS() const
{
  return formatIOFStatus(tStatus);
}

wstring oAbstractRunner::getIOFTotalStatusS() const
{
  return formatIOFStatus(getTotalStatus());
}

void oRunner::addPunches(pCard card, vector<int> &missingPunches) {
  RunnerStatus oldStatus = getStatus();
  int oldFinishTime = getFinishTime();
  pCard oldCard = Card;

  if (Card && card != Card) {
    Card->tOwner = 0;
  }

  Card = card;
  card->adaptTimes(getStartTime());
  updateChanged();

  if (card) {
    if (CardNo==0)
      CardNo=card->cardNo;
    //315422
    assert(card->tOwner==0 || card->tOwner==this);
  }
  // Auto-select shortening
  pCourse mainCourse = getCourse(false);
  if (mainCourse && Card) {
    pCourse shortVersion = mainCourse->getShorterVersion();
    if (shortVersion) {
      //int s = mainCourse->getStartPunchType();
      //int f = mainCourse->getFinishPunchType();
      const int numCtrl = Card->getNumControlPunches(-1,-1);
      int numCtrlLong = mainCourse->getNumControls();
      int numCtrlShort = shortVersion->getNumControls();

      SICard sic;
      Card->getSICard(sic);
      int level = 0;
      while (mainCourse->distance(sic) < 0 && abs(numCtrl-numCtrlShort) < abs(numCtrl-numCtrlLong)) {
        level++;
        if (shortVersion->distance(sic) >= 0) {
          setNumShortening(level); // We passed at some level
          break;
        }
        mainCourse = shortVersion;
        shortVersion = mainCourse->getShorterVersion();
        numCtrlLong = numCtrlShort;
        if (!shortVersion) {
          break;
        }
        numCtrlShort = shortVersion->getNumControls();
      }
    }
  }

  if (Card)
    Card->tOwner=this;

  evaluateCard(true, missingPunches, 0, true);

  if (oe->isClient() && oe->getPropertyInt("UseDirectSocket", true)!=0) {
    if (oldStatus != getStatus() || oldFinishTime != getFinishTime()) {
      SocketPunchInfo pi;
      pi.runnerId = getId();
      pi.time = getFinishTime();
      pi.status = getStatus();
      pi.iHashType = oPunch::PunchFinish;
      oe->getDirectSocket().sendPunch(pi);
    }
  }

  oe->pushDirectChange();
  if (oldCard && Card && oldCard != Card && oldCard->isConstructedFromPunches())
    oldCard->remove(); // Remove card constructed from punches
}

pCourse oRunner::getCourse(bool useAdaptedCourse) const {
  pCourse tCrs = 0;
  if (Course)
    tCrs = Course;
  else if (Class) {
    const oClass *cls = getClassRef(true);

    if (cls->hasMultiCourse()) {
      if (tInTeam) {
        if (size_t(tLeg) >= tInTeam->Runners.size() || tInTeam->Runners[tLeg] != this) {
          tInTeam->quickApply();
        }
      }
      
      if (Class == cls) {
        if (tInTeam && Class->hasUnorderedLegs()) {
          vector< pair<int, pCourse> > group;
          Class->getParallelCourseGroup(tLeg, StartNo, group);

          if (group.size() == 1) {
            tCrs = group[0].second;
          }
          else {
            // Remove used courses
            int myStart = 0;

            for (size_t k = 0; k < group.size(); k++) {
              if (group[k].first == tLeg)
                myStart = k;

              pRunner tr = tInTeam->getRunner(group[k].first);
              if (tr && tr->Course) {
                // The course is assigned. Remove from group
                for (size_t j = 0; j < group.size(); j++) {
                  if (group[j].second == tr->Course) {
                    group[j].second = 0;
                    break;
                  }
                }
              }
            }

            // Clear out already preliminary assigned courses 
            for (int k = 0; k < myStart; k++) {
              pRunner r = tInTeam->getRunner(group[k].first);
              if (r && !r->Course) {
                size_t j = k;
                while (j < group.size()) {
                  if (group[j].second) {
                    group[j].second = 0;
                    break;
                  }
                  else j++;
                }
              }
            }

            for (size_t j = 0; j < group.size(); j++) {
              int ix = (j + myStart) % group.size();
              pCourse gcrs = group[ix].second;
              if (gcrs) {
                tCrs = gcrs;
                break;
              }
            }
          }
        }
        else if (tInTeam) {
          unsigned leg = legToRun();
          tCrs = Class->getCourse(leg, StartNo);
        }
        else {
          if (unsigned(tDuplicateLeg) < Class->MultiCourse.size()) {
            vector<pCourse> &courses = Class->MultiCourse[tDuplicateLeg];
            if (courses.size() > 0) {
              int index = StartNo % courses.size();
              tCrs = courses[index];
            }
          }
        }
      }
      else {
        // Final / qualification classes
        tCrs = cls->getCourse(0, StartNo);
      }
    }
    else
      tCrs = cls->Course;
  }

  if (tCrs && useAdaptedCourse) {
    // Find shortened version of course
    int ns = getNumShortening();
    pCourse shortCrs = tCrs;
    while (ns > 0 && shortCrs) {
      shortCrs = shortCrs->getShorterVersion();
      if (shortCrs)
        tCrs = shortCrs;
      ns--;
    }
  }

  if (tCrs && useAdaptedCourse && Card && tCrs->getCommonControl() != 0) {
    if (tAdaptedCourse && tAdaptedCourseRevision == oe->dataRevision) {
      return tAdaptedCourse;
    }
    if (!tAdaptedCourse)
      tAdaptedCourse = new oCourse(oe, -1);

    tCrs = tCrs->getAdapetedCourse(*Card, *tAdaptedCourse);
    tAdaptedCourseRevision = oe->dataRevision;
    return tCrs;
  }

  return tCrs;
}

const wstring &oRunner::getCourseName() const
{
  pCourse oc=getCourse(false);
  if (oc) return oc->getName();
  return makeDash(L"-");
}

#define NOTATIME 0xF0000000
void oAbstractRunner::resetTmpStore() {
  tmpStore.startTime = startTime;
  tmpStore.status = status;
  tmpStore.startNo = StartNo;
  tmpStore.bib = getBib();
}

bool oAbstractRunner::setTmpStore() {
  bool res = false;
  setStartNo(tmpStore.startNo, false);
  res |= setStartTime(tmpStore.startTime, false, false, false);
  res |= setStatus(tmpStore.status, false, false, false);
  setBib(tmpStore.bib, 0, false, false);
  return res;
}

bool oRunner::evaluateCard(bool doApply, vector<int> & MissingPunches,
                              int addpunch, bool sync) {
  if (unsigned(status) >= 100u)
    status = StatusUnknown; //Reset bad input

  MissingPunches.clear();
  int oldFT = FinishTime;
  int oldStartTime;
  RunnerStatus oldStatus;
  int *refStartTime;
  RunnerStatus *refStatus;

  if (doApply) {
    oldStartTime = tStartTime;
    tStartTime = startTime;
    oldStatus = tStatus;
    tStatus = status;
    refStartTime = &tStartTime;
    refStatus = &tStatus;

    resetTmpStore();
    apply(sync, 0, true);
  }
  else {
    // tmp initialized from outside. Do not change tStatus, tStartTime. Work with tmpStore instead!
    oldStartTime = tStartTime;
    oldStatus = tStatus;
    refStartTime = &tmpStore.startTime;
    refStatus = &tmpStore.status;

    createMultiRunner(false, sync);
  }

  // Reset card data
  oPunchList::iterator p_it;
  if (Card) {
    for (p_it=Card->punches.begin(); p_it!=Card->punches.end(); ++p_it) {
        p_it->tRogainingIndex = -1;
        p_it->tRogainingPoints = 0;
        p_it->isUsed = false;
        p_it->tIndex = -1;
        p_it->tMatchControlId = -1;
        p_it->tTimeAdjust = 0;
    }
  }

  bool inTeam = tInTeam != 0;
  tProblemDescription.clear();
  tReduction = 0;
  tRogainingPointsGross = 0;
  tRogainingOvertime = 0;

  vector<SplitData> oldTimes;
  swap(splitTimes, oldTimes);

  if (!Card) {
    if ((inTeam || !tUseStartPunch) && doApply)
      apply(sync, 0, false); //Post apply. Update start times.

    if (storeTimes() && Class && sync) {
      set<int> cls;
      cls.insert(Class->getId());
      oe->reEvaluateAll(cls, sync);
    }
    normalizedSplitTimes.clear();
    if (oldTimes.size() > 0 && Class)
      Class->clearSplitAnalysis();
    return false;
  }
  //Try to match class?!
  if (!Class)
    return false;

  if (Class->ignoreStartPunch())
    tUseStartPunch = false;

  const pCourse course = getCourse(true);

  if (!course) {
    // Reset rogaining. Store start/finish
    for (p_it=Card->punches.begin(); p_it!=Card->punches.end(); ++p_it) {
      if (p_it->isStart() && tUseStartPunch)
        *refStartTime = p_it->Time;
      else if (p_it->isFinish())
        setFinishTime(p_it->Time);
    }
    if ((inTeam || !tUseStartPunch) && doApply)
      apply(sync, 0, false); //Post apply. Update start times.

    storeTimes();
    return false;
  }

  int startPunchCode = course->getStartPunchType();
  int finishPunchCode = course->getFinishPunchType();

  bool hasRogaining = course->hasRogaining();

  // Pairs: <control index, point>
  intkeymap< pair<int, int> > rogaining(course->getNumControls());
  for (int k = 0; k< course->nControls; k++) {
    if (course->Controls[k] && course->Controls[k]->isRogaining(hasRogaining)) {
      int pt = course->Controls[k]->getRogainingPoints();
      for (int j = 0; j<course->Controls[k]->nNumbers; j++) {
        rogaining.insert(course->Controls[k]->Numbers[j], make_pair(k, pt));
      }
    }
  }

  if (addpunch && Card->punches.empty()) {
    Card->addPunch(addpunch, -1, course->Controls[0] ? course->Controls[0]->getId():0);
  }

  if (Card->punches.empty()) {
    for(int k=0;k<course->nControls;k++) {
      if (course->Controls[k]) {
        course->Controls[k]->startCheckControl();
        course->Controls[k]->addUncheckedPunches(MissingPunches, hasRogaining);
      }
    }
    if ((inTeam || !tUseStartPunch) && doApply)
      apply(sync, 0, false); //Post apply. Update start times.

    if (storeTimes() && Class && sync) {
      set<int> cls;
      cls.insert(Class->getId());
      oe->reEvaluateAll(cls, sync);
    }

    normalizedSplitTimes.clear();
    if (oldTimes.size() > 0 && Class)
      Class->clearSplitAnalysis();
    tRogainingPoints = max(0, getPointAdjustment());
    return false;
  }

  // Reset rogaining
  for (p_it=Card->punches.begin(); p_it!=Card->punches.end(); ++p_it) {
    p_it->tRogainingIndex = -1;
    p_it->tRogainingPoints = 0;
  }

  bool clearSplitAnalysis = false;


  //Search for start and update start time.
  p_it=Card->punches.begin();
  while ( p_it!=Card->punches.end()) {
    if (p_it->Type == startPunchCode) {
      if (tUseStartPunch && p_it->getAdjustedTime() != *refStartTime) {
        p_it->setTimeAdjust(0);
        *refStartTime = p_it->getAdjustedTime();
        if (*refStartTime != oldStartTime)
          clearSplitAnalysis = true;
        //updateChanged();
      }
      break;
    }
    ++p_it;
  }

  inthashmap expectedPunchCount(course->nControls);
  inthashmap punchCount(Card->punches.size());
  for (int k=0; k<course->nControls; k++) {
    pControl ctrl=course->Controls[k];
    if (ctrl && !ctrl->isRogaining(hasRogaining)) {
      for (int j = 0; j<ctrl->nNumbers; j++)
        ++expectedPunchCount[ctrl->Numbers[j]];
    }
  }

  for (p_it = Card->punches.begin(); p_it != Card->punches.end(); ++p_it) {
    if (p_it->Type>=10 && p_it->Type<=1024)
      ++punchCount[p_it->Type];
  }

  p_it = Card->punches.begin();
  splitTimes.resize(course->nControls, SplitData(NOTATIME, SplitData::Missing));
  int k=0;


  for (k=0;k<course->nControls;k++) {
    //Skip start finish check
    while(p_it!=Card->punches.end() &&
          (p_it->isCheck() || p_it->isFinish() || p_it->isStart())) {
      p_it->setTimeAdjust(0);
      ++p_it;
    }

    if (p_it==Card->punches.end())
      break;

    oPunchList::iterator tp_it=p_it;
    pControl ctrl=course->Controls[k];
    int skippedPunches = 0;

    if (ctrl) {
      int timeAdjust=ctrl->getTimeAdjust();
      ctrl->startCheckControl();

      // Add rogaining punches
      if (addpunch && ctrl->isRogaining(hasRogaining) && ctrl->getFirstNumber() == addpunch) {
        if ( Card->getPunchByType(addpunch) == 0) {
          oPunch op(oe);
          op.Type=addpunch;
          op.Time=-1;
          op.isUsed=true;
          op.tIndex = k;
          op.tMatchControlId=ctrl->getId();
          Card->punches.insert(tp_it, op);
          Card->updateChanged();
        }
      }

      if (ctrl->getStatus() == oControl::StatusBad || ctrl->getStatus() == oControl::StatusOptional) {
        // The control is marked "bad" but we found it anyway in the card. Mark it as used.
        if (tp_it!=Card->punches.end() && ctrl->hasNumberUnchecked(tp_it->Type)) {
          tp_it->isUsed=true; //Show that this is used when splittimes are calculated.
                            // Adjust if the time of this control was incorrectly set.
          tp_it->setTimeAdjust(timeAdjust);
          tp_it->tMatchControlId=ctrl->getId();
          tp_it->tIndex = k;
          splitTimes[k].setPunchTime(tp_it->getAdjustedTime());
          ++tp_it;
          p_it=tp_it;
        }
      }
      else {
        while(!ctrl->controlCompleted(hasRogaining) && tp_it!=Card->punches.end()) {
          if (ctrl->hasNumberUnchecked(tp_it->Type)) {

            if (skippedPunches>0) {
              if (ctrl->Status == oControl::StatusOK) {
                int code = tp_it->Type;
                if (expectedPunchCount[code]>1 && punchCount[code] < expectedPunchCount[code]) {
                  tp_it==Card->punches.end();
                  ctrl->uncheckNumber(code);
                  break;
                }
              }
            }
            tp_it->isUsed=true; //Show that this is used when splittimes are calculated.
            // Adjust if the time of this control was incorrectly set.
            tp_it->setTimeAdjust(timeAdjust);
            tp_it->tMatchControlId=ctrl->getId();
            tp_it->tIndex = k;
            if (ctrl->controlCompleted(hasRogaining))
              splitTimes[k].setPunchTime(tp_it->getAdjustedTime());
            ++tp_it;
            p_it=tp_it;
          }
          else {
            if (ctrl->hasNumberUnchecked(addpunch)){
              //Add this punch.
              oPunch op(oe);
              op.Type=addpunch;
              op.Time=-1;
              op.isUsed=true;

              op.tMatchControlId=ctrl->getId();
              op.tIndex = k;
              Card->punches.insert(tp_it, op);
              Card->updateChanged();
              if (ctrl->controlCompleted(hasRogaining))
                splitTimes[k].setPunched();
            }
            else {
              skippedPunches++;
              tp_it->isUsed=false;
              ++tp_it;
            }
          }
        }
      }

      if (tp_it==Card->punches.end() && !ctrl->controlCompleted(hasRogaining)
                    && ctrl->hasNumberUnchecked(addpunch) ) {
        Card->addPunch(addpunch, -1, ctrl->getId());
        if (ctrl->controlCompleted(hasRogaining))
          splitTimes[k].setPunched();
        Card->punches.back().isUsed=true;
        Card->punches.back().tMatchControlId=ctrl->getId();
        Card->punches.back().tIndex = k;
      }

      if (ctrl->controlCompleted(hasRogaining) && splitTimes[k].time == NOTATIME)
        splitTimes[k].setPunched();
    }
    else //if (ctrl && ctrl->Status==oControl::StatusBad){
      splitTimes[k].setNotPunched();

    //Add missing punches
    if (ctrl && !ctrl->controlCompleted(hasRogaining))
      ctrl->addUncheckedPunches(MissingPunches, hasRogaining);
  }

  //Add missing punches for remaining controls
  while (k<course->nControls) {
    if (course->Controls[k]) {
      pControl ctrl = course->Controls[k];
      ctrl->startCheckControl();

      if (ctrl->hasNumberUnchecked(addpunch)) {
        Card->addPunch(addpunch, -1, ctrl->getId());
        Card->updateChanged();
        if (ctrl->controlCompleted(hasRogaining))
          splitTimes[k].setNotPunched();
      }
      ctrl->addUncheckedPunches(MissingPunches, hasRogaining);
    }
    k++;
  }

  //Set the rest (if exist -- probably not) to "not used"
  while(p_it!=Card->punches.end()){
    p_it->isUsed=false;
    p_it->tIndex = -1;
    p_it->setTimeAdjust(0);
    ++p_it;
  }

  int OK = MissingPunches.empty();

  tRogaining.clear();
  tRogainingPoints = 0;
  int time_limit = 0;

  // Rogaining logic
  if (rogaining.size() > 0) {
    set<int> visitedControls;
    for (p_it=Card->punches.begin(); p_it != Card->punches.end(); ++p_it) {
      pair<int, int> pt;
      if (rogaining.lookup(p_it->Type, pt)) {
        if (visitedControls.count(pt.first) == 0) {
          visitedControls.insert(pt.first); // May noy be revisited
          p_it->isUsed = true;
          p_it->tRogainingIndex = pt.first;
          p_it->tMatchControlId = course->Controls[pt.first]->getId();
          p_it->tRogainingPoints = pt.second;
          tRogaining.push_back(make_pair(course->Controls[pt.first], p_it->getAdjustedTime()));
          splitTimes[pt.first].setPunchTime(p_it->getAdjustedTime());
          tRogainingPoints += pt.second;
        }
      }
    }

    // Manual point adjustment
    tRogainingPoints = max(0, tRogainingPoints + getPointAdjustment());

    int point_limit = course->getMinimumRogainingPoints();
    if (point_limit>0 && tRogainingPoints<point_limit) {
      tProblemDescription = L"X po�ng fattas.#" + itow(point_limit-tRogainingPoints);
      OK = false;
    }

    // Check this later
    time_limit = course->getMaximumRogainingTime();

    for (int k = 0; k<course->nControls; k++) {
      if (course->Controls[k] && course->Controls[k]->isRogaining(hasRogaining)) {
        if (!visitedControls.count(k))
          splitTimes[k].setNotPunched();// = splitTimes[k-1];
      }
    }
  }

  int maxTimeStatus = 0;
  if (Class && FinishTime>0) {
    int mt = Class->getMaximumRunnerTime();
    if (mt>0) {
      if (getRunningTime() > mt)
        maxTimeStatus = 1;
      else
        maxTimeStatus = 2;
    }
    else
      maxTimeStatus = 2;
  }

  if (*refStatus == StatusMAX && maxTimeStatus == 2)
    *refStatus = StatusUnknown;

  if (OK && (*refStatus==0 || *refStatus==StatusDNS || *refStatus == StatusCANCEL || *refStatus==StatusMP || *refStatus==StatusOK || *refStatus==StatusDNF))
    *refStatus = StatusOK;
  else	*refStatus = RunnerStatus(max(int(StatusMP), int(*refStatus)));

  oPunchList::reverse_iterator backIter = Card->punches.rbegin();

  if (finishPunchCode != oPunch::PunchFinish) {
    while (backIter != Card->punches.rend()) {
      if (backIter->Type == finishPunchCode)
        break;
      ++backIter;
    }
  }

  if (backIter != Card->punches.rend() && backIter->Type == finishPunchCode) {
    FinishTime = backIter->Time;
    if (finishPunchCode == oPunch::PunchFinish)
      backIter->tMatchControlId=oPunch::PunchFinish;
  }
  else if (FinishTime<=0) {
    *refStatus=RunnerStatus(max(int(StatusDNF), int(tStatus)));
    tProblemDescription = L"M�ltid saknas.";
    FinishTime=0;
  }

  if (*refStatus == StatusOK && maxTimeStatus == 1)
    *refStatus = StatusMAX; //Maxtime

  if (!MissingPunches.empty()) {
    tProblemDescription  = L"St�mplingar saknas: X#" + itow(MissingPunches[0]);
    for (unsigned j = 1; j<3; j++) {
      if (MissingPunches.size()>j)
        tProblemDescription += L", " + itow(MissingPunches[j]);
    }
    if (MissingPunches.size()>3)
      tProblemDescription += L"...";
    else
      tProblemDescription += L".";
  }

  // Adjust times on course, including finish time
  doAdjustTimes(course);

  tRogainingPointsGross = tRogainingPoints;
  
  if (oldStatus!=*refStatus || oldFT!=FinishTime) {
    clearSplitAnalysis = true;
  }

  if ((inTeam || !tUseStartPunch) && doApply)
    apply(sync, 0, false); //Post apply. Update start times.

  if (tCachedRunningTime != FinishTime - *refStartTime) {
    tCachedRunningTime = FinishTime - *refStartTime;
    clearSplitAnalysis = true;
  }

  if (time_limit > 0) {
    int rt = getRunningTime();
    if (rt > 0) {
      int overTime = rt - time_limit;
      if (overTime > 0) {
        tRogainingOvertime = overTime;
        tReduction = course->calculateReduction(overTime);
        tProblemDescription = L"Tidsavdrag: X po�ng.#" + itow(tReduction);
        tRogainingPoints = max(0, tRogainingPoints - tReduction);
      }
    }
  }



  // Clear split analysis data if necessary
  bool clear = splitTimes.size() != oldTimes.size() || clearSplitAnalysis;
  for (size_t k = 0; !clear && k<oldTimes.size(); k++) {
    if (splitTimes[k].time != oldTimes[k].time)
      clear = true;
  }

  if (clear) {
    normalizedSplitTimes.clear();
    if (Class)
      Class->clearSplitAnalysis();
  }

  if (doApply)
    storeTimes();
  if (Class && sync) {
    bool update = false;
    if (tInTeam) {
      int t1 = Class->getTotalLegLeaderTime(tLeg, false);
      int t2 = tInTeam->getLegRunningTime(tLeg, false);
      if (t2<=t1 && t2>0)
        update = true;

      int t3 = Class->getTotalLegLeaderTime(tLeg, true);
      int t4 = tInTeam->getLegRunningTime(tLeg, true);
      if (t4<=t3 && t4>0)
        update = true;
    }

    if (!update) {
      int t1 = Class->getBestLegTime(tLeg);
      int t2 = getRunningTime();
      if (t2<=t1 && t2>0)
        update = true;
    }
    if (update) {
      set<int> cls;
      cls.insert(Class->getId());
      oe->reEvaluateAll(cls, sync);
    }
  }
  return true;
}

void oRunner::clearOnChangedRunningTime() {
  if (tCachedRunningTime != FinishTime - tStartTime) {
    tCachedRunningTime = FinishTime - tStartTime;
    normalizedSplitTimes.clear();
    if (Class)
      Class->clearSplitAnalysis();
  }
}

void oRunner::doAdjustTimes(pCourse course) {
  if (!Card)
    return;

  assert(course->nControls == splitTimes.size());
  int adjustment = 0;
  oPunchList::iterator it = Card->punches.begin();

  adjustTimes.resize(splitTimes.size());
  for (int n = 0; n < course->nControls; n++) {
    pControl ctrl = course->Controls[n];
    if (!ctrl)
      continue;

    while (it != Card->punches.end() && !it->isUsed) {
      it->setTimeAdjust(adjustment);
      ++it;
    }

    int minTime = ctrl->getMinTime();
    if (ctrl->getStatus() == oControl::StatusNoTiming) {
      int t = 0;
      if (n>0 && splitTimes[n].time>0 && splitTimes[n-1].time>0) {
        t = splitTimes[n].time + adjustment - splitTimes[n-1].time;
      }
      else if (n == 0 && splitTimes[n].time>0) {
        t = splitTimes[n].time - tStartTime;
      }
      adjustment -= t;
    }
    else if (minTime>0) {
      int t = 0;
      if (n>0 && splitTimes[n].time>0 && splitTimes[n-1].time>0) {
        t = splitTimes[n].time + adjustment - splitTimes[n-1].time;
      }
      else if (n == 0 && splitTimes[n].time>0) {
        t = splitTimes[n].time - tStartTime;
      }
      int maxadjust = max(minTime-t, 0);
      adjustment += maxadjust;
    }

    if (it != Card->punches.end() && it->tMatchControlId == ctrl->getId()) {
      it->adjustTimeAdjust(adjustment);
      ++it;
    }

    adjustTimes[n] = adjustment;
    if (splitTimes[n].time>0)
      splitTimes[n].time += adjustment;
  }

  // Adjust remaining
  while (it != Card->punches.end()) {
    it->setTimeAdjust(adjustment);
    ++it;
  }

  FinishTime += adjustment;
}

bool oRunner::storeTimes() {
  bool updated = storeTimesAux(Class);
  if (tInTeam && tInTeam->Class && tInTeam->Class != Class)
    updated |= storeTimesAux(tInTeam->Class);

  return updated;
}

bool oRunner::storeTimesAux(pClass targetClass) {
  if (tInTeam) {
    if (tInTeam->getNumShortening(tLeg) > 0)
      return false;
  }
  else {
    if (getNumShortening() > 0)
      return false;
  }
  bool updated = false;
  //Store best time in class
  if (tInTeam && tInTeam->Class == targetClass) {
    if (targetClass && unsigned(tLeg)<targetClass->tLeaderTime.size()) {
      // Update for extra/optional legs
      int firstLeg = tLeg;
      int lastLeg = tLeg + 1;
      while(firstLeg>0 && targetClass->legInfo[firstLeg].isOptional())
        firstLeg--;
      int nleg = targetClass->legInfo.size();
      while(lastLeg<nleg && targetClass->legInfo[lastLeg].isOptional())
        lastLeg++;

      for (int leg = firstLeg; leg<lastLeg; leg++) {
        if (tStatus==StatusOK) {
          int &bt=targetClass->tLeaderTime[leg].bestTimeOnLeg;
          int rt=getRunningTime();
          if (rt > 0 && (bt == 0 || rt < bt)) {
            bt=rt;
            updated = true;
          }
        }
      }

      bool updateTotal = true;
      bool updateTotalInput = true;

      int basePLeg = firstLeg;
      while (basePLeg > 0 && targetClass->legInfo[basePLeg].isParallel())
        basePLeg--;

      int ix = basePLeg;
      while (ix < nleg && (ix == basePLeg || targetClass->legInfo[ix].isParallel()) ) {
        updateTotal = updateTotal && tInTeam->getLegStatus(ix, false)==StatusOK;
        updateTotalInput = updateTotalInput && tInTeam->getLegStatus(ix, true)==StatusOK;
        ix++;
      }

      if (updateTotal) {
        int rt = 0;
        int ix = basePLeg;
        while (ix < nleg && (ix == basePLeg || targetClass->legInfo[ix].isParallel()) ) {
          rt = max(rt, tInTeam->getLegRunningTime(ix, false));
          ix++;
        }

        for (int leg = firstLeg; leg<lastLeg; leg++) {
          int &bt=targetClass->tLeaderTime[leg].totalLeaderTime;
          if (rt > 0 && (bt == 0 || rt < bt)) {
            bt=rt;
            updated = true;
          }
        }
      }
      if (updateTotalInput) {
        //int rt=tInTeam->getLegRunningTime(tLeg, true);
        int rt = 0;
        int ix = basePLeg;
        while (ix < nleg && (ix == basePLeg || targetClass->legInfo[ix].isParallel()) ) {
          rt = max(rt, tInTeam->getLegRunningTime(ix, true));
          ix++;
        }
        for (int leg = firstLeg; leg<lastLeg; leg++) {
          int &bt=targetClass->tLeaderTime[leg].totalLeaderTimeInput;
          if (rt > 0 && (bt == 0 || rt < bt)) {
            bt=rt;
            updated = true;
          }
        }
      }
    }
  }
  else {
    if (targetClass && unsigned(tDuplicateLeg)<targetClass->tLeaderTime.size()) {
      if (tStatus==StatusOK) {
        int &bt=targetClass->tLeaderTime[tDuplicateLeg].bestTimeOnLeg;
        int rt=getRunningTime();
        if (rt > 0 && (bt==0 || rt<bt)) {
          bt=rt;
          updated = true;
        }
      }
      int &bt=targetClass->tLeaderTime[tDuplicateLeg].totalLeaderTime;
      int rt=getRaceRunningTime(tDuplicateLeg);
      if (rt>0 && (bt==0 || rt<bt)) {
        bt = rt;
        updated = true;
        targetClass->tLeaderTime[tDuplicateLeg].totalLeaderTimeInput = rt;
      }
    }
  }

  // Best input time
  if (targetClass && unsigned(tLeg)<targetClass->tLeaderTime.size()) {
    int &it = targetClass->tLeaderTime[tLeg].inputTime;
    if (inputTime > 0 && inputStatus == StatusOK && (it == 0 || inputTime < it) ) {
      it = inputTime;
      updated = true;
    }
  }

  if (targetClass && tStatus==StatusOK) {
    int rt = getRunningTime();
    pCourse pCrs = getCourse(false);
    if (pCrs && rt > 0) {
      map<int, int>::iterator res = targetClass->tBestTimePerCourse.find(pCrs->getId());
      if (res == targetClass->tBestTimePerCourse.end()) {
        targetClass->tBestTimePerCourse[pCrs->getId()] = rt;
        updated = true;
      }
      else if (rt < res->second) {
        res->second = rt;
        updated = true;
      }
    }
  }
  return updated;
}

int oRunner::getRaceRunningTime(int leg) const
{
  if (tParentRunner)
    return tParentRunner->getRaceRunningTime(leg);

  if (leg==-1)
    leg=multiRunner.size()-1;

  if (leg==0) {
    if (getTotalStatus() == StatusOK)
      return getRunningTime() + inputTime;
    else return 0;
  }
  leg--;

  if (unsigned(leg) < multiRunner.size() && multiRunner[leg]) {
    if (Class) {
      pClass pc=Class;
      LegTypes lt=pc->getLegType(leg);
      pRunner r=multiRunner[leg];

      switch(lt) {
        case LTNormal:
          if (r->statusOK()) {
            int dt=leg>0 ? r->getRaceRunningTime(leg)+r->getRunningTime():0;
            return max(r->getFinishTime()-tStartTime, dt); // ### Luckor, jaktstart???
          }
          else return 0;
        break;

        case LTSum:
          if (r->statusOK())
            return r->getRunningTime()+getRaceRunningTime(leg);
          else return 0;

        default:
          return 0;
      }
    }
    else return getRunningTime();
  }
  return 0;
}

bool oRunner::sortSplit(const oRunner &a, const oRunner &b)
{
  int acid=a.getClassId(true);
  int bcid=b.getClassId(true);
  if (acid!=bcid)
    return acid<bcid;
  else if (a.tempStatus != b.tempStatus)
    return a.tempStatus<b.tempStatus;
  else {
    if (a.tempStatus==StatusOK) {
      if (a.tempRT!=b.tempRT)
        return a.tempRT<b.tempRT;
    }
    return CompareString(LOCALE_USER_DEFAULT, 0,
                        a.tRealName.c_str(), a.tRealName.length(),
                        b.tRealName.c_str(), b.tRealName.length()) == CSTR_LESS_THAN;
  }
}

bool oRunner::operator<(const oRunner &c) const {
  const oClass * myClass = getClassRef(true);
  const oClass * cClass = c.getClassRef(true);
  if (!myClass || !cClass)
    return size_t(myClass)<size_t(cClass);
  else if (Class == cClass && Class->getClassStatus() != oClass::Normal)
    return CompareString(LOCALE_USER_DEFAULT, 0,
                         tRealName.c_str(), tRealName.length(),
                         c.tRealName.c_str(), c.tRealName.length()) == CSTR_LESS_THAN;

  if (oe->CurrentSortOrder==ClassStartTime) {
    if (myClass->Id != cClass->Id) {
      if (myClass->tSortIndex != cClass->tSortIndex)
        return myClass->tSortIndex < cClass->tSortIndex;
      else
        return myClass->Id < cClass->Id;
    }
    else if (tStartTime != c.tStartTime) {
      if (tStartTime <= 0 && c.tStartTime > 0)
        return false;
      else if (c.tStartTime <= 0 && tStartTime > 0)
        return true;
      else return tStartTime < c.tStartTime;
    }
    else {
      //if (StartNo != c.StartNo && !(getBib().empty() && c.getBib().empty()))
      //  return StartNo < c.StartNo;
      const wstring &b1 = getBib();
      const wstring &b2 = c.getBib();
      if (b1 != b2) {
        return compareBib(b1, b2);
      }
    }
  }
  else if (oe->CurrentSortOrder==ClassResult) {
    RunnerStatus stat = tStatus == StatusUnknown ? StatusOK : tStatus;
    RunnerStatus cstat = c.tStatus == StatusUnknown ? StatusOK : c.tStatus;
    
    if (myClass != cClass) 
      return myClass->tSortIndex < cClass->tSortIndex || (myClass->tSortIndex == cClass->tSortIndex && myClass->Id < cClass->Id);
    else if (tLegEquClass != c.tLegEquClass) 
      return tLegEquClass < c.tLegEquClass;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
    else if (stat != cstat)
      return RunnerStatusOrderMap[stat] < RunnerStatusOrderMap[cstat];
    else {
      if (stat==StatusOK) {
        if (Class->getNoTiming()) {
          return CompareString(LOCALE_USER_DEFAULT, 0,
                               tRealName.c_str(), tRealName.length(),
                               c.tRealName.c_str(), c.tRealName.length()) == CSTR_LESS_THAN;
        }
        int s = getNumShortening();
        int cs = c.getNumShortening();
        if (s != cs)
          return s < cs;

        int t=getRunningTime();
        if (t<=0)
          t = 3600*100;
        int ct=c.getRunningTime();
        if (ct<=0)
          ct = 3600*100;

        if (t!=ct)
          return t<ct;
      }
    }
  }
  else if (oe->CurrentSortOrder == ClassCourseResult) {
    if (myClass != cClass)
      return myClass->tSortIndex < cClass->tSortIndex;

    const pCourse crs1 = getCourse(false);
    const pCourse crs2 = c.getCourse(false);
    if (crs1 != crs2) {
      int id1 = crs1 ? crs1->getId() : 0;
      int id2 = crs2 ? crs2->getId() : 0;
      return id1 < id2;
    }
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
    else if (tStatus != c.tStatus)
      return RunnerStatusOrderMap[tStatus] < RunnerStatusOrderMap[c.tStatus];
    else {
      if (tStatus==StatusOK) {
        if (Class->getNoTiming()) {
          return CompareString(LOCALE_USER_DEFAULT, 0,
                               tRealName.c_str(), tRealName.length(),
                               c.tRealName.c_str(), c.tRealName.length()) == CSTR_LESS_THAN;
        }
        int s = getNumShortening();
        int cs = c.getNumShortening();
        if (s != cs)
          return s < cs;

        int t=getRunningTime();
        int ct=c.getRunningTime();
        if (t!=ct)
          return t<ct;
      }
    }
  }
  else if (oe->CurrentSortOrder==SortByName) {
    return CompareString(LOCALE_USER_DEFAULT, 0,
                      tRealName.c_str(), tRealName.length(),
                      c.tRealName.c_str(), c.tRealName.length()) == CSTR_LESS_THAN;
  }
  else if (oe->CurrentSortOrder==SortByLastName) {
    wstring a = getFamilyName();
    wstring b = c.getFamilyName();
    if (a.empty() && !b.empty())
      return false;
    else if (b.empty() && !a.empty())
      return true;
    else if (a != b) {
      return CompareString(LOCALE_USER_DEFAULT, 0,
                           a.c_str(), a.length(),
                           b.c_str(), b.length()) == CSTR_LESS_THAN;
    }
    a = getGivenName();
    b = c.getGivenName();
    if (a != b) {
      return CompareString(LOCALE_USER_DEFAULT, 0,
                           a.c_str(), a.length(),
                           b.c_str(), b.length()) == CSTR_LESS_THAN;
    }
  }
  else if (oe->CurrentSortOrder==SortByFinishTime) {
    if (tStatus != c.tStatus)
      return RunnerStatusOrderMap[tStatus] < RunnerStatusOrderMap[c.tStatus];
    else {
      int ft = getFinishTimeAdjusted();
      int cft = c.getFinishTimeAdjusted();
      if (tStatus==StatusOK && ft != cft)
        return ft < cft;
    }
  }
  else if (oe->CurrentSortOrder==SortByFinishTimeReverse) {
    int ft = getFinishTimeAdjusted();
    int cft = c.getFinishTimeAdjusted();
    if (ft != cft)
      return ft > cft;
  }
  else if (oe->CurrentSortOrder == ClassFinishTime){
    if (myClass != cClass)
      return myClass->tSortIndex < cClass->tSortIndex || (myClass->tSortIndex == cClass->tSortIndex && myClass->Id < cClass->Id);
    if (tStatus != c.tStatus)
      return RunnerStatusOrderMap[tStatus] < RunnerStatusOrderMap[c.tStatus];
    else{
      int ft = getFinishTimeAdjusted();
      int cft = c.getFinishTimeAdjusted();
      if (tStatus==StatusOK && ft != cft)
        return ft<cft;
    }
  }
  else if (oe->CurrentSortOrder==SortByStartTime){
    if (tStartTime < c.tStartTime)
      return true;
    else  if (tStartTime > c.tStartTime)
      return false;
  }
  else if (oe->CurrentSortOrder == SortByEntryTime) {
    auto dci = getDCI(), cdci = c.getDCI();
    int ed = dci.getInt("EntryDate");
    int ced = cdci.getInt("EntryDate");
    if (ed != ced)
      return ed > ced;
    int et = dci.getInt("EntryTime");
    int cet = cdci.getInt("EntryTime");
    if (et != cet)
      return et > cet;
  }
  else if (oe->CurrentSortOrder == ClassPoints) {
    if (myClass != cClass)
      return myClass->tSortIndex < cClass->tSortIndex || (myClass->tSortIndex == cClass->tSortIndex && myClass->Id < cClass->Id);
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
    else if (tStatus != c.tStatus)
      return RunnerStatusOrderMap[tStatus] < RunnerStatusOrderMap[c.tStatus];
    else {
      if (tStatus==StatusOK) {
        if (tRogainingPoints != c.tRogainingPoints)
          return tRogainingPoints > c.tRogainingPoints;
        int t=getRunningTime();
        int ct=c.getRunningTime();
        if (t != ct)
          return t < ct;
      }
    }
  }
  else if (oe->CurrentSortOrder==ClassTotalResult) {
    if (myClass != cClass)
      return myClass->tSortIndex < cClass->tSortIndex || (myClass->tSortIndex == cClass->tSortIndex && myClass->Id < cClass->Id);
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
    else {
      RunnerStatus s1, s2;
      s1 = getTotalStatus();
      s2 = c.getTotalStatus();
      if (s1 != s2)
        return s1 < s2;
      else if (s1 == StatusOK) {
        if (Class->getNoTiming()) {
          return CompareString(LOCALE_USER_DEFAULT, 0,
                               tRealName.c_str(), tRealName.length(),
                               c.tRealName.c_str(), c.tRealName.length()) == CSTR_LESS_THAN;
        }
        int t = getTotalRunningTime(FinishTime, true);
        int ct = c.getTotalRunningTime(c.FinishTime, true);
        if (t!=ct)
          return t < ct;
      }
    }
  }
  else if(oe->CurrentSortOrder == CoursePoints){
    const pCourse crs1 = getCourse(false);
    const pCourse crs2 = c.getCourse(false);
    if (crs1 != crs2) {
      int id1 = crs1 ? crs1->getId() : 0;
      int id2 = crs2 ? crs2->getId() : 0;
      return id1 < id2;
    }
		if(status != c.status)
			return unsigned(status-1) < unsigned(c.status-1);
		else {
			if (status==StatusOK) {
        if(tRogainingPoints != c.tRogainingPoints)
			    return tRogainingPoints > c.tRogainingPoints;
				int t=getRunningTime();
				int ct=c.getRunningTime();
				if (t != ct)
					return t < ct;
			}			
			return tRealName  <c.tRealName;
		}
	}
  else if (oe->CurrentSortOrder == CourseResult) {
    const pCourse crs1 = getCourse(false);
    const pCourse crs2 = c.getCourse(false);
    if (crs1 != crs2) {
      int id1 = crs1 ? crs1->getId() : 0;
      int id2 = crs2 ? crs2->getId() : 0;
      return id1 < id2;
    }
    else if (tStatus != c.tStatus)
      return RunnerStatusOrderMap[tStatus] < RunnerStatusOrderMap[c.tStatus];
    else {
      if (tStatus==StatusOK) {
  
        int s = getNumShortening();
        int cs = c.getNumShortening();
        if (s != cs)
          return s < cs;

        int t=getRunningTime();
        int ct=c.getRunningTime();
        if (t != ct) {
          return t<ct;
        }
      }
    }
  }
  else if (oe->CurrentSortOrder==ClassStartTimeClub) {
    if (myClass != cClass)
      return myClass->tSortIndex < cClass->tSortIndex || (myClass->tSortIndex == cClass->tSortIndex && myClass->Id < cClass->Id);
    else if (tStartTime != c.tStartTime) {
      if (tStartTime <= 0 && c.tStartTime > 0)
        return false;
      else if (c.tStartTime <= 0 && tStartTime > 0)
        return true;
      else return tStartTime < c.tStartTime;
    }
    else if (Club != c.Club) {
      return getClub() < c.getClub();
    }
  }
  else if (oe->CurrentSortOrder==ClassTeamLeg) {
    if (myClass->Id != cClass->Id)
      return myClass->tSortIndex < cClass->tSortIndex || (myClass->tSortIndex == cClass->tSortIndex && myClass->Id < cClass->Id);
    else if (tInTeam != c.tInTeam) {
      if (tInTeam == 0)
        return true;  
      else if (c.tInTeam == 0)
        return false;
      if (tInTeam->StartNo != c.tInTeam->StartNo)
        return tInTeam->StartNo < c.tInTeam->StartNo;
      else
        return tInTeam->sName < c.tInTeam->sName;
    }
    else if (tInTeam && tLeg != c.tLeg)
      return tLeg < c.tLeg;
    else if (tStartTime != c.tStartTime) {
      if (tStartTime <= 0 && c.tStartTime > 0)
        return false;
      else if (c.tStartTime <= 0 && tStartTime > 0)
        return true;
      else return tStartTime < c.tStartTime;
    }
    else {
      const wstring &b1 = getBib();
      const wstring &b2 = c.getBib();
      if (StartNo != c.StartNo && b1 != b2)
        return StartNo < c.StartNo;
    }
  }
  
  return CompareString(LOCALE_USER_DEFAULT, 0,
                      tRealName.c_str(), tRealName.length(),
                      c.tRealName.c_str(), c.tRealName.length()) == CSTR_LESS_THAN;

}

void oAbstractRunner::setClub(const wstring &clubName)
{
  pClub pc=Club;
  Club = clubName.empty() ? 0 : oe->getClubCreate(0, clubName);
  if (pc != Club) {
    updateChanged();
    if (Class) {
      // Vacant clubs have special logic
      getClassRef(true)->tResultInfo.clear();
    }
    if (Club && Club->isVacant()) { // Clear entry date/time for vacant
      getDI().setInt("EntryDate", 0);
      getDI().setInt("EntryTime", 0);
    }
  }
}

pClub oAbstractRunner::setClubId(int clubId)
{
  pClub pc=Club;
  Club = oe->getClub(clubId);
  if (pc != Club) {
    updateChanged();
    if (Class) {
      // Vacant clubs have special logic
      Class->tResultInfo.clear();
    }
    if (Club && Club->isVacant()) { // Clear entry date/time for vacant
      getDI().setInt("EntryDate", 0);
      getDI().setInt("EntryTime", 0);
    }
  }
  return Club;
}

void oRunner::setClub(const wstring &clubName)
{
  if (tParentRunner)
    tParentRunner->setClub(clubName);
  else {
    oAbstractRunner::setClub(clubName);

    for (size_t k=0;k<multiRunner.size();k++)
      if (multiRunner[k] && multiRunner[k]->Club!=Club) {
        multiRunner[k]->Club = Club;
        multiRunner[k]->updateChanged();
      }
  }
}

pClub oRunner::setClubId(int clubId)
{
  if (tParentRunner)
    tParentRunner->setClubId(clubId);
  else {
    oAbstractRunner::setClubId(clubId);

    for (size_t k=0;k<multiRunner.size();k++)
      if (multiRunner[k] && multiRunner[k]->Club!=Club) {
        multiRunner[k]->Club = Club;
        multiRunner[k]->updateChanged();
      }
  }
  return Club;
}


void oAbstractRunner::setStartNo(int no, bool tmpOnly) {
  if (tmpOnly) {
    tmpStore.startNo = no;
    return;
  }

  if (no!=StartNo) {
    if (oe)
      oe->bibStartNoToRunnerTeam.clear();
    StartNo=no;
    updateChanged();
  }
}

void oRunner::setStartNo(int no, bool tmpOnly)
{
  if (tParentRunner)
    tParentRunner->setStartNo(no, tmpOnly);
  else {
    oAbstractRunner::setStartNo(no, tmpOnly);

    for (size_t k=0;k<multiRunner.size();k++)
      if (multiRunner[k])
        multiRunner[k]->oAbstractRunner::setStartNo(no, tmpOnly);
  }
}

int oRunner::getPlace() const
{
  return tPlace;
}

int oRunner::getCoursePlace() const
{
  return tCoursePlace;
}


int oRunner::getTotalPlace() const
{
  if (tInTeam)
    return tInTeam->getLegPlace(tLeg, true);
  else
    return tTotalPlace;
}

wstring oAbstractRunner::getPlaceS() const
{
  wchar_t bf[16];
  int p=getPlace();
  if (p>0 && p<10000){
    _itow_s(p, bf, 16, 10);
    return bf;
  }
  else return _EmptyWString;
}

wstring oAbstractRunner::getPrintPlaceS(bool withDot) const
{
  wchar_t bf[16];
  int p=getPlace();
  if (p>0 && p<10000){
    if (withDot) {
      _itow_s(p, bf, 16, 10);
      return wstring(bf)+L".";
    }
    else
      return itow(p);
  }
  else return _EmptyWString;
}

wstring oAbstractRunner::getTotalPlaceS() const
{
  wchar_t bf[16];
  int p=getTotalPlace();
  if (p>0 && p<10000){
    _itow_s(p, bf, 16, 10);
    return bf;
  }
  else return _EmptyWString;
}

wstring oAbstractRunner::getPrintTotalPlaceS(bool withDot) const
{
  wchar_t bf[16];
  int p=getTotalPlace();
  if (p>0 && p<10000){
    if (withDot) {
      _itow_s(p, bf, 16, 10);
      return wstring(bf)+L".";
    }
    else
      return itow(p);
  }
  else return _EmptyWString;
}
wstring oRunner::getGivenName() const
{
  return ::getGivenName(sName);
}

wstring oRunner::getFamilyName() const
{
  return ::getFamilyName(sName);
}

void oRunner::setCardNo(int cno, bool matchCard, bool updateFromDatabase)
{
  if (cno!=CardNo){
    int oldNo = CardNo;
    CardNo=cno;

		if (static_cast<oExtendedEvent*>(oe)->isRentedCard(cno))
			getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));

    oFreePunch::rehashPunches(*oe, oldNo, 0);
    oFreePunch::rehashPunches(*oe, CardNo, 0);

    if (matchCard && !Card) {
      pCard c=oe->getCardByNumber(cno);

      if (c && !c->tOwner) {
        vector<int> mp;
        addPunches(c, mp);
      }
    }

    if (!updateFromDatabase)
      updateChanged();
  }
}

int oRunner::setCard(int cardId)
{
  pCard c=cardId ? oe->getCard(cardId) : 0;
  int oldId=0;

  if (Card!=c) {
    if (Card) {
      oldId=Card->getId();
      Card->tOwner=0;
    }
    if (c) {
      if (c->tOwner) {
        pRunner otherR=c->tOwner;
        assert(otherR!=this);
        otherR->Card=0;
        otherR->updateChanged();
        otherR->setStatus(StatusUnknown, true, false);
        otherR->synchronize(true);
      }
      c->tOwner=this;
      CardNo=c->cardNo;
    }
    Card=c;
    vector<int> mp;
    evaluateCard(true, mp);
    updateChanged();
    synchronize(true);
  }
  return oldId;
}

void oAbstractRunner::setName(const wstring &n, bool manualUpdate)
{
  wstring tn = trim(n);
  if (tn.empty())
    throw std::exception("Tomt namn �r inte till�tet.");
  if (tn != sName){
    sName.swap(tn);
    if (manualUpdate)
      setFlag(FlagUpdateName, true);
    updateChanged();
  }
}

void oRunner::setName(const wstring &in, bool manualUpdate)
{
  wstring n = trim(in);
  
  if (n.empty())
    throw std::exception("Tomt namn �r inte till�tet.");
  for (size_t k = 0; k < n.length(); k++) {
    if (iswspace(n[k]))
      n[k] = ' ';
  }
  if (n.length() <= 4 || n == lang.tl("N.N."))
    manualUpdate = false; // Never consider default names manual

  if (tParentRunner)
    tParentRunner->setName(n, manualUpdate);
  else {
    wstring oldName = sName;
    wstring oldRealName = tRealName;
    wstring newRealName;
    getRealName(n, newRealName);
    if (newRealName != tRealName || n != sName) {
      sName = n;
      tRealName = newRealName;

      if (manualUpdate)
        setFlag(FlagUpdateName, true);

      updateChanged();
    }

    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k] && n!=multiRunner[k]->sName) {
        multiRunner[k]->sName = n;
        multiRunner[k]->tRealName = tRealName;
        multiRunner[k]->updateChanged();
      }
    }
    if (tInTeam && Class && Class->isSingleRunnerMultiStage()) {
      if (tInTeam->sName == oldName || tInTeam->sName == oldRealName)
        tInTeam->setName(tRealName, manualUpdate);
    }
  }
}

const wstring &oRunner::getName() const {
  return tRealName;
}

const wstring &oRunner::getNameLastFirst() const {
  if (sName.find_first_of(',') != sName.npos)
    return sName;  // Already "Fiske, Eric"
  if (sName.find_first_of(' ') == sName.npos)
    return sName; // No space "Vacant", "Eric"
  
  wstring &res = StringCache::getInstance().wget();
  res = getFamilyName() + L", " + getGivenName();
  return res;
}

void oRunner::getRealName(const wstring &input, wstring &output) {
  size_t comma = input.find_first_of(',');
  if (comma == string::npos)
    output = input;
  else 
    output = trim(input.substr(comma+1) + L" " + input.substr(0, comma));
}

bool oAbstractRunner::setStatus(RunnerStatus st, bool updateSource, bool tmpOnly, bool recalculate) {
  assert(!(updateSource && tmpOnly));
  if (tmpOnly) {
    tmpStore.status = st;
    return false;
  }
  bool ch = false;
  if (tStatus!=st) {
    ch = true;
    tStatus=st;

    if (Class) {
      Class->clearCache(recalculate);
    }
  }

  if (st != status) {
    status = st;
    if (updateSource)
      updateChanged();
    else
      changedObject();
  }

  return ch;
}

int oAbstractRunner::getPrelRunningTime() const
{
  if (FinishTime>0 && tStatus!=StatusDNS && tStatus != StatusCANCEL && tStatus!=StatusDNF && tStatus!=StatusNotCompetiting)
    return getRunningTime();
  else if (tStatus==StatusUnknown)
    return oe->getComputerTime()-tStartTime;
  else return 0;
}

wstring oAbstractRunner::getPrelRunningTimeS() const
{
  int rt=getPrelRunningTime();
  return formatTime(rt);
}

oDataContainer &oRunner::getDataBuffers(pvoid &data, pvoid &olddata, pvectorstr &strData) const {
  data = (pvoid)oData;
  olddata = (pvoid)oDataOld;
  strData = 0;
  return *oe->oRunnerData;
}

void oEvent::getRunners(int classId, int courseId, vector<pRunner> &r, bool sort) {
  if (sort) {
    synchronizeList(oLRunnerId);
    sortRunners(SortByName);
  }
  r.clear();
  if (Classes.size() > 0)
    r.reserve((Runners.size()*min<size_t>(Classes.size(), 4)) / Classes.size());

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    if (courseId > 0) {
      pCourse pc = it->getCourse(false);
      if (pc == 0 || pc->getId() != courseId)
        continue;
    }

    if (classId <= 0 || it->getClassId(true) == classId)
      r.push_back(&*it);
  }
}

void oEvent::getRunnersByCard(int cardNo, vector<pRunner> &r) {
  synchronizeList(oLRunnerId);
  sortRunners(SortByName);
  r.clear();

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->getCardNo() == cardNo)
      r.push_back(&*it);
  }
}


pRunner oEvent::getRunner(int Id, int stage) const
{
  pRunner value;

  if (runnerById.lookup(Id, value) && value) {
    if (value->isRemoved())
      return 0;
    assert(value->Id == Id);
    if (stage==0)
      return value;
    else if (unsigned(stage)<=value->multiRunner.size())
      return value->multiRunner[stage-1];
  }
  return 0;
}

pRunner oRunner::nextNeedReadout() const {
  if (tInTeam) {
    // For a runner in a team, first the team for the card
    for (size_t k = 0; k < tInTeam->Runners.size(); k++) {
      pRunner tr = tInTeam->Runners[k];
      if (tr && tr->getCardNo() == CardNo && !tr->Card && !tr->statusOK())
        return tr;
    }
  }

  if (!Card || Card->cardNo!=CardNo || Card->isConstructedFromPunches()) //-1 means card constructed from punches
    return pRunner(this);

  for (size_t k=0;k<multiRunner.size();k++) {
    if (multiRunner[k] && (!multiRunner[k]->Card ||
           multiRunner[k]->Card->cardNo!=CardNo))
      return multiRunner[k];
  }
  return 0;
}

void oEvent::setupCardHash(bool clear) {
  if (clear) {
    cardHash.clear();
  }
  else {
    assert(cardHash.empty());
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->isRemoved())
        continue;
      if (it->CardNo != 0)
        cardHash.insert(make_pair(it->CardNo, &*it));
    }
  }
}

typedef unordered_multimap<int, pRunner>::const_iterator hashConstIter;

pRunner oEvent::getRunnerByCardNo(int cardNo, int time, bool onlyWithNoCard, bool ignoreRunnersWithNoStart) const
{
  oRunnerList::const_iterator it;
  vector<pRunner> cand;
  bool forceRet = false;
  if (!onlyWithNoCard && !cardHash.empty()) {
    forceRet = true;
    pair<hashConstIter, hashConstIter> range = cardHash.equal_range(cardNo);
    if (range.first != range.second) {
      hashConstIter t = range.first;
      ++t;
      if (t == range.second) {
        pRunner r = range.first->second;
        assert(r->getCardNo() == cardNo);
        if (ignoreRunnersWithNoStart && (r->getStatus() == StatusDNS || r->getStatus() == StatusCANCEL))
          return 0;
        if (r->getStatus() == StatusNotCompetiting)
          return 0;
        return r; // Only one runner with this card
      }
    }

    for (hashConstIter it = range.first; it != range.second; ++it) {
      pRunner r = it->second;
      assert(r->getCardNo() == cardNo);
      if (ignoreRunnersWithNoStart && (r->getStatus() == StatusDNS || r->getStatus() == StatusCANCEL))
        continue;
      if (r->getStatus() == StatusNotCompetiting)
        continue;
      if (!r->isRemoved())
        cand.push_back(r);
    }
  }
  else {
    if (time <= 0) { //No time specified. Card readout search

      pRunner secondTry = 0;
      //First try runners with no card read or a different card read.
      for (it=Runners.begin(); it != Runners.end(); ++it) {
        if (it->isRemoved())
          continue;
        if (ignoreRunnersWithNoStart && (it->getStatus() == StatusDNS || it->getStatus() == StatusCANCEL))
          continue;
        if (it->getStatus() == StatusNotCompetiting)
          continue;

        pRunner ret;
        if (it->CardNo == cardNo && (ret = it->nextNeedReadout()) != 0) {
          if (!it->skip())
            return ret;
          else if (secondTry == 0 || secondTry->tLeg > ret->tLeg)
            secondTry = ret;
        }
      }
      if (secondTry)
        return secondTry;
    }
    else {
      for (it=Runners.begin(); it != Runners.end(); ++it) {
        pRunner r = pRunner(&*it);
        if (r->CardNo != cardNo || r->isRemoved())
          continue;
        if (ignoreRunnersWithNoStart && (r->getStatus() == StatusDNS || r->getStatus() == StatusCANCEL))
          continue;
        if (r->getStatus() == StatusNotCompetiting)
          continue;
        cand.push_back(r);
      }
    }
  }
  pRunner bestR = 0;
  const int K = 3600*24;
  int dist = 10*K;
  for (size_t k = 0; k < cand.size(); k++) {
    pRunner r = cand[k];
    if (time <= 0)
      return r; // No time specified.
    //int start = r->getStartTime();
    //int finish = r->getFinishTime();
    int start = r->getStartTime();
    int finish = r->getFinishTime();
    if (r->getCard()) {
      pair<int, int> cc = r->getCard()->getTimeRange();
      if (cc.first > 0)
        start = min(start, cc.first);
      if (cc.second > 0)
        finish = max(finish, cc.second);
    }
    start = max(0, start - 3 * 60); // Allow some extra time before start

    if (start > 0 && finish > 0 && time >= start && time <= finish)
      return r;
    int d = 3*K;
    if (start > 0 && finish > 0 && start<finish) {
      if (time < start)
        d += K + (start-time);
      else if (time > finish)
        d += K + (time-finish);
    }
    else {
      if (start > 0) {
        if (time < start)
          d = K + start-time;
        else
          d = time - start;
      }
      if (finish > 0) {
        if (time > finish)
          d += K + time - finish;
      }
    }
    if (d < dist) {
      bestR = r;
      dist = d;
    }
  }

  if (bestR != 0 || forceRet)
    return bestR;

  if (!onlyWithNoCard) 	{
    //Then try all runners.
    for (it=Runners.begin(); it != Runners.end(); ++it){
      if (ignoreRunnersWithNoStart && (it->getStatus() == StatusDNS || it->getStatus() == StatusCANCEL))
        continue;
      if (it->getStatus() == StatusNotCompetiting)
        continue;
      if (!it->isRemoved() && it->CardNo==cardNo) {
        pRunner r = it->nextNeedReadout();
        return r ? r : pRunner(&*it);
      }
    }
  }

  return 0;
}

void oEvent::getRunnersByCardNo(int cardNo, bool ignoreRunnersWithNoStart, bool skipDuplicates, vector<pRunner> &out) const {
  out.clear();
  if (!cardHash.empty()) {
    pair<hashConstIter, hashConstIter> range = cardHash.equal_range(cardNo);
    
    for (hashConstIter it = range.first; it != range.second; ++it) {
      pRunner r = it->second;
      assert(r->getCardNo() == cardNo);
      if (ignoreRunnersWithNoStart && (r->getStatus() == StatusDNS || r->getStatus() == StatusCANCEL))
        continue;
      if (skipDuplicates && r->getRaceNo() != 0)
        continue;
      if (r->getStatus() == StatusNotCompetiting)
        continue;
      if (!r->isRemoved())
        out.push_back(r);
    }
  }
  else {
    for (oRunnerList::const_iterator it=Runners.begin(); it != Runners.end(); ++it) {
      if (it->CardNo != cardNo)
        continue;
      if (ignoreRunnersWithNoStart && (it->getStatus() == StatusDNS || it->getStatus() == StatusCANCEL))
        continue;
      if (it->getStatus() == StatusNotCompetiting)
        continue;
      if (skipDuplicates && it->getRaceNo() != 0)
        continue;
      if (!it->isRemoved())
        out.push_back(pRunner(&*it));
    }
  }
}

int oRunner::getRaceIdentifier() const {
  if (tParentRunner)
    return tParentRunner->getRaceIdentifier();// A unique person has a unique race identifier, even if the race is "split" into several

  int stored = getDCI().getInt("RaceId");
  if (stored != 0)
    return stored;

  if (!tInTeam)
    return 1000000 + (Id&0xFFFFFFF) * 2;//Even
  else
    return 1000000 * (tLeg+1) + (tInTeam->Id & 0xFFFFFFF) * 2 + 1;//Odd
}

static int getEncodedBib(const wstring &bib) {
  int enc = 0;
  for (size_t j = 0; j < bib.length(); j++) { //WCS
    int x = toupper(bib[j])-32;
    if (x<0)
      return 0; // Not a valid bib
    enc = enc * 97 - x;
  }
  return enc;
}

int oAbstractRunner::getEncodedBib() const {
  return ::getEncodedBib(getBib());
}


typedef multimap<int, oAbstractRunner*>::iterator BSRTIterator;

pRunner oEvent::getRunnerByBibOrStartNo(const wstring &bib, bool findWithoutCardNo) const {
  if (bib.empty() || bib == L"0")
    return 0;

  if (bibStartNoToRunnerTeam.empty()) {
    for (oTeamList::const_iterator tit = Teams.begin(); tit != Teams.end(); ++tit) {
      const oTeam &t=*tit;
      if (t.skip())
        continue;

      int sno = t.getStartNo();
      if (sno != 0)
        bibStartNoToRunnerTeam.insert(make_pair(sno, (oAbstractRunner *)&t));
      int enc = t.getEncodedBib();
      if (enc != 0)
        bibStartNoToRunnerTeam.insert(make_pair(enc, (oAbstractRunner *)&t));
    }

    for (oRunnerList::const_iterator it=Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip())
        continue;
       const oRunner &t=*it;

      int sno = t.getStartNo();
      if (sno != 0)
        bibStartNoToRunnerTeam.insert(make_pair(sno, (oAbstractRunner *)&t));
      int enc = t.getEncodedBib();
      if (enc != 0)
        bibStartNoToRunnerTeam.insert(make_pair(enc, (oAbstractRunner *)&t));
    }
  }

  int sno = _wtoi(bib.c_str());

  pair<BSRTIterator, BSRTIterator> res;
  if (sno > 0) {
    // Require that a bib starts with numbers
    int bibenc = getEncodedBib(bib);
    res = bibStartNoToRunnerTeam.equal_range(bibenc);
    if (res.first == res.second)
      res = bibStartNoToRunnerTeam.equal_range(sno); // Try startno instead

    for(BSRTIterator it = res.first; it != res.second; ++it) {
      oAbstractRunner *pa = it->second;
      if (pa->isRemoved())
        continue;

      if (typeid(*pa)==typeid(oRunner)) {
        oRunner &r = dynamic_cast<oRunner &>(*pa);
        if (r.getStartNo()==sno || stringMatch(r.getBib(), bib)) {
          if (findWithoutCardNo) {
            if (r.getCardNo() == 0 && r.needNoCard() == false)
              return &r;
          }
          else {
            if (r.getNumMulti()==0 || r.tStatus == StatusUnknown)
              return &r;
            else {
              for(int race = 0; race < r.getNumMulti(); race++) {
                pRunner r2 = r.getMultiRunner(race);
                if (r2 && r2->tStatus == StatusUnknown)
                  return r2;
              }
              return &r;
            }
          }
        }
      }
      else {
        oTeam &t = dynamic_cast<oTeam &>(*pa);
        if (t.getStartNo()==sno || stringMatch(t.getBib(), bib)) {
          if (!findWithoutCardNo) {
            for (int leg=0; leg<t.getNumRunners(); leg++) {
              if (t.Runners[leg] && t.Runners[leg]->getCardNo() > 0 && t.Runners[leg]->getStatus()==StatusUnknown)
                return t.Runners[leg];
            }
          }
          else {
            for (int leg=0; leg<t.getNumRunners(); leg++) {
              if (t.Runners[leg] && t.Runners[leg]->getCardNo() == 0 && t.Runners[leg]->needNoCard() == false)
                return t.Runners[leg];
            }
          }
        }
      }
    }
  }
  return 0;
}

pRunner oEvent::getRunnerByName(const wstring &pname, const wstring &pclub) const
{
  oRunnerList::const_iterator it;
  vector<pRunner> cnd;

  for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (!it->skip() && it->matchName(pname)) {
      if (pclub.empty() || pclub==it->getClub())
        cnd.push_back(pRunner(&*it));
    }
  }

  if (cnd.size() == 1)
    return cnd[0]; // Only return if uniquely defined.

  return 0;
}

void oEvent::fillRunners(gdioutput &gdi, const string &id, bool longName, int filter)
{
  vector< pair<wstring, size_t> > d;
  oe->fillRunners(d, longName, filter, unordered_set<int>());
  gdi.addItem(id, d);
}

const vector< pair<wstring, size_t> > &oEvent::fillRunners(vector< pair<wstring, size_t> > &out,
                                                           bool longName, int filter,
                                                           const unordered_set<int> &personFilter)
{
  const bool showAll = (filter & RunnerFilterShowAll) == RunnerFilterShowAll;
  const bool noResult = (filter & RunnerFilterOnlyNoResult) ==  RunnerFilterOnlyNoResult;
  const bool withResult = (filter & RunnerFilterWithResult) ==  RunnerFilterWithResult;
  const bool compact = (filter & RunnerCompactMode) == RunnerCompactMode;

  synchronizeList(oLRunnerId);
  oRunnerList::iterator it;
  int lVacId = getVacantClubIfExist(false);
  if (getNameMode() == LastFirst)
    CurrentSortOrder = SortByLastName;
  else
    CurrentSortOrder = SortByName;
  Runners.sort();
  out.clear();
  if (personFilter.empty())
    out.reserve(Runners.size());
  else
    out.reserve(personFilter.size());

  wchar_t bf[512];
  const bool usePersonFilter = !personFilter.empty();

  if (longName) {
    for (it=Runners.begin(); it != Runners.end(); ++it) {
      if (noResult && (it->Card || it->FinishTime>0))
        continue;
      if (withResult && !it->Card && it->FinishTime == 0)
        continue;
      if (usePersonFilter && personFilter.count(it->Id) == 0)
        continue;
      if (!it->skip() || (showAll && !it->isRemoved())) {
        if (compact) {
          swprintf_s(bf, L"%s, %s (%s)", it->getNameAndRace(true).c_str(),
                                         it->getClub().c_str(),
                                         it->getClass(true).c_str());

        } else {
          swprintf_s(bf, L"%s\t%s\t%s", it->getNameAndRace(true).c_str(),
                                        it->getClass(true).c_str(),
                                        it->getClub().c_str());
        }
        out.push_back(make_pair(bf, it->Id));
      }
    }
  }
  else {
    for (it=Runners.begin(); it != Runners.end(); ++it) {
      if (noResult && (it->Card || it->FinishTime>0))
        continue;
      if (withResult && !it->Card && it->FinishTime == 0)
        continue;
      if (usePersonFilter && personFilter.count(it->Id) == 0)
        continue;

      if (!it->skip() || (showAll && !it->isRemoved())) {
        if ( it->getClubId() != lVacId || lVacId == 0)
          out.push_back(make_pair(it->getUIName(), it->Id));
        else {
          swprintf_s(bf, L"%s (%s)", it->getUIName().c_str(), it->getClass(true).c_str());
          out.push_back(make_pair(bf, it->Id));
        }
      }
    }
  }
  return out;
}

void oRunner::resetPersonalData()
{
  oDataInterface di = getDI();
  di.setInt("BirthYear", 0);
  di.setString("Nationality", L"");
  di.setString("Country", L"");
  di.setInt64("ExtId", 0);
}

wstring oRunner::getNameAndRace(bool userInterface) const
{
  if (tDuplicateLeg>0 || multiRunner.size()>0) {
    wchar_t bf[16];
    swprintf_s(bf, L" (%d)", getRaceNo()+1);
    if (userInterface)
      return getUIName() + bf;
    return getName()+bf;
  }
  else if (userInterface)
    return getUIName();
  else return getName();
}

pRunner oRunner::getMultiRunner(int race) const
{
  if (race==0) {
    if (!tParentRunner)
      return pRunner(this);
    else return tParentRunner;
  }

  const vector<pRunner> &mr = tParentRunner ? tParentRunner->multiRunner : multiRunner;

  if (unsigned(race-1)>=mr.size()) {
    assert(tParentRunner);
    return 0;
  }

  return mr[race-1];
}

void oRunner::createMultiRunner(bool createMaster, bool sync)
{
  if (tDuplicateLeg)
    return; //Never allow chains.

  if (multiRunnerId.size()>0) {
    multiRunner.resize(multiRunnerId.size() - 1);
    for (size_t k=0;k<multiRunner.size();k++) {
      multiRunner[k]=oe->getRunner(multiRunnerId[k], 0);
      if (multiRunner[k]) {
        if (multiRunner[k]->multiRunnerId.size() > 1 || !multiRunner[k]->multiRunner.empty())
          multiRunner[k]->markForCorrection();

        multiRunner[k]->multiRunner.clear(); //Do not allow chains
        multiRunner[k]->multiRunnerId.clear();
        multiRunner[k]->tDuplicateLeg = k+1;
        multiRunner[k]->tParentRunner = this;
        multiRunner[k]->CardNo=0;

        if (multiRunner[k]->Id != multiRunnerId[k])
          markForCorrection();
      }
      else if (multiRunnerId[k]>0)
        markForCorrection();

      assert(multiRunner[k]);
    }
    multiRunnerId.clear();
  }

  if (!Class || !createMaster)
    return;

  int ndup=0;

  if (!tInTeam)
    ndup=Class->getNumMultiRunners(0);
  else
    ndup=Class->getNumMultiRunners(tLeg);

  bool update = false;

  vector<int> toRemove;

  for (size_t k = ndup-1; k<multiRunner.size();k++) {
    if (multiRunner[k] && multiRunner[k]->getStatus()==StatusUnknown) {
      toRemove.push_back(multiRunner[k]->getId());
      multiRunner[k]->tParentRunner = 0;
      if (multiRunner[k]->tInTeam && size_t(multiRunner[k]->tLeg)<multiRunner[k]->tInTeam->Runners.size()) {
        if (multiRunner[k]->tInTeam->Runners[multiRunner[k]->tLeg]==multiRunner[k])
          multiRunner[k]->tInTeam->Runners[multiRunner[k]->tLeg] = 0;
      }
    }
  }

  multiRunner.resize(ndup-1);
  for(int k=1;k<ndup; k++) {
    if (!multiRunner[k-1]) {
      update = true;
      multiRunner[k-1]=oe->addRunner(sName, getClubId(),
                                     getClassId(false), 0, 0, false);
      multiRunner[k-1]->tDuplicateLeg=k;
      multiRunner[k-1]->tParentRunner=this;

      if (sync)
        multiRunner[k-1]->synchronize();
    }
  }
  if (update)
    updateChanged();

  if (sync) {
    synchronize(true);
    oe->removeRunner(toRemove);
  }
}

pRunner oRunner::getPredecessor() const
{
  if (!tParentRunner || unsigned(tDuplicateLeg-1)>=16)
    return 0;

  if (tDuplicateLeg==1)
    return tParentRunner;
  else
    return tParentRunner->multiRunner[tDuplicateLeg-2];
}

bool oRunner::apply(bool sync, pRunner src, bool setTmpOnly) {
  createMultiRunner(false, sync);
  if (sync) {
    for (size_t k = 0; k < multiRunner.size(); k++) {
      if (multiRunner[k])
        multiRunner[k]->synchronize(true);
    }
  }
  tLeg = -1;
  tLegEquClass = 0;
  tUseStartPunch=true;
  if (tInTeam)
    tInTeam->apply(sync, this, setTmpOnly);
  else {
    if (Class && Class->hasMultiCourse()) {
      pClass pc=Class;
      StartTypes st=pc->getStartType(tDuplicateLeg);
      if (st==STTime) {
        pCourse crs = getCourse(false);
        int startType = crs ? crs->getStartPunchType() : oPunch::PunchStart;
        if (!Card || Card->getPunchByType(startType) == 0 || !pc->hasFreeStart()) {
          setStartTime(pc->getStartData(tDuplicateLeg), false, setTmpOnly);
          tUseStartPunch = false;
        }
      }
      else if (st==STChange) {
        pRunner r=getPredecessor();
        int lastStart=0;
        if (r && r->FinishTime>0)
          lastStart = r->FinishTime;

        int restart=pc->getRestartTime(tDuplicateLeg);
        int rope=pc->getRopeTime(tDuplicateLeg);

        if (restart && rope && (lastStart>rope || lastStart==0))
          lastStart=restart; //Runner in restart

        setStartTime(lastStart, false, setTmpOnly);
        tUseStartPunch=false;
      }
      else if (st==STHunting) {
        pRunner r=getPredecessor();
        int lastStart=0;

        if (r && r->FinishTime>0 && r->statusOK()) {
          int rt=r->getRaceRunningTime(tDuplicateLeg-1);
          int timeAfter=rt-pc->getTotalLegLeaderTime(r->tDuplicateLeg, true);
          if (rt>0 && timeAfter>=0)
            lastStart=pc->getStartData(tDuplicateLeg)+timeAfter;
        }
        int restart=pc->getRestartTime(tDuplicateLeg);
        int rope=pc->getRopeTime(tDuplicateLeg);

        if (restart && rope && (lastStart>rope || lastStart==0))
          lastStart=restart; //Runner in restart

        setStartTime(lastStart, false, setTmpOnly);
        tUseStartPunch=false;
      }
    }
  }

  if (tLeg==-1) {
    tLeg=0;
    tInTeam=0;
    return false;
  }
  else return true;
}

void oRunner::cloneStartTime(const pRunner r) {
  if (tParentRunner)
    tParentRunner->cloneStartTime(r);
  else {
    setStartTime(r->getStartTime(), true, false);

    for (size_t k=0; k < min(multiRunner.size(), r->multiRunner.size()); k++) {
      if (multiRunner[k]!=0 && r->multiRunner[k]!=0)
        multiRunner[k]->setStartTime(r->multiRunner[k]->getStartTime(), true, false);
    }
    apply(false, 0, false);
  }
}

void oRunner::cloneData(const pRunner r) {
  if (tParentRunner)
    tParentRunner->cloneData(r);
  else {
    size_t t = sizeof(oData);
    memcpy(oData, r->oData, t);
  }
}


Table *oEvent::getRunnersTB()//Table mode
{
  if (tables.count("runner") == 0) {
    Table *table=new Table(this, 20, L"Deltagare", "runners");

    table->addColumn("Id", 70, true, true);
    table->addColumn("�ndrad", 70, false);

    table->addColumn("Namn", 200, false);
    table->addColumn("Klass", 120, false);
    table->addColumn("Bana", 120, false);

    table->addColumn("Klubb", 120, false);
    table->addColumn("Lag", 120, false);
    table->addColumn("Str�cka", 70, true);

    table->addColumn("SI", 90, true, false);

    table->addColumn("Start", 70, false, true);
    table->addColumn("M�l", 70, false, true);
    table->addColumn("Status", 70, false);
    table->addColumn("Tid", 70, false, true);
    table->addColumn("Plac.", 70, true, true);
    table->addColumn("Start nr.", 70, true, false);

    oe->oRunnerData->buildTableCol(table);

    table->addColumn("Tid in", 70, false, true);
    table->addColumn("Status in", 70, false, true);
    table->addColumn("Po�ng in", 70, true);
    table->addColumn("Placering in", 70, true);

    tables["runner"] = table;
    table->addOwnership();
  }
  tables["runner"]->update();
  return tables["runner"];
}

void oEvent::generateRunnerTableData(Table &table, oRunner *addRunner)
{
  if (addRunner) {
    addRunner->addTableRow(table);
    return;
  }

  synchronizeList(oLRunnerId);
  oRunnerList::iterator it;
  table.reserve(Runners.size());
  for (it=Runners.begin(); it != Runners.end(); ++it){
    if (!it->isRemoved()){
      it->addTableRow(table);
    }
  }
}

pRunner oRunner::getReference() const
{
  int rid = getDCI().getInt("Reference");
  if (rid != 0)
    return oe->getRunner(rid, 0);
  else 
    return 0;
}

void oRunner::setReference(int runnerId)
{
  getDI().setInt("Reference", runnerId);
}

const wstring &oRunner::getUIName() const {
  oEvent::NameMode nameMode = oe->getNameMode();
  
  switch (nameMode) {
  case oEvent::Raw: 
    return getNameRaw();
  case oEvent::LastFirst:
    return getNameLastFirst();
  default:
    return getName();
  }
}


void oRunner::addTableRow(Table &table) const
{
  oRunner &it = *pRunner(this);
  table.addRow(getId(), &it);

  int row = 0;
  table.set(row++, it, TID_ID, itow(getId()), false);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false);

  if (tParentRunner == 0)
    table.set(row++, it, TID_RUNNER, getUIName(), true);
  else
    table.set(row++, it, TID_RUNNER, getUIName() + L" (" + itow(tDuplicateLeg+1) + L")", false);
  table.set(row++, it, TID_CLASSNAME, getClass(true), true, cellSelection);
  table.set(row++, it, TID_COURSE, getCourseName(), true, cellSelection);
  table.set(row++, it, TID_CLUB, getClub(), tParentRunner == 0, cellCombo);

  table.set(row++, it, TID_TEAM, tInTeam ? tInTeam->getName() : L"", false);
  table.set(row++, it, TID_LEG, tInTeam ? itow(tLeg+1) : L"" , false);

  int cno = getCardNo();
  table.set(row++, it, TID_CARD, cno>0 ? itow(cno) : L"", true);

  table.set(row++, it, TID_START, getStartTimeS(), true);
  table.set(row++, it, TID_FINISH, getFinishTimeS(), true);
  table.set(row++, it, TID_STATUS, getStatusS(), true, cellSelection);
  table.set(row++, it, TID_RUNNINGTIME, getRunningTimeS(), false);

  table.set(row++, it, TID_PLACE, getPlaceS(), false);
  table.set(row++, it, TID_STARTNO, itow(getStartNo()), true);

  row = oe->oRunnerData->fillTableCol(it, table, true);

  table.set(row++, it, TID_INPUTTIME, getInputTimeS(), true);
  table.set(row++, it, TID_INPUTSTATUS, getInputStatusS(), true, cellSelection);
  table.set(row++, it, TID_INPUTPOINTS, itow(inputPoints), true);
  table.set(row++, it, TID_INPUTPLACE, itow(inputPlace), true);
}

bool oRunner::inputData(int id, const wstring &input,
                        int inputId, wstring &output, bool noUpdate)
{
  int t,s;
  vector<int> mp;
  synchronize(false);

  if (id>1000) {
    return oe->oRunnerData->inputData(this, id, input,
                                        inputId, output, noUpdate);
  }
  switch(id) {
    case TID_CARD:
      setCardNo(_wtoi(input.c_str()), true);
      synchronizeAll();
      output = itow(getCardNo());
      return true;
    case TID_RUNNER:
      if (trim(input).empty())
        throw std::exception("Tomt namn inte till�tet.");

      if (sName != input && tRealName != input) {
        updateFromDB(input, getClubId(), getClassId(false), getCardNo(), getBirthYear());
        setName(input, true);
        synchronizeAll();
      }
      output = getName();
      return true;
    break;

    case TID_START:
      setStartTimeS(input);
      t=getStartTime();
      evaluateCard(true, mp);
      s=getStartTime();
      if (s!=t)
        throw std::exception("Starttiden �r definerad genom klassen eller l�parens startst�mpling.");
      synchronize(true);
      output = getStartTimeS();
      return true;
    break;

    case TID_FINISH:
      setFinishTimeS(input);
      t=getFinishTime();
      evaluateCard(true, mp);
      s=getFinishTime();
      if (s!=t)
        throw std::exception("F�r att �ndra m�ltiden m�ste l�parens m�lst�mplingstid �ndras.");
      synchronize(true);
      output = getStartTimeS();
      return true;
    break;

    case TID_COURSE:
      setCourseId(inputId);
      synchronize(true);
      output = getCourseName();
      break;

    case TID_CLUB:
      {
        pClub pc = 0;
        if (inputId > 0)
          pc = oe->getClub(inputId);
        else
          pc = oe->getClubCreate(0, input);

        updateFromDB(getName(), pc ? pc->getId():0, getClassId(false), getCardNo(), getBirthYear());

        setClub(pc ? pc->getName() : L"");
        synchronize(true);
        output = getClub();
      }
      break;

    case TID_CLASSNAME:
      setClassId(inputId, true);
      synchronize(true);
      output = getClass(true);
      break;

    case TID_STATUS: {
      setStatus(RunnerStatus(inputId), true, false);
      int s = getStatus();
      evaluateCard(true, mp);
      if (s!=getStatus())
        throw std::exception("Status matchar inte data i l�parbrickan.");
      synchronize(true);
      output = getStatusS();
    }
    break;

    case TID_STARTNO:
      setStartNo(_wtoi(input.c_str()), false);
      synchronize(true);
      output = itow(getStartNo());
      break;

    case TID_INPUTSTATUS:
      setInputStatus(RunnerStatus(inputId));
      synchronize(true);
      output = getInputStatusS();
      break;

    case TID_INPUTTIME:
      setInputTime(input);
      synchronize(true);
      output = getInputTimeS();
      break;

    case TID_INPUTPOINTS:
      setInputPoints(_wtoi(input.c_str()));
      synchronize(true);
      output = itow(getInputPoints());
      break;

    case TID_INPUTPLACE:
      setInputPlace(_wtoi(input.c_str()));
      synchronize(true);
      output = itow(getInputPlace());
      break;
  }

  return false;
}

void oRunner::fillInput(int id, vector< pair<wstring, size_t> > &out, size_t &selected)
{
  if (id>1000) {
    oe->oRunnerData->fillInput(oData, id, 0, out, selected);
    return;
  }

  if (id==TID_COURSE) {
    oe->fillCourses(out, true);
    out.push_back(make_pair(lang.tl(L"Klassens bana"), 0));
    selected = getCourseId();
  }
  else if (id==TID_CLASSNAME) {
    oe->fillClasses(out, oEvent::extraNone, oEvent::filterNone);
    out.push_back(make_pair(lang.tl(L"Ingen klass"), 0));
    selected = getClassId(true);
  }
  else if (id==TID_CLUB) {
    oe->fillClubs(out);
    out.push_back(make_pair(lang.tl(L"Klubbl�s"), 0));
    selected = getClubId();
  }
  else if (id==TID_STATUS) {
    oe->fillStatus(out);
    selected = getStatus();
  }
  else if (id==TID_INPUTSTATUS) {
    oe->fillStatus(out);
    selected = inputStatus;
  }
}

int oRunner::getSplitTime(int controlNumber, bool normalized) const
{
  const vector<SplitData> &st = getSplitTimes(normalized);
  if (controlNumber>0 && controlNumber == st.size() && FinishTime>0) {
    int t = st.back().time;
    if (t >0)
      return max(FinishTime - t, -1);
  }
  else if ( unsigned(controlNumber)<st.size() ) {
    if (controlNumber==0)
      return (tStartTime>0 && st[0].time>0) ? max(st[0].time-tStartTime, -1) : -1;
    else if (st[controlNumber].time>0 && st[controlNumber-1].time>0)
      return max(st[controlNumber].time - st[controlNumber-1].time, -1);
    else return -1;
  }
  return -1;
}

int oRunner::getTimeAdjust(int controlNumber) const
{
  if ( unsigned(controlNumber)<adjustTimes.size() ) {
    return adjustTimes[controlNumber];
  }
  return 0;
}

int oRunner::getNamedSplit(int controlNumber) const
{
  pCourse crs=getCourse(true);
  if (!crs || unsigned(controlNumber)>=unsigned(crs->nControls)
        || unsigned(controlNumber)>=splitTimes.size())
    return -1;

  pControl ctrl=crs->Controls[controlNumber];
  if (!ctrl || !ctrl->hasName())
    return -1;

  int k=controlNumber-1;

  //Measure from previous named control
  while(k>=0) {
    pControl c=crs->Controls[k];

    if (c && c->hasName()) {
      if (splitTimes[controlNumber].time>0 && splitTimes[k].time>0)
        return max(splitTimes[controlNumber].time - splitTimes[k].time, -1);
      else return -1;
    }
    k--;
  }

  //Measure from start time
  if (splitTimes[controlNumber].time>0)
    return max(splitTimes[controlNumber].time - tStartTime, -1);

  return -1;
}

wstring oRunner::getSplitTimeS(int controlNumber, bool normalized) const
{
  return formatTime(getSplitTime(controlNumber, normalized));
}

wstring oRunner::getNamedSplitS(int controlNumber) const
{
  return formatTime(getNamedSplit(controlNumber));
}

int oRunner::getPunchTime(int controlNumber, bool normalized) const
{
  const vector<SplitData> &st = getSplitTimes(normalized);

  if ( unsigned(controlNumber)<st.size() ) {
    if (st[controlNumber].time>0)
      return st[controlNumber].time-tStartTime;
    else return -1;
  }
  else if ( unsigned(controlNumber)==st.size() )
    return FinishTime-tStartTime;

  return -1;
}

wstring oRunner::getPunchTimeS(int controlNumber, bool normalized) const
{
  return formatTime(getPunchTime(controlNumber, normalized));
}

bool oAbstractRunner::isVacant() const
{
  int vacClub = oe->getVacantClubIfExist(false);
  return vacClub > 0 && getClubId()==vacClub;
}

bool oRunner::needNoCard() const {
  const_cast<oRunner*>(this)->apply(false, 0, false);
  return tNeedNoCard;
}

void oRunner::getSplitTime(int courseControlId, RunnerStatus &stat, int &rt) const
{
  rt = 0;
  stat = StatusUnknown;
  int cardno = getCardNo();

  if (courseControlId==oPunch::PunchFinish &&
      FinishTime>0 && tStatus!=StatusUnknown) {
    stat = tStatus;
    rt = getFinishTimeAdjusted();
  }
  else if (Card) {
    oPunch *p=Card->getPunchById(courseControlId);
    if (p && p->Time>0) {
      rt=p->getAdjustedTime();
      stat = StatusOK;
    }
    else if (p && p->Time == -1 && statusOK()) {
      rt = getFinishTimeAdjusted();
      if (rt > 0)
        stat = StatusOK;
      else
        stat = StatusMP;
    }
    else
      stat = courseControlId==oPunch::PunchFinish ? StatusDNF: StatusMP;
  }
  else if (cardno) {
    oFreePunch *fp=oe->getPunch(getId(), courseControlId, cardno);

    if (fp) {
      rt=fp->getAdjustedTime();
      stat=StatusOK;
    }
    if (courseControlId==oPunch::PunchFinish && tStatus!=StatusUnknown)
      stat = tStatus;
  }
  rt-=tStartTime;

  if (rt<0)
    rt=0;
}

void oRunner::fillSpeakerObject(int leg, int courseControlId, int previousControlCourseId,
                                bool totalResult, oSpeakerObject &spk) const {
  spk.status=StatusUnknown;
  spk.owner=const_cast<oRunner *>(this);

  getSplitTime(courseControlId, spk.status, spk.runningTime.time);

  if (courseControlId == oPunch::PunchFinish)
    spk.timeSinceChange = oe->getComputerTime() - FinishTime;
  else
    spk.timeSinceChange = oe->getComputerTime() - (spk.runningTime.time + tStartTime);

  spk.bib = getBib();
  spk.names.push_back(getName());

  spk.club = getClub();
  spk.finishStatus=totalResult ? getTotalStatus() : getStatus();

  spk.startTimeS=getStartTimeCompact();
  spk.missingStartTime = tStartTime<=0;

  spk.isRendered=false;

  map<int, int>::const_iterator mapit = priority.find(courseControlId);
  if (mapit!=priority.end())
    spk.priority=mapit->second;
  else
    spk.priority=0;

  spk.runningTime.preliminary = getPrelRunningTime();

  if (spk.status==StatusOK) {
    spk.runningTimeLeg=spk.runningTime;
    spk.runningTime.preliminary = spk.runningTime.time;
    spk.runningTimeLeg.preliminary = spk.runningTime.time;
  }
  else {
    spk.runningTimeLeg.time = spk.runningTime.preliminary;
    spk.runningTimeLeg.preliminary = spk.runningTime.preliminary;
  }

  if (totalResult) {
    if (spk.runningTime.preliminary > 0)
      spk.runningTime.preliminary += inputTime;
    if (spk.runningTime.time > 0)
      spk.runningTime.time += inputTime;

    if (inputStatus != StatusOK)
      spk.status = spk.finishStatus;
  }
}

pRunner oEvent::findRunner(const wstring &s, int lastId, const unordered_set<int> &inputFilter,
                           unordered_set<int> &matchFilter) const
{
  matchFilter.clear();
  wstring trm = trim(s);
  int len = trm.length();
  int sn = _wtoi(trm.c_str());
  wchar_t s_lc[1024];
  wcscpy_s(s_lc, s.c_str());
  CharLowerBuff(s_lc, len);

  pRunner res = 0;

  if (!inputFilter.empty() && inputFilter.size() < Runners.size() / 2) {
    for (unordered_set<int>::const_iterator it = inputFilter.begin(); it!= inputFilter.end(); ++it) {
      int id = *it;
      pRunner r = getRunner(id, 0);
      if (!r)
        continue;

      if (sn>0) {
        if (matchNumber(r->StartNo, s_lc) || matchNumber(r->CardNo, s_lc)) {
          matchFilter.insert(id);
          if (res == 0)
            res = r;
        }
      }
      else {
        if (filterMatchString(r->tRealName, s_lc)) {
          matchFilter.insert(id);
          if (res == 0)
            res = r;
        }
      }
    }
    return res;
  }

  oRunnerList::const_iterator itstart = Runners.begin();

  if (lastId) {
    for (; itstart != Runners.end(); ++itstart) {
      if (itstart->Id==lastId) {
        ++itstart;
        break;
      }
    }
  }

  oRunnerList::const_iterator it;
  for (it=itstart; it != Runners.end(); ++it) {
    pRunner r = pRunner(&(*it));
    if (r->skip())
       continue;

    if (sn>0) {
      if (matchNumber(r->StartNo, s_lc) || matchNumber(r->CardNo, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
    else {
      if (filterMatchString(r->tRealName, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
  }
  for (it=Runners.begin(); it != itstart; ++it) {
    pRunner r = pRunner(&(*it));
    if (r->skip())
       continue;

    if (sn>0) {
      if (matchNumber(r->StartNo, s_lc) || matchNumber(r->CardNo, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
    else {
      if (filterMatchString(r->tRealName, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
  }

  return res;
}

int oRunner::getTimeAfter(int leg) const
{
  if (leg==-1)
    leg=tDuplicateLeg;

  if (!Class || Class->tLeaderTime.size()<=unsigned(leg))
    return -1;

  int t=getRaceRunningTime(leg);

  if (t<=0)
    return -1;

  return t-Class->getTotalLegLeaderTime(leg, true);
}

int oRunner::getTimeAfter() const
{
  int leg=0;
  if (tInTeam)
    leg=tLeg;
  else
    leg=tDuplicateLeg;

  if (!Class || Class->tLeaderTime.size()<=unsigned(leg))
    return -1;

  int t=getRunningTime();

  if (t<=0)
    return -1;

  return t-Class->getBestLegTime(leg);
}

int oRunner::getTimeAfterCourse() const
{
  if (!Class)
    return -1;

  const pCourse crs = getCourse(false);
  if (!crs)
    return -1;

  int t = getRunningTime();

  if (t<=0)
    return -1;

  int bt = Class->getBestTimeCourse(crs->getId());

  if (bt <= 0)
    return -1;

  return t - bt;
}

bool oRunner::synchronizeAll()
{
  if (tParentRunner)
    tParentRunner->synchronizeAll();
  else {
    synchronize();
    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k])
        multiRunner[k]->synchronize();
    }
    if (Class && Class->isSingleRunnerMultiStage())
      if (tInTeam)
        tInTeam->synchronize(false);
  }
  return true;
}

const wstring &oAbstractRunner::getBib() const
{
  return getDCI().getString("Bib");
}

void oRunner::setBib(const wstring &bib, int bibNumerical, bool updateStartNo, bool tmpOnly) {
  const bool freeBib = !Class || Class->getBibMode() == BibMode::BibFree;

  if (tParentRunner && !freeBib)
    tParentRunner->setBib(bib, bibNumerical, updateStartNo, tmpOnly);
  else {
    if (updateStartNo)
      setStartNo(bibNumerical, tmpOnly); // Updates multi too.

    if (tmpOnly) {
      tmpStore.bib = bib;
      return;
    }
    if (getDI().setString("Bib", bib)) {
      if (oe)
        oe->bibStartNoToRunnerTeam.clear();
    }
    if (!freeBib) {
      for (size_t k = 0; k < multiRunner.size(); k++) {
        if (multiRunner[k]) {
          multiRunner[k]->getDI().setString("Bib", bib);
        }
      }
    }
  }
}


void oEvent::analyseDNS(vector<pRunner> &unknown_dns, vector<pRunner> &known_dns,
                        vector<pRunner> &known, vector<pRunner> &unknown, bool &hasSetDNS)
{
  autoSynchronizeLists(true);

  vector<pRunner> stUnknown;
  vector<pRunner> stDNS;

  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end();++it) {
    if (!it->isRemoved() && !it->needNoCard()) {
      if (it->getStatus() == StatusUnknown)
        stUnknown.push_back(&*it);
      else if (it->getStatus() == StatusDNS) {
        stDNS.push_back(&*it);
        if (it->hasFlag(oAbstractRunner::FlagAutoDNS))
          hasSetDNS = true;
      }
    }
  }

  // Map cardNo -> punch
  multimap<int, pFreePunch> punchHash;
  map<int, int> cardCount;

  for (oRunnerList::const_iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (!it->isRemoved() && it->getCardNo() > 0)
      ++cardCount[it->getCardNo()];
  }

  typedef multimap<int, pFreePunch>::const_iterator TPunchIter;
  for (oFreePunchList::iterator it = punches.begin(); it != punches.end(); ++it) {
    punchHash.insert(make_pair(it->getCardNo(), &*it));
  }

  set<int> knownCards;
  for (oCardList::iterator it = Cards.begin(); it!=Cards.end(); ++it) {
    if (it->tOwner == 0)
      knownCards.insert(it->cardNo);
  }

  unknown.clear();
  known.clear();

  for (size_t k=0;k<stUnknown.size();k++) {
    int card = stUnknown[k]->CardNo;
    if (card == 0)
      unknown.push_back(stUnknown[k]);
    else {
      bool hitCard = knownCards.count(card)==1 && cardCount[card] == 1;
      if (!hitCard) {
        pair<TPunchIter, TPunchIter> res = punchHash.equal_range(card);
        while (res.first != res.second) {
          if (cardCount[card] == 1 || res.first->second->tRunnerId == stUnknown[k]->getId()) {
            hitCard = true;
            break;
          }
          ++res.first;
        }
      }
      if (hitCard)
        known.push_back(stUnknown[k]);
      else
        unknown.push_back(stUnknown[k]); //These can be given "dns"
    }
  }

  unknown_dns.clear();
  known_dns.clear();

  for (size_t k=0;k<stDNS.size(); k++) {
    int card = stDNS[k]->CardNo;
    if (card == 0)
      unknown_dns.push_back(stDNS[k]);
    else {
      bool hitCard = knownCards.count(card)==1 && cardCount[card] == 1;
      if (!hitCard) {
        pair<TPunchIter, TPunchIter> res = punchHash.equal_range(card);
        while (res.first != res.second) {
          if (cardCount[card] == 1 || res.first->second->tRunnerId == stDNS[k]->getId()) {
            hitCard = true;
            break;
          }
          ++res.first;
        }
      }
      if (hitCard)
        known_dns.push_back(stDNS[k]);
      else
        unknown_dns.push_back(stDNS[k]);
    }
  }
}

static int findNextControl(const vector<pControl> &ctrl, int startIndex, int id, int &offset, bool supportRogaining)
{
  vector<pControl>::const_iterator it=ctrl.begin();
  int index=0;
  offset = 1;
  while(startIndex>0 && it!=ctrl.end()) {
    int multi = (*it)->getNumMulti();
    offset += multi-1;
    ++it, --startIndex, ++index;
    if (it!=ctrl.end() && (*it)->isRogaining(supportRogaining))
      index--;
  }

  while(it!=ctrl.end() && (*it) && (*it)->getId()!=id) {
    int multi = (*it)->getNumMulti();
    offset += multi-1;
    ++it, ++index;
    if (it!=ctrl.end() && (*it)->isRogaining(supportRogaining))
      index--;
  }

  if (it==ctrl.end())
    return -1;
  else
    return index;
}

static void gotoNextLine(gdioutput &gdi, int &xcol, int &cx, int &cy, int colDeltaX, int numCol, int baseCX) {
  if (++xcol < numCol) {
    cx += colDeltaX;
  }
  else {
    xcol = 0;
    cy += int(gdi.getLineHeight()*1.1);
    cx = baseCX;
  }
}

static void addMissingControl(bool wideFormat, gdioutput &gdi, 
                              int &xcol, int &cx, int &cy, 
                              int colDeltaX, int numCol, int baseCX) {
  int xx = cx;
  wstring str = makeDash(L"-");
  int posy = wideFormat ? cy : cy-int(gdi.getLineHeight()*0.4);
  const int endx = cx + colDeltaX - 27;

  while (xx < endx) {
    gdi.addStringUT(posy, xx, fontSmall, str);
    xx += 20;
  }

  // Make a thin line for list format, otherwise, take a full place
  if (wideFormat) {
    gotoNextLine(gdi, xcol, cx, cy, colDeltaX, numCol, baseCX);
  }
  else
    cy+=int(gdi.getLineHeight()*0.3);
}

void oRunner::printSplits(gdioutput &gdi) const {
  bool withAnalysis = (oe->getDI().getInt("Analysis") & 1) == 0;
  bool withSpeed = (oe->getDI().getInt("Analysis") & 2) == 0;
  bool withResult = (oe->getDI().getInt("Analysis") & 4) == 0;
  const bool wideFormat = oe->getPropertyInt("WideSplitFormat", 0) == 1;

  const int numCol = 4;

  if (Class && Class->getNoTiming()) {
    withResult = false;
    withAnalysis = false;
  }

  
  if (getCourse(false) && getCourse(false)->hasRogaining()) {
	  vector<pControl> ctrl;
    bool rogaining(true);
    getCourse(false)->getControls(ctrl);
    for (vector<pControl>::const_iterator it=ctrl.begin(); it!=ctrl.end(); ++it)
      if (!(*it)->isRogaining(true))
        rogaining = false;
    if (rogaining) {  
      printRogainingSplits( gdi);
      return;
    }
  }

  gdiFonts head = boldText;
  gdiFonts normal = fontSmall;
  gdiFonts bnormal = boldSmall;
  if (wideFormat) {
    head = boldLarge;
    normal = normalText;
    bnormal = boldText;
  }
  else {
    gdi.setCX(10);
  }
  gdi.fillDown();
  gdi.addStringUT(head, oe->getName());
  gdi.addStringUT(normal, oe->getDate());
  gdi.dropLine(0.5);
  pCourse pc = getCourse(true);

  gdi.addStringUT(bnormal, getName() + L", " + getClass(true));
  gdi.addStringUT(normal, getClub());
  gdi.dropLine(0.5);
  gdi.addStringUT(normal, lang.tl("Start: ") + getStartTimeS() + lang.tl(", M�l: ") + getFinishTimeS());
  wstring statInfo = lang.tl("Status: ") + getStatusS() + lang.tl(", Tid: ") + getRunningTimeS();
  if (withSpeed && pc && pc->getLength() > 0) {
    int kmt = (getRunningTime() * 1000) / pc->getLength();
    statInfo += L" (" + formatTime(kmt) + lang.tl(" min/km") + L")";
  }
  if (pc && withSpeed) {
    if (pc->legLengths.empty() || *max_element(pc->legLengths.begin(), pc->legLengths.end()) <= 0)
      withSpeed = false; // No leg lenghts available
  }
  gdi.addStringUT(normal, statInfo);
  oe->calculateResults(oEvent::RTClassResult);
	if (getPlaceS() != _EmptyString)
    gdi.addStringUT(fontSmall, lang.tl("Aktuell klassposition") + " in " + getClass() + ": " + getPlaceS());
  

  int cy = gdi.getCY()+4;
  int cx = gdi.getCX();

  int spMax = 0;
  int totMax = 0;
  if (pc) {
    for (int n = 0; n < pc->nControls; n++) {
      spMax = max(spMax, getSplitTime(n, false));
      totMax = max(totMax, getPunchTime(n, false));
    }
  }
  bool moreThanHour = max(totMax, getRunningTime()) >= 3600;
  bool moreThanHourSplit = spMax >= 3600;

  const int c1=35;
  const int c2=95 + (moreThanHourSplit ? 65 : 55);
  const int c3 = c2 + 10;
  const int c4 = moreThanHour ? c3+153 : c3+133;
  const int c5 = withSpeed ? c4 + 80 : c4;
  const int baseCX = cx;
  const int colDeltaX = c5 + 32;

  char bf[256];
  int lastIndex = -1;
  int adjust = 0;
  int offset = 1;

  vector<pControl> ctrl;

  int finishType = -1;
  int startType = -1, startOffset = 0;
  if (pc) {
    pc->getControls(ctrl);
    finishType = pc->getFinishPunchType();

    if (pc->useFirstAsStart()) {
      startType = pc->getStartPunchType();
      startOffset = -1;
    }
  }

  set<int> headerPos;
  set<int> checkedIndex;

  if (Card && pc) {
    bool hasRogaining = pc->hasRogaining();

    const int cyHead = cy;
    cy += int(gdi.getLineHeight()*0.9);
    int xcol = 0;
    int baseY = cy;

    oPunchList &p=Card->punches;
    for (oPunchList::iterator it=p.begin();it!=p.end();++it) {
      if (headerPos.count(cx) == 0) {
        headerPos.insert(cx);
        gdi.addString("", cyHead, cx, italicSmall, "Kontroll");
        gdi.addString("", cyHead, cx+c2-55, italicSmall, "Tid");
        if (withSpeed)
          gdi.addString("", cyHead, cx+c5, italicSmall|textRight, "min/km");
      }

      bool any = false;
      if (it->tRogainingIndex>=0) {
        const pControl c = pc->getControl(it->tRogainingIndex);
        string point = c ? itos(c->getRogainingPoints()) + "p." : "";

        gdi.addStringUT(cy, cx + c1 + 10, fontSmall, point);
        any = true;

        sprintf_s(bf, "%d", it->Type);
        gdi.addStringUT(cy, cx, fontSmall, bf);
        int st = Card->getSplitTime(getStartTime(), &*it);

        if (st>0)
          gdi.addStringUT(cy, cx + c2, fontSmall|textRight, formatTime(st));

        gdi.addStringUT(cy, cx+c3, fontSmall, it->getTime());

        int pt = it->getAdjustedTime();
        st = getStartTime();
        if (st>0 && pt>0 && pt>st) {
          wstring punchTime = formatTime(pt-st);
          gdi.addStringUT(cy, cx+c4, fontSmall|textRight, punchTime);
        }

        cy+=int(gdi.getLineHeight()*0.9);
        continue;
      }

      int cid = it->tMatchControlId;
      wstring punchTime;
      int sp;
      int controlLegIndex = -1;
      if (it->isFinish(finishType)) {
        // Check if the last normal control was missing, and indicate this
        for (int j = pc->getNumControls() - 1; j >= 0; j--) {
          pControl ctrl = pc->getControl(j);
          if (ctrl && ctrl->isSingleStatusOK()) {
            if (checkedIndex.count(j) == 0) {
              addMissingControl(wideFormat, gdi, xcol, cx, cy, colDeltaX, numCol, baseCX);
            }
            break;
          }
        }

        gdi.addString("", cy, cx, fontSmall, "M�l");
        sp = getSplitTime(splitTimes.size(), false);
        if (sp>0) {
          gdi.addStringUT(cy, cx+c2, fontSmall|textRight, formatTime(sp));
          punchTime = formatTime(getRunningTime());
        }
        gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));
        any = true;
        if (!punchTime.empty()) {
          gdi.addStringUT(cy, cx+c4, fontSmall|textRight, punchTime);
        }
        controlLegIndex = pc->getNumControls();
      }
      else if (it->Type>10) { //Filter away check and start
        int index = -1;
        if (cid>0)
          index = findNextControl(ctrl, lastIndex+1, cid, offset, hasRogaining);
        if (index>=0) {
          if (index > lastIndex + 1) {
            addMissingControl(wideFormat, gdi, xcol, cx, cy, colDeltaX, numCol, baseCX);

            /*int xx = cx;
            string str = MakeDash("-");
            int posy = wideFormat ? cy : cy-int(gdi.getLineHeight()*0.4);
            const int endx = cx+c5 + 5;

            while (xx < endx) {
              gdi.addStringUT(posy, xx, fontSmall, str);
              xx += 20;
            }

            // Make a thin line for list format, otherwise, take a full place
            if (wideFormat) {
              gotoNextLine(gdi, xcol, cx, cy, colDeltaX, numCol, baseCX);
            }
            else
              cy+=int(gdi.getLineHeight()*0.3);*/
          }
          lastIndex = index;

          if (it->Type == startType && (index+offset) == 1)
            continue; // Skip start control

          sprintf_s(bf, "%d.", index+offset+startOffset);
          gdi.addStringUT(cy, cx, fontSmall, bf);
          sprintf_s(bf, "(%d)", it->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);

          controlLegIndex = it->tIndex;
          checkedIndex.insert(controlLegIndex);
          adjust = getTimeAdjust(controlLegIndex);
          sp = getSplitTime(controlLegIndex, false);
          if (sp>0) {
            punchTime = getPunchTimeS(controlLegIndex, false);
						gdi.addStringUT(cy, cx+c2, getLegPlace(it->tIndex) == 1 ? boldSmall|textRight : fontSmall|textRight, formatTime(sp));
          }
        }
        else {
          if (!it->isUsed) {
            gdi.addStringUT(cy, cx, fontSmall, makeDash(L"-"));
          }
          sprintf_s(bf, "(%d)", it->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
        }
        if (it->Time > 0)
          gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));
        else {
          wstring str = makeDash(L"-");
          gdi.addStringUT(cy, cx+c3, fontSmall, str);
        }

        if (!punchTime.empty()) {
          gdi.addStringUT(cy, cx+c4, getLegPlaceAcc(it->tIndex) == 1 ? boldSmall|textRight : fontSmall|textRight, punchTime);
        }
        any = true;
      }

      if (withSpeed && controlLegIndex>=0 && size_t(controlLegIndex) < pc->legLengths.size()) {
        int length = pc->legLengths[controlLegIndex];
        if (length > 0) {
          int tempo=(sp*1000)/length;
          gdi.addStringUT(cy, cx+c5, fontSmall|textRight, formatTime(tempo));
        }
      }

      if (any) {
        if (!wideFormat) {
          cy+=int(gdi.getLineHeight()*0.9);
        }
        else {
          gotoNextLine(gdi, xcol, cx, cy, colDeltaX, numCol, baseCX);
        }
      }
    }
    gdi.dropLine();
    if (wideFormat) {
      for (int i = 0; i < numCol-1; i++) {
        RECT rc;
        rc.top = baseY;
        rc.bottom = cy;
        rc.left = baseCX + colDeltaX*(i+1)-10;
        rc.right = rc.left + 1;
        gdi.addRectangle(rc, colorBlack);
      }
    }
    
    if (withAnalysis) {
      vector<wstring> misses;
      int last = ctrl.size();
      if (pc->useLastAsFinish())
        last--;

      for (int k = pc->useFirstAsStart() ? 1 : 0; k < last; k++) {
        int missed = getMissedTime(k);
        if (missed>0) {
          misses.push_back(pc->getControlOrdinal(k) + L"/" + formatTime(missed));
        }
      }
      if (misses.size()==0) {
        vector<pRunner> rOut;
        oe->getRunners(0, pc->getId(), rOut, false);
        int count = 0;
        for (size_t k = 0; k < rOut.size(); k++) {
          if (rOut[k]->getCard())
            count++;
        }

        if (count < 3)
          gdi.addString("", normal, "Underlag saknas f�r bomanalys.");
        else
          gdi.addString("", normal, "Inga bommar registrerade.");
      }
      else {
        wstring out = lang.tl("Tidsf�rluster (kontroll-tid): ");
        for (size_t k = 0; k<misses.size(); k++) {
          if (out.length() > (wideFormat ? 80u : (withSpeed ? 40u : 35u))) {
            gdi.addStringUT(normal, out);
            out.clear();
          }
          out += misses[k];
          if (k < misses.size()-1)
            out += L", ";
          else
            out += L".";
        }
        gdi.addStringUT(fontSmall, out);
      }
    }

    if (withResult && statusOK()) {
      gdi.dropLine(0.5);
      oe->calculateResults(oEvent::RTClassResult);
      if (hasInputData())
        oe->calculateResults(oEvent::RTTotalResult);
      if (tInTeam)
        oe->calculateTeamResults(tLeg, true);

      wstring place = oe->formatListString(lRunnerGeneralPlace, pRunner(this), L"%s");
      wstring timestatus;
      if (tInTeam || hasInputData()) {
        timestatus = oe->formatListString(lRunnerGeneralTimeStatus, pRunner(this));
        if (!place.empty() && !timestatus.empty())
          timestatus = L", " + timestatus;
      }

      wstring after = oe->formatListString(lRunnerGeneralTimeAfter, pRunner(this));
      if (!after.empty() && !(place.empty() && timestatus.empty()))
        after = L", " + after;
      
      gdi.fillRight();
      gdi.pushX();
      if (!place.empty()) 
        gdi.addString("", bnormal, "Placering:");
      else
        gdi.addString("", bnormal, "Resultat:");
      gdi.fillDown();
      gdi.addString("", normal,  place + timestatus + after);
      gdi.popX();
    }
  }
  gdi.dropLine();

  vector< pair<wstring, int> > lines;
  oe->getExtraLines("SPExtra", lines);

  for (size_t k = 0; k < lines.size(); k++) {
    gdi.addStringUT(lines[k].second, lines[k].first);
  }
  if (lines.size()>0)
    gdi.dropLine(0.5);

  gdi.addString("", fontSmall, "Av MeOS: www.melin.nu/meos");
}

void oRunner::printRogainingSplits(gdioutput &gdi) const {
  const int ct1=160;

  gdi.setCX(10);
  gdi.fillDown();
  gdi.addStringUT(boldText, oe->getName());
  gdi.addStringUT(fontSmall, oe->getDate());
  gdi.dropLine(0.5);
  
  gdi.addStringUT(boldSmall, getName() + " " + getClub());
  int cy = gdi.getCY();
  gdi.addStringUT(boldSmall, lang.tl("Po�ng: ") + itos(getRogainingPoints(false)));    
  gdi.addStringUT(cy, gdi.getCX() + ct1, boldSmall, lang.tl("Tid: ") + getRunningTimeS());
	if (getCard())
		gdi.addStringUT(fontSmall, lang.tl("SportIdent: ") + getCard()->getCardNoString());
  gdi.dropLine(0.5);
  cy = gdi.getCY();
  gdi.addStringUT(fontSmall, lang.tl("Bana: ") + getCourseName());
  gdi.addStringUT(cy, gdi.getCX() + ct1, fontSmall, lang.tl("Klass: ") + getClass());
  cy = gdi.getCY();
  gdi.addStringUT(fontSmall, lang.tl("Gross: ") + itos(getRogainingPoints(false) + getRogainingReduction()));
  gdi.addStringUT(cy, gdi.getCX() + ct1, fontSmall, lang.tl("Penalty: ") + itos(getRogainingReduction()));
  cy = gdi.getCY();
  gdi.addStringUT(fontSmall, lang.tl("Start: ") + getStartTimeS());
  gdi.addStringUT(cy, gdi.getCX() + ct1, fontSmall, lang.tl("M�l: ") + getFinishTimeS());

  cy = gdi.getCY()+4;
  int cx = gdi.getCX();
  const int c1=90;
  const int c2=130;
  const int c3=170;
  const int c4=200;
  const int c5=240;
  const int width = 68;
  char bf[256];
  int lastIndex = -1;
  int adjust = 0;
  int offset = 1;
  int runningTotal = 0;

  bool moreThanHour = getRunningTime()>3600;
  vector<pControl> ctrl;
  pCourse pc = getCourse(false);
  if (pc)
    pc->getControls(ctrl);

  if (Card) {
		std::vector<pPunch> p;
    Card->getPunches(p);
		/*gdi.addStringUT(cy, cx, fontSmall, lang.tl("Kontroll"));
		gdi.addStringUT(cy, cx + c3, fontSmall, lang.tl("Po�ng"));
		gdi.addStringUT(cy, cx + c4, fontSmall, lang.tl("L�ptid"));*/
		cy+=int(gdi.getLineHeight()*0.9);
    for (unsigned int i = 0; i < p.size(); i++) {
      if (p[i]->tRogainingIndex>=0) { 
        const pControl c = pc->getControl(p[i]->tRogainingIndex);
        string point = c ? c->getRogainingPointsS() : "";
        runningTotal += c ? c->getRogainingPoints() : 0;   
        
        if (c->getName().length() > 0) 
          gdi.addStringUT(cy, cx, fontSmall, c->getName() + " (" + itos(p[i]->getControlNumber()) + ")");
        else
          gdi.addStringUT(cy, cx, fontSmall, itos(p[i]->getControlNumber()));
        gdi.addStringUT(cy, cx + c1, fontSmall, point);

        int st = Card->getSplitTime(getStartTime(), p[i]);
        
        if (st>0) {
          gdi.addStringUT(cy, cx + c2, fontSmall, formatTime(st));
					if (c->getRogainingPoints() > 0) {
						float pps = ((float)c->getRogainingPoints() * 60.0f)/st;
						sprintf_s(bf, "%01.1f", pps);
						//gdi.addStringUT(cy, cx+c2, fontSmall, bf);
					}
				}

        int pt = p[i]->getAdjustedTime();
        st = getStartTime();
        if (st>0 && pt>0 && pt>st) {
          string punchTime = formatTime(pt-st);
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c5, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c5+width, fontSmall|textRight, punchTime);
        }

        gdi.addStringUT(cy, cx+c4, fontSmall, itos(runningTotal));

        cy+=int(gdi.getLineHeight()*0.9);
        continue;
      }

      int cid = p[i]->tMatchControlId;
      string punchTime; 
      if (p[i]->isFinish()) {
        gdi.addString("", cy, cx, fontSmall, "M�l");
        int sp = getSplitTime(splitTimes.size(), false);
        if (sp>0) {
          gdi.addStringUT(cy, cx+c2, fontSmall, formatTime(sp));
          punchTime = formatTime(getRunningTime());
        }
        gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(p[i]->Time + adjust));

        if (!punchTime.empty()) {
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c5, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c5+width, fontSmall|textRight, punchTime);
        }
        cy+=gdi.getLineHeight();
      }
      else if (p[i]->Type>10) { //Filter away check and start
        int index = -1;
        if (cid>0)
          index = findNextControl(ctrl, lastIndex+1, cid, offset, true);
        if (index>=0) {
          if (index > lastIndex + 1) {
            int xx = cx;
            string str = MakeDash("-");
            int posy = cy-int(gdi.getLineHeight()*0.4);
            const int endx = cx+c5 + (moreThanHour ? width : 50);

            while (xx < endx) {
              gdi.addStringUT(posy, xx, fontSmall, str);
              xx += 20;
            }
            
            cy+=int(gdi.getLineHeight()*0.3);
          }
          
          lastIndex = index;
          sprintf_s(bf, "%d.", index+offset);
          gdi.addStringUT(cy, cx, fontSmall, bf);
          sprintf_s(bf, "(%d)", p[i]->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
          
          adjust = getTimeAdjust(p[i]->tIndex);
          int sp = getSplitTime(p[i]->tIndex, false);
          if (sp>0) {
            punchTime = getPunchTimeS(p[i]->tIndex, false);
            gdi.addStringUT(cy, cx+c2, fontSmall, formatTime(sp));
          }
        }
        else {
          if (!p[i]->isUsed) {
            gdi.addStringUT(cy, cx, fontSmall, MakeDash("-"));
          }
          sprintf_s(bf, "(%d)", p[i]->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
        }
        if (p[i]->Time > 0)
          gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(p[i]->Time + adjust));
        else {
          string str = MakeDash("-");
          gdi.addStringUT(cy, cx+c3, fontSmall, str);
        }

        if (!punchTime.empty()) {
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c5, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c5+width, fontSmall|textRight, punchTime);
        }
        cy+=int(gdi.getLineHeight()*0.9);
      }
    }


		if (getProblemDescription().size() > 0)
			gdi.addStringUT(fontSmall, lang.tl(getProblemDescription()));
  }

  gdi.dropLine();

  vector< pair<string, int> > lines;
  oe->getExtraLines("SPExtra", lines);

  for (size_t k = 0; k < lines.size(); k++) {
    gdi.addStringUT(lines[k].second, lines[k].first);
  }
  if (lines.size()>0)
    gdi.dropLine(0.5);

  gdi.addString("", fontSmall, "Av MeOS: www.melin.nu/meos");
}

void oRunner::printLabel(gdioutput &gdi) const {
  
  bool rogaining(false);
  if (getCourse(false) && getCourse(false)->hasRogaining()) {
		rogaining = true;
	  vector<pControl> ctrl;
    getCourse(false)->getControls(ctrl);
    for (vector<pControl>::const_iterator it=ctrl.begin(); it!=ctrl.end(); ++it)
      if (!(*it)->isRogaining(true))
        rogaining = false;
  }

  const int c2=400;
 	gdi.setCX(0);
	gdi.setCY(0);
	int cx = gdi.getCX();
	int cy = gdi.getCY();
  gdi.fillDown();

	gdi.addStringUT(getName().length() < 20 ? boldHuge : boldLarge, getName());
	gdi.dropLine(getName().length() < 20 ? 0.2 : 0.4);
	cy = gdi.getCY();
	gdi.addStringUT(cy, cx, boldLarge, getClass());
	if (getStatus()==StatusOK)
			{
			if (rogaining)
				gdi.addStringUT(cy, cx+c2, boldHuge, itos(getRogainingPoints(false)));    
			else
				gdi.addStringUT(cy, cx+c2, boldHuge, getRunningTimeS());
			}
		else
			gdi.addStringUT(cy, cx+c2, boldHuge,  getStatusS());
  gdi.dropLine(-0.1);
	cy = gdi.getCY();
	gdi.addStringUT(cy, cx, fontMedium, getClub());
	if (getCourse(false))
		gdi.addStringUT(cy, cx + c2, fontMedium, getCourse(false)->getName());

}


void oRunner::printStartInfo(gdioutput &gdi) const {
  gdi.setCX(10);
  gdi.fillDown();
  gdi.addString("", boldText, L"Startbevis X#" + oe->getName());
  gdi.addStringUT(fontSmall, oe->getDate());
  gdi.dropLine(0.5);

  wstring bib = getBib();
  if (!bib.empty())
    bib = bib + L": ";

  gdi.addStringUT(boldSmall, bib + getName() + L", " + getClass(true));
  gdi.addStringUT(fontSmall, getClub());
  gdi.dropLine(0.5);
  
  wstring startName;
  if (getCourse(false)) {
    startName = trim(getCourse(false)->getStart());
    if (!startName.empty())
      startName = L" (" + startName + L")";
  }    
  if (getStartTime() > 0)
    gdi.addStringUT(fontSmall, lang.tl(L"Start: ") + getStartTimeS() + startName);
  else
    gdi.addStringUT(fontSmall, lang.tl(L"Fri starttid") + startName);

  wstring borrowed = getDCI().getInt("CardFee") != 0 ? L" (" + lang.tl(L"Hyrd") + L")" : L"";
      
  gdi.addStringUT(fontSmall, lang.tl(L"Bricka: ") + itow(getCardNo()) +  borrowed);
  
  int cardFee = getDCI().getInt("CardFee");
  if (cardFee < 0)
    cardFee = 0;

  int fee = oe->getMeOSFeatures().hasFeature(MeOSFeatures::Economy) ? getDCI().getInt("Fee") + cardFee : 0;

  if (fee > 0) {
    wstring info;
    if (getDCI().getInt("Paid") == fee)
      info = lang.tl("Betalat");
    else
      info = lang.tl("Faktureras");
    
    gdi.addStringUT(fontSmall, lang.tl("Anm�lningsavgift: ") + itow(fee)  + L" (" + info + L")");
  }

  gdi.dropLine(1);
  vector< pair<wstring, int> > lines;
  oe->getExtraLines("EntryExtra", lines);

  for (size_t k = 0; k < lines.size(); k++) {
    gdi.addStringUT(lines[k].second, lines[k].first);
  }
  if (lines.size()>0)
    gdi.dropLine(0.5);

  gdi.addStringUT(fontSmall, L"Av MeOS " + getMeosCompectVersion() + L" / www.melin.nu/meos");
}

vector<pRunner> oRunner::getRunnersOrdered() const {
  if (tParentRunner)
    return tParentRunner->getRunnersOrdered();

  vector<pRunner> r(multiRunner.size()+1);
  r[0] = (pRunner)this;
  for (size_t k=0;k<multiRunner.size();k++)
    r[k+1] = (pRunner)multiRunner[k];

  return r;
}

int oRunner::getMultiIndex() {
  if (!tParentRunner)
    return 0;

  const vector<pRunner> &r = tParentRunner->multiRunner;

  for (size_t k=0;k<r.size(); k++)
    if (r[k]==this)
      return k+1;

  // Error
  tParentRunner = 0;
  markForCorrection();
  return -1;
}

void oRunner::correctRemove(pRunner r) {
  for(unsigned i=0;i<multiRunner.size(); i++)
    if (r!=0 && multiRunner[i]==r) {
      multiRunner[i] = 0;
      r->tParentRunner = 0;
      r->tLeg = 0;
      r->tLegEquClass = 0;
      if (i+1==multiRunner.size())
        multiRunner.pop_back();

      correctionNeeded = true;
      r->correctionNeeded = true;
    }
}

void oEvent::updateRunnersFromDB()
{
  oRunnerList::iterator it;
  if (!oe->useRunnerDb())
    return;

  for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (!it->isVacant() && !it->isRemoved())
      it->updateFromDB(it->sName, it->getClubId(), it->getClassId(false), it->getCardNo(), it->getBirthYear());
  }
}

bool oRunner::updateFromDB(const wstring &name, int clubId, int classId,
                           int cardNo, int birthYear) {
  if (!oe->useRunnerDb())
    return false;

  pRunner db_r = 0;
  if (cardNo>0) {
    db_r = oe->dbLookUpByCard(cardNo);

    if (db_r && db_r->matchName(name)) {
      //setName(db_r->getName());
      //setClub(db_r->getClub()); Don't...
      setExtIdentifier(db_r->getExtIdentifier());
      setBirthYear(db_r->getBirthYear());
      setSex(db_r->getSex());
      setNationality(db_r->getNationality());
      return true;
    }
  }

  db_r = oe->dbLookUpByName(name, clubId, classId, birthYear);

  if (db_r) {
    setExtIdentifier(db_r->getExtIdentifier());
    setBirthYear(db_r->getBirthYear());
    setSex(db_r->getSex());
    setNationality(db_r->getNationality());
    return true;
  }
  else if (getExtIdentifier()>0) {
    db_r = oe->dbLookUpById(getExtIdentifier());
    if (db_r && db_r->matchName(name)) {
      setBirthYear(db_r->getBirthYear());
      setSex(db_r->getSex());
      setNationality(db_r->getNationality());
      return true;
    }
    // Reset external identifier
    setExtIdentifier(0);
    setBirthYear(0);
    // Do not reset nationality and sex,
    // since they are likely correct.
  }

  return false;
}

void oRunner::setSex(PersonSex sex)
{
  getDI().setString("Sex", encodeSex(sex));
}

PersonSex oRunner::getSex() const
{
  return interpretSex(getDCI().getString("Sex"));
}

void oRunner::setBirthYear(int year)
{
  getDI().setInt("BirthYear", year);
}

int oRunner::getBirthYear() const
{
  return getDCI().getInt("BirthYear");
}

void oAbstractRunner::setSpeakerPriority(int year)
{
  if (Class) {
    oe->classChanged(Class, false);
  }
  getDI().setInt("Priority", year);
}

int oAbstractRunner::getSpeakerPriority() const
{
  return getDCI().getInt("Priority");
}

int oRunner::getSpeakerPriority() const {
  int p = oAbstractRunner::getSpeakerPriority();

  if (tParentRunner)
    p = max(p, tParentRunner->getSpeakerPriority());
  else if (tInTeam) {
    p = max(p, tInTeam->getSpeakerPriority());
  }

  return p;
}

void oRunner::setNationality(const wstring &nat)
{
  getDI().setString("Nationality", nat);
}

wstring oRunner::getNationality() const
{
  return getDCI().getString("Nationality");
}

bool oRunner::matchName(const wstring &pname) const
{
  if (pname == sName || pname == tRealName)
    return true;

  vector<wstring> myNames, inNames;

  split(tRealName, L" ", myNames);
  split(pname, L" ", inNames);

  for (size_t k = 0; k < myNames.size(); k++)
    myNames[k] = canonizeName(myNames[k].c_str());

  int nMatched = 0;
  for (size_t j = 0; j < inNames.size(); j++) {
    wstring inName = canonizeName(inNames[j].c_str());
    for (size_t k = 0; k < myNames.size(); k++) {
      if (myNames[k] == inName) {
        nMatched++;

        // Suppert changed last name in the most common case
        if (j == 0 && k == 0 && inNames.size() == 2 && myNames.size() == 2) {
          return true;
        }
        break;
      }
    }
  }

  return nMatched >= min<int>(myNames.size(), 2);
}

bool oRunner::autoAssignBib() {
  if (Class == 0 || !getBib().empty())
    return !getBib().empty();

  int maxbib = 0;
  wchar_t pattern[32];
  int noBib = 0;
  int withBib = 0;
  for(oRunnerList::iterator it = oe->Runners.begin(); it !=oe->Runners.end();++it) {
    if (it->Class == Class) {
      const wstring &bib = it->getBib();
      if (!bib.empty()) {
        withBib++;
        int ibib = oClass::extractBibPattern(bib, pattern); 
        maxbib = max(ibib, maxbib);
      }
      else
        noBib++;
    }
  }

  if (maxbib>0 && withBib>noBib) {
    wchar_t bib[32];
    swprintf_s(bib, pattern, maxbib+1);
    setBib(bib, maxbib+1, true, false);
    return true;
  }
  return false;
}

void oRunner::getSplitAnalysis(vector<int> &deltaTimes) const {
  deltaTimes.clear();
  vector<int> mp;

  if (splitTimes.empty() || !Class)
    return;

  if (Class->tSplitRevision == tSplitRevision)
    deltaTimes = tMissedTime;

  pCourse pc = getCourse(true);
  if (!pc)
    return;
  vector<int> reorder;
  if (pc->isAdapted())
    reorder = pc->getMapToOriginalOrder();
  else {
    reorder.reserve(pc->nControls+1);
    for (int k = 0; k <= pc->nControls; k++)
      reorder.push_back(k);
  }

  int id = pc->getId();

  if (Class->tSplitAnalysisData.count(id) == 0)
    Class->calculateSplits();

  const vector<int> &baseLine = Class->tSplitAnalysisData[id];
  const unsigned nc = pc->getNumControls();

  if (baseLine.size() != nc+1)
    return;

  vector<double> res(nc+1);

  double resSum = 0;
  double baseSum = 0;
  double bestTime = 0;
  for (size_t k = 0; k <= nc; k++) {
    res[k] = getSplitTime(k, false);
    if (res[k] > 0) {
      resSum += res[k];
      baseSum += baseLine[reorder[k]];
    }
    bestTime += baseLine[reorder[k]];
  }

  deltaTimes.resize(nc+1);

  // Adjust expected time by removing mistakes
  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[reorder[k]] / bestTime;
      int deltaAbs = int(floor(delta * resSum + 0.5));
      if (res[k]-deltaAbs < baseLine[reorder[k]])
        deltaAbs = int(res[k] - baseLine[reorder[k]]);

      if (deltaAbs>0)
        resSum -= deltaAbs;
    }
  }

  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[reorder[k]] / bestTime;

      int deltaAbs = int(floor(delta * resSum + 0.5));

      if (deltaAbs > 0) {
        if ( fabs(delta) > 1.0/100 && (20.0*deltaAbs)>res[k] && deltaAbs>=15)
          deltaTimes[k] = deltaAbs;

        res[k] -= deltaAbs;
        if (res[k] < baseLine[reorder[k]])
          res[k] = baseLine[reorder[k]];
      }
    }
  }

  resSum = 0;
  for (size_t k = 0; k <= nc; k++) {
    if (res[k] > 0) {
      resSum += res[k];
    }
  }

  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[reorder[k]] / bestTime;
      int deltaAbs = int(floor(delta * resSum + 0.5));

      if (deltaTimes[k]==0 && fabs(delta) > 1.0/100 && deltaAbs>=15)
        deltaTimes[k] = deltaAbs;
    }
  }
}

void oRunner::getLegPlaces(vector<int> &places) const
{
  places.clear();
  pCourse pc = getCourse(true);
  if (!pc || !Class || splitTimes.empty())
    return;
  if (Class->tSplitRevision == tSplitRevision)
    places = tPlaceLeg;

  int id = pc->getId();

  if (Class->tSplitAnalysisData.count(id) == 0)
    Class->calculateSplits();

  const unsigned nc = pc->getNumControls();

  places.resize(nc+1);
  int cc = pc->getCommonControl();
  for (unsigned k = 0; k<=nc; k++) {
    int to = cc;
    if (k<nc)
      to = pc->getControl(k) ? pc->getControl(k)->getId() : 0;
    int from = cc;
    if (k>0)
      from = pc->getControl(k-1) ? pc->getControl(k-1)->getId() : 0;

    int time = getSplitTime(k, false);

    if (time>0)
      places[k] = Class->getLegPlace(from, to, time);
    else
      places[k] = 0;
  }
}

void oRunner::getLegTimeAfter(vector<int> &times) const
{
  times.clear();
  if (splitTimes.empty() || !Class)
    return;
  if (Class->tSplitRevision == tSplitRevision) {
    times = tAfterLeg;
    return;
  }

  pCourse pc = getCourse(false);
  if (!pc)
    return;

  int id = pc->getId();

  if (Class->tCourseLegLeaderTime.count(id) == 0)
    Class->calculateSplits();

  const unsigned nc = pc->getNumControls();

  const vector<int> leaders = Class->tCourseLegLeaderTime[id];

  if (leaders.size() != nc + 1)
    return;

  times.resize(nc+1);

  for (unsigned k = 0; k<=nc; k++) {
    int s = getSplitTime(k, true);

    if (s>0) {
      times[k] = s - leaders[k];
      if (times[k]<0)
        times[k] = -1;
    }
    else
      times[k] = -1;
  }
  // Normalized order
  const vector<int> &reorder = getCourse(true)->getMapToOriginalOrder();
  if (!reorder.empty()) {
    vector<int> orderedTimes(times.size());
    for (size_t k = 0; k < times.size(); k++) {
      orderedTimes[k] = times[reorder[k]];
    }
    times.swap(orderedTimes);
  }
}

void oRunner::getLegTimeAfterAcc(vector<int> &times) const
{
  times.clear();
  if (splitTimes.empty() || !Class || tStartTime<=0)
    return;
  if (Class->tSplitRevision == tSplitRevision)
    times = tAfterLegAcc;

  pCourse pc = getCourse(false); //XXX Does not work for loop courses
  if (!pc)
    return;

  int id = pc->getId();

  if (Class->tCourseAccLegLeaderTime.count(id) == 0)
    Class->calculateSplits();

  const unsigned nc = pc->getNumControls();

  const vector<int> leaders = Class->tCourseAccLegLeaderTime[id];
  const vector<SplitData> &sp = getSplitTimes(true);
  if (leaders.size() != nc + 1)
    return;
  //xxx reorder output
  times.resize(nc+1);

  for (unsigned k = 0; k<=nc; k++) {
    int s = 0;
    if (k < sp.size())
      s = sp[k].time;
    else if (k==nc)
      s = FinishTime;

    if (s>0) {
      times[k] = s - tStartTime - leaders[k];
      if (times[k]<0)
        times[k] = -1;
    }
    else
      times[k] = -1;
  }

   // Normalized order
  const vector<int> &reorder = getCourse(true)->getMapToOriginalOrder();
  if (!reorder.empty()) {
    vector<int> orderedTimes(times.size());
    for (size_t k = 0; k < times.size(); k++) {
      orderedTimes[k] = times[reorder[k]];
    }
    times.swap(orderedTimes);
  }
}

void oRunner::getLegPlacesAcc(vector<int> &places) const
{
  places.clear();
  pCourse pc = getCourse(false);
  if (!pc || !Class)
    return;
  if (splitTimes.empty() || tStartTime<=0)
    return;
  if (Class->tSplitRevision == tSplitRevision) {
    places = tPlaceLegAcc;
    return;
  }

  int id = pc->getId();
  const unsigned nc = pc->getNumControls();
  const vector<SplitData> &sp = getSplitTimes(true);
  places.resize(nc+1);
  for (unsigned k = 0; k<=nc; k++) {
    int s = 0;
    if (k < sp.size())
      s = sp[k].time;
    else if (k==nc)
      s = FinishTime;

    if (s>0) {
      int time = s - tStartTime;

      if (time>0)
        places[k] = Class->getAccLegPlace(id, k, time);
      else
        places[k] = 0;
    }
  }

  // Normalized order
  const vector<int> &reorder = getCourse(true)->getMapToOriginalOrder();
  if (!reorder.empty()) {
    vector<int> orderedPlaces(places.size());
    for (size_t k = 0; k < places.size(); k++) {
      orderedPlaces[k] = places[reorder[k]];
    }
    places.swap(orderedPlaces);
  }
}

void oRunner::setupRunnerStatistics() const
{
  if (!Class)
    return;
  if (Class->tSplitRevision == tSplitRevision)
    return;

  getSplitAnalysis(tMissedTime);
  getLegPlaces(tPlaceLeg);
  getLegTimeAfter(tAfterLeg);
  getLegPlacesAcc(tPlaceLegAcc);
  getLegPlacesAcc(tAfterLegAcc);
  tSplitRevision = Class->tSplitRevision;
}

int oRunner::getMissedTime(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tMissedTime.size())
    return tMissedTime[ctrlNo];
  else
    return -1;
}

wstring oRunner::getMissedTimeS() const
{
  setupRunnerStatistics();
  int t = 0;
  for (size_t k = 0; k<tMissedTime.size(); k++)
    if (tMissedTime[k]>0)
      t += tMissedTime[k];

  return getTimeMS(t);
}

wstring oRunner::getMissedTimeS(int ctrlNo) const
{
  int t = getMissedTime(ctrlNo);
  if (t>0)
    return getTimeMS(t);
  else
    return L"";
}

int oRunner::getLegPlace(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tPlaceLeg.size())
    return tPlaceLeg[ctrlNo];
  else
    return 0;
}

int oRunner::getLegTimeAfter(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tAfterLeg.size())
    return tAfterLeg[ctrlNo];
  else
    return -1;
}

int oRunner::getLegPlaceAcc(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tPlaceLegAcc.size())
    return tPlaceLegAcc[ctrlNo];
  else
    return 0;
}

int oRunner::getLegTimeAfterAcc(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tAfterLegAcc.size())
    return tAfterLegAcc[ctrlNo];
  else
    return -1;
}

int oRunner::getTimeWhenPlaceFixed() const {
  if (!Class || !statusOK())
    return -1;

#ifndef MEOSDB
  if (unsigned(tLeg) >= Class->tResultInfo.size()) {
    oe->analyzeClassResultStatus();
    if (unsigned(tLeg) >= Class->tResultInfo.size())
      return -1;
  }
#endif

  int lst =  Class->tResultInfo[tLeg].lastStartTime;
  return lst > 0 ? lst + getRunningTime() : lst;
}


pRunner oRunner::getMatchedRunner(const SICard &sic) const {
  if (multiRunner.size() == 0 && tParentRunner == 0)
    return pRunner(this);
  if (!Class)
    return pRunner(this);
  if (Class->getLegType(tLeg) != LTExtra)
    return pRunner(this);

  const vector<pRunner> &multiV = tParentRunner ? tParentRunner->multiRunner : multiRunner;
  
  vector<pRunner> multiOrdered;
  multiOrdered.push_back( tParentRunner ? tParentRunner : pRunner(this));
  multiOrdered.insert(multiOrdered.end(), multiV.begin(), multiV.end());

  int Distance=-1000;
  pRunner r = 0; //Best runner
  
  for (size_t k = 0; k<multiOrdered.size(); k++) {
    if (!multiOrdered[k] || multiOrdered[k]->Card || multiOrdered[k]->getStatus() != StatusUnknown)
      continue;

    if (Class->getLegType(multiOrdered[k]->tLeg) != LTExtra)
      return pRunner(this);

    vector<pCourse> crs;

    if (Class->hasCoursePool()) {
      Class->getCourses(multiOrdered[k]->tLeg, crs);
    }
    else {
      pCourse pc = multiOrdered[k]->getCourse(false);
      crs.push_back(pc);
    }

    for (size_t j = 0; j < crs.size(); j++) { 
      pCourse pc = crs[j];
      if (!pc)
        continue;

      int d = pc->distance(sic);

      if (d>=0) {
        if (Distance<0) Distance=1000;
        if (d<Distance) {
          Distance=d;
          r = multiOrdered[k];
        }
      }
      else {
        if (Distance<0 && d>Distance) {
          Distance=d;
          r = multiOrdered[k];
        }
      }
    }
  }

  if (r)
    return r;
  else
    return pRunner(this);
}

int oRunner::getTotalRunningTime(int time, bool includeInput) const {
  if (tStartTime < 0)
    return 0;
  if (tInTeam == 0 || tLeg == 0) {
    if (time == FinishTime)
      return getRunningTime() + (includeInput ? inputTime : 0);
    else
      return time-tStartTime + (includeInput ? inputTime : 0);
  }
  else {
    if (Class == 0 || unsigned(tLeg) >= Class->legInfo.size())
      return 0;

    if (time == FinishTime) {
      return tInTeam->getLegRunningTime(tLeg, includeInput); // Use the official running time in this case (which works with parallel legs)
    }

    int baseleg = tLeg;
    while (baseleg>0 && (Class->legInfo[baseleg].isParallel() ||
                         Class->legInfo[baseleg].isOptional())) {
      baseleg--;
    }

    int leg = baseleg-1;
    while (leg>0 && (Class->legInfo[leg].legMethod == LTExtra || Class->legInfo[leg].legMethod == LTIgnore)) {
      leg--;
    }

    int pt = leg>=0 ? tInTeam->getLegRunningTime(leg, includeInput) : 0;
    if (pt>0)
      return pt + time - tStartTime;
    else if (tInTeam->tStartTime > 0)
      return (time - tInTeam->tStartTime) + (includeInput ? tInTeam->inputTime : 0);
    else
      return 0;
  }
}

  // Get the complete name, including team and club.
wstring oRunner::getCompleteIdentification(bool includeExtra) const {
  if (tInTeam == 0 || !Class || tInTeam->getName() == sName) {
    if (Club)
      return getName() + L" (" + Club->name + L")";
    else
      return getName();
  }
  else {
    wstring names;
    pClass clsToUse = tInTeam->Class != 0 ? tInTeam->Class : Class;
    // Get many names for paralell legs
    int firstLeg = tLeg;
    LegTypes lt=clsToUse->getLegType(firstLeg--);
    while(firstLeg>=0 && (lt==LTIgnore || lt==LTParallel || lt==LTParallelOptional || (lt==LTExtra && includeExtra)) )
      lt=clsToUse->getLegType(firstLeg--);

    for (size_t k = firstLeg+1; k < clsToUse->legInfo.size(); k++) {
      pRunner r = tInTeam->getRunner(k);
      if (r) {
        if (names.empty())
          names = r->tRealName;
        else
          names += L"/" + r->tRealName;
      }
      lt = clsToUse->getLegType(k + 1);
      if ( !(lt==LTIgnore || lt==LTParallel || lt == LTParallelOptional || (lt==LTExtra && includeExtra)))
        break;
    }

    if (clsToUse->legInfo.size() <= 2)
      return names + L" (" + tInTeam->sName + L")";
    else
      return tInTeam->sName + L" (" + names + L")";
  }
}

RunnerStatus oAbstractRunner::getTotalStatus() const {
  if (tStatus == StatusUnknown && inputStatus != StatusNotCompetiting)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  return max(tStatus, inputStatus);
}

RunnerStatus oRunner::getTotalStatus() const {
  if (tStatus == StatusUnknown && inputStatus != StatusNotCompetiting)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  if (tInTeam == 0 || tLeg == 0)
    return max(tStatus, inputStatus);
  else {
    RunnerStatus st = tInTeam->getLegStatus(tLeg-1, true);

    if (tLeg + 1 == tInTeam->getNumRunners())
      st = max(st, tInTeam->getStatus());

    if (st == StatusOK || st == StatusUnknown)
      return tStatus;
    else
      return max(max(tStatus, st), inputStatus);
  }
}

void oRunner::remove()
{
  if (oe) {
    vector<int> me;
    me.push_back(Id);
    oe->removeRunner(me);
  }
}

bool oRunner::canRemove() const
{
  return !oe->isRunnerUsed(Id);
}

void oAbstractRunner::setInputTime(const wstring &time) {
  int t = convertAbsoluteTimeMS(time);
  if (t != inputTime) {
    inputTime = t;
    updateChanged();
  }
}

wstring oAbstractRunner::getInputTimeS() const {
  if (inputTime > 0)
    return formatTime(inputTime);
  else
    return makeDash(L"-");
}

void oAbstractRunner::setInputStatus(RunnerStatus s) {
  if (inputStatus != s) {
    inputStatus = s;
    updateChanged();
  }
}

wstring oAbstractRunner::getInputStatusS() const {
  return oe->formatStatus(inputStatus);
}

void oAbstractRunner::setInputPoints(int p)
{
  if (p != inputPoints) {
    inputPoints = p;
    updateChanged();
  }
}

void oAbstractRunner::setInputPlace(int p)
{
  if (p != inputPlace) {
    inputPlace = p;
    updateChanged();
  }
}

void oRunner::setInputData(const oRunner &r) {
  if (!r.multiRunner.empty() && r.multiRunner.back() && r.multiRunner.back() != &r)
    setInputData(*r.multiRunner.back());
  else {
    oDataInterface dest = getDI();
    oDataConstInterface src = r.getDCI();

    if (r.tStatus != StatusNotCompetiting) {
      inputTime = r.getTotalRunningTime(r.FinishTime, true);
      inputStatus = r.getTotalStatus();
      if (r.tInTeam) { // If a team has not status ok, transfer this status to all team members.
        if (r.tInTeam->getTotalStatus() > StatusOK)
          inputStatus = r.tInTeam->getTotalStatus();
      }
      inputPoints = r.getRogainingPoints(true) + r.inputPoints;
      inputPlace = r.tTotalPlace < 99000 ? r.tTotalPlace : 0;
    }
    else {
      // Copy input
      inputTime = r.inputTime;
      inputStatus = r.inputStatus;
      inputPoints = r.inputPoints;
      inputPlace = r.inputPlace;
    }

    if (r.getClubRef())
      setClub(r.getClub());
      
    if (!Card && r.isTransferCardNoNextStage()) {
      setCardNo(r.getCardNo(), false);
      dest.setInt("CardFee", src.getInt("CardFee"));
      setTransferCardNoNextStage(true);
    }
    // Copy flags.
    // copy....
    
    dest.setInt("TransferFlags", src.getInt("TransferFlags"));
    dest.setString("Nationality", src.getString("Nationality"));
    dest.setString("Country", src.getString("Country"));

    dest.setInt("Fee", src.getInt("Fee"));
    dest.setInt("Paid", src.getInt("Paid"));
    dest.setInt("Taxable", src.getInt("Taxable"));
  }
}

void oEvent::getDBRunnersInEvent(intkeymap<pClass, __int64> &runners) const {
  runners.clear();
  for (oRunnerList::const_iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    __int64 id = it->getExtIdentifier();
    if (id != 0)
      runners.insert(id, it->Class);
  }
}

void oRunner::init(const RunnerWDBEntry &dbr) {
  setTemporary();
  dbr.getName(sName);
  getRealName(sName, tRealName);
  CardNo = dbr.dbe().cardNo;
  Club = oe->getRunnerDatabase().getClub(dbr.dbe().clubNo);
  getDI().setString("Nationality", dbr.getNationality());
  getDI().setInt("BirthYear", dbr.getBirthYear());
  getDI().setString("Sex", dbr.getSex());
  setExtIdentifier(dbr.getExtId());
}

void oEvent::selectRunners(const wstring &classType, int lowAge,
                           int highAge, const wstring &firstDate,
                           const wstring &lastDate, bool includeWithFee,
                           vector<pRunner> &output) const {
  oRunnerList::const_iterator it;
  int cid = 0;
  if (classType.length() > 2 && classType.substr(0,2) == L"::")
    cid = _wtoi(classType.c_str() + 2);

  output.clear();

  int firstD = 0, lastD = 0;
  if (!firstDate.empty()) {
    firstD = convertDateYMS(firstDate, true);
    if (firstD <= 0)
      throw meosException(L"Felaktigt datumformat 'X' (Anv�nd ����-MM-DD).#" + firstDate);
  }

  if (!lastDate.empty()) {
    lastD = convertDateYMS(lastDate, true);
    if (lastD <= 0)
      throw meosException(L"Felaktigt datumformat 'X' (Anv�nd ����-MM-DD).#" + lastDate);
  }


  bool allClass = classType == L"*";
  for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->skip())
      continue;

    const pClass pc = it->Class;
    if (cid > 0 && (pc == 0 || pc->getId() != cid))
      continue;

    if (cid == 0 && !allClass) {
      if ((pc && pc->getType()!=classType) || (pc==0 && !classType.empty()))
        continue;
    }

    int age = it->getBirthAge();
    if (age > 0 && (lowAge > 0 || highAge > 0)) {
      if (lowAge > highAge)
        throw meosException("Undre �ldersgr�nsen �r h�gre �n den �vre.");

      if (age < lowAge || age > highAge)
        continue;
      /*
      bool ageOK = false;
      if (lowAge > 0 && age <= lowAge)
        ageOK = true;
      else if (highAge > 0 && age >= highAge)
        ageOK = true;

      if (!ageOK)
        continue;*/
    }

    int date = it->getDCI().getInt("EntryDate");
    if (date > 0) {
      if (firstD > 0 && date < firstD)
        continue;
      if (lastD > 0 && date > lastD)
        continue;

    }

    if (!includeWithFee) {
      int fee = it->getDCI().getInt("Fee");
      if (fee != 0)
        continue;
    }
    //    string date = di.getDate("EntryDate");

    output.push_back(pRunner(&*it));
  }
}

void oRunner::changeId(int newId) {
  pRunner old = oe->runnerById[Id];
  if (old == this)
    oe->runnerById.remove(Id);

  oBase::changeId(newId);

  oe->runnerById[newId] = this;
}

const vector<SplitData> &oRunner::getSplitTimes(bool normalized) const {
  if (!normalized)
    return splitTimes;
  else {
    pCourse pc = getCourse(true);
    if (pc && pc->isAdapted() && splitTimes.size() == pc->nControls) {
      if (!normalizedSplitTimes.empty())
        return normalizedSplitTimes;
      const vector<int> &mapToOriginal = pc->getMapToOriginalOrder();
      normalizedSplitTimes.resize(splitTimes.size()); // nControls
      vector<int> orderedSplits(splitTimes.size() + 1, -1);

      for (int k = 0; k < pc->nControls; k++) {
        if (splitTimes[k].hasTime()) {
          int t = -1;
          int j = k - 1;
          while (j >= -1 && t == -1) {
            if (j == -1)
              t = getStartTime();
            else if (splitTimes[j].hasTime())
              t = splitTimes[j].time;
            j--;
          }
          orderedSplits[mapToOriginal[k]] = splitTimes[k].time - t;
        }
      }

      // Last to finish
      {
        int t = -1;
        int j = pc->nControls - 1;
        while (j >= -1 && t == -1) {
          if (j == -1)
            t = getStartTime();
          else if (splitTimes[j].hasTime())
            t = splitTimes[j].time;
          j--;
        }
        orderedSplits[mapToOriginal[pc->nControls]] = FinishTime - t;
      }

      int accumulatedTime = getStartTime();
      for (int k = 0; k < pc->nControls; k++) {
        if (orderedSplits[k] > 0) {
          accumulatedTime += orderedSplits[k];
          normalizedSplitTimes[k].setPunchTime(accumulatedTime);
        }
        else
          normalizedSplitTimes[k].setNotPunched();
      }

      return normalizedSplitTimes;
    }
    return splitTimes;
  }
}

void oRunner::markClassChanged(int controlId) {
  assert(controlId < 4096);
  if (Class) {
    Class->markSQLChanged(tLeg, controlId);
    pClass cls2 = getClassRef(true);
    if (cls2 != Class)
      cls2->markSQLChanged(-1, controlId);

    if (tInTeam && tInTeam->Class != Class && tInTeam->Class) {
      tInTeam->Class->markSQLChanged(tLeg, controlId);
    }
  }
  else if (oe)
    oe->globalModification = true;
}

void oAbstractRunner::changedObject() {
  markClassChanged(-1);
}

int oAbstractRunner::getTimeAdjustment() const {
  if (oe->dataRevision != tAdjustDataRevision) {
    oDataConstInterface dci = getDCI();
    tTimeAdjustment = dci.getInt("TimeAdjust");
    tPointAdjustment = dci.getInt("PointAdjust");
    tAdjustDataRevision = oe->dataRevision;
  }
  return tTimeAdjustment;
}
 
int oAbstractRunner::getPointAdjustment() const {
  if (oe->dataRevision != tAdjustDataRevision) {
    getTimeAdjustment(); //Setup cache
  }
  return tPointAdjustment;
}

void oAbstractRunner::setTimeAdjustment(int adjust) {
  tTimeAdjustment = adjust;
  getDI().setInt("TimeAdjust", adjust);
}

void oAbstractRunner::setPointAdjustment(int adjust) {
  tPointAdjustment = adjust;
  getDI().setInt("PointAdjust", adjust);
}

int oRunner::getRogainingPoints(bool multidayTotal) const {
  if (multidayTotal)
    return inputPoints + tRogainingPoints;
  else
    return tRogainingPoints;
}

int oRunner::getRogainingReduction() const {
  return tReduction;
}

int oRunner::getRogainingPointsGross() const {
  return tRogainingPointsGross;
}

int oRunner::getRogainingOvertime() const {
  return tRogainingOvertime;
}

void oAbstractRunner::TempResult::reset() {
  runningTime = 0;
  timeAfter = 0;
  points = 0;
  place = 0;
  startTime = 0;
  status = StatusUnknown;
  internalScore = 0;
}

oAbstractRunner::TempResult::TempResult() {
  reset();
}

oAbstractRunner::TempResult::TempResult(RunnerStatus statusIn, 
                                        int startTimeIn, 
                                        int runningTimeIn,
                                        int pointsIn) :status(statusIn),
                                        startTime(startTimeIn), runningTime(runningTimeIn),
                                        timeAfter(0), points(0), place(0) {
}

const oAbstractRunner::TempResult &oAbstractRunner::getTempResult(int tempResultIndex) const {
  return tmpResult; //Ignore index for now...
  /*if (tempResultIndex == 0)
    return tmpResult;
  else
    throw meosException("Not implemented");*/
}

oAbstractRunner::TempResult &oAbstractRunner::getTempResult()  {
  return tmpResult; 
}


void oAbstractRunner::setTempResultZero(const TempResult &tr)  {
  tmpResult = tr;
}

const wstring &oAbstractRunner::TempResult::getStatusS(RunnerStatus inputStatus) const {
  if (inputStatus == StatusOK)
    return oEvent::formatStatus(getStatus());
  else if (inputStatus == StatusUnknown)
    return oEvent::formatStatus(StatusUnknown);
  else
    return oEvent::formatStatus(max(inputStatus, getStatus()));
}

const wstring &oAbstractRunner::TempResult::getPrintPlaceS(bool withDot) const {
  int p=getPlace();
  if (p>0 && p<10000){
    if (withDot) {
      wstring &res = StringCache::getInstance().wget();
      res = itow(p);
      res += L".";
      return res;
    }
    else
      return itow(p);
  }
  return _EmptyWString;
}

const wstring &oAbstractRunner::TempResult::getRunningTimeS(int inputTime) const {
  return formatTime(getRunningTime() + inputTime);
}

const wstring &oAbstractRunner::TempResult::getFinishTimeS(const oEvent *oe) const {
  return oe->getAbsTime(getFinishTime());
}

const wstring &oAbstractRunner::TempResult::getStartTimeS(const oEvent *oe) const {
  int st = getStartTime();
  if (st > 0)
      return oe->getAbsTime(st);
  else return makeDash(L"-");
}

const wstring &oAbstractRunner::TempResult::getOutputTime(int ix) const {
  int t = size_t(ix) < outputTimes.size() ? outputTimes[ix] : 0;
  return formatTime(t);
}

int oAbstractRunner::TempResult::getOutputNumber(int ix) const {
  return size_t(ix) < outputNumbers.size() ? outputNumbers[ix] : 0;
}

void oAbstractRunner::resetInputData() {
  setInputPlace(0);
  if (0 != inputTime) {
    inputTime = 0;
    updateChanged();
  }
  setInputStatus(StatusNotCompetiting);
  setInputPoints(0);
}

bool oRunner::isTransferCardNoNextStage() const {
  return hasFlag(FlagUpdateCard);
}

void oRunner::setTransferCardNoNextStage(bool state) {
  setFlag(FlagUpdateCard, state);
}

bool oAbstractRunner::hasFlag(TransferFlags flag) const {
  return (getDCI().getInt("TransferFlags") & flag) != 0;
}

void oAbstractRunner::setFlag(TransferFlags flag, bool onoff) {
  int cf = getDCI().getInt("TransferFlags");
  cf = onoff ? (cf | flag) : (cf & (~flag));
  getDI().setInt("TransferFlags", cf);
}

int oRunner::getNumShortening() const {
  if (oe->dataRevision != tShortenDataRevision) {
    oDataConstInterface dci = getDCI();
    tNumShortening = dci.getInt("Shorten");
    tShortenDataRevision = oe->dataRevision;
  }
  return tNumShortening;
}

void oRunner::setNumShortening(int numShorten) {
  tNumShortening = numShorten;
  tShortenDataRevision = oe->dataRevision;
  oDataInterface di = getDI();
  di.setInt("Shorten", numShorten);
}

int oAbstractRunner::getEntrySource() const {
  return getDCI().getInt("EntrySource");
}

void oAbstractRunner::setEntrySource(int src) {
  getDI().setInt("EntrySource", src);
}

void oAbstractRunner::flagEntryTouched(bool flag) {
  tEntryTouched = flag;
}

bool oAbstractRunner::isEntryTouched() const {
  return tEntryTouched;
}

int oRunner::getTotalTimeInput() const {
  if (tInTeam) {
    if (getLegNumber()>0) { 
      return tInTeam->getLegRunningTime(getLegNumber()-1, true);
    }
    else {
      return tInTeam->getInputTime();
    }
  }
  else {
    return getInputTime();
  }
}


RunnerStatus oRunner::getTotalStatusInput() const {
  RunnerStatus inStatus = StatusOK;
  if (tInTeam) {
    const pTeam t = tInTeam;
    if (getLegNumber()>0) { 
      inStatus = t->getLegStatus(getLegNumber()-1, true);
    }
    else {
      inStatus = t->getInputStatus();
    }
  }
  else {
    inStatus = getInputStatus();
  }
  return inStatus;
}

bool oAbstractRunner::startTimeAvailable() const {
  if (getFinishTime() > 0)
    return true;

  return getStartTime() > 0;
}

bool oRunner::startTimeAvailable() const {
  if (getFinishTime() > 0)
    return true;

  int st = getStartTime();
  bool definedTime = st > 0;

  if (!definedTime)
    return false;

  if (!Class || !tInTeam || tLeg == 0)
    return definedTime; 
  
  // Check if time is restart time
  int restart = Class->getRestartTime(tLeg);
  if (st == restart && Class->getStartType(tLeg) == STChange) {
    int currentTime = oe->getComputerTime();
    int rope = Class->getRopeTime(tLeg);
    return rope != 0 && currentTime + 600 > rope;
  }

  return true;
}

int oRunner::getRanking() const {
  int rank = getDCI().getInt("Rank");
  if (rank <= 0)
    return MaxRankingConstant;
  else
    return rank;
}

void oAbstractRunner::hasManuallyUpdatedTimeStatus() {
  if (Class && Class->hasClassGlobalDependence()) {
    set<int> cls;
    oe->reEvaluateAll(cls, false);
  }
  if (Class) {
    Class->updateFinalClasses(dynamic_cast<oRunner *>(this), false);
  }
}

bool oRunner::canShareCard(const pRunner other, int newCardNo) const {
  if (!other || other->CardNo != newCardNo || newCardNo == 0)
    return true;

  if (getCard() && getCard()->getCardNo() == newCardNo)
    return true;

  if (other->skip() || other->getCard() || other == this ||
      other->getMultiRunner(0) == getMultiRunner(0))
    return true;

  if (!getTeam() || other->getTeam() != getTeam())
    return false;

  const oClass * tCls = getTeam()->getClassRef(false);
  if (!tCls || tCls != Class)
    return false;

  LegTypes lt1 = tCls->getLegType(tLeg);
  LegTypes lt2 = tCls->getLegType(other->tLeg);

  if (lt1 == LTGroup || lt2 == LTGroup)
    return false;

  int ln1, ln2, ord; 
  tCls->splitLegNumberParallel(tLeg, ln1, ord);
  tCls->splitLegNumberParallel(other->tLeg, ln2, ord);
  return ln1 != ln2;
}

int oAbstractRunner::getPaymentMode() const {
  return getDCI().getInt("PayMode");
}

void oAbstractRunner::setPaymentMode(int mode) {
  getDI().setInt("PayMode", mode);
}

bool oAbstractRunner::hasLateEntryFee() const {
  if (!Class)
    return false;
  int highFee = Class->getDCI().getInt("HighClassFee");
  int normalFee = Class->getDCI().getInt("ClassFee");
  
  int fee = getDCI().getInt("Fee");
  if (fee == normalFee || fee == 0)
    return false;
  else if (fee == highFee && highFee > normalFee && normalFee > 0)
    return true;

  wstring date = getEntryDate(true);
  oDataConstInterface odc = oe->getDCI();
  wstring oentry = odc.getDate("OrdinaryEntry");
  bool late = date > oentry && oentry >= L"2010-01-01";

  return late;
}

int oRunner::classInstance() const {
  if (classInstanceRev.first == oe->dataRevision)
    return classInstanceRev.second;
  classInstanceRev.second = getDCI().getInt("Heat");
  if (Class)
    classInstanceRev.second = min(classInstanceRev.second, Class->getNumQualificationFinalClasses());
  classInstanceRev.first = oe->dataRevision;
  return classInstanceRev.second;
}

const wstring &oAbstractRunner::getClass(bool virtualClass) const {
  if (Class) {
    if (virtualClass)
      return Class->getVirtualClass(classInstance())->Name;
    else
      return Class->Name;
  }
  
  else return _EmptyWString; 
}
