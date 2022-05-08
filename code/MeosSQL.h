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


#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

#include "oEvent.h"

class oRunner;
class oCard;
class oClub;
class oCourse;
class oClass;
class oControl;
class oBase;
class oFreePunch;
class oDataInterface;
class oTeam;
class oDataContainer;
struct SqlUpdated;

namespace sqlwrapper {  
  class ResNSel;
  class RowWrapper;
  class QueryWrapper;
  class ConnectionWrapper;
}

using namespace sqlwrapper;

enum OpFailStatus {
  opStatusOKSkipped = 3,
  opStatusOK = 2,
  opStatusFail = 0,
  opStatusWarning = 1,
  opUnreachable = -1,
};

class MeosSQL {
protected:
  bool warnedOldVersion;
  int monitorId;
  int buildVersion;
  shared_ptr<ConnectionWrapper> con;
  string CmpDataBase;
  void alert(const string &s);
  bool writeTime = false;

  vector<oBase *> missingObjects;

  string errorMessage;

  string serverName;
  string serverUser;
  string serverPassword;
  unsigned int serverPort;

  bool isOld(int counter, const string &time, oBase *ob);
  string andWhereOld(oBase *ob);

  OpFailStatus updateTime(const char *oTable, oBase *ob);
  // Update object in database with fixed query. If useId is false, Id is ignored (used
  OpFailStatus syncUpdate(QueryWrapper &updateqry, const char *oTable, oBase *ob);
  bool storeData(oDataInterface odi, const RowWrapper &row, unsigned long &revision);

  void importLists(oEvent *oe, const char *bf);
  void encodeLists(const oEvent *oe, string &listEnc) const;

  //Set DB to default competition DB
  void setDefaultDB();

  // Update the courses of a class.
  OpFailStatus syncReadClassCourses(oClass *c, const set<int> &courses,
                                    bool readRecursive);
  OpFailStatus syncRead(bool forceRead, oTeam *t, bool readRecursive);
  OpFailStatus syncRead(bool forceRead, oRunner *r, bool readClassClub, bool readCourseCard);
  OpFailStatus syncReadCourse(bool forceRead, oCourse *c, set<int> &readControls);
  OpFailStatus syncRead(bool forceRead, oClass *c, bool readCourses);
  OpFailStatus syncReadControls(oEvent *oe, const set<int> &controlIds);

  void storeClub(const RowWrapper &row, oClub &c);
  void storeControl(const RowWrapper &row, oControl &c);
  void storeCard(const RowWrapper &row, oCard &c);
  void storePunch(const RowWrapper &row, oFreePunch &p, bool rehash);

  OpFailStatus storeTeam(const RowWrapper &row, oTeam &t,
                         bool readRecursive, bool allowSubRead);

  OpFailStatus storeRunner(const RowWrapper &row, oRunner &r,
                           bool readCourseCard,
                           bool readClassClub,
                           bool readRunners,
                           bool allowSubRead);
  OpFailStatus storeCourse(const RowWrapper &row, oCourse &c,
                           set<int> &readControls,
                           bool allowSubRead);
  OpFailStatus storeClass(const RowWrapper &row, oClass &c,
                          bool readCourses,
                          bool allowSubRead);

  void getColumns(const string &table, set<string> &output);

  void upgradeDB(const string &db, oDataContainer const *odi);

  void warnOldDB();
  bool checkOldVersion(oEvent *oe, RowWrapper &row);

  map<pair<int, int>, DWORD> readTimes;
  void synchronized(const oBase &entity);
  bool skipSynchronize(const oBase &entity) const;

  ResNSel updateCounter(const char *oTable, int id, QueryWrapper *updateqry);
  string selectUpdated(const char *oTable, const SqlUpdated &updated);

  void addedFromDatabase(oBase *object);

  void updateCounters(OpFailStatus st, 
                      int counter, 
                      const string &modified, 
                      SqlUpdated &update, int &maxCounter);

  void checkAgainstDB(const char *oTable, map<int, oBase *> &existing, vector<pair<int, oBase *>> &idsToUpdate);

  template<typename T>
  bool checkTableCheckSum(const char *oTable, const list<T> &def, int p1, int p2, int p3);

  bool syncListRunner(oEvent *oe);
  bool syncListClass(oEvent *oe);
  bool syncListCourse(oEvent *oe);
  bool syncListControl(oEvent *oe);
  bool syncListCard(oEvent *oe);
  bool syncListClub(oEvent *oe);
  bool syncListPunch(oEvent *oe);
  bool syncListTeam(oEvent *oe);

  OpFailStatus SyncEvent(oEvent *oe);
  OpFailStatus SyncUpdate(oEvent *oe);
  OpFailStatus SyncRead(oEvent *oe);

  OpFailStatus syncUpdate(oRunner *r, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oRunner *r);

  OpFailStatus syncUpdate(oCard *c, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oCard *c);

  OpFailStatus syncUpdate(oClass *c, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oClass *c);

  OpFailStatus syncUpdate(oClub *c, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oClub *c);

  OpFailStatus syncUpdate(oCourse *c, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oCourse *c);

  OpFailStatus syncUpdate(oControl *c, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oControl *c);

  OpFailStatus syncUpdate(oFreePunch *c, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oFreePunch *c, bool rehash);

  OpFailStatus syncUpdate(oTeam *t, bool forceWriteAll);
  OpFailStatus syncRead(bool forceRead, oTeam *t);


public:

  bool synchronizeList(oEvent *oe, oListId lid);
  OpFailStatus synchronizeUpdate(oBase *obj);

  bool checkConsistency(oEvent *oe, bool force);
  void clearReadTimes();

  bool dropDatabase(oEvent *oe);
  bool checkConnection(oEvent *oe);
  void processMissingObjects();

  bool repairTables(const string &db, vector<string> &output);

  bool getErrorMessage(string &err);
  bool reConnect();
  bool listCompetitions(oEvent *oe, bool keepConnection);
  bool remove(oBase *ob);

  // Create database of runners and clubs
  bool createRunnerDB(oEvent *oe, QueryWrapper &query);

  // Upload runner database to server
  OpFailStatus uploadRunnerDB(oEvent *oe);

  bool openDB(oEvent *oe);
  bool closeDB();
  string serverVersion() const;

  /** General interface. TypeId lookup */
  OpFailStatus syncRead(bool forceRead, oBase *c);

  int getModifiedMask(oEvent &oe);

  MeosSQL(void);
  virtual ~MeosSQL(void);
};
