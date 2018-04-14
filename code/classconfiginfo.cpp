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

#include "oEvent.h"
#include "classconfiginfo.h"
#include "meos_util.h"

void ClassConfigInfo::clear() {
  individual.clear();
  relay.clear();
  patrol.clear();

  legNStart.clear();
  raceNStart.clear();

  legResult.clear();
  raceNRes.clear();

  rogainingClasses.clear();
  timeStart.clear();
  hasMultiCourse = false;
  hasMultiEvent = false;
  hasRentedCard = false;
  classWithoutCourse.clear();
  maximumLegNumber = 0;
  results = false;
  starttimes = false;
}

bool ClassConfigInfo::empty() const {
  return individual.empty() && relay.empty() && patrol.empty() && raceNStart.empty();
}

void ClassConfigInfo::getIndividual(set<int> &sel) const {
  sel.insert(individual.begin(), individual.end());
}

void ClassConfigInfo::getRelay(set<int> &sel) const {
  sel.insert(relay.begin(), relay.end());
}

void ClassConfigInfo::getTeamClass(set<int> &sel) const {
  sel.insert(relay.begin(), relay.end());
  sel.insert(patrol.begin(), patrol.end());
  if (!raceNStart.empty())
    sel.insert(raceNRes[0].begin(), raceNRes[0].end());
}


bool ClassConfigInfo::hasTeamClass() const {
  return !relay.empty() || !patrol.empty() || !raceNRes.empty();
}

void ClassConfigInfo::getPatrol(set<int> &sel) const {
  sel.insert(patrol.begin(), patrol.end());
}

void ClassConfigInfo::getRogaining(set<int> &sel) const {
  sel.insert(rogainingClasses.begin(), rogainingClasses.end());
}


void ClassConfigInfo::getRaceNStart(int race, set<int> &sel) const {
  if (size_t(race) < raceNStart.size() && !raceNStart[race].empty())
    sel.insert(raceNStart[race].begin(), raceNStart[race].end());
  else
    sel.clear();
}

void ClassConfigInfo::getLegNStart(int leg, set<int> &sel) const {
  if (size_t(leg) < legNStart.size() && !legNStart[leg].empty())
    sel.insert(legNStart[leg].begin(), legNStart[leg].end());
  else
    sel.clear();
}

void ClassConfigInfo::getRaceNRes(int race, set<int> &sel) const {
  if (size_t(race) < raceNRes.size() && !raceNRes[race].empty())
    sel.insert(raceNRes[race].begin(), raceNRes[race].end());
  else
    sel.clear();
}

void ClassConfigInfo::getLegNRes(int leg, set<int> &sel) const {
  map<int, vector<int> >::const_iterator res = legResult.find(leg);
  if (res != legResult.end())
    sel.insert(res->second.begin(), res->second.end());
  else
    sel.clear();
}

void oEvent::getClassConfigurationInfo(ClassConfigInfo &cnf) const
{
  oClassList::const_iterator it;
  cnf.clear();

  cnf.hasMultiEvent = hasPrevStage() || hasNextStage();

  for (it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;

    cnf.maximumLegNumber = max<int>(cnf.maximumLegNumber, it->getNumStages());
    ClassType ct = it->getClassType();

    if (it->isRogaining())
      cnf.rogainingClasses.push_back(it->getId());

    if (it->getCourse() == 0)
      cnf.classWithoutCourse.push_back(it->getName()); //MultiCourse not analysed...

    if ( !it->hasCoursePool() ) {
      for (size_t k = 0; k< it->MultiCourse.size(); k++) {
        if (it->MultiCourse[k].size() > 1)
          cnf.hasMultiCourse = true;
      }
    }
    if (ct == oClassIndividual) {
      cnf.individual.push_back(it->getId());
      if (cnf.timeStart.empty())
        cnf.timeStart.resize(1);
      cnf.timeStart[0].push_back(it->getId());
    }
    else if (ct == oClassPatrol)
      cnf.patrol.push_back(it->getId());
    else if (ct == oClassRelay) {
      cnf.relay.push_back(it->getId());

      if (cnf.legNStart.size() < it->getNumStages())
        cnf.legNStart.resize(it->getNumStages());
      
      for (size_t k = 0; k < it->getNumStages(); k++) {
        StartTypes st = it->getStartType(k);
        if (st == STDrawn || st == STHunting) {
          cnf.legNStart[k].push_back(it->getId());
          if (cnf.timeStart.size() <= k)
            cnf.timeStart.resize(k+1);
          cnf.timeStart[k].push_back(it->getId());
        }

        LegTypes lt = it->getLegType(k);
        if (!it->isOptional(k) && !it->isParallel(k) && lt != LTGroup) {
          int trueN, order;
          it->splitLegNumberParallel(k, trueN, order);
          cnf.legResult[trueN].push_back(it->getId());
        }
      }
    }
    else if (ct == oClassIndividRelay) {
      if (cnf.raceNStart.size() < it->getNumStages())
        cnf.raceNStart.resize(it->getNumStages());
      if (cnf.raceNRes.size() < it->getNumStages())
        cnf.raceNRes.resize(it->getNumStages());

      for (size_t k = 0; k < it->getNumStages(); k++) {
        StartTypes st = it->getStartType(k);
        if (st == STDrawn || st == STHunting) {
          cnf.raceNStart[k].push_back(it->getId());
          if (cnf.timeStart.size() <= k)
            cnf.timeStart.resize(k+1);
          cnf.timeStart[k].push_back(it->getId());

        }
        LegTypes lt = it->getLegType(k);
        if (lt != LTIgnore && lt != LTExtra && lt != LTGroup)
          cnf.raceNRes[k].push_back(it->getId());
      }
    }
  }

  oRunnerList::const_iterator rit;
  for (rit = Runners.begin(); rit != Runners.end(); ++rit) {
    if (rit->isRemoved())
      continue;

    if (rit->getDCI().getInt("CardFee") != 0) {
      cnf.hasRentedCard = true;
    }
    RunnerStatus st = rit->getStatus();
    if (st != StatusUnknown && st != StatusDNS && st != StatusNotCompetiting)
      cnf.results = true;

    if (rit->getStartTime() > 0)
      cnf.starttimes = true;
  }
}

