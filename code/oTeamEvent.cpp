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

// oEvent.cpp: implementation of the oEvent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <vector>
#include "oEvent.h"
#include "gdioutput.h"
#include "oDataContainer.h"

#include "random.h"
#include "SportIdent.h"
#include "Localizer.h"
#include "intkeymapimpl.hpp"
#include "MeOSFeatures.h"

#include "meos_util.h"
#include "meos.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#include <io.h>
#include <fcntl.h>
#include <cassert>


void oEvent::fillTeams(gdioutput &gdi, const string &id, int classId)
{
  vector< pair<wstring, size_t> > d;
  oe->fillTeams(d, classId);
  gdi.addItem(id, d);
}

const vector< pair<wstring, size_t> > &oEvent::fillTeams(vector< pair<wstring, size_t> > &out, int ClassId)
{
  synchronizeList(oLTeamId);
  oTeamList::iterator it;
  Teams.sort(oTeam::compareSNO);

  out.clear();

  wstring tn;
  int maxBib = 0;
  for (it=Teams.begin(); it != Teams.end(); ++it) {
    if (!it->Removed) {
      maxBib = max<int>(it->getBib().length(), maxBib);
    }
  }
  
  wstring rawDash = L"";
  for (int i = 0; i < min(maxBib, 2); i++)
    rawDash  += L"-";
  rawDash += L" ";
  wstring dashes = getMeOSFeatures().hasFeature(MeOSFeatures::Bib) ?  makeDash(rawDash) : _EmptyWString;
  
  for (it=Teams.begin(); it != Teams.end(); ++it) {
    if (!it->Removed) {
      const wstring &bib = it->getBib();
      if (!bib.empty()) {
        /*int nb = atoi(bib.c_str());
        if (nb > 0 && nb == it->getStartNo()) {
          char bf[24];
          if (maxSno>999)
            sprintf_s(bf, "%04d ", nb);
          else
            sprintf_s(bf, "%03d ", nb);

          tn = bf + it->Name;
        }
        else*/
        wstring paddedBib;
        for (int i = 0; i < int(maxBib - bib.length()); i++)
          paddedBib += L"0";
        
        tn = paddedBib + bib + L" " + it->getName();
      }
      else {
        tn = dashes + it->getName();
      }
      if (it->Class)
        out.push_back(make_pair(tn + L" (" + it->getClass(true) + L")", it->Id));
      else
        out.push_back(make_pair(tn, it->Id));
    }
  }

  return out;
}


pTeam oEvent::getTeam(int Id) const {
  pTeam value;
  if (teamById.lookup(Id, value) && value) {
    if (value->isRemoved())
      return 0;
    assert(value->Id == Id);
    return value;
  }
  return 0;
}


int oEvent::getFreeStartNo() const {
  oTeamList::const_iterator it;
  int sno=0;

  for (it=Teams.begin(); it != Teams.end(); ++it) {
    if (it->isRemoved())
      continue;
    sno=max(it->getStartNo(), sno);
  }
  return sno+1;
}


pTeam oEvent::getTeamByName(const wstring &pName) const {
  oTeamList::const_iterator it;

  for (it=Teams.begin(); it != Teams.end(); ++it) {
    if (!it->isRemoved() && it->sName==pName)
      return pTeam(&*it);
  }
  return 0;
}

pTeam oEvent::addTeam(const wstring &pname, int ClubId, int ClassId)
{
  oTeam t(this);
  t.sName=pname;

  if (ClubId>0)
    t.Club=getClub(ClubId);

  if (ClassId>0)
    t.Class=getClass(ClassId);

  bibStartNoToRunnerTeam.clear();
  Teams.push_back(t);
  teamById[t.Id] = &Teams.back();

  oe->updateTabs();

  Teams.back().StartNo = ++nextFreeStartNo; // Need not be unique
  Teams.back().getEntryDate(false);// Store entry time
  Teams.back().apply(false, 0, false);
  Teams.back().updateChanged();
  return &Teams.back();
}

pTeam oEvent::addTeam(const oTeam &t, bool autoAssignStartNo) {
  if (t.Id==0)
    return 0;
  if (getTeam(t.Id))
    return 0;

  bibStartNoToRunnerTeam.clear();
  Teams.push_back(t);

  pTeam pt = &Teams.back();
  teamById[pt->Id] = pt;

  if (pt->StartNo == 0 && autoAssignStartNo) {
    pt->StartNo = ++nextFreeStartNo; // Need not be unique
  }
  else {
    nextFreeStartNo = max(nextFreeStartNo, pt->StartNo);
  }
  //Important: Must not auto sync!

  return pt;
}

int oEvent::getFreeTeamId()
{
  if (qFreeTeamId > int(Teams.size() + 1000)) {
    for (int j = qFreeTeamId - Teams.size(); j > 0; j -= Teams.size()) {
      if (getTeam(j) == 0)
        return j;
    }
  }

  qFreeTeamId++;
  return qFreeTeamId;
}


bool oEvent::writeTeams(xmlparser &xml)
{
  oTeamList::iterator it;

  xml.startTag("TeamList");

  for (it=Teams.begin(); it != Teams.end(); ++it)
    it->write(xml);

  xml.endTag();

  return true;
}

pTeam oEvent::findTeam(const wstring &s, int lastId, unordered_set<int> &filter) const
{
  wstring trm = trim(s);
  int len = trm.length();
  wchar_t s_lc[1024];
  wcscpy_s(s_lc, trm.c_str());
  CharLowerBuff(s_lc, len);

  int sn = _wtoi(s.c_str());
  oTeamList::const_iterator it;
/*
  if (sn>0) {
    for (it=Teams.begin(); it != Teams.end(); ++it) {
      if (it->skip())
        continue;

      if (it->StartNo==sn)
        return pTeam(&*it);

      for(size_t k=0;k<it->Runners.size();k++)
        if (it->Runners[k] && it->Runners[k]->CardNo==sn)
          return pTeam(&*it);
    }
  }
*/
  oTeamList::const_iterator itstart=Teams.begin();


  if (lastId) {
    for (; itstart != Teams.end(); ++itstart)
      if (itstart->Id==lastId) {
        ++itstart;
        break;
      }
  }

  pTeam ret = 0;
  for (it=itstart; it != Teams.end(); ++it) {
    pTeam t = pTeam(&*it);

    if (!t->skip() && t->matchTeam(sn, s_lc)) {
      filter.insert(t->Id);
      if (ret == 0)
        ret = t;
    }
  }

  for (it=Teams.begin(); it != itstart; ++it) {
    pTeam t = pTeam(&*it);

    if (!t->skip() && t->matchTeam(sn, s_lc)) {
      filter.insert(t->Id);
      if (ret == 0)
        ret = t;
    }
  }

  return ret;
}

bool oTeam::matchTeam(int number, const wchar_t *s_lc) const
{
  if (number) {
    if (matchNumber(StartNo, s_lc ))
        return true;

    for(size_t k = 0; k < Runners.size(); k++) {
      if (Runners[k] && matchNumber(Runners[k]->CardNo, s_lc))
        return true;
    }
  }

  if (filterMatchString(sName, s_lc))
    return true;

  for(size_t k=0;k<Runners.size();k++)
    if (Runners[k] && filterMatchString(Runners[k]->tRealName, s_lc))
      return true;

  return false;
}



void oEvent::fillPredefinedCmp(gdioutput &gdi, const string &name) const
{
  bool hasPatrol = getMeOSFeatures().hasFeature(MeOSFeatures::Patrol);
  bool hasMulti = getMeOSFeatures().hasFeature(MeOSFeatures::MultipleRaces);
  bool hasRelay = getMeOSFeatures().hasFeature(MeOSFeatures::Relay);
  bool hasForked = getMeOSFeatures().hasFeature(MeOSFeatures::ForkedIndividual);

  gdi.clearList(name);
  gdi.addItem(name, lang.tl("Endast en bana"), PNoMulti);
  gdi.addItem(name, lang.tl("Utan inst�llningar"), PNoSettings);
  if (hasForked) {
    gdi.addItem(name, lang.tl("En gafflad str�cka"), PForking);
    gdi.addItem(name, lang.tl("Banpool, gemensam start"), PPool);
    gdi.addItem(name, lang.tl("Banpool, lottad startlista"), PPoolDrawn);
  }
  if (hasMulti)
    gdi.addItem(name, lang.tl("Prolog + jaktstart"), PHunting);
  if (hasPatrol) {
    gdi.addItem(name, lang.tl("Patrull, 2 SI-pinnar"), PPatrol);
    gdi.addItem(name, lang.tl("Par- eller singelklass"), PPatrolOptional);
    gdi.addItem(name, lang.tl("Patrull, 1 SI-pinne"), PPatrolOneSI);
  }
  if (hasRelay)
    gdi.addItem(name, lang.tl("Stafett"), PRelay);
  if (hasMulti)
    gdi.addItem(name, lang.tl("Tv�mannastafett"), PTwinRelay);
  if (hasRelay)
    gdi.addItem(name, lang.tl("Extral�parstafett"), PYouthRelay);
}

void oEvent::setupRelayInfo(PredefinedTypes type, bool &useNLeg, bool &useStart)
{
  useNLeg = false;
  useStart = false;

    switch(type) {
    case PNoMulti:
      break;

    case PNoSettings:
      useNLeg = true;
      break;

    case PPool:
      useStart = true;
      break;

    case PForking:
      useStart = true;
      break;

    case PPoolDrawn:
      break;

    case PPatrol:
       break;

    case PPatrolOptional:
       break;

    case PPatrolOneSI:
       break;

    case PRelay:
      useStart = true;
      useNLeg = true;
      break;

    case PTwinRelay:
      useStart = true;
      useNLeg = true;
      break;

    case PYouthRelay:
      useStart = true;
      useNLeg = true;
      break;

    case PHunting:
      useStart = true;
      break;

    default:
      throw std::exception("Bad setup number");
  }
}

void oEvent::setupRelay(oClass &cls, PredefinedTypes type, int nleg, const wstring &start)
{
  // Make sure we are up-to-date
  autoSynchronizeLists(false);

  pCourse crs = cls.getCourse();
  int crsId = crs ? crs->getId() : 0;

  nleg=min(nleg, 40);
  cls.setNumStages(0);
  switch(type) {
    case PNoMulti:
      cls.setNumStages(0);
      cls.setCoursePool(false);
      break;

    case PNoSettings:
      cls.setNumStages(nleg);
      break;

    case PPool:
    case PForking:
      cls.setNumStages(1);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STTime, false);
      cls.setStartData(0, start);
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");
      cls.setCoursePool(type == PPool);

      if (crs) {
        cls.addStageCourse(0, crsId);
      }

      break;

    case PPoolDrawn:
      cls.setNumStages(1);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STDrawn, false);
      cls.setStartData(0, L"-");
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");
      cls.setCoursePool(true);

      if (crs) {
        cls.addStageCourse(0, crsId);
      }
      break;

    case PPatrol:
      cls.setNumStages(2);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STDrawn, false);
      cls.setStartData(0, L"-");
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      cls.setLegType(1, LTParallel);
      cls.setStartType(1, STDrawn, false);
      cls.setStartData(1, L"-");
      cls.setRestartTime(1, L"-");
      cls.setRopeTime(1, L"-");

      if (crs) {
        cls.addStageCourse(0, crsId);
        cls.addStageCourse(1, crsId);
      }
      cls.setCoursePool(false);
      break;

    case PPatrolOptional:
      cls.setNumStages(2);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STDrawn, false);
      cls.setStartData(0, L"-");
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      cls.setLegType(1, LTParallelOptional);
      cls.setStartType(1, STDrawn, false);
      cls.setStartData(1, L"-");
      cls.setRestartTime(1, L"-");
      cls.setRopeTime(1, L"-");

      if (crs) {
        cls.addStageCourse(0, crsId);
        cls.addStageCourse(1, crsId);
      }
      cls.setCoursePool(false);
      break;

    case PPatrolOneSI:
      cls.setNumStages(2);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STDrawn, false);
      cls.setStartData(0, L"-");
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      cls.setLegType(1, LTIgnore);
      cls.setStartType(1, STDrawn, false);
      cls.setStartData(1, start);
      cls.setRestartTime(1, L"-");
      cls.setRopeTime(1, L"-");

      if (crs) {
        cls.addStageCourse(0, crsId);
        cls.addStageCourse(1, crsId);
      }

      cls.setCoursePool(false);
      break;

    case PRelay:
      cls.setNumStages(nleg);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STTime, false);
      cls.setStartData(0, start);
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      for (int k=1;k<nleg;k++) {
        cls.setLegType(k, LTNormal);
        cls.setStartType(k, STChange, false);
        cls.setStartData(k, L"-");
        cls.setRestartTime(k, L"-");
        cls.setRopeTime(k, L"-");
      }
      cls.setCoursePool(false);
      break;

    case PTwinRelay:
      cls.setNumStages(nleg);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STTime, false);
      cls.setStartData(0, start);
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      for (int k=1;k<nleg;k++) {
        cls.setLegType(k, LTNormal);
        cls.setStartType(k, STChange, false);
        cls.setStartData(k, L"-");
        cls.setRestartTime(k, L"-");
        cls.setRopeTime(k, L"-");

        if (k>=2)
          cls.setLegRunner(k, k%2);
      }

      cls.setCoursePool(false);
      break;

    case PYouthRelay:
      nleg=max(nleg, 3);
      int last;
      cls.setNumStages(nleg+(nleg-2)*2);
      cls.setLegType(0, LTNormal);
      cls.setStartType(0, STTime, false);
      cls.setStartData(0, start);
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      last=nleg+(nleg-2)*2-1;
      cls.setLegType(last, LTNormal);
      cls.setStartType(last, STChange, false);
      cls.setStartData(last, L"-");
      cls.setRestartTime(last, L"-");
      cls.setRopeTime(last, L"-");

      for (int k=0;k<nleg-2;k++) {
        cls.setLegType(1+k*3, LTNormal);
        cls.setStartType(1+k*3, STChange, false);
        cls.setStartData(1+k*3, L"-");
        cls.setRestartTime(1+k*3, L"-");
        cls.setRopeTime(1+k*3, L"-");
        for (int j=0;j<2;j++) {
          cls.setLegType(2+k*3+j, LTExtra);
          cls.setStartType(2+k*3+j, STChange, false);
          cls.setStartData(2+k*3+j, L"-");
          cls.setRestartTime(2+k*3+j, L"-");
          cls.setRopeTime(2+k*3+j, L"-");
        }
      }
      cls.setCoursePool(false);
      break;

    case PHunting: {
      cls.setNumStages(2);
      cls.setLegType(0, LTSum);
      cls.setStartType(0, STDrawn, false);
      cls.setStartData(0, start);
      cls.setRestartTime(0, L"-");
      cls.setRopeTime(0, L"-");

      cls.setLegType(1, LTSum);
      cls.setStartType(1, STHunting, false); 
      int t = convertAbsoluteTimeHMS(start, ZeroTime)+3600;
      cls.setStartData(1, formatTimeHMS(t));
      cls.setRestartTime(1, formatTimeHMS(t+1800));
      cls.setRopeTime(1, formatTimeHMS(t+1800));
      cls.setLegRunner(1, 0);
      cls.setCoursePool(false);
      break;
    }
    default:
      throw std::exception("Bad setup number");
  }
  cls.apply();
  cls.synchronize(true);
  adjustTeamMultiRunners(&cls);
}

void oEvent::adjustTeamMultiRunners(pClass cls)
{
  if (cls) {
    bool multi = cls->getNumStages() > 1;
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip() || it->getClassId(false) != cls->getId())
        continue;
      if (multi && it->tInTeam == 0) {
        oe->autoAddTeam(&*it);
      }

      if (!multi && it->tInTeam) {
        assert( it->tInTeam->getClassId(false) == cls->getId());
        removeTeam(it->tInTeam->getId());
      }

      it->synchronizeAll();
    }

    vector<int> tr;
    for (oTeamList::iterator it=Teams.begin(); it != Teams.end(); ++it) {
      if (!multi && !it->isRemoved() && it->getClassId(false) == cls->getId()) {
        tr.push_back(it->getId());
      }
    }
    while(!tr.empty()) {
      removeTeam(tr.back());
      tr.pop_back();
    }
  }
  disableRecalculate = true;
  try {
    for (oTeamList::iterator it=Teams.begin(); it != Teams.end(); ++it) {
      it->adjustMultiRunners(true);
    }
  }
  catch(...) {
    disableRecalculate = false;
    throw;
  }
  disableRecalculate = false;
}

bool oTeam::adjustMultiRunners(bool sync)
{
  if (!Class)
    return false;

  for (size_t k = Class->getNumStages(); k<Runners.size(); k++) {
    setRunnerInternal(k, 0);
  }

  if (Class && Runners.size() != size_t(Class->getNumStages())) {
    Runners.resize(Class->getNumStages());
    updateChanged();
  }

  // Create multi runners.
  for (size_t i=0;i<Runners.size(); i++) {
    if (!Runners[i] && Class) {
       unsigned lr = Class->getLegRunner(i);

       if (lr<i && Runners[lr]) {
         Runners[lr]->createMultiRunner(true, sync);
         int dup=Class->getLegRunnerIndex(i);
         Runners[i]=Runners[lr]->getMultiRunner(dup);
       }
    }
  }

  return apply(sync, 0, false);
}

void oEvent::makeUniqueTeamNames() {
  sortTeams(ClassStartTime, 0, true);
  for (oClassList::const_iterator cls = Classes.begin(); cls != Classes.end(); ++cls) {
    if (cls->isRemoved())
      continue;
    map<wstring, list<pTeam> > teams;
    for (oTeamList::iterator it = Teams.begin(); it != Teams.end(); ++it) {
      if (it->skip())
        continue;
      if (it->Class != &*cls)
        continue;
      teams[it->sName].push_back(&*it);
    }

    for (map<wstring, list<pTeam> >::iterator it = teams.begin(); it != teams.end(); ++it) {
      list<pTeam> &t = it->second;
      if (t.size() > 1) {
        int counter = 1;
        for (list<pTeam>::iterator tit = t.begin(); tit != t.end(); ) {
          wstring name = (*tit)->sName + L" " + itow(counter);
          if (teams.count(name) == 0) {
            (*tit)->setName(name, true);
            (*tit)->synchronize();
            ++tit;
          }
          counter++;
        }
      }
    }
  }
}

void oTeam::changeId(int newId) {
  pTeam old = oe->teamById[Id];
  if (old == this)
    oe->teamById.remove(Id);

  oBase::changeId(newId);

  oe->teamById[newId] = this;
}

void oTeam::checkClassesWithReferences(oEvent &oe, std::set<int> &clsWithRef) {
  vector<pRunner> r;
  oe.getRunners(-1, -1, r, false);
  map<int, pair<int, int> > pairedUnpairedPerClass;
  for (size_t k = 0; k < r.size(); k++) {
    if (r[k]->getReference())
      ++pairedUnpairedPerClass[r[k]->getClassId(false)].first;
    else
      ++pairedUnpairedPerClass[r[k]->getClassId(false)].second;
  }

  for (auto &it : pairedUnpairedPerClass) {
    if (it.second.first > it.second.second)
      clsWithRef.insert(it.first);
  }
}

void oTeam::convertClassWithReferenceToPatrol(oEvent &oe, const std::set<int> &clsWithRef) {
  vector<pRunner> r;
  oe.getRunners(-1, -1, r, true);

  for(auto it : r) {
    if (clsWithRef.count(it->getClassId(false))) {
      pClass cls = it->getClassRef(false);

      if (cls->getNumStages() == 0) {
        pCourse crs = cls->getCourse();

        cls->setNumStages(2);

        if (crs) { // Preserve courses
          vector< vector<int> > mc(2);
          mc[0].push_back(crs->getId());
          mc[1].push_back(crs->getId());
          cls->importCourses(mc);
        }
        cls->setLegType(0, LTNormal);
        cls->setLegType(1, LTParallel);
        cls->synchronize();
      }

      if (cls->getClassType() == oClassPatrol) {
        wstring pname = it->getName();
        if (it->getReference())
          pname += L"/" + it->getReference()->getName();

        if (it->getTeam() == 0) {          
          pTeam patrol = oe.addTeam(pname, it->getClubId(), cls->getId());
          patrol->setRunner(0, it, false);
          patrol->setRunner(1, it->getReference(), false);
          patrol->synchronize();
        }
        else {
          pTeam patrol = it->getTeam();

          bool hasOther = it->getReference() == 0 || patrol->getRunner(0) == it->getReference() || patrol->getRunner(1) == it->getReference();

          if (!hasOther) {
            patrol->setName(pname, false);
            patrol->setRunner(0, it, false);
            patrol->setRunner(1, it->getReference(), false);
            patrol->synchronize();
          }
        }        
      }
    }
  }
}
