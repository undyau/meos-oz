#pragma once
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

#include "tabbase.h"
#include "gdioutput.h"
#include "Printer.h"
#include <string>
#include "oListInfo.h"

using namespace std;

class TabAuto;
class gdioutput;
class oEvent;

enum AutoSyncType {SyncNone, SyncTimer, SyncDataUp};

enum Machines {
  mPunchMachine,
  mPrintResultsMachine,
  mSplitsMachine,
  mPrewarningMachine,
  mOnlineResults,
  mOnlineInput,
  mSaveBackup,
  mInfoService,
};

class AutoMachine
{
private:
  int myid;
  static int uniqueId;
protected:
  bool editMode;

  void settingsTitle(gdioutput &gdi, char *title);
  enum IntervalType {IntervalNone, IntervalMinute, IntervalSecond};
  void startCancelInterval(gdioutput &gdi, char *startCommand, bool created, IntervalType type, const wstring &interval);
  
public:
  static AutoMachine *getMachine(int id);
  static void resetGlobalId() {uniqueId = 1;}
  int getId() const {return myid;}
  static AutoMachine* construct(Machines);
  void setEditMode(bool em) {editMode = em;}
  string name;
  DWORD interval; //Interval seconds
  DWORD timeout; //Timeout (TickCount)
  bool synchronize;
  bool synchronizePunches;
  virtual void settings(gdioutput &gdi, oEvent &oe, bool created) = 0;
  virtual void save(oEvent &oe, gdioutput &gdi) {}
  virtual void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast) = 0;
  virtual bool isEditMode() const {return editMode;}
  virtual void status(gdioutput &gdi) = 0;
  virtual bool stop() {return true;}
  virtual AutoMachine *clone() const = 0;
  AutoMachine(const string &s) : myid(uniqueId++), name(s), interval(0), timeout(0),
            synchronize(false), synchronizePunches(false), editMode(false) {}
  virtual ~AutoMachine() = 0 {}
};

class PrintResultMachine :
  public AutoMachine
{
protected:
  wstring exportFile;
  wstring exportScript;
  bool doExport;
  bool doPrint;
  bool structuredExport;
  PrinterObject po;
  set<int> classesToPrint;
  bool pageBreak;
  bool showInterResult;
  bool splitAnalysis;
  bool notShown;
  oListInfo listInfo;
  bool readOnly;
  int htmlRefresh;
  bool lock; // true while printing
  bool errorLock; // true while showing error dialog
public:
  PrintResultMachine *clone() const {
    PrintResultMachine *prm = new PrintResultMachine(*this);
    prm->lock = false;
    prm->errorLock = false;
    return prm;
  }
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  void settings(gdioutput &gdi, oEvent &oe, bool created);

  PrintResultMachine(int v):AutoMachine("Resultatutskrift") {
    interval=v;
    pageBreak = true;
    showInterResult = true;
    notShown = true;
    splitAnalysis = true;
    lock = false;
    errorLock = false;
    readOnly = false;
    doExport = false;
    doPrint = true;
    structuredExport = true;
    htmlRefresh = v;
  }
  PrintResultMachine(int v, const oListInfo &li):AutoMachine("Utskrift / export"), listInfo(li) {
    interval=v;
    pageBreak = true;
    showInterResult = true;
    notShown = true;
    splitAnalysis = true;
    lock = false;
    errorLock = false;
    readOnly = true;
    doExport = false;
    doPrint = true;
    structuredExport = false;
    htmlRefresh = v;
  }
  friend class TabAuto;
};

class SaveMachine :
  public AutoMachine
{
protected:
  wstring baseFile;
  int saveIter;
public:
  SaveMachine *clone() const {
    SaveMachine *prm = new SaveMachine(*this);
    return prm;
  }

  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  void saveSettings(gdioutput &gdi);

  SaveMachine():AutoMachine("S�kerhetskopiera") , saveIter(0) {
  }
};


class PrewarningMachine :
  public AutoMachine
{
protected:
  wstring waveFolder;
  set<int> controls;
  set<int> controlsSI;
public:
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  PrewarningMachine *clone() const {return new PrewarningMachine(*this);}
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  PrewarningMachine():AutoMachine("F�rvarningsr�st") {}
  friend class TabAuto;
};

class MySQLReconnect :
  public AutoMachine
{
protected:
  wstring error;
  wstring timeError;
  wstring timeReconnect;
  HANDLE hThread;
public:
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  MySQLReconnect *clone() const {return new MySQLReconnect(*this);}
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  bool stop();
  MySQLReconnect(const wstring &error);
  virtual ~MySQLReconnect();
  friend class TabAuto;
};

bool isThreadReconnecting();

class PunchMachine :
  public AutoMachine
{
protected:
  int radio;
public:
  PunchMachine *clone() const {return new PunchMachine(*this);}
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  PunchMachine():AutoMachine("St�mplingsautomat"), radio(0) {}
  friend class TabAuto;
};

class SplitsMachine :
  public AutoMachine
{
protected:
  wstring file;
  set<int> classes;
  int leg;
public:
  SplitsMachine *clone() const {return new SplitsMachine(*this);}
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  SplitsMachine() : AutoMachine("Str�cktider/WinSplits"), leg(-1) {}
  friend class TabAuto;
};



class TabAuto :
  public TabBase
{
private:
  //DWORD printResultIntervalSec;
  //DWORD printResultTimeOut;
  bool editMode;

  bool synchronize;
  bool synchronizePunches;
  void updateSyncInfo();

  list<AutoMachine *> machines;
  void setTimer(AutoMachine *am);

  void timerCallback(gdioutput &gdi);
  void syncCallback(gdioutput &gdi);

  void settings(gdioutput &gdi, AutoMachine *sm, Machines type);

protected:
  void clearCompetitionData();

public:

  AutoMachine *getMachine(int id);
  //AutoMachine *getMachine(const string &name);
  bool stopMachine(AutoMachine *am);
  void killMachines();
  void addMachine(const AutoMachine &am) {
    machines.push_back(am.clone());
    setTimer(machines.back());
  }

  int processButton(gdioutput &gdi, const ButtonInfo &bu);
  int processListBox(gdioutput &gdi, const ListBoxInfo &bu);

  bool loadPage(gdioutput &gdi);

  const char * getTypeStr() const {return "TAutoTab";}
  TabType getType() const {return TAutoTab;}

  TabAuto(oEvent *poe);
  ~TabAuto(void);

  friend class AutoTask;
  friend void tabForceSync(gdioutput &gdi, pEvent oe);
};

void tabAutoKillMachines();
void tabAutoRegister(TabAuto *ta);
void tabAutoAddMachinge(const AutoMachine &am);
