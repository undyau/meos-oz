/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2018 Melin Software HB

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

#include "stdafx.h"
#include "meos_util.h"
#include "oEvent.h"
#include <assert.h>
#include <algorithm>
#include "meosdb/sqltypes.h"
#include "Table.h"
#include "localizer.h"
#include "meosException.h"
#include "gdioutput.h"

oTeam::oTeam(oEvent *poe): oAbstractRunner(poe, false)
{
  Id=oe->getFreeTeamId();
  getDI().initData();
  correctionNeeded = false;
  StartNo=0;
}

oTeam::oTeam(oEvent *poe, int id): oAbstractRunner(poe, true) {
  Id=id;
  oe->qFreeTeamId = max(id, oe->qFreeTeamId);
  getDI().initData();
  correctionNeeded = false;
  StartNo=0;
}

oTeam::~oTeam(void) {
  /*for(unsigned i=0; i<Runners.size(); i++) {
    if (Runners[i] && Runners[i]->tInTeam==this) {
      assert(Runners[i]->tInTeam!=this);
      Runners[i]=0;
    }
  }*/
}

void oTeam::prepareRemove()
{
  for(unsigned i=0; i<Runners.size(); i++)
    setRunnerInternal(i, 0);
}

bool oTeam::write(xmlparser &xml)
{
  if (Removed) return true;

  xml.startTag("Team");
  xml.write("Id", Id);
  xml.write("StartNo", StartNo);
  xml.write("Updated", Modified.getStamp());
  xml.write("Name", sName);
  xml.write("Start", startTime);
  xml.write("Finish", FinishTime);
  xml.write("Status", status);
  xml.write("Runners", getRunners());

  if (Club) xml.write("Club", Club->Id);
  if (Class) xml.write("Class", Class->Id);

  xml.write("InputPoint", inputPoints);
  if (inputStatus != StatusOK)
    xml.write("InputStatus", itos(inputStatus)); //Force write of 0
  xml.write("InputTime", inputTime);
  xml.write("InputPlace", inputPlace);


  getDI().write(xml);
  xml.endTag();

  return true;
}

void oTeam::set(const xmlobject &xo)
{
  xmlList xl;
  xo.getObjects(xl);
  xmlList::const_iterator it;

  for(it=xl.begin(); it != xl.end(); ++it){
    if (it->is("Id")){
      Id=it->getInt();
    }
    else if (it->is("Name")){
      sName=it->getw();
    }
    else if (it->is("StartNo")){
      StartNo=it->getInt();
    }
    else if (it->is("Start")){
      tStartTime = startTime = it->getInt();
    }
    else if (it->is("Finish")){
      FinishTime=it->getInt();
    }
    else if (it->is("Status")) {
      unsigned rawStatus = it->getInt();
      tStatus = status=RunnerStatus(rawStatus < 100u ? rawStatus : 0);
    }
    else if (it->is("Class")){
      Class=oe->getClass(it->getInt());
    }
    else if (it->is("Club")){
      Club=oe->getClub(it->getInt());
    }
    else if (it->is("Runners")){
      vector<int> r;
      decodeRunners(it->getRaw(), r);
      importRunners(r);
    }
    else if (it->is("InputTime")) {
      inputTime = it->getInt();
    }
    else if (it->is("InputStatus")) {
      unsigned rawStatus = it->getInt();
      inputStatus = RunnerStatus(rawStatus < 100u ? rawStatus : 0);
    }
    else if (it->is("InputPoint")) {
      inputPoints = it->getInt();
    }
    else if (it->is("InputPlace")) {
      inputPlace = it->getInt();
    }
    else if (it->is("Updated")){
      Modified.setStamp(it->getRaw());
    }
    else if (it->is("oData")) {
      getDI().set(*it);
    }
  }
}

string oTeam::getRunners() const
{
  string str="";
  char bf[16];
  unsigned m=0;

  for(m=0;m<Runners.size();m++){
    if (Runners[m]){
      sprintf_s(bf, 16, "%d;", Runners[m]->Id);
      str+=bf;
    }
    else str+="0;";
  }
  return str;
}

void oTeam::decodeRunners(const string &rns, vector<int> &rid)
{
  const char *str=rns.c_str();
  rid.clear();

  int n=0;

  while (*str) {
    int cid=atoi(str);

    while(*str && (*str!=';' && *str!=',')) str++;
    if (*str==';'  || *str==',') str++;

    rid.push_back(cid);
    n++;
  }
}

void oTeam::importRunners(const vector<int> &rns)
{
  Runners.resize(rns.size());
  for (size_t n=0;n<rns.size(); n++) {
    if (rns[n]>0)
      Runners[n] = oe->getRunner(rns[n], 0);
    else
      Runners[n] = 0;
  }
}

void oTeam::importRunners(const vector<pRunner> &rns)
{
  // Unlink old runners
  for (size_t k = rns.size(); k<Runners.size(); k++) {
    if (Runners[k] && Runners[k]->tInTeam == this) {
      Runners[k]->tInTeam = 0;
      Runners[k]->tLeg = 0;
    }
  }

  Runners.resize(rns.size());
  for (size_t n=0;n<rns.size(); n++) {
    if (Runners[n] && rns[n] != Runners[n]) {
      if (Runners[n]->tInTeam == this) {
        Runners[n]->tInTeam = 0;
        Runners[n]->tLeg = 0;
      }
    }
    Runners[n] = rns[n];
  }
}


void oEvent::removeTeam(int Id)
{
  oTeamList::iterator it;
  for (it = Teams.begin(); it != Teams.end(); ++it) {
    if (it->getId() == Id) {
      if (HasDBConnection && !it->isRemoved())
        msRemove(&*it);
      dataRevision++;
      it->prepareRemove();
      Teams.erase(it);
      teamById.erase(Id);
      updateTabs();
      return;
    }
  }
}

void oTeam::setRunner(unsigned i, pRunner r, bool sync)
{
  if (i>=Runners.size()) {
    if (i>=0 && i<100)
      Runners.resize(i+1);
    else
      throw std::exception("Bad runner index");
  }

  if (Runners[i]==r)
    return;

  int oldRaceId = 0;
  if (Runners[i]) {
    oldRaceId = Runners[i]->getDCI().getInt("RaceId");
    Runners[i]->getDI().setInt("RaceId", 0);
  }
  setRunnerInternal(i, r);

  if (r) {
    if (tStatus == StatusDNS)
      setStatus(StatusUnknown, true, false);
    r->getDI().setInt("RaceId", oldRaceId);
    r->tInTeam=this;
    r->tLeg=i;
    r->createMultiRunner(true, sync);
  }

  if (Class) {
    int index=1; //Set multi runners
    for (unsigned k=i+1;k<Class->getNumStages(); k++) {
      if (Class->getLegRunner(k)==i) {
        if (r!=0) {
          pRunner mr=r->getMultiRunner(index++);

          if (mr) {
            mr->setName(r->getName(), true);
            mr->synchronize();
            setRunnerInternal(k, mr);
          }
        }
        else setRunnerInternal(k, 0);
      }
    }
  }
}

void oTeam::setRunnerInternal(int k, pRunner r)
{
  if (r==Runners[k]) {
    if (r) {
      r->tInTeam = this;
      r->tLeg = k;
    }
    return;
  }

  if (Runners[k]) {
    assert(Runners[k]->tInTeam == 0 || Runners[k]->tInTeam == this);
    Runners[k]->tInTeam = 0;
    Runners[k]->tLeg = 0;
  }

  // Reset old team
  if (r && r->tInTeam) {
    if (r->tInTeam->Runners[r->tLeg] != 0) {
      r->tInTeam->Runners[r->tLeg] = 0;
      r->tInTeam->updateChanged();
      if (r->tInTeam != this)
        r->tInTeam->synchronize(true);
    }
  }

  Runners[k]=r;

  if (Runners[k]) {
    Runners[k]->tInTeam = this;
    Runners[k]->tLeg = k;
    if (Class && Class->getLegType(k) != LTGroup)
      Runners[k]->setClassId(getClassId(false), false);
  }
  updateChanged();
}

wstring oTeam::getLegFinishTimeS(int leg) const
{
  if (leg==-1)
    leg=Runners.size()-1;

  if (unsigned(leg)<Runners.size() && Runners[leg])
    return Runners[leg]->getFinishTimeS();
  else return L"-";
}

int oTeam::getLegToUse(int leg) const {
  if (Runners.empty())
    return 0;
  if (leg==-1)
    leg=Runners.size()-1;
  int oleg = leg;
  if (Class && !Runners[leg]) {
    LegTypes lt = Class->getLegType(leg);
    while (leg>=0 && (lt == LTParallelOptional || lt == LTExtra || lt == LTIgnore) && !Runners[leg]) {
      if (leg == 0)
        return oleg; //Suitable leg not found
      leg--;
      lt = Class->getLegType(leg);
    }
  }
  return leg;
}

int oTeam::getLegFinishTime(int leg) const
{
  leg = getLegToUse(leg);
  //if (leg==-1)
  //leg=Runners.size()-1;

  if (Class) {
    pClass pc = Class;
    LegTypes lt = pc->getLegType(leg);
    while (leg> 0  && (lt == LTIgnore ||
              (lt == LTExtra && (!Runners[leg] || Runners[leg]->getFinishTime() <= 0)) ) ) {
      leg--;
      lt = pc->getLegType(leg);
    }
  }

  if (unsigned(leg)<Runners.size() && Runners[leg]) {
    int ft = Runners[leg]->getFinishTime();
    if (Class) {
      bool extra = Class->getLegType(leg) == LTExtra ||
                   Class->getLegType(leg+1) == LTExtra; 

      bool par = Class->isParallel(leg) ||
                 Class->isParallel(leg+1); 

      if (extra) {
        ft = 0;
        // Minimum over extra legs
        int ileg = leg;
        while (ileg > 0 && Class->getLegType(ileg) == LTExtra)
          ileg--;

        while (size_t(ileg) < Class->getNumStages()) {
          int ift = 0;
          if (Runners[ileg]) {
            ift = Runners[ileg]->getFinishTimeAdjusted();
          }

          if (ift > 0) {
            if (ft == 0)
              ft = ift;
            else
              ft = min(ft, ift);
          }

          ileg++;
          if (Class->getLegType(ileg) != LTExtra)
            break;
        }
      }
      else if (par) {
        ft = 0;
        // Maximum over parallel legs
        int ileg = leg;
        while (ileg > 0 && Class->isParallel(ileg))
          ileg--;

        while (size_t(ileg) < Class->getNumStages()) {
          int ift = 0;
          if (Runners[ileg]) {
            ift = Runners[ileg]->getFinishTimeAdjusted();
          }

          if (ift > 0) {
            if (ft == 0)
              ft = ift;
            else
              ft = max(ft, ift);
          }

          ileg++;
          if (!Class->isParallel(ileg))
            break;
        }
      }
    }
    return ft;
  }
  else return 0;
}

int oTeam::getRunningTime() const {
  return getLegRunningTime(-1, false);
}

int oTeam::getLegRunningTime(int leg, bool multidayTotal) const {
  return getLegRunningTimeUnadjusted(leg, multidayTotal) + getTimeAdjustment();
}

int oTeam::getLegRestingTime(int leg) const {
  if (!Class)
    return 0;

  int rest = 0;
  int R = min<int>(Runners.size(), leg+1);
  for (int k = 1; k < R; k++) {
    if (Class->getStartType(k) == STHunting && !Class->isParallel(k) &&
        Runners[k] && Runners[k-1]) {
         
      int ft = getLegRunningTimeUnadjusted(k-1, false) + tStartTime;
      int st = Runners[k]->getStartTime();

      if (ft > 0 && st > 0)
        rest += st - ft;      
    }
  }
  return rest;
}
  

int oTeam::getLegRunningTimeUnadjusted(int leg, bool multidayTotal) const {
  leg = getLegToUse(leg);

  int addon = 0;
  if (multidayTotal)
    addon = inputTime;

  if (unsigned(leg)<Runners.size() && Runners[leg]) {
    if (Class) {
      pClass pc=Class;

      LegTypes lt = pc->getLegType(leg);
      LegTypes ltNext = pc->getLegType(leg+1);
      if (ltNext == LTParallel || ltNext == LTParallelOptional || ltNext == LTExtra) // If the following leg is parallel, then so is this.
        lt = ltNext;

      switch(lt) {
        case LTNormal:
          if (Runners[leg]->prelStatusOK()) {
            int dt = leg>0 ? getLegRunningTimeUnadjusted(leg-1, false)+Runners[leg]->getRunningTime():0;
            return addon + max(Runners[leg]->getFinishTimeAdjusted()-(tStartTime + getLegRestingTime(leg)), dt);
          }
          else return 0;
        break;

        case LTParallelOptional:
        case LTParallel: //Take the longest time of this runner and the previous
          if (Runners[leg]->prelStatusOK()) {
            int pt=leg>0 ? getLegRunningTimeUnadjusted(leg-1, false) : 0;
            int rest = getLegRestingTime(leg);
            int finishT = Runners[leg]->getFinishTimeAdjusted();
            return addon + max(finishT-(tStartTime + rest), pt);
          }
          else return 0;
        break;

        case LTExtra: //Take the best time of this runner and the previous
          if (leg==0)
            return addon + max(Runners[leg]->getFinishTime()-tStartTime, 0);
          else {
            int baseLeg = leg;
            while (baseLeg > 0 && pc->getLegType(baseLeg) == LTExtra)
              baseLeg--;
            int baseTime = 0;
            if (baseLeg > 0)
              baseTime = getLegRunningTimeUnadjusted(baseLeg-1, multidayTotal);
            else 
              baseTime = addon;
        
            int cLeg = baseLeg;
            int legTime = 0;
            bool bad = false;
            do {
              if (Runners[cLeg] && Runners[cLeg]->getFinishTime() > 0) {
                int rt = Runners[cLeg]->getRunningTime();
                if (legTime == 0 || rt < legTime) {
                  bad = !Runners[cLeg]->prelStatusOK();
                  legTime = rt;
                }
              }
              cLeg++;
            }
            while (pc->getLegType(cLeg) == LTExtra);

            if (bad || legTime == 0)
              return 0;
            else 
              return baseTime + legTime;
          }
        break;

        case LTSum:
          if (Runners[leg]->prelStatusOK()) {
            if (leg==0)
              return addon + Runners[leg]->getRunningTime();
            else {
              int prev = getLegRunningTimeUnadjusted(leg-1, multidayTotal);
              if (prev == 0)
                return 0;
              else
                return Runners[leg]->getRunningTime() + prev;
            }
          }
          else return 0;

        case LTIgnore:
          if (leg==0)
            return 0;
          else
            return getLegRunningTimeUnadjusted(leg-1, multidayTotal);

        break;

        case LTGroup:
          return 0;
      }
    }
    else {
      int dt=addon + max(Runners[leg]->getFinishTime()-tStartTime, 0);
      int dt2=0;

      if (leg>0)
        dt2=getLegRunningTimeUnadjusted(leg-1, multidayTotal)+Runners[leg]->getRunningTime();

      return max(dt, dt2);
    }
  }
  return 0;
}

wstring oTeam::getLegRunningTimeS(int leg, bool multidayTotal) const
{
  if (leg==-1)
    leg = Runners.size()-1;

  int rt=getLegRunningTime(leg, multidayTotal);
  const wstring &bf = formatTime(rt);
  if (rt>0) {
    if ((unsigned(leg)<Runners.size() && Runners[leg] &&
      Class && Runners[leg]->getStartTime()==Class->getRestartTime(leg)) || getNumShortening(leg)>0)
      return L"*" + bf;
  }
  return bf;
}


RunnerStatus oTeam::getLegStatus(int leg, bool multidayTotal) const
{
  if (leg==-1)
    leg=Runners.size()-1;

  if (unsigned(leg)>=Runners.size())
    return StatusUnknown;

  if (multidayTotal) {
    RunnerStatus s = getLegStatus(leg, false);
    if (s == StatusUnknown && inputStatus != StatusNotCompetiting)
      return StatusUnknown;
    if (inputStatus == StatusUnknown)
      return StatusDNS;
    return max(inputStatus, s);
  }

  //Let the user specify a global team status disqualified.
  if (leg == (Runners.size()-1) && tStatus==StatusDQ)
    return tStatus;

  leg = getLegToUse(leg); // Ignore optional runners

  int s=0;

  if (!Class)
    return StatusUnknown;

  while(leg>0 && Class->getLegType(leg)==LTIgnore)
    leg--;

  for (int i=0;i<=leg;i++) {
    // Ignore runners to be ignored
    while(i<leg && Class->getLegType(i)==LTIgnore)
      i++;

    int st=Runners[i] ? Runners[i]->getStatus(): StatusDNS;
    int bestTime=Runners[i] ? Runners[i]->getFinishTime():0;

    //When Type Extra is used, the runner with the best time
    //is used for change. Then the status of this runner
    //should be carried forward.
    if (Class) while( (i+1) < int(Runners.size()) && Class->getLegType(i+1)==LTExtra) {
      i++;

      if (Runners[i]) {
        if (bestTime==0 || (Runners[i]->getFinishTime()>0 &&
                         Runners[i]->getFinishTime()<bestTime) ) {
          st=Runners[i]->getStatus();
          bestTime = Runners[i]->getFinishTime();
        }
      }
    }

    if (st==0)
      return RunnerStatus(s==StatusOK ? 0 : s);

    s=max(s, st);
  }

  // Allow global status DNS
  if (s==StatusUnknown && tStatus==StatusDNS)
    return tStatus;

  return RunnerStatus(s);
}

const wstring &oTeam::getLegStatusS(int leg, bool multidayTotal) const
{
  return oe->formatStatus(getLegStatus(leg, multidayTotal));
}

int oTeam::getLegPlace(int leg, bool multidayTotal) const {
  if (Class) {
    while(size_t(leg) < Class->legInfo.size() && Class->legInfo[leg].legMethod == LTIgnore)
      leg--;
  }
  if (!multidayTotal)
    return leg>=0 && leg<maxRunnersTeam ? _places[leg].p:_places[Runners.size()-1].p;
  else
    return leg>=0 && leg<maxRunnersTeam ? _places[leg].totalP:_places[Runners.size()-1].totalP;
}


wstring oTeam::getLegPlaceS(int leg, bool multidayTotal) const
{
  int p=getLegPlace(leg, multidayTotal);
  wchar_t bf[16];
  if (p>0 && p<10000){
    swprintf_s(bf, L"%d", p);
    return bf;
  }
  return _EmptyWString;
}

wstring oTeam::getLegPrintPlaceS(int leg, bool multidayTotal, bool withDot) const
{
  int p=getLegPlace(leg, multidayTotal);
  wchar_t bf[16];
  if (p>0 && p<10000){
    if (withDot) {
      swprintf_s(bf, L"%d.", p);
      return bf;
    }
    else
      return itow(p);
  }
  return _EmptyWString;
}

bool oTeam::compareResult(const oTeam &a, const oTeam &b)
{
  if (a.Class != b.Class) {
    if (a.Class) {
      if (b.Class) return a.Class->tSortIndex < b.Class->tSortIndex || (a.Class->tSortIndex == b.Class->tSortIndex && a.Class->Id < b.Class->Id);
      else return true;
    }
    else return false;
  }
  else if (a._sortStatus!=b._sortStatus)
    return a._sortStatus<b._sortStatus;
  else if (a._sortTime!=b._sortTime)
    return a._sortTime<b._sortTime;

  const wstring &as = a.getBib();
  const wstring &bs = b.getBib();
  if (as != bs) {
    return compareBib(as, bs);
  }

  int aix = a.getDCI().getInt("SortIndex");
  int bix = b.getDCI().getInt("SortIndex");
  if (aix != bix)
    return aix < bix;

  return CompareString(LOCALE_USER_DEFAULT, 0,
                       a.sName.c_str(), a.sName.length(),
                       b.sName.c_str(), b.sName.length()) == CSTR_LESS_THAN;
}

bool oTeam::compareStartTime(const oTeam &a, const oTeam &b)
{
  if (a.Class != b.Class) {
    if (a.Class) {
      if (b.Class)
        return a.Class->tSortIndex<b.Class->tSortIndex || (a.Class->tSortIndex == b.Class->tSortIndex && a.Class->Id < b.Class->Id);
      else return true;
    }
  }
  else if (a.tStartTime != b.tStartTime)
    return a.tStartTime < b.tStartTime;

  const wstring &as = a.getBib();
  const wstring &bs = b.getBib();
  if (as != bs) {
    return compareBib(as, bs);
  }

  int aix = a.getDCI().getInt("SortIndex");
  int bix = b.getDCI().getInt("SortIndex");
  if (aix != bix)
    return aix < bix;

  return CompareString(LOCALE_USER_DEFAULT, 0,
                       a.sName.c_str(), a.sName.length(),
                       b.sName.c_str(), b.sName.length()) == CSTR_LESS_THAN;
}

bool oTeam::compareSNO(const oTeam &a, const oTeam &b) {
  const wstring &as = a.getBib();
  const wstring &bs = b.getBib();

  if (as != bs) {
    return compareBib(as, bs);
  }
  else if (a.Class != b.Class) {
    if (a.Class) {
      if (b.Class) 
        return a.Class->tSortIndex < b.Class->tSortIndex || (a.Class->tSortIndex == b.Class->tSortIndex && a.Class->Id < b.Class->Id);
      else return true;
    }
  }

  return CompareString(LOCALE_USER_DEFAULT, 0,
                       a.sName.c_str(), a.sName.length(),
                       b.sName.c_str(), b.sName.length()) == CSTR_LESS_THAN;
}


bool oTeam::isRunnerUsed(int Id) const
{
  for(unsigned i=0;i<Runners.size(); i++) {
    if (Runners[i] && Runners[i]->Id==Id)
      return true;
  }
  return false;
}

void oTeam::setTeamNoStart(bool dns, RunnerStatus dnsStatus)
{
  if (dns) {
    assert(dnsStatus == StatusCANCEL || dnsStatus == StatusDNS);
    setStatus(dnsStatus, true, false);
    for(unsigned i=0;i<Runners.size(); i++) {
      if (Runners[i] && Runners[i]->getStatus()==StatusUnknown) {
        Runners[i]->setStatus(dnsStatus, true, false);
      }
    }
  }
  else {
    setStatus(StatusUnknown, true, false);
    for(unsigned i=0;i<Runners.size(); i++) {
      if (Runners[i] && (Runners[i]->getStatus()==StatusDNS || Runners[i]->getStatus() == StatusCANCEL)) {
        Runners[i]->setStatus(StatusUnknown, true, false);
      }
    }
  }
  // Do not sync here
}

static void compressStartTimes(vector<int> &availableStartTimes, int finishTime)
{
  for (size_t j=0; j<availableStartTimes.size(); j++)
    finishTime = max(finishTime, availableStartTimes[j]);

  availableStartTimes.resize(1);
  availableStartTimes[0] = finishTime;
}

static void addStartTime(vector<int> &availableStartTimes, int finishTime)
{
  for (size_t k=0; k<availableStartTimes.size(); k++)
    if (finishTime >= availableStartTimes[k]) {
      availableStartTimes.insert(availableStartTimes.begin()+k, finishTime);
      return;
    }

  availableStartTimes.push_back(finishTime);
}

static int getBestStartTime(vector<int> &availableStartTimes) {
  if (availableStartTimes.empty())
    return 0;
  int t = availableStartTimes.back();
  availableStartTimes.pop_back();
  return t;
}

void oTeam::quickApply() {
  if (unsigned(status) >= 100)
    status = StatusUnknown; // Enforce valid
  
  if (Class && Runners.size()!=size_t(Class->getNumStages())) {
    for (size_t k = Class->getNumStages(); k < Runners.size(); k++) {
      if (Runners[k] && Runners[k]->tInTeam) {
        Runners[k]->tInTeam = 0;
        Runners[k]->tLeg = 0;
        Runners[k]->tLegEquClass = 0;
        if (Runners[k]->Class == Class)
          Runners[k]->Class = 0;
        Runners[k]->updateChanged();
      }
    }
    Runners.resize(Class->getNumStages()); 
  }

  for (size_t i = 0; i < Runners.size(); i++) {
    if (Runners[i]) {
      if (Runners[i]->tInTeam && Runners[i]->tInTeam!=this) {
        Runners[i]->tInTeam->correctRemove(Runners[i]);
      }
      Runners[i]->tInTeam=this;
      Runners[i]->tLeg=i;
    }
  }
}



bool oTeam::apply(bool sync, pRunner source, bool setTmpOnly) {
  if (unsigned(status) >= 100)
    status = StatusUnknown; // Enforce correct status
  
  int lastStartTime = 0;
  RunnerStatus lastStatus = StatusUnknown;
  bool freeStart = Class ? Class->hasFreeStart() : false;
  int extraFinishTime = -1;

  if (Class && Runners.size()!=size_t(Class->getNumStages())) {
    for (size_t k = Class->getNumStages(); k < Runners.size(); k++) {
      if (Runners[k] && Runners[k]->tInTeam) {
        Runners[k]->tInTeam = 0;
        Runners[k]->tLeg = 0;
        Runners[k]->tLegEquClass = 0;
        if (Runners[k]->Class == Class)
          Runners[k]->Class = 0;
        Runners[k]->updateChanged();
      }
    }
    Runners.resize(Class->getNumStages());
  }
  const wstring &bib = getBib();
  BibMode bibMode = (BibMode)-1;
  tNumRestarts = 0;
  vector<int> availableStartTimes;
  for (size_t i=0;i<Runners.size(); i++) {
    if (!sync && i>0 && source!=0 && Runners[i-1] == source)
      return true;
    if (!Runners[i] && Class) {
       unsigned lr = Class->getLegRunner(i);

       if (lr<i && Runners[lr]) {
         Runners[lr]->createMultiRunner(false, sync);
         int dup=Class->getLegRunnerIndex(i);
         Runners[i]=Runners[lr]->getMultiRunner(dup);
       }
    }

    if (sync && Runners[i] && Class) {
      unsigned lr = Class->getLegRunner(i);
      if (lr == i && Runners[i]->tParentRunner) {
        pRunner parent = Runners[i]->tParentRunner;
        for (size_t kk = 0; kk<parent->multiRunner.size(); ++kk) {
          if (parent->multiRunner[kk] == Runners[i]) {
            parent->multiRunner.erase(parent->multiRunner.begin() + kk);
            Runners[i]->tParentRunner = 0;
            Runners[i]->tDuplicateLeg = 0;
            parent->markForCorrection();
            parent->updateChanged();
            Runners[i]->markForCorrection();
            Runners[i]->updateChanged();
            break;
          }
        }
      }
    }
    // Avoid duplicates, same runner running more
    // than one leg
    //(note: quadric complexity, assume total runner count is low)
    if (Runners[i]) {
      for (size_t k=0;k<i; k++)
        if (Runners[i] == Runners[k])
          Runners[i] = 0;
    }

    if (Runners[i]) {
      pClass actualClass = Runners[i]->getClassRef(true);
      if (Runners[i]->tInTeam && Runners[i]->tInTeam!=this) {
        Runners[i]->tInTeam->correctRemove(Runners[i]);
      }
      //assert(Runners[i]->tInTeam==0 || Runners[i]->tInTeam==this);
      Runners[i]->tInTeam = this;
      Runners[i]->tLeg = i;
      if (Class) {
        int unused;
        Class->splitLegNumberParallel(i, Runners[i]->tLegEquClass, unused);
      }
      else {
        Runners[i]->tLegEquClass = i;
      }

      if (actualClass == Class)
        Runners[i]->setStartNo(StartNo, setTmpOnly);
      if (!bib.empty() && Runners[i]->isChanged()) {
        if (bibMode == -1 && Class)
          bibMode = Class->getBibMode();
        if (bibMode == BibSame)
          Runners[i]->setBib(bib, 0, false, setTmpOnly);
        else if (bibMode == BibAdd) {
          wchar_t pattern[32], bf[32];
          int ibib = oClass::extractBibPattern(bib, pattern) + i;
          swprintf_s(bf, pattern, ibib);
          Runners[i]->setBib(bf, 0, false, setTmpOnly);
        }
        else if (bibMode == BibLeg) {
          wstring rbib = bib + L"-" + Class->getLegNumber(i);
          Runners[i]->setBib(rbib, 0, false, setTmpOnly);
        }
      }
      LegTypes legType = Class ? Class->getLegType(i) : LTIgnore;

      if (Runners[i]->Class!=Class && legType != LTGroup) {
        Runners[i]->Class=Class;
        Runners[i]->updateChanged();
      }

      Runners[i]->tNeedNoCard=false;
      if (Class) {
        pClass pc=Class;

        //Ignored runners need no SI-card (used by SI assign function)
        if (legType == LTIgnore) {
          Runners[i]->tNeedNoCard=true;
          if (lastStatus != StatusUnknown) {
            Runners[i]->setStatus(max(Runners[i]->tStatus, lastStatus), false, setTmpOnly);
          }
        }
        else
          lastStatus = Runners[i]->getStatus();

        StartTypes st = actualClass == pc ? pc->getStartType(i) : actualClass->getStartType(0);
        LegTypes lt = legType;

        if ((lt==LTParallel || lt==LTParallelOptional) && i==0) {
          pc->setLegType(0, LTNormal);
          throw std::exception("F�rsta str�ckan kan inte vara parallell.");
        }
        if (lt==LTIgnore || lt==LTExtra) {
          if (st != STDrawn)
            Runners[i]->setStartTime(lastStartTime, false, setTmpOnly);
          Runners[i]->tUseStartPunch = (st == STDrawn);
        }
        else { //Calculate start time.
          switch (st) {
            case STDrawn: //Do nothing
              if (lt==LTParallel || lt==LTParallelOptional) {
                Runners[i]->setStartTime(lastStartTime, false, setTmpOnly);
                Runners[i]->tUseStartPunch=false;
              }
              else
                lastStartTime = Runners[i]->getStartTime();

              break;

            case STTime: {
              bool prs = false;
              if (Runners[i] && Runners[i]->Card && freeStart) {
                pCourse crs = Runners[i]->getCourse(false);
                int startType = crs ? crs->getStartPunchType() : oPunch::PunchStart;
                oPunch *pnc = Runners[i]->Card->getPunchByType(startType);
                if (pnc && pnc->getAdjustedTime() > 0) {
                  prs = true;
                  lastStartTime = pnc->getAdjustedTime();
                }
              }
              if (!prs) {
                if (lt == LTNormal || lt == LTSum || lt == LTGroup) {
                  if (actualClass == pc)
                    lastStartTime = pc->getStartData(i);
                  else
                    lastStartTime = actualClass->getStartData(0); // Qualification/final classes
                }
                Runners[i]->setStartTime(lastStartTime, false, setTmpOnly);
                Runners[i]->tUseStartPunch=false;
              }
            }
            break;

            case STChange: {
              int probeIndex = 1;
              int startData = pc->getStartData(i);

              if (startData < 0) {
                // A specified leg
                probeIndex = -startData;
              }
              else {
                // Allow for empty slots when ignore/extra
                while ((i-probeIndex)>=0 && !Runners[i-probeIndex]) {
                  LegTypes tlt = pc->getLegType(i-probeIndex);
                  if (tlt == LTIgnore || tlt==LTExtra || tlt == LTGroup)
                    probeIndex++;
                  else
                    break;
                }
              }

              if ( (i-probeIndex)>=0 && Runners[i-probeIndex]) {
                int z = i-probeIndex;
                LegTypes tlt = pc->getLegType(z);
                int ft = 0;
                if (availableStartTimes.empty() || startData < 0) {

                  if (!availableStartTimes.empty()) {
                    // Parallel, but there is a specification. Take one from parallel anyway.
                    ft = getBestStartTime(availableStartTimes);
                  }

                  //We are not involved in parallel legs
                  ft = (tlt != LTIgnore) ? Runners[z]->getFinishTime() : 0;

                  // Take the best time for extra runners
                  while (z>0 && (tlt==LTExtra || tlt==LTIgnore)) {
                    tlt = pc->getLegType(--z);
                    if (Runners[z] && Runners[z]->getStatus() == StatusOK) {
                      int tft = Runners[z]->getFinishTime();
                      if (tft>0 && tlt != LTIgnore)
                        ft = ft>0 ? min(tft, ft) : tft;
                    }
                  }
                }
                else {
                  ft = getBestStartTime(availableStartTimes);
                }

                if (ft<=0)
                  ft=0;

                int restart=pc->getRestartTime(i);
                int rope=pc->getRopeTime(i);

                if ((restart>0 && rope>0 && (ft==0 || ft>rope)) || (ft == 0 && restart>0)) {
                  ft=restart; //Runner in restart
                  tNumRestarts++;
                }

                if (ft > 0)
                  Runners[i]->setStartTime(ft, false, setTmpOnly);
                Runners[i]->tUseStartPunch=false;
                lastStartTime=ft;
              }
              else {//The else below should only be run by mistake (for an incomplete team)
                Runners[i]->setStartTime(Class->getRestartTime(i), false, setTmpOnly);
                Runners[i]->tUseStartPunch=false;
              }
            }
            break;

            case STHunting: {
              bool setStart = false;
              if (i>0 && Runners[i-1]) {
                if (lt == LTNormal || lt == LTSum || availableStartTimes.empty()) {
                  int rt = getLegRunningTimeUnadjusted(i-1, false);
                
                  if (rt>0)
                    setStart = true;
                  int leaderTime = pc->getTotalLegLeaderTime(i-1, false);
                  int timeAfter = leaderTime > 0 ? rt - leaderTime : 0;

                  if (rt>0 && timeAfter>=0)
                    lastStartTime=pc->getStartData(i)+timeAfter;

                  int restart=pc->getRestartTime(i);
                  int rope=pc->getRopeTime(i);

                  RunnerStatus hst = getLegStatus(i-1, false);
                  if (hst != StatusUnknown && hst != StatusOK) {
                    setStart = true;
                    lastStartTime = restart;
                  }

                  if (restart>0 && rope>0 && (lastStartTime>rope)) {
                    lastStartTime=restart; //Runner in restart
                    tNumRestarts++;
                  }
                  if (!availableStartTimes.empty()) {
                    // Single -> to parallel pursuit
                    if (setStart)
                      fill(availableStartTimes.begin(), availableStartTimes.end(), lastStartTime);
                    else
                      fill(availableStartTimes.begin(), availableStartTimes.end(), 0);

                    availableStartTimes.pop_back(); // Used one
                  }
                }
                else if (lt == LTParallel || lt == LTParallelOptional) {
                  lastStartTime = getBestStartTime(availableStartTimes);
                  setStart = true;
                }

                if (Runners[i]->getFinishTime()>0) {
                  setStart = true;
                  if (lastStartTime == 0)
                    lastStartTime = pc->getRestartTime(i);
                }
                if (!setStart)
                  lastStartTime=0;
              }
              else
                lastStartTime=0;

              Runners[i]->tUseStartPunch=false;
              Runners[i]->setStartTime(lastStartTime, false, setTmpOnly);
            }
            break;
          }
        }

        size_t nextNonPar = i+1;
        while (nextNonPar < Runners.size() && pc->isOptional(nextNonPar) && Runners[nextNonPar] == 0)
          nextNonPar++;
        
        int nextBaseLeg = nextNonPar;
        while (nextNonPar < Runners.size() && pc->isParallel(nextNonPar))
          nextNonPar++;

        // Extra finish time is used to split extra legs to parallel legs
        if (lt == LTExtra || pc->getLegType(i+1) == LTExtra) {
          if (Runners[i]->getFinishTime()>0) {
            if (extraFinishTime <= 0)
              extraFinishTime =  Runners[i]->getFinishTime();
            else
              extraFinishTime = min(extraFinishTime, Runners[i]->getFinishTime());
          }
        }
        else
          extraFinishTime = -1;

        //Add available start times for parallel
        if (nextNonPar < Runners.size()) {
          st=pc->getStartType(nextNonPar);
          int finishTime = Runners[i]->getFinishTime();
          if (lt == LTExtra)
            finishTime = extraFinishTime;

          if (st==STDrawn || st==STTime)
            availableStartTimes.clear();
          else if (finishTime>0) {
            int nRCurrent = pc->getNumParallel(i);
            int nRNext = pc->getNumParallel(nextBaseLeg);
            if (nRCurrent>1 || nRNext>1) {
              if (nRCurrent<nRNext) {
                // Going from single leg to parallel legs
                for (int j=0; j<nRNext/nRCurrent; j++)
                  availableStartTimes.push_back(finishTime);
              }
              else if (nRNext==1)
                compressStartTimes(availableStartTimes, finishTime);
              else
                addStartTime(availableStartTimes, finishTime);
            }
            else
              availableStartTimes.clear();
          }
        }
      }
      if (sync)
        Runners[i]->synchronize(true);
    }
  }

  if (!Runners.empty() && Runners[0]) {
    if (setTmpOnly)
      setStartTime(Runners[0]->tmpStore.startTime, false, setTmpOnly);
    else
      setStartTime(Runners[0]->getStartTime(), false, setTmpOnly);
  }
  else if (Class && Class->getStartType(0) != STDrawn)
    setStartTime(Class->getStartData(0), false, setTmpOnly);

  setFinishTime(getLegFinishTime(-1));
  setStatus(getLegStatus(-1, false), false, setTmpOnly); //XXX Maybe getLegStatus needs to work agains tmp store?

  if (sync)
    synchronize(true);
  return true;
}

void oTeam::evaluate(bool sync)
{
  for(unsigned i=0;i<Runners.size(); i++) {
    if (Runners[i])
      Runners[i]->resetTmpStore();
  }
  resetTmpStore();

  apply(false, 0, true);
  vector<int> mp;
  for(unsigned i=0;i<Runners.size(); i++) {
    if (Runners[i])
      Runners[i]->evaluateCard(false, mp);
  }
  apply(false, 0, true);

  for(unsigned i=0;i<Runners.size(); i++) {
    if (Runners[i]) {
      Runners[i]->setTmpStore();
      if (sync)
        Runners[i]->synchronize(true);
    }
  }
  setTmpStore();
  if (sync)
    synchronize(true);
}

void oTeam::correctRemove(pRunner r) {
  for(unsigned i=0;i<Runners.size(); i++)
    if (r!=0 && Runners[i]==r) {
      Runners[i] = 0;
      r->tInTeam = 0;
      r->tLeg = 0;
      r->tLegEquClass = 0;
      correctionNeeded = true;
      r->correctionNeeded = true;
    }
}

void oTeam::speakerLegInfo(int leg, int specifiedLeg, int courseControlId,
                           int &missingLeg, int &totalLeg, 
                           RunnerStatus &status, int &runningTime) const {
  missingLeg = 0;
  totalLeg = 0;
  bool extra = false, firstExtra = true;
  for (int i = leg; i <= specifiedLeg; i++) {
    LegTypes lt=Class->getLegType(i);
    if (lt == LTExtra)
      extra = true;

    if (lt == LTSum || lt == LTNormal || lt == LTParallel || lt == LTParallelOptional || lt == LTExtra) {
      int lrt = 0;
      RunnerStatus lst = StatusUnknown;
      if (Runners[i]) {
        Runners[i]->getSplitTime(courseControlId, lst, lrt);
        totalLeg++;
      }
      else if (lt == LTParallelOptional) {
        lst = StatusOK; // No requrement
      }
      else if (!extra) {
        totalLeg++; // will never reach...
      }

      if (extra) {
        // Extra legs, take best result
        if (firstExtra && i > 0 && !Runners[i-1]) {
          missingLeg = 0;
          totalLeg = 0;
        }
        firstExtra = false;
        if (lrt>0 && (lrt < runningTime || runningTime == 0)) {
          runningTime = lrt;
          status = lst; // Take status/time from best runner
        }
        if (Runners[i] && lst == StatusUnknown)
          missingLeg++;
      }
      else {
        if (lst > StatusOK) {
          status = lst;
          break;
        }
        else if (lst == StatusUnknown) {
          missingLeg++;
        }
        else {
          runningTime = max(lrt, runningTime);
        }
      }
    }
  }

  if (missingLeg == 0 && status == StatusUnknown)
    status = StatusOK;
}

void oTeam::fillSpeakerObject(int leg, int courseControlId, int previousControlCourseId,
                              bool totalResult, oSpeakerObject &spk) const {
  if (leg==-1)
    leg = Runners.size()-1;

  spk.club = getName();
  spk.missingStartTime = true;
  //Defaults (if early return)

  if (totalResult && inputStatus > StatusOK)
    spk.status = spk.finishStatus = inputStatus;
  else
    spk.status = spk.finishStatus = StatusUnknown;

  if (!Class || unsigned(leg) >= Runners.size())
    return;

  // Ignore initial optional and not used legs
  while (leg > 0 && !Runners[leg]) {
    Class->isOptional(leg);
    leg--;
  }

  if (!Runners[leg])
    return;

  // Get many names for paralell legs
  int firstLeg = leg;
  int requestedLeg = leg;
  LegTypes lt=Class->getLegType(firstLeg--);
  while(firstLeg>=0 && (lt==LTIgnore || lt==LTParallel || lt==LTParallelOptional || lt==LTExtra) )
    lt=Class->getLegType(firstLeg--);

  spk.names.clear();
  for(int k=firstLeg+1;k<=leg;k++) {
    if (Runners[k]) {
      const wstring &n = Runners[k]->getName();
      spk.names.push_back(n);
    }
  }
  // Add start number
  spk.bib = getBib();

  if (courseControlId == 2) {
    unsigned nextLeg = leg + 1;
    while (nextLeg < Runners.size()) {
      if (Runners[nextLeg])
        spk.outgoingnames.push_back(Runners[nextLeg]->getName());

      nextLeg++;
      if (nextLeg < Runners.size()) {
        LegTypes lt=Class->getLegType(nextLeg);
        if (!(lt==LTIgnore || lt==LTParallel || lt==LTParallelOptional || lt==LTExtra))
          break;
      }
    }
  }

  int specifiedLeg = leg;
  leg = firstLeg+1; //This does not work well with parallel or extra...
  while (!Runners[leg]) // Ensure the leg is set
    leg++;
  
  int missingLeg = 0;

  int timeOffset=0;
  RunnerStatus inheritStatus = StatusUnknown;
  if (firstLeg>=0) {
    timeOffset = getLegRunningTime(firstLeg, totalResult);
    inheritStatus = getLegStatus(firstLeg, totalResult);
  }
  else if (totalResult) {
    timeOffset = getInputTime();
    inheritStatus = getInputStatus();
  }

  speakerLegInfo(leg, specifiedLeg, courseControlId,
                  missingLeg, spk.runnersTotalLeg, spk.status, spk.runningTimeLeg.time);

  spk.runnersFinishedLeg = spk.runnersTotalLeg - missingLeg;
  if (spk.runnersTotalLeg > 1) {
    spk.parallelScore = (spk.runnersFinishedLeg * 100) / spk.runnersTotalLeg;
  }
  if (previousControlCourseId > 0 && spk.status <= 1 && Class->getStartType(0) == STTime) {
    spk.useSinceLast = true;
    RunnerStatus pStat = StatusUnknown;
    int lastTime = 0;
    int dummy;
    speakerLegInfo(leg, specifiedLeg, previousControlCourseId,
                   missingLeg, dummy, pStat, lastTime);

    if (pStat == StatusOK) {
      if (spk.runningTimeLeg.time > 0) {
        spk.runningTimeSinceLast.time = spk.runningTimeLeg.time - lastTime;
        spk.runningTimeSinceLast.preliminary = spk.runningTimeSinceLast.time;
      }
      else if (spk.runningTimeLeg.time == 0) {
        spk.runningTimeSinceLast.preliminary = oe->getComputerTime() - lastTime;
        //string db = Name + " " + itos(lastTime) + " " + itos(spk.runningTimeSinceLast.preliminary) +"\n";
        //OutputDebugString(db.c_str());
      }
    }
  }

  spk.timeSinceChange = oe->getComputerTime() - (spk.runningTimeLeg.time + Runners[leg]->tStartTime);

  spk.owner=Runners[leg];
  spk.finishStatus=getLegStatus(specifiedLeg, totalResult);

  spk.missingStartTime = false;
  int stMax = 0;
  for (int i = leg; i <= requestedLeg; i++) {
    if (!Runners[i])
      continue;
    if (Class->getLegType(i) == LTIgnore || Class->getLegType(i) == LTGroup)
      continue;
    
    int st=Runners[i]->getStartTime();
    if (st <= 0)
      spk.missingStartTime = true;
    else {
      if (st > stMax) {
        spk.startTimeS = Runners[i]->getStartTimeCompact();
        stMax = st;
      }
    }
  }
  
  map<int, int>::iterator mapit=Runners[leg]->priority.find(courseControlId);
  if (mapit!=Runners[leg]->priority.end())
    spk.priority=mapit->second;
  else
    spk.priority=0;

  spk.runningTimeLeg.preliminary = 0;
  for (int i = leg; i <= requestedLeg; i++) {
    if (!Runners[i])
      continue;
    int pt = Runners[i]->getPrelRunningTime();
    if (Class->getLegType(i) == LTParallel)
      spk.runningTimeLeg.preliminary = max(spk.runningTimeLeg.preliminary, pt);
    else if (Class->getLegType(i) == LTExtra) {
      if (spk.runningTimeLeg.preliminary == 0)
        spk.runningTimeLeg.preliminary = pt;
      else if (pt > 0)
        spk.runningTimeLeg.preliminary = min(spk.runningTimeLeg.preliminary, pt);
    }
    else
      spk.runningTimeLeg.preliminary = pt;
  }

  if (inheritStatus>StatusOK)
    spk.status=inheritStatus;

  if (spk.status==StatusOK) {
    if (courseControlId == 2)
      spk.runningTime.time = getLegRunningTime(requestedLeg, totalResult); // Get official time

    if (spk.runningTime.time == 0)
      spk.runningTime.time = spk.runningTimeLeg.time + timeOffset; //Preliminary time

    spk.runningTime.preliminary = spk.runningTime.time;
    spk.runningTimeLeg.preliminary = spk.runningTimeLeg.time;
  }
  else if (spk.status==StatusUnknown && spk.finishStatus==StatusUnknown) {
    spk.runningTime.time = spk.runningTimeLeg.preliminary + timeOffset;
    spk.runningTime.preliminary = spk.runningTimeLeg.preliminary + timeOffset;
    spk.runningTimeLeg.time = spk.runningTimeLeg.preliminary;
  }
  else if (spk.status==StatusUnknown)
    spk.status=StatusMP;

  if (totalResult && inputStatus != StatusOK)
    spk.status = spk.finishStatus;
}

int oTeam::getTimeAfter(int leg) const
{
  if (leg==-1)
    leg=Runners.size()-1;

  if (!Class || Class->tLeaderTime.size()<=unsigned(leg))
    return -1;

  int t=getLegRunningTime(leg, false);

  if (t<=0)
    return -1;

  return t-Class->getTotalLegLeaderTime(leg, false);
}

int oTeam::getLegStartTime(int leg) const
{
  if (leg==0)
    return tStartTime;
  else if (unsigned(leg)<Runners.size() && Runners[leg])
    return Runners[leg]->getStartTime();
  else return 0;
}

wstring oTeam::getLegStartTimeS(int leg) const
{
  int s=getLegStartTime(leg);
  if (s>0)
    return oe->getAbsTime(s);
  else 
    return makeDash(L"-");
}

wstring oTeam::getLegStartTimeCompact(int leg) const
{
  int s=getLegStartTime(leg);
  if (s>0)
    if (oe->useStartSeconds())
      return oe->getAbsTime(s);
    else
      return oe->getAbsTimeHM(s);

  else return makeDash(L"-");
}

void oTeam::setBib(const wstring &bib, int bibnumerical, bool updateStartNo, bool setTmpOnly) {
  if (updateStartNo)
    updateStartNo = !Class || !Class->lockedForking();

  if (setTmpOnly) {
    tmpStore.bib = bib;
    if (updateStartNo)
      setStartNo(bibnumerical, true);
    return;
  }

  if (getDI().setString("Bib", bib)) {
    if (oe)
      oe->bibStartNoToRunnerTeam.clear();
  }

  if (updateStartNo) {
    setStartNo(bibnumerical, false);
  }
}

oDataContainer &oTeam::getDataBuffers(pvoid &data, pvoid &olddata, pvectorstr &strData) const {
  data = (pvoid)oData;
  olddata = (pvoid)oDataOld;
  strData = 0;
  return *oe->oTeamData;
}

pRunner oTeam::getRunner(unsigned leg) const
{
  if (leg==-1)
    leg=Runners.size()-1;

  return leg<Runners.size() ? Runners[leg]: 0;
}

int oTeam::getRogainingPoints(bool multidayTotal) const {
  int pt = 0;
  bool simpleSum = false;
  if (simpleSum) {
    for (size_t k = 0; k < Runners.size(); k++) {
      if (Runners[k])
        pt += Runners[k]->tRogainingPoints;
    }
  }
  else {
    std::set<int> rogainingControls;
    for (size_t k = 0; k < Runners.size(); k++) {
      if (Runners[k]) {
        pCard c = Runners[k]->getCard();
        pCourse crs = Runners[k]->getCourse(true);
        if (c && crs) {
          for (oPunchList::const_iterator it = c->punches.begin(); it != c->punches.end();++it) {
            if (rogainingControls.count(it->tMatchControlId) == 0) {
              rogainingControls.insert(it->tMatchControlId);
              pt += it->tRogainingPoints;
            }
          }
        }
      }
    }
  }
  pt = max(pt - getRogainingReduction(), 0);
  // Manual point adjustment
  pt = max(0, pt + getPointAdjustment());

  if (multidayTotal)
    return pt + inputPoints;
  return pt;
}

int oTeam::getRogainingOvertime() const {
  int overTime = 0;
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k]) {
      overTime += Runners[k]->tRogainingOvertime;
    }
  }
  return overTime;
}

int oTeam::getRogainingPointsGross() const {
  int gross = 0;
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k]) {
      gross += Runners[k]->tRogainingPointsGross;
    }
  }
  return gross;
}
 

int oTeam::getRogainingReduction() const {
  pCourse sampleCourse = 0;
  int overTime = 0;
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k]) {
      if (sampleCourse == 0 &&(Runners[k]->tRogainingPoints > 0 || Runners[k]->tReduction > 0)) 
        sampleCourse = Runners[k]->getCourse(false);
      overTime += Runners[k]->tRogainingOvertime; 
    }
  }
  return sampleCourse->calculateReduction(overTime);
}
 
void oTeam::remove()
{
  if (oe)
    oe->removeTeam(Id);
}

bool oTeam::canRemove() const
{
  return true;
}

wstring oTeam::getDisplayName() const {
  if (!Class)
    return sName;

  ClassType ct = Class->getClassType();
  if (ct == oClassIndividRelay || ct == oClassPatrol) {
    if (Club) {
      wstring cname = getDisplayClub();
      if (!cname.empty())
        return cname;
    }
  }
  return sName;
}

wstring oTeam::getDisplayClub() const {
  vector<pClub> clubs;
  if (Club)
    clubs.push_back(Club);
  for (size_t k = 0; k<Runners.size(); k++) {
    if (Runners[k] && Runners[k]->Club) {
      if (count(clubs.begin(), clubs.end(), Runners[k]->Club) == 0)
        clubs.push_back(Runners[k]->Club);
    }
  }
  if (clubs.size() == 1)
    return clubs[0]->getDisplayName();

  wstring res;
  for (size_t k = 0; k<clubs.size(); k++) {
    if (k == 0)
      res = clubs[k]->getDisplayName();
    else
      res += L" / " + clubs[k]->getDisplayName();
  }
  return res;
}

void oTeam::setInputData(const oTeam &t) {
  inputTime = t.getTotalRunningTime();
  inputStatus = t.getTotalStatus();
  inputPoints = t.getRogainingPoints(true);
  int tp = t.getTotalPlace();
  inputPlace = tp < 99000 ? tp : 0;

  oDataInterface dest = getDI();
  oDataConstInterface src = t.getDCI();

  dest.setInt("TransferFlags", src.getInt("TransferFlags"));
  
  dest.setString("Nationality", src.getString("Nationality"));
  dest.setString("Country", src.getString("Country"));
  dest.setInt("Fee", src.getInt("Fee"));
  dest.setInt("Paid", src.getInt("Paid"));
  dest.setInt("Taxable", src.getInt("Taxable"));
}

RunnerStatus oTeam::getTotalStatus() const {
  if (tStatus == StatusUnknown)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  return max(tStatus, inputStatus);
}

void oEvent::getTeams(int classId, vector<pTeam> &t, bool sort) {
  if (sort) {
    synchronizeList(oLTeamId);
    //sortTeams(SortByName);
  }
  t.clear();
  if (Classes.size() > 0)
    t.reserve((Teams.size()*min<size_t>(Classes.size(), 4)) / Classes.size());

  for (oTeamList::iterator it = Teams.begin(); it != Teams.end(); ++it) {
    if (it->isRemoved())
      continue;

    if (classId == 0 || it->getClassId(false) == classId)
      t.push_back(&*it);
  }
}

wstring oTeam::getEntryDate(bool dummy) const {
  oDataConstInterface dci = getDCI();
  int date = dci.getInt("EntryDate");
  if (date == 0 && !isVacant()) {
    auto di = (const_cast<oTeam *>(this)->getDI());
    di.setDate("EntryDate", getLocalDate());
    di.setInt("EntryTime", getLocalAbsTime());
  }
  return dci.getDate("EntryDate");
}

unsigned static nRunnerMaxStored = -1;

Table *oEvent::getTeamsTB() //Table mode
{

  vector<pClass> cls;
  oe->getClasses(cls, true);
  unsigned nRunnerMax = 0;
  for (size_t k = 0; k < cls.size(); k++) {
    nRunnerMax = max(nRunnerMax, cls[k]->getNumStages());
  }

  bool forceUpdate = nRunnerMax != nRunnerMaxStored;

  if (forceUpdate && tables.count("team")) {
    tables["team"]->releaseOwnership();
    tables.erase("team");
  }

  if (tables.count("team") == 0) {

    oCardList::iterator it;

    Table *table=new Table(this, 20, L"Lag(flera)", "teams");

    table->addColumn("Id", 70, true, true);
    table->addColumn("�ndrad", 70, false);

    table->addColumn("Namn", 200, false);
    table->addColumn("Klass", 120, false);
    table->addColumn("Klubb", 120, false);

    table->addColumn("Start", 70, false, true);
    table->addColumn("M�l", 70, false, true);
    table->addColumn("Status", 70, false);
    table->addColumn("Tid", 70, false, true);

    table->addColumn("Plac.", 70, true, true);
    table->addColumn("Start nr.", 70, true, false);

    for (unsigned k = 0; k < nRunnerMax; k++) {
      table->addColumn("Str�cka X#" + itos(k+1), 200, false, false);
      table->addColumn("Bricka X#" + itos(k+1), 70, true, true);
    }
    nRunnerMaxStored = nRunnerMax;

    oe->oTeamData->buildTableCol(table);

    table->addColumn("Tid in", 70, false, true);
    table->addColumn("Status in", 70, false, true);
    table->addColumn("Po�ng in", 70, true);
    table->addColumn("Placering in", 70, true);

    tables["team"] = table;
    table->addOwnership();
  }

  tables["team"]->update();
  return tables["team"];
}


void oEvent::generateTeamTableData(Table &table, oTeam *addTeam)
{
  vector<pClass> cls;
  oe->getClasses(cls, true);
  unsigned nRunnerMax = 0;

  for (size_t k = 0; k < cls.size(); k++) {
    nRunnerMax = max(nRunnerMax, cls[k]->getNumStages());
  }

  if (nRunnerMax != nRunnerMaxStored)
    throw meosException("Internal table error: Restart MeOS");

  if (addTeam) {
    addTeam->addTableRow(table);
    return;
  }

  synchronizeList(oLTeamId);
  oTeamList::iterator it;
  table.reserve(Teams.size());
  for (it=Teams.begin(); it != Teams.end(); ++it){
    if (!it->skip()){
      it->addTableRow(table);
    }
  }
}

void oTeam::addTableRow(Table &table) const {
  oRunner &it = *pRunner(this);
  table.addRow(getId(), &it);

  int row = 0;
  table.set(row++, it, TID_ID, itow(getId()), false);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false);

  table.set(row++, it, TID_NAME, getName(), true);
  table.set(row++, it, TID_CLASSNAME, getClass(true), true, cellSelection);
  table.set(row++, it, TID_CLUB, getClub(), true, cellCombo);

  table.set(row++, it, TID_START, getStartTimeS(), true);
  table.set(row++, it, TID_FINISH, getFinishTimeS(), true);
  table.set(row++, it, TID_STATUS, getStatusS(), true, cellSelection);
  table.set(row++, it, TID_RUNNINGTIME, getRunningTimeS(), false);

  table.set(row++, it, TID_PLACE, getPlaceS(), false);
  table.set(row++, it, TID_STARTNO, itow(getStartNo()), true);

  for (unsigned k = 0; k < nRunnerMaxStored; k++) {
    pRunner r = getRunner(k);
    if (r) {
      table.set(row++, it, 100+2*k, r->getUIName(), r->getRaceNo() == 0);
      table.set(row++, it, 101+2*k, itow(r->getCardNo()), true);
    }
    else {
      table.set(row++, it, 100+2*k, L"", Class && Class->getLegRunner(k) == k);
      table.set(row++, it, 101+2*k, L"", false);
    }
  }

  row = oe->oTeamData->fillTableCol(it, table, true);

  table.set(row++, it, TID_INPUTTIME, getInputTimeS(), true);
  table.set(row++, it, TID_INPUTSTATUS, getInputStatusS(), true, cellSelection);
  table.set(row++, it, TID_INPUTPOINTS, itow(inputPoints), true);
  table.set(row++, it, TID_INPUTPLACE, itow(inputPlace), true);
}

bool oTeam::inputData(int id, const wstring &input,
                        int inputId, wstring &output, bool noUpdate)
{
  int s,t;
  synchronize(false);

  if (id>1000) {
    return oe->oTeamData->inputData(this, id, input,
                                    inputId, output, noUpdate);
  }
  else if (id >= 100) {

    bool isName = (id&1) == 0;
    size_t ix = (id-100)/2;
    if (ix < Runners.size()) {
      if (Runners[ix]) {
        if (isName) {
          if (input.empty()) {
            removeRunner(oe->gdibase, false, ix);
          }
          else {
            Runners[ix]->setName(input, true);
            Runners[ix]->synchronize(true);
            output = Runners[ix]->getName();
          }
        }
        else {
          Runners[ix]->setCardNo(_wtoi(input.c_str()), true);
          Runners[ix]->synchronize(true);
          output = itow(Runners[ix]->getCardNo());
        }
      }
      else {
        if (isName && !input.empty() && Class) {
          pRunner r = oe->addRunner(input, getClubId(), getClassId(false), 0, 0, false);
          setRunner(ix, r, true);
          output = r->getName();
        }

      }
    }
  }

  switch(id) {

    case TID_NAME:
      setName(input, true);
      synchronize(true);
      output = getName();
      break;

    case TID_START:
      setStartTimeS(input);
      if (getRunner(0))
        getRunner(0)->setStartTimeS(input);
      t=getStartTime();
      apply(false, 0, false);
      s=getStartTime();
      if (s!=t)
        throw std::exception("Starttiden �r definerad genom klassen eller l�parens startst�mpling.");
      synchronize(true);
      output = getStartTimeS();
      return true;
    break;

    case TID_CLUB:
      {
        pClub pc = 0;
        if (inputId > 0)
          pc = oe->getClub(inputId);
        else
          pc = oe->getClubCreate(0, input);

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
      RunnerStatus sIn = RunnerStatus(inputId);
      if (sIn != StatusDNS && sIn != StatusCANCEL) {
        if ((getStatus() == StatusDNS || getStatus() == StatusCANCEL) && sIn == StatusUnknown)
          setTeamNoStart(false, StatusUnknown);
        else
          setStatus(sIn, true, false);
      }
      else if (getStatus() != sIn)
        setTeamNoStart(true, sIn);

      apply(true, 0, false);
      RunnerStatus sOut = getStatus();
      if (sOut != sIn)
        throw meosException("Status matchar inte deltagarnas status.");
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

void oTeam::fillInput(int id, vector< pair<wstring, size_t> > &out, size_t &selected)
{
  if (id>1000) {
    oe->oRunnerData->fillInput(oData, id, 0, out, selected);
    return;
  }
  else if (id==TID_CLASSNAME) {
    oe->fillClasses(out, oEvent::extraNone, oEvent::filterOnlyMulti);
    out.push_back(make_pair(lang.tl(L"Ingen klass"), 0));
    selected = getClassId(true);
  }
  else if (id==TID_CLUB) {
    oe->fillClubs(out);
    out.push_back(make_pair(lang.tl(L"Klubbl�s"), 0));
    selected = getClubId();
  }
  else if (id==TID_STATUS || id==TID_INPUTSTATUS) {
    oe->fillStatus(out);
    selected = getStatus();
  }
}

void oTeam::removeRunner(gdioutput &gdi, bool askRemoveRunner, int i) {
  pRunner p_old = getRunner(i);
  setRunner(i, 0, true);

  //Remove multi runners
  if (Class) {
    for (unsigned k = i+1;k < Class->getNumStages(); k++) {
      if (Class->getLegRunner(k)==i)
        setRunner(k, 0, true);
    }
  }

  //No need to delete multi runners. Disappears when parent is gone.
  if (p_old && !oe->isRunnerUsed(p_old->getId())){
    if (!askRemoveRunner || gdi.ask(L"Ska X raderas fr�n t�vlingen?#" + p_old->getName())){
      vector<int> oldR;
      oldR.push_back(p_old->getId());
      oe->removeRunner(oldR);
    }
    else {
      p_old->getDI().setInt("RaceId", 0); // Clear race id.
      p_old->setClassId(0, false); // Clear class
      p_old->synchronize(true);
    }
  }
}

int oTeam::getTeamFee() const {
  int f = getDCI().getInt("Fee");
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k])
      f += Runners[k]->getDCI().getInt("Fee");
  }
  return f;
}

void oTeam::markClassChanged(int controlId) {
  if (Class)
    Class->markSQLChanged(-1, controlId);
}

void oTeam::resetResultCalcCache() const {
  resultCalculationCache.resize(RCCLast);
  for (int k = 0; k < RCCLast; k++)
    resultCalculationCache[k].resize(Runners.size());
}

vector< vector<int> > &oTeam::getResultCache(ResultCalcCacheSymbol symb) const {
  return resultCalculationCache[symb];
}

void oTeam::setResultCache(ResultCalcCacheSymbol symb, int leg, vector<int> &data) const {
  if (!resultCalculationCache.empty() && size_t(leg) < resultCalculationCache[symb].size())
    resultCalculationCache[symb][leg].swap(data);
}

int oTeam::getNumShortening() const {
  return getNumShortening(-1);
}

int oTeam::getNumShortening(int leg) const {
  int ns = 0;
  if (Class) {
    for (size_t k = 0; k < Runners.size() && k <= size_t(leg); k++) {
      if (Runners[k] && !Class->isOptional(k))
        ns += Runners[k]->getNumShortening();
    }
  }
  else {
   for (size_t k = 0; k < Runners.size() && k <= size_t(leg); k++) {
      if (Runners[k])
        ns += Runners[k]->getNumShortening();
    }
  }
  return ns;
}

bool oTeam::checkValdParSetup() {
  if (!Class)
    return false;
  bool cor = false;
  for (size_t k = 0; k < Runners.size(); k++) {
    if (!Class->isOptional(k) && !Class->isParallel(k) && !Runners[k]) {
      int m = 1;
      while((m+k) < Runners.size() && (Class->isOptional(k+m) || Class->isParallel(k+m))) {
        if (Runners[m+k]) {
          // Move to where a runner is needed
          Runners[k] = Runners[k+m];
          Runners[k]->tLeg = k;          
          Runners[k+m] = 0;
          updateChanged();
          cor = true;
          k+=m;
          break;
        }
        else 
          m++;
      }
    }
  }
  return cor;
}


int oTeam::getRanking() const {
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k]) {
      return Runners[k]->getRanking();
    }
  }
  return MaxRankingConstant;
}

int oTeam::getRogainingPatrolPoints(bool multidayTotal) const {
  int madj = multidayTotal ? getInputPoints() : 0;

  if (tTeamPatrolRogainingAndVersion.first == oe->dataRevision)
    return tTeamPatrolRogainingAndVersion.second.points + madj;

  tTeamPatrolRogainingAndVersion.first = oe->dataRevision;
  tTeamPatrolRogainingAndVersion.second.reset();

  int reduction = 0;
  int overtime = 0;
  map<int, vector<pair<int, int>>> control2PunchTimeRunner;
  std::set<int> runnerToCheck;
  vector<pPunch> punches;
  for (pRunner r : Runners) {
    if (r) {
      pCourse pc = r->getCourse(false);
      if (r->getCard() && pc) {
        reduction = max(reduction, r->getRogainingReduction());
        overtime = max(overtime, r->getRogainingOvertime());
        int rid = r->getId();
        r->getCard()->getPunches(punches);
        for (auto p : punches) {
          if (p->anyRogainingMatchControlId > 0) {
            pControl ctrl = oe->getControl(p->anyRogainingMatchControlId);
            if (ctrl) {
              auto &cl = control2PunchTimeRunner[ctrl->getId()];
              cl.push_back(make_pair(p->getTimeInt(), rid));
            }
          }
        }
      }
      else if (r->getStatus() == StatusDNS || r->getStatus() == StatusCANCEL)
        continue; // Accept missing punches

      runnerToCheck.insert(r->getId());
    }
  }
  int timeLimit = oe->getDCI().getInt("DiffTime");
  if (timeLimit == 0)
    timeLimit = 10000000;

  vector<pControl> acceptedControls;
  for (auto &ctrl : control2PunchTimeRunner) {
    int ctrlId = ctrl.first;
    auto &punchList = ctrl.second;
    sort(punchList.begin(), punchList.end()); // Sort times in order. Zero time means unknown time
    bool ok = false;
    for (size_t k = 0; !ok && k < punchList.size(); k++) {
      std::set<int> checked;
      for (size_t z = 0; z < punchList.size() && punchList[z].first <= 0; z++) {
        checked.insert(punchList[z].second); // Missing time. Accept any
        k = max(k, z);
      }

      if (k < punchList.size()) {
        int startTime = punchList[k].first;
        for (size_t j = k; j < punchList.size() && (punchList[j].first - startTime) < timeLimit; j++) {
          checked.insert(punchList[j].second); // Accept competitor if in time interval
        }
      }

      ok = checked.size() >= runnerToCheck.size();
    }

    if (ok) {
      acceptedControls.push_back(oe->getControl(ctrlId));
    }
  }
  int points = 0;
  for (pControl ctrl : acceptedControls) {
    points += ctrl->getRogainingPoints();
  }
  points = max(0, points + getPointAdjustment() - reduction);
  tTeamPatrolRogainingAndVersion.second.points = points;
  tTeamPatrolRogainingAndVersion.second.reduction = reduction;
  tTeamPatrolRogainingAndVersion.second.overtime = overtime;

  return tTeamPatrolRogainingAndVersion.second.points + madj;
}

int oTeam::getRogainingPatrolReduction() const {
  getRogainingPatrolPoints(false);
  return tTeamPatrolRogainingAndVersion.second.reduction;
}

int oTeam::getRogainingPatrolOvertime() const {
  getRogainingPatrolPoints(false);
  return tTeamPatrolRogainingAndVersion.second.overtime;
}
