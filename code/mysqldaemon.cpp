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
#include "TabAuto.h"
#include "meos_util.h"
#include "gdiconstants.h"
#include "meosdb/sqltypes.h"
#include <process.h>

MySQLReconnect::MySQLReconnect(const wstring &errorIn) : AutoMachine("MySQL-daemon"), error(errorIn)
{
  timeError = getLocalTime();
  hThread=0;
}

MySQLReconnect::~MySQLReconnect()
{
  CloseHandle(hThread);
  hThread=0;
}

bool MySQLReconnect::stop()
{
  if (interval==0)
    return true;

  return MessageBox(0, L"If this daemon is stopped, then MeOS will not reconnect to the network. Continue?",
    L"Warning", MB_YESNO|MB_ICONWARNING)==IDYES;
}

static CRITICAL_SECTION CS_MySQL;
static volatile DWORD mysqlConnecting=0;
static volatile DWORD mysqlStatus=0;

void initMySQLCriticalSection(bool init) {
  if (init)
    InitializeCriticalSection(&CS_MySQL);
  else
    DeleteCriticalSection(&CS_MySQL);
}

bool isThreadReconnecting()
{
  EnterCriticalSection(&CS_MySQL);
  bool res = (mysqlConnecting != 0);
  LeaveCriticalSection(&CS_MySQL);
  return res;
}

unsigned __stdcall reconnectThread(void *v) {
  EnterCriticalSection(&CS_MySQL);
    mysqlConnecting=1;
    mysqlStatus=0;
  LeaveCriticalSection(&CS_MySQL);

  bool res = msReConnect();

  EnterCriticalSection(&CS_MySQL);
    if (res)
      mysqlStatus=1;
    else
      mysqlStatus=-1;

    mysqlConnecting=0;
  LeaveCriticalSection(&CS_MySQL);

  return 0;
}

void MySQLReconnect::settings(gdioutput &gdi, oEvent &oe, bool created) {
}

void MySQLReconnect::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
  if (isThreadReconnecting())
    return;

  if (mysqlStatus==1) {
    if (hThread){
      CloseHandle(hThread);
      hThread=0;
    }
    mysqlStatus=0;
    char bf[256];
    if (!oe->reConnect(bf)) {
      gdi.addInfoBox("", L"warning:dbproblem#" + gdi.widen(bf), 9000);
      interval = 10;
    }
    else {
      gdi.addInfoBox("", L"�teransluten mot databasen, t�vlingen synkroniserad.", 10000);
      timeReconnect = getLocalTime();
      gdi.setDBErrorState(false);
      gdi.setWindowTitle(oe->getTitleName());
      interval=0;
    }
  }
  else if (mysqlStatus==-1) {
    if (hThread){
      CloseHandle(hThread);
      hThread=0;
    }
    mysqlStatus=0;
    interval = 10;//Wait ten seconds for next attempt

    gdi.setDBErrorState(true);
    char bf[256];
    if (oe->HasDBConnection) {
      msGetErrorState(bf);
    }
    return;
  }
  else {
    mysqlConnecting = 1;
    hThread = (HANDLE) _beginthreadex (0, 0, &reconnectThread, 0, 0, 0);
    interval = 1;
  }
}

int AutomaticCB(gdioutput *gdi, int type, void *data);

void MySQLReconnect::status(gdioutput &gdi) {
  gdi.dropLine(0.5);
  gdi.addString("", 1, name);
  gdi.pushX();
  if (interval>0){
    gdi.addStringUT(1, timeError + L": " + lang.tl("DATABASE ERROR")).setColor(colorDarkRed);
    gdi.fillRight();
    gdi.addString("", 0, "N�sta f�rs�k:");
    gdi.addTimer(gdi.getCY(),  gdi.getCX()+10, timerCanBeNegative, (GetTickCount()-timeout)/1000);
  }
  else {
    gdi.addStringUT(0, timeError + L": " + lang.tl("DATABASE ERROR")).setColor(colorDarkGrey);
    gdi.fillRight();
    gdi.addStringUT(0, timeReconnect + L":");
    gdi.addString("", 1, "�teransluten mot databasen, t�vlingen synkroniserad.").setColor(colorDarkGreen);
    gdi.dropLine();
    gdi.fillDown();
    gdi.popX();
  }
  gdi.popX();
  gdi.fillDown();
  gdi.dropLine();

  gdi.popX();
  gdi.dropLine(0.3);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(getId());
}
