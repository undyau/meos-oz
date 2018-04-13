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

// meos.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include <shlobj.h>

#include "oEvent.h"
#include "xmlparser.h"
#include "recorder.h"

#include "gdioutput.h"
#include "commctrl.h"
#include "SportIdent.h"
#include "TabBase.h"
#include "TabCompetition.h"
#include "TabAuto.h"
#include "TabClass.h"
#include "TabCourse.h"
#include "TabControl.h"
#include "TabSI.h"
#include "TabList.h"
#include "TabTeam.h"
#include "TabSpeaker.h"
#include "TabMulti.h"
#include "TabRunner.h"
#include "TabClub.h"
#include "progress.h"
#include "inthashmap.h"
#include <cassert>
#include "localizer.h"
#include "intkeymap.hpp"
#include "intkeymapimpl.hpp"
#include "download.h"
#include "meos_util.h"
#include <sys/stat.h>
#include "random.h"
#include "metalist.h"
#include "gdiconstants.h"
#include "socket.h"
#include "oExtendedEvent.h"
#include "autotask.h"
#include "meosexception.h"
#include "parser.h"
#include "restserver.h"

gdioutput *gdi_main=0;
oEvent *gEvent=0;
SportIdent *gSI=0;
Localizer lang;
AutoTask *autoTask = 0;
#ifdef _DEBUG
  bool enableTests = true;
#else
  bool enableTests = false;
#endif

vector<gdioutput *> gdi_extra;
void initMySQLCriticalSection(bool init);

HWND hWndMain;
HWND hWndWorkspace;

#define MAX_LOADSTRING 100

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void removeTempFiles();
void Setup(bool overwrite, bool overwriteAll);

// Global Variables:
HINSTANCE hInst; // current instance
TCHAR szTitle[MAX_LOADSTRING]; // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING]; // The title bar text
TCHAR szWorkSpaceClass[MAX_LOADSTRING]; // The title bar text

// Foward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WorkSpaceWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
void registerToolbar(HINSTANCE hInstance);
extern const wchar_t *szToolClass;

HHOOK g_hhk; //- handle to the hook procedure.

HWND hMainTab=NULL;

list<TabObject> *tabList=0;
void scrollVertical(gdioutput *gdi, int yInc, HWND hWnd);
static int currentFocusIx = 0;

void resetSaveTimer() {
  if (autoTask)
    autoTask->resetSaveTimer();
}

void LoadPage(const string &name)
{
  list<TabObject>::iterator it;

  for (it=tabList->begin(); it!=tabList->end(); ++it) {
    if (it->name==name)
      it->loadPage(*gdi_main);
  }
}

void LoadClassPage(gdioutput &gdi)
{
  LoadPage("Klasser");
}

void dumpLeaks() {
  _CrtDumpMemoryLeaks();
}

void LoadPage(gdioutput &gdi, TabType type) {
  gdi.setWaitCursor(true);
  TabBase *t = gdi.getTabs().get(type);
  if (t)
    t->loadPage(gdi);
  gdi.setWaitCursor(false);
}

// Path to settings file
static wchar_t settings[260];
// Startup path
static wchar_t programPath[MAX_PATH];

void mainMessageLoop(HACCEL hAccelTable, DWORD time) {
  MSG msg;
  BOOL bRet;
  
  if (time > 0) {
    time += GetTickCount();
  }
  // Main message loop:
  while ( (bRet = GetMessage(&msg, NULL, 0, 0)) != 0 ) {
    if (bRet == -1)
      return;
    if (gEvent != 0)
      RestServer::computeRequested(*gEvent);

    if (hAccelTable == 0 || !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    if (time != 0) {
      if (GetTickCount() > time)
        return;
    }
  }
}


int APIENTRY WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nCmdShow)
{
  atexit(dumpLeaks);	//
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  if (strstr(lpCmdLine, "-s") != 0) {
    Setup(true, false);
    exit(0);
  }
  else if (strstr(lpCmdLine, "-test") != 0) {
    enableTests = true;
  }

  for (int k = 0; k < 100; k++) {
    RunnerStatusOrderMap[k] = 0;
  }
  RunnerStatusOrderMap[StatusOK] = 0;
  RunnerStatusOrderMap[StatusMAX] = 1;
  RunnerStatusOrderMap[StatusMP] = 2;
  RunnerStatusOrderMap[StatusDNF] = 3;
  RunnerStatusOrderMap[StatusDQ] = 4;
  RunnerStatusOrderMap[StatusCANCEL] = 5;
  RunnerStatusOrderMap[StatusDNS] = 6;
  RunnerStatusOrderMap[StatusUnknown] = 7;
  RunnerStatusOrderMap[StatusNotCompetiting] = 8;

  lang.init();
  StringCache::getInstance().init();

  GetCurrentDirectory(MAX_PATH, programPath);

  getUserFile(settings, L"meoswpref.xml");
  Parser::test();

  int rInit = (GetTickCount() / 100);
  InitRanom(rInit, rInit/379);

  tabList=new list<TabObject>;

  HACCEL hAccelTable;

  gdi_main = new gdioutput("main", 1.0, 0);
  gdi_extra.push_back(gdi_main);

  try {
	  gEvent = new oExtendedEvent(*gdi_main);
  }
  catch (meosException &ex) {
    gdi_main->alert(wstring(L"Failed to create base event: ") + ex.wwhat());
    return 0;
  }
  catch (std::exception &ex) {
    gdi_main->alert(string("Failed to create base event: ") + ex.what());
    return 0;
  }

  if (fileExist(settings)) {
    gEvent->loadProperties(settings);
  }
  else {
    wchar_t oldSettings[260];  
    // Read from older version
    getUserFile(oldSettings, L"meospref.xml");
    gEvent->loadProperties(oldSettings);
  }
  
  lang.get().addLangResource(L"English", L"104");
  lang.get().addLangResource(L"Svenska", L"103");
  lang.get().addLangResource(L"Deutsch", L"105");
  lang.get().addLangResource(L"Dansk", L"106");
  lang.get().addLangResource(L"Fran�ais", L"110");
  lang.get().addLangResource(L"Russian (ISO 8859-5)", L"107");
  lang.get().addLangResource(L"English (ISO 8859-2)", L"108");
  lang.get().addLangResource(L"English (ISO 8859-8)", L"109");

  if (fileExist(L"extra.lng")) {
    lang.get().addLangResource(L"Extraspr�k", L"extra.lng");
  }
  else {
    wchar_t lpath[260];
    getUserFile(lpath, L"extra.lng");
    if (fileExist(lpath))
      lang.get().addLangResource(L"Extraspr�k", lpath);
  }

  wstring defLang = gEvent->getPropertyString("Language", L"English");

  // Backward compatibility
  if (defLang==L"103")
    defLang = L"Svenska";
  else if (defLang==L"104")
    defLang = L"English";

  gEvent->setProperty("Language", defLang);

  try {
    lang.get().loadLangResource(defLang);
  }
  catch (std::exception &) {
    lang.get().loadLangResource(L"English");
  }

  try {
    wchar_t listpath[MAX_PATH];
    getUserFile(listpath, L"");
    vector<wstring> res;
    expandDirectory(listpath, L"*.lxml", res);
    expandDirectory(listpath, L"*.listdef", res);
#
#ifdef _DEBUG
    expandDirectory(L".\\Lists\\", L"*.lxml", res);
    expandDirectory(L".\\Lists\\", L"*.listdef", res);
#endif
    wstring err;

    for (size_t k = 0; k<res.size(); k++) {
      try {
        xmlparser xml;

        wcscpy_s(listpath, res[k].c_str());
        xml.read(listpath);

        xmlobject xlist = xml.getObject(0);
        gEvent->getListContainer().load(MetaListContainer::InternalList, xlist, true);
      }
      catch (meosException &ex) {
        wstring errLoc = L"Kunde inte ladda X\n\n(Y)#" + wstring(listpath) + L"#" + lang.tl(ex.wwhat());
        if (err.empty())
          err = lang.tl(errLoc);
        else
          err += L"\n" + lang.tl(errLoc);
      }
      catch (std::exception &ex) {
        wstring errLoc = L"Kunde inte ladda X\n\n(Y)#" + wstring(listpath) + L"#" + lang.tl(ex.what());
        if (err.empty())
          err = lang.tl(errLoc);
        else
          err += L"\n" + lang.tl(errLoc);
      }
    }
    if (!err.empty())
      gdi_main->alert(err);
  }
  catch (meosException &ex) {
    gdi_main->alert(ex.wwhat());
  }
  catch (std::exception &ex) {
    gdi_main->alert(ex.what());
  }

  gEvent->openRunnerDatabase(L"database");
  wcscpy_s(szTitle, L"MeOS");
  wcscpy_s(szWindowClass, L"MeosMainClass");
  wcscpy_s(szWorkSpaceClass, L"MeosWorkSpace");
  MyRegisterClass(hInstance);
  registerToolbar(hInstance);

  string encoding = gdi_main->narrow(lang.tl("encoding"));
/*FontEncoding interpetEncoding(const string &enc) {
  if (enc == "RUSSIAN")
    return Russian;
  else if (enc == "EASTEUROPE")
    return EastEurope;
  else if (enc == "HEBREW")
    return Hebrew;
  else
    return ANSI;
}*/

  gdi_main->setFont(gEvent->getPropertyInt("TextSize", 0),
                    gEvent->getPropertyString("TextFont", L"Arial"));

  // Perform application initialization:
  if (!InitInstance (hInstance, nCmdShow)) {
    return FALSE;
  }

  RECT rc;
  GetClientRect(hWndMain, &rc);
  SendMessage(hWndMain, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));

  gdi_main->init(hWndWorkspace, hWndMain, hMainTab);
  gdi_main->getTabs().get(TCmpTab)->loadPage(*gdi_main);

  autoTask = new AutoTask(hWndMain, *gEvent, *gdi_main);

  autoTask->setTimers();

  // Install a hook procedure to monitor the message stream for mouse
  // messages intended for the controls in the dialog box.
  g_hhk = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
      (HINSTANCE) NULL, GetCurrentThreadId());

  hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_MEOS);

  initMySQLCriticalSection(true);
  // Main message loop:
  mainMessageLoop(hAccelTable, 0);

  tabAutoRegister(0);
  tabList->clear();
  delete tabList;
  tabList=0;

  delete autoTask;
  autoTask = 0;

  for (size_t k = 0; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      DestroyWindow(gdi_extra[k]->getHWNDMain());
      if (k < gdi_extra.size()) {
        delete gdi_extra[k];
        gdi_extra[k] = 0;
      }
    }
  }

  gdi_extra.clear();

  if (gEvent)
    gEvent->saveProperties(settings);

  delete gEvent;
  gEvent = 0;

  initMySQLCriticalSection(false);

  removeTempFiles();

  #ifdef _DEBUG
    lang.get().debugDump(L"untranslated.txt", L"translated.txt");
  #endif

  StringCache::getInstance().clear();
  lang.unload();

  return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style			= CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc	= (WNDPROC)WndProc;
  wcex.cbClsExtra		= 0;
  wcex.cbWndExtra		= 0;
  wcex.hInstance		= hInstance;
  wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MEOS);
  wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName	= 0;//(LPCSTR)IDC_MEOS;
  wcex.lpszClassName	= szWindowClass;
  wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

  RegisterClassEx(&wcex);

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wcex.lpfnWndProc	= (WNDPROC)WorkSpaceWndProc;
  wcex.cbClsExtra		= 0;
  wcex.cbWndExtra		= 0;
  wcex.hInstance		= hInstance;
  wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MEOS);
  wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground	= 0;
  wcex.lpszMenuName	= 0;
  wcex.lpszClassName	= szWorkSpaceClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
  RegisterClassEx(&wcex);

  return true;
}


// GetMsgProc - monitors the message stream for mouse messages intended
//     for a control window in the dialog box.
// Returns a message-dependent value.
// nCode - hook code.
// wParam - message flag (not used).
// lParam - address of an MSG structure.
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MSG *lpmsg;

    lpmsg = (MSG *) lParam;
    if (nCode < 0 || !(IsChild(hWndWorkspace, lpmsg->hwnd)))
        return (CallNextHookEx(g_hhk, nCode, wParam, lParam));

    switch (lpmsg->message) {
  case WM_MOUSEMOVE:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
    if (gdi_main->getToolTip() != NULL) {
      MSG msg;

      msg.lParam = lpmsg->lParam;
      msg.wParam = lpmsg->wParam;
      msg.message = lpmsg->message;
      msg.hwnd = lpmsg->hwnd;
      SendMessage(gdi_main->getToolTip(), TTM_RELAYEVENT, 0,
        (LPARAM) (LPMSG) &msg);
    }
    break;
  default:
    break;
    }
    return (CallNextHookEx(g_hhk, nCode, wParam, lParam));
}

void flushEvent(const string &id, const string &origin, DWORD data, int extraData)
{
  for (size_t k = 0; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      gdi_extra[k]->makeEvent(id, origin, data, extraData, false);
    }
  }
}

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
  if (code<0)
    return CallNextHookEx(0, code, wParam, lParam);

  gdioutput *gdi = 0;

  if (size_t(currentFocusIx) < gdi_extra.size())
    gdi = gdi_extra[currentFocusIx];

  if (!gdi)
    gdi = gdi_main;


  HWND hWnd = gdi ? gdi->getHWNDTarget() : 0;

  bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) == 0x8000;
  bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;

  //if (code<0) return CallNextHookEx(
  if (wParam==VK_TAB) {
    if ( (lParam& (1<<31))) {
      SHORT state=GetKeyState(VK_SHIFT);
      if (gdi) {
        if (state&(1<<16))
          gdi->TabFocus(-1);
        else
          gdi->TabFocus(1);
      }
    }
    return 1;
  }
  else if (wParam==VK_RETURN && (lParam & (1<<31))) {
    if (gdi)
      gdi->Enter();
  }
  else if (wParam==VK_UP) {
    bool c = false;
    if (gdi  && (lParam & (1<<31)))
      c = gdi->UpDown(1);

    if (!c  && !(lParam & (1<<31)) && !(gdi && gdi->lockUpDown))
      SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
  }
  else if (wParam == VK_NEXT && !(lParam & (1<<31)) && !(gdi && gdi->lockUpDown)) {
    SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_PAGEDOWN, 0), 0);
  }
  else if (wParam == VK_PRIOR && !(lParam & (1<<31)) && !(gdi && gdi->lockUpDown)) {
    SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_PAGEUP, 0), 0);
  }
  else if (wParam==VK_DOWN) {
    bool c = false;
    if (gdi && (lParam & (1<<31)))
      c = gdi->UpDown(-1);

    if (!c && !(lParam & (1<<31)) && !(gdi && gdi->lockUpDown))
      SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
  }
  else if (wParam==VK_LEFT && !(lParam & (1<<31))) {
    if (!gdi || !gdi->hasEditControl())
      SendMessage(hWnd, WM_HSCROLL, MAKELONG(SB_LINEUP, 0), 0);
  }
  else if (wParam==VK_RIGHT && !(lParam & (1<<31))) {
    if (!gdi || !gdi->hasEditControl())
      SendMessage(hWnd, WM_HSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
  }
  else if (wParam==VK_ESCAPE && (lParam & (1<<31))) {
    if (gdi)
      gdi->Escape();
  }
  else if (wParam==VK_F2) {
    ProgressWindow pw(hWnd);

    pw.init();
    for (int k=0;k<=20;k++) {
      pw.setProgress(k*50);
      Sleep(100);
    }
    //pw.draw();
  }
  else if (ctrlPressed && (wParam == VK_ADD || wParam == VK_SUBTRACT ||
           wParam == VK_F5 || wParam == VK_F6)) {
    if (gdi) {
      if (wParam == VK_ADD || wParam == VK_F5)
        gdi->scaleSize(1.1);
      else
        gdi->scaleSize(1.0/1.1);
    }
  }
  else if (wParam == 'C' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_COPY);
  }
  else if (wParam == 'V'  && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_PASTE);
  }
  else if (wParam == 'F'  && ctrlPressed) {
    if (gdi) {
      if (!shiftPressed)
        gdi->keyCommand(KC_FIND);
      else
        gdi->keyCommand(KC_FINDBACK);
    }
  }
  else if (wParam == VK_DELETE) {
    if (gdi)
      gdi->keyCommand(KC_DELETE);
  }
  else if (wParam == 'I' &&  ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_INSERT);
  }
  else if (wParam == 'P' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_PRINT);
  }
  else if (wParam == VK_F5 && !ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_REFRESH);
  }
  else if (wParam == 'M' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_SPEEDUP);
  }
  else if (wParam == 'N' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_SLOWDOWN);
  }
  else if (wParam == ' ' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_AUTOCOMPLETE);
  }

  return 0;
}
//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  HWND hWnd;

  hInst = hInstance; // Store instance handle in our global variable
  //WS_EX_CONTROLPARENT
  HWND hDskTop=GetDesktopWindow();
  RECT rc;
  GetClientRect(hDskTop, &rc);

  int xp = gEvent->getPropertyInt("xpos", 50);
  int yp = gEvent->getPropertyInt("ypos", 20);

  int xs = gEvent->getPropertyInt("xsize", max(850, min(int(rc.right)-yp, 1124)));
  int ys = gEvent->getPropertyInt("ysize", max(650, min(int(rc.bottom)-yp-40, 800)));

  gEvent->setProperty("ypos", yp + 16);
  gEvent->setProperty("xpos", xp + 32);
  gEvent->saveProperties(settings); // For other instance starting while running
  gEvent->setProperty("ypos", yp);
  gEvent->setProperty("xpos", xp);

  hWnd = CreateWindowEx(0, szWindowClass, szTitle,
          WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
          xp, yp, max(min(int(rc.right)-yp, xs), 200),
                  max(min(int(rc.bottom)-yp-40, ys), 100),
          NULL, NULL, hInstance, NULL);

  if (!hWnd)
    return FALSE;

  hWndMain = hWnd;

  SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, 0, GetCurrentThreadId());
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  hWnd = CreateWindowEx(0, szWorkSpaceClass, L"WorkSpace", WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
    50, 200, 200, 100, hWndMain, NULL, hInstance, NULL);

  if (!hWnd)
    return FALSE;

  hWndWorkspace=hWnd;
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

void destroyExtraWindows() {
  for (size_t k = 1; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      DestroyWindow(gdi_extra[k]->getHWNDMain());
    }
  }
}

string uniqueTag(const char *base) {
  int j = 0;
  string b = base;
  while(true) {
    string tag = b + itos(j++);
    if (getExtraWindow(tag, false) == 0)
      return tag;
  }
}

vector<string> getExtraWindows() {
  vector<string> res;
  for (size_t k = 0; k < gdi_extra.size(); k++) {
    if (gdi_extra[k])
      res.push_back(gdi_extra[k]->getTag());
  }
  return res;
}

gdioutput *getExtraWindow(const string &tag, bool toForeGround) {
  for (size_t k = 0; k<gdi_extra.size(); k++) {
    if (gdi_extra[k] && gdi_extra[k]->hasTag(tag)) {
      if (toForeGround)
        SetForegroundWindow(gdi_extra[k]->getHWNDMain());
      return gdi_extra[k];
    }
  }
  return 0;
}

gdioutput *createExtraWindow(const string &tag, const wstring &title, int max_x, int max_y) {
  if (getExtraWindow(tag, false) != 0)
    throw meosException("Window already exists");

  HWND hWnd;


  HWND hDskTop=GetDesktopWindow();
  RECT rc;
  GetClientRect(hDskTop, &rc);

  int xp = gEvent->getPropertyInt("xpos", 50) + 16;
  int yp = gEvent->getPropertyInt("ypos", 20) + 32;

  for (size_t k = 0; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      HWND hWnd = gdi_extra[k]->getHWNDTarget();
      RECT rc;
      if (GetWindowRect(hWnd, &rc)) {
        xp = max<int>(rc.left + 16, xp);
        yp = max<int>(rc.top + 32, yp);
      }
    }
  }

  int xs = gEvent->getPropertyInt("xsize", max(850, min(int(rc.right)-yp, 1124)));
  int ys = gEvent->getPropertyInt("ysize", max(650, min(int(rc.bottom)-yp-40, 800)));

  if (max_x>0)
    xs = min(max_x, xs);
  if (max_y>0)
    ys = min(max_y, ys);

  hWnd = CreateWindowEx(0, szWorkSpaceClass, title.c_str(),
    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
    xp, yp, max(xs, 200), max(ys, 100), 0, NULL, hInst, NULL);

  if (!hWnd)
    return 0;

  ShowWindow(hWnd, SW_SHOWNORMAL);
  UpdateWindow(hWnd);
  gdioutput *gdi = new gdioutput(tag, 1.0, 0);
  gdi->setFont(gEvent->getPropertyInt("TextSize", 0),
               gEvent->getPropertyString("TextFont", L"Arial"));

  gdi->init(hWnd, hWnd, 0);
  gdi->isTestMode = gdi_main->isTestMode;
  if (gdi->isTestMode) {
    if (!gdi_main->cmdAnswers.empty()) {
      gdi->dbPushDialogAnswer(gdi_main->cmdAnswers.front());
      gdi_main->cmdAnswers.pop_front();
    }
  }
  else {
    gdi->initRecorder(&gdi_main->getRecorder());
  }
  SetWindowLong(hWnd, GWL_USERDATA, gdi_extra.size());
  currentFocusIx = gdi_extra.size();
  gdi_extra.push_back(gdi);

  return gdi;
}

/** Returns the tag of the last extra window. */
const string &getLastExtraWindow() {
  if (gdi_extra.empty())
    throw meosException("Empty");
  else
    return gdi_extra.back()->getTag();
}
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
/*
void CallBack(gdioutput *gdi, int type, void *data)
{
  ButtonInfo bi=*(ButtonInfo* )data;

  string t=gdi->getText("input");
  gdi->ClearPage();

  gdi->addButton(bi.text+" *"+t, bi.xp+5, bi.yp+30, "", CallBack);
}

void CallBackLB(gdioutput *gdi, int type, void *data)
{
  ListBoxInfo lbi=*(ListBoxInfo* )data;

  gdi->setText("input", lbi.text);
}

void CallBackINPUT(gdioutput *gdi, int type, void *data)
{
  InputInfo lbi=*(InputInfo* )data;

  MessageBox(NULL, "MB_OK", 0, MB_OK);
  //gdi->setText("input", lbi.text);
}*/

void InsertSICard(gdioutput &gdi, SICard &sic);

#define TabCtrl_InsertItemW(hwnd, iItem, pitem)   \
    (int)SNDMSG((hwnd), TCM_INSERTITEMW, (WPARAM)(int)(iItem), (LPARAM)(const TC_ITEM *)(pitem))

//static int xPos=0, yPos=0;
void createTabs(bool force, bool onlyMain, bool skipTeam, bool skipSpeaker,
                bool skipEconomy, bool skipLists, bool skipRunners, bool skipControls)
{
  static bool onlyMainP = false;
  static bool skipTeamP = false;
  static bool skipSpeakerP = false;
  static bool skipEconomyP = false;
  static bool skipListsP = false;
  static bool skipRunnersP = false;
  static bool skipControlsP = false;

  if (!force && onlyMain==onlyMainP && skipTeam==skipTeamP && skipSpeaker==skipSpeakerP &&
      skipEconomy==skipEconomyP && skipLists==skipListsP &&
      skipRunners==skipRunnersP && skipControls==skipControlsP)
    return;

  onlyMainP = onlyMain;
  skipTeamP = skipTeam;
  skipSpeakerP = skipSpeaker;
  skipEconomyP = skipEconomy;
  skipListsP = skipLists;
  skipRunnersP = skipRunners;
  skipControlsP = skipControls;

  int oldid=TabCtrl_GetCurSel(hMainTab);
  TabObject *to = 0;
  for (list<TabObject>::iterator it=tabList->begin();it!=tabList->end();++it) {
    if (it->id==oldid) {
      to = &*it;
    }
  }

  SendMessage(hMainTab, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
  int id=0;
  TabCtrl_DeleteAllItems(hMainTab);
  for (list<TabObject>::iterator it=tabList->begin();it!=tabList->end();++it) {
    it->setId(-1);

    if (onlyMain && it->getType() != typeid(TabCompetition) && it->getType() != typeid(TabSI))
      continue;

    if (skipTeam && it->getType() == typeid(TabTeam))
      continue;

    if (skipSpeaker && it->getType() == typeid(TabSpeaker))
      continue;

    if (skipEconomy && it->getType() == typeid(TabClub))
      continue;

    if (skipRunners && it->getType() == typeid(TabRunner))
      continue;

    if (skipControls && it->getType() == typeid(TabControl))
      continue;

    if (skipLists && (it->getType() == typeid(TabList) || it->getType() == typeid(TabAuto)))
      continue;

    TCITEMW ti;
    //char bf[256];
    //strcpy_s(bf, lang.tl(it->name).c_str());
    ti.pszText=(LPWSTR)lang.tl(it->name).c_str();
    ti.mask=TCIF_TEXT;
    it->setId(id++);

    TabCtrl_InsertItemW(hMainTab, it->id, &ti);
  }

  if (to && (to->id)>=0)
    TabCtrl_SetCurSel(hMainTab, to->id);
}

void hideTabs()
{
  TabCtrl_DeleteAllItems(hMainTab);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  PAINTSTRUCT ps;
  HDC hdc;

  switch (message)
  {
    case WM_CREATE:

      tabList->push_back(TabObject(gdi_main->getTabs().get(TCmpTab), "T�vling"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TRunnerTab), "Deltagare"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TTeamTab), "Lag(flera)"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TListTab), "Listor"));
      {
        TabAuto *ta = (TabAuto *)gdi_main->getTabs().get(TAutoTab);
        tabList->push_back(TabObject(ta, "Automater"));
        tabAutoRegister(ta);
      }
      tabList->push_back(TabObject(gdi_main->getTabs().get(TSpeakerTab), "Speaker"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TClassTab), "Klasser"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TCourseTab), "Banor"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TControlTab), "Kontroller"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TClubTab), "Klubbar"));

      tabList->push_back(TabObject(gdi_main->getTabs().get(TSITab), "SportIdent"));

      INITCOMMONCONTROLSEX ic;

      ic.dwSize=sizeof(ic);
      ic.dwICC=ICC_TAB_CLASSES ;
      InitCommonControlsEx(&ic);
      hMainTab=CreateWindowEx(0, WC_TABCONTROL, L"tabs", WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, 0, 0, 300, 20, hWnd, 0, hInst, 0);
      createTabs(true, true, false, false, false, false, false, false);

      SetTimer(hWnd, 4, 10000, 0); //Connection check
      break;

    case WM_MOUSEWHEEL: {
      int dz = GET_WHEEL_DELTA_WPARAM(wParam);
      scrollVertical(gdi_main, -dz, hWndWorkspace);
      }
      break;

    case WM_SIZE:
      MoveWindow(hMainTab, 0,0, LOWORD(lParam), 30, 1);
      MoveWindow(hWndWorkspace, 0, 30, LOWORD(lParam), HIWORD(lParam)-30, 1);

      RECT rc;
      GetClientRect(hWndWorkspace, &rc);
      PostMessage(hWndWorkspace, WM_SIZE, wParam, MAKELONG(rc.right, rc.bottom));
      break;

    case WM_WINDOWPOSCHANGED:
      if (gEvent) {
        LPWINDOWPOS wp = (LPWINDOWPOS) lParam; // points to size and position data

        if (wp->x>=0 && wp->y>=0 && wp->cx>300 && wp->cy>200) {
          gEvent->setProperty("xpos", wp->x);
          gEvent->setProperty("ypos", wp->y);
          gEvent->setProperty("xsize", wp->cx);
          gEvent->setProperty("ysize", wp->cy);
        }
        else {
          Sleep(0);
        }
      }
      return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_TIMER:

      if (!gdi_main) return 0;

      if (wParam==1) {
        if (autoTask)
          autoTask->autoSave();
      }
      else if (wParam==2) {
        // Interface timeouts (no synch)
        if (autoTask)
          autoTask->interfaceTimeout(gdi_extra);
      }
      else if (wParam==3) {
        if (autoTask)
          autoTask->synchronize(gdi_extra);
      }
      else if (wParam==4) {
        // Verify database link
        if (gEvent)
          gEvent->verifyConnection();
        //OutputDebugString("Verify link\n");
        //Sleep(0);
        if (gdi_main) {
          if (gEvent->hasClientChanged()) {
            gdi_main->makeEvent("Connections", "verify_connection", 0, 0, false);
            gEvent->validateClients();
          }
        }
      }
      break;

    case WM_ACTIVATE:
      if (LOWORD(wParam) != WA_INACTIVE)
        currentFocusIx = 0;
      return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NCACTIVATE:
      if (gdi_main && gdi_main->hasToolbar())
        gdi_main->activateToolbar(wParam != 0);
      return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR) lParam;

      if (pnmh->hwndFrom==hMainTab && gdi_main && gEvent)
      {
        if (pnmh->code==TCN_SELCHANGE)
        {
          int id=TabCtrl_GetCurSel(hMainTab);

          for (list<TabObject>::iterator it=tabList->begin();it!=tabList->end();++it) {
            if (it->id==id) {
              try {
                gdi_main->setWaitCursor(true);
                string cmd = "showTab("+ string(it->getTab().getTypeStr()) + "); //" + it->name;
                it->loadPage(*gdi_main);
                gdi_main->getRecorder().record(cmd);
              }
              catch (meosException &ex) {
                gdi_main->alert(ex.wwhat());
              }
              catch(std::exception &ex) {
                gdi_main->alert(ex.what());
              }
              gdi_main->setWaitCursor(false);
            }
          }
        }
        else if (pnmh->code==TCN_SELCHANGING) {
          if (gdi_main == 0) {
            MessageBeep(-1);
            return true;
          }
          else {
            if (!gdi_main->canClear())
              return true;

            return false;
          }
        }
      }
      break;
    }

    case WM_USER:
      //The card has been read and posted to a synchronized
      //queue by different thread. Read and process this card.
      {
        SICard sic;
        while (gSI && gSI->getCard(sic))
          InsertSICard(*gdi_main, sic);
        break;
      }
    case WM_USER+1:
      MessageBox(hWnd, lang.tl(L"Kommunikationen med en SI-enhet avbr�ts.").c_str(), L"SportIdent", MB_OK);
      break;

    case WM_USER + 3:
      //OutputDebugString("Get punch from queue\n");
      if (autoTask)
        autoTask->advancePunchInformation(gdi_extra);
      break;

    case WM_USER + 4:
      if (autoTask)
        autoTask->synchronize(gdi_extra);
      break;
    case WM_COMMAND:
      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      // Parse the menu selections:
      switch (wmId) {
        case IDM_EXIT:
           //DestroyWindow(hWnd);
          PostMessage(hWnd, WM_CLOSE, 0,0);
           break;
        default:
           return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      // TODO: Add any drawing code here...


      EndPaint(hWnd, &ps);
      break;

    case WM_CLOSE:
      if (!gEvent || gEvent->empty() || gdi_main->ask(L"Vill du verkligen st�nga MeOS?"))
          DestroyWindow(hWnd);
      break;

    case WM_DESTROY:
      delete gSI;
      gSI=0;

      for (size_t k = 0; k < gdi_extra.size(); k++) {
        if (gdi_extra[k])
          gdi_extra[k]->clearPage(false, false);
      }

      if (gEvent) {
        try {
          gEvent->save();
        }
        catch (meosException &ex) {
          MessageBox(hWnd, lang.tl(ex.wwhat()).c_str(), L"Fel n�r t�vlingen skulle sparas", MB_OK);
        }
        catch(std::exception &ex) {
          MessageBox(hWnd, lang.tl(ex.what()).c_str(), L"Fel n�r t�vlingen skulle sparas", MB_OK);
        }

        try {
          gEvent->saveRunnerDatabase(L"database", true);
        }
        catch (meosException &ex) {
          MessageBox(hWnd, lang.tl(ex.wwhat()).c_str(), L"Fel n�r l�pardatabas skulle sparas", MB_OK);
        }
        catch(std::exception &ex) {
          MessageBox(hWnd, lang.tl(ex.what()).c_str(), L"Fel n�r l�pardatabas skulle sparas", MB_OK);
        }

        if (gEvent)
          gEvent->saveProperties(settings);

        delete gEvent;
        gEvent=0;
      }

      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void scrollVertical(gdioutput *gdi, int yInc, HWND hWnd) {
  SCROLLINFO si;
  si.cbSize=sizeof(si);
  si.fMask=SIF_ALL;
  GetScrollInfo(hWnd, SB_VERT, &si);
  if (si.nPage==0)
    yInc = 0;

  int yPos=gdi->GetOffsetY();
  int a=si.nMax-signed(si.nPage-1) - yPos;

  if ( (yInc = max( -yPos, min(yInc, a)))!=0 ) {
    yPos += yInc;
    RECT ScrollArea, ClipArea;
    GetClientRect(hWnd, &ScrollArea);
    ClipArea=ScrollArea;

    ScrollArea.top=-gdi->getHeight()-100;
    ScrollArea.bottom+=gdi->getHeight();
    ScrollArea.right=gdi->getWidth()-gdi->GetOffsetX()+15;
    ScrollArea.left = -2000;
    gdi->SetOffsetY(yPos);

    bool inv = true; //Inv = false works only for lists etc. where there are not controls in the scroll area.

    RECT invalidArea;
    ScrollWindowEx (hWnd, 0,  -yInc,
      &ScrollArea, &ClipArea,
      (HRGN) NULL, &invalidArea, SW_SCROLLCHILDREN | (inv ? SW_INVALIDATE : 0));

   //	gdi->UpdateObjectPositions();

    si.cbSize = sizeof(si);
    si.fMask  = SIF_POS;
    si.nPos   = yPos;

    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

    if (inv)
      UpdateWindow(hWnd);
    else {
      HDC hDC = GetDC(hWnd);
      IntersectClipRect(hDC, invalidArea.left, invalidArea.top, invalidArea.right, invalidArea.bottom);
      gdi->draw(hDC, ScrollArea, invalidArea);
      ReleaseDC(hWnd, hDC);
    }
  }
}

void updateScrollInfo(HWND hWnd, gdioutput &gdi, int nHeight, int nWidth) {
  SCROLLINFO si;
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE|SIF_RANGE;

  int maxx, maxy;
  gdi.clipOffset(nWidth, nHeight, maxx, maxy);

  si.nMin=0;

  if (maxy>0) {
    si.nMax=maxy+nHeight;
    si.nPos=gdi.GetOffsetY();
    si.nPage=nHeight;
  }
  else {
    si.nMax=0;
    si.nPos=0;
    si.nPage=0;
  }
  SetScrollInfo(hWnd, SB_VERT, &si, true);

  si.nMin=0;
  if (maxx>0) {
    si.nMax=maxx+nWidth;
    si.nPos=gdi.GetOffsetX();
    si.nPage=nWidth;
  }
  else {
    si.nMax=0;
    si.nPos=0;
    si.nPage=0;
  }

  SetScrollInfo(hWnd, SB_HORZ, &si, true);
}

LRESULT CALLBACK WorkSpaceWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  PAINTSTRUCT ps;
  HDC hdc;

  LONG ix = GetWindowLong(hWnd, GWL_USERDATA);
  gdioutput *gdi = 0;
  if (ix < LONG(gdi_extra.size()))
    gdi = gdi_extra[ix];

  if (gdi) {
    LRESULT res = gdi->ProcessMsg(message, lParam, wParam);
    if (res)
      return res;
  }
  switch (message)
  {
    case WM_CREATE:
      break;

    case WM_SIZE:
      //SCROLLINFO si;
      //si.cbSize=sizeof(si);
      //si.fMask=SIF_PAGE|SIF_RANGE;

      int nHeight;
      nHeight = HIWORD(lParam);
      int nWidth;
      nWidth = LOWORD(lParam);
      updateScrollInfo(hWnd, *gdi, nHeight, nWidth);
      /*

      int maxx, maxy;
      gdi->clipOffset(nWidth, nHeight, maxx, maxy);

      si.nMin=0;

      if (maxy>0) {
        si.nMax=maxy+nHeight;
        si.nPos=gdi->GetOffsetY();
        si.nPage=nHeight;
      }
      else {
        si.nMax=0;
        si.nPos=0;
        si.nPage=0;
      }
      SetScrollInfo(hWnd, SB_VERT, &si, true);

      si.nMin=0;
      if (maxx>0) {
        si.nMax=maxx+nWidth;
        si.nPos=gdi->GetOffsetX();
        si.nPage=nWidth;
      }
      else {
        si.nMax=0;
        si.nPos=0;
        si.nPage=0;
      }
      SetScrollInfo(hWnd, SB_HORZ, &si, true);
      */
      InvalidateRect(hWnd, NULL, true);
      break;
    case WM_KEYDOWN:
      //gdi->keyCommand(;
      break;

    case WM_VSCROLL:
    {
      int	nScrollCode = (int) LOWORD(wParam); // scroll bar value
      //int hwndScrollBar = (HWND) lParam;      // handle to scroll bar

      int yInc;
      int yPos=gdi->GetOffsetY();
      RECT rc;
      GetClientRect(hWnd, &rc);
      int pagestep = max(50, int(0.9*rc.bottom));

      switch(nScrollCode)
      {
        // User clicked shaft left of the scroll box.
        case SB_PAGEUP:
           yInc = -pagestep;
           break;

        // User clicked shaft right of the scroll box.
        case SB_PAGEDOWN:
           yInc = pagestep;
           break;

        // User clicked the left arrow.
        case SB_LINEUP:
           yInc = -10;
           break;

        // User clicked the right arrow.
        case SB_LINEDOWN:
           yInc = 10;
           break;

        // User dragged the scroll box.
        case SB_THUMBTRACK: {
            // Initialize SCROLLINFO structure
            SCROLLINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;

            if (!GetScrollInfo(hWnd, SB_VERT, &si) )
                return 1; // GetScrollInfo failed

            yInc = si.nTrackPos - yPos;
          break;
        }

        default:
        yInc = 0;
      }

      scrollVertical(gdi, yInc, hWnd);
      gdi->storeAutoPos(gdi->GetOffsetY());
      break;
    }

    case WM_HSCROLL:
    {
      int	nScrollCode = (int) LOWORD(wParam); // scroll bar value
      //int hwndScrollBar = (HWND) lParam;      // handle to scroll bar

      int xInc;
      int xPos=gdi->GetOffsetX();

      switch(nScrollCode)
      {
        case SB_ENDSCROLL:
          InvalidateRect(hWnd, 0, false);
          return 0;
        
        // User clicked shaft left of the scroll box.
        case SB_PAGEUP:
           xInc = -80;
           break;

        // User clicked shaft right of the scroll box.
        case SB_PAGEDOWN:
           xInc = 80;
           break;

        // User clicked the left arrow.
        case SB_LINEUP:
           xInc = -10;
           break;

        // User clicked the right arrow.
        case SB_LINEDOWN:
           xInc = 10;
           break;

        // User dragged the scroll box.
        case SB_THUMBTRACK:  {
            // Initialize SCROLLINFO structure
            SCROLLINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;

            if (!GetScrollInfo(hWnd, SB_HORZ, &si) )
                return 1; // GetScrollInfo failed

            xInc = si.nTrackPos - xPos;
          break;
        }
          //xInc = HIWORD(wParam) - xPos;
          //break;
        default:
          xInc = 0;
      }

      SCROLLINFO si;
      si.cbSize=sizeof(si);
      si.fMask=SIF_ALL;
      GetScrollInfo(hWnd, SB_HORZ, &si);

      if (si.nPage==0)
        xInc = 0;

      int a=si.nMax-signed(si.nPage-1) - xPos;

      if ((xInc = max( -xPos, min(xInc, a)))!=0) {
        xPos += xInc;
        RECT ScrollArea, ClipArea;
        GetClientRect(hWnd, &ScrollArea);
        ClipArea=ScrollArea;

        gdi->SetOffsetX(xPos);

        ScrollWindowEx (hWnd, -xInc,  0,
          0, &ClipArea,
          (HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE|SW_SCROLLCHILDREN);

        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS;
        si.nPos   = xPos;

        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
        UpdateWindow (hWnd);
      }
      break;
    }

    case WM_MOUSEWHEEL: {
      int dz = GET_WHEEL_DELTA_WPARAM(wParam);
      scrollVertical(gdi, -dz, hWnd);
      gdi->storeAutoPos(gdi->GetOffsetY());
      }
      break;

    case WM_TIMER:
      if (wParam == 1001) {
        double autoScroll, pos;
        gdi->getAutoScroll(autoScroll, pos);

        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;

        GetScrollInfo(hWnd, SB_VERT, &si);
        int dir = gdi->getAutoScrollDir();
        int dy = 0;
        if ((dir<0 && si.nPos <= si.nMin) ||
            (dir>0 && (si.nPos + int(si.nPage)) >= si.nMax)) {
          autoScroll = -autoScroll;
          gdi->setAutoScroll(-1); // Mirror

          double nextPos = pos + autoScroll;
          dy = int(nextPos - si.nPos);
          gdi->storeAutoPos(nextPos);

          //gdi->setData("AutoScroll", -int(data));
        }
        else {
          double nextPos = pos + autoScroll;
          dy = int(nextPos - si.nPos);
          gdi->storeAutoPos(nextPos);
          //gdi->setData("Discrete", DWORD(nextPos*1e3));
        }

        scrollVertical(gdi, dy, hWnd);
      }
      else
        MessageBox(hWnd, L"Runtime exception", 0, MB_OK);
      break;

    case WM_ACTIVATE: {
      int fActive = LOWORD(wParam);
      if (fActive != WA_INACTIVE)
        currentFocusIx = ix;

      return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_USER + 2:
      if (gdi)
       LoadPage(*gdi, TabType(wParam));
      break;

    case WM_COMMAND:
      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      // Parse the menu selections:
      switch (wmId)
      {
      case 0: break;
        default:
           return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;

    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      RECT rt;
      GetClientRect(hWnd, &rt);

      if (gdi && (ps.rcPaint.right|ps.rcPaint.left|ps.rcPaint.top|ps.rcPaint.bottom) != 0 )
        gdi->draw(hdc, rt, ps.rcPaint);
      /*{
      HANDLE icon = LoadImage(hInst, (LPCTSTR)IDI_MEOS, IMAGE_ICON, 64, 64, LR_SHARED);
      DrawIconEx(hdc, 0,0, (HICON)icon, 64, 64, 0, NULL, DI_NORMAL | DI_COMPAT);
      }*/
      EndPaint(hWnd, &ps);
      break;

    case WM_ERASEBKGND:
      return 0;
      break;

    case WM_DESTROY:
      if (ix > 0) {
        gdi->makeEvent("CloseWindow", "meos", 0, 0, false);
        gdi_extra[ix] = 0;
        delete gdi;

        while(!gdi_extra.empty() && gdi_extra.back() == 0)
          gdi_extra.pop_back();
      }
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
      {
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;
      }
      break;
  }
    return FALSE;
}


namespace setup {
const int nFiles=7;
const wchar_t *fileList[nFiles]={L"baseclass.xml",
                                 L"wfamily.mwd",
                                 L"wgiven.mwd",
                                 L"wclub.mwd",
                                 L"wclass.mwd",
                                 L"database.wclubs",
                                 L"database.wpersons"};
}

void Setup(bool overwrite, bool overwriteAll)
{
  static bool isSetup=false;
  if (isSetup && overwrite==false)
    return;
  isSetup=true; //Run at most once.

  vector<pair<wstring, bool> > toInstall;
  for(int k=0;k<setup::nFiles;k++) {
    toInstall.push_back(make_pair(wstring(setup::fileList[k]), overwriteAll));
  }

  wchar_t dir[260];
  GetCurrentDirectory(260, dir);
  vector<wstring> dyn;
  expandDirectory(dir, L"*.lxml", dyn);
  expandDirectory(dir, L"*.listdef", dyn);
  expandDirectory(dir, L"*.meos", dyn);
  for (size_t k = 0; k < dyn.size(); k++)
    toInstall.push_back(make_pair(dyn[k], true));

  wchar_t bf[260];
  for(size_t k=0; k<toInstall.size(); k++) {
    const wstring src = toInstall[k].first;
    wchar_t filename[128];
    wchar_t ext[32];
    _wsplitpath_s(src.c_str(), NULL, 0, NULL,0, filename, 128, ext, 32);
    wstring fullFile = wstring(filename) + ext;
    
    getUserFile(bf, fullFile.c_str());
    bool canOverwrite = overwrite && toInstall[k].second;
    CopyFile(toInstall[k].first.c_str(), bf, !canOverwrite);
  }
}

void exportSetup()
{
  wchar_t bf[260];
  for(int k=0;k<setup::nFiles;k++) {
    getUserFile(bf, setup::fileList[k]);
    CopyFile(bf, setup::fileList[k], false);
  }
}

bool getMeOSFile(wchar_t *FileNamePath, const wchar_t *FileName) {
  wchar_t Path[MAX_PATH];

  wcscpy_s(Path, programPath);
  int i=wcslen(Path);
  if (Path[i-1]!='\\')
    wcscat_s(Path, MAX_PATH, L"\\");

  wcscat_s(Path, FileName);
  wcscpy_s(FileNamePath, MAX_PATH, Path);
  return true;
}


bool getUserFile(wchar_t *FileNamePath, const wchar_t *FileName)
{
  wchar_t Path[MAX_PATH];
  wchar_t AppPath[MAX_PATH];

  if (SHGetSpecialFolderPath(hWndMain, Path, CSIDL_APPDATA, 1)!=NOERROR) {
    int i=wcslen(Path);
    if (Path[i-1]!='\\')
      wcscat_s(Path, MAX_PATH, L"\\");

    wcscpy_s(AppPath, MAX_PATH, Path);
    wcscat_s(AppPath, MAX_PATH, L"Meos\\");

    CreateDirectory(AppPath, NULL);

    Setup(false, false);

    wcscpy_s(FileNamePath, MAX_PATH, AppPath);
    wcscat_s(FileNamePath, MAX_PATH, FileName);

    //return true;
  }
  else wcscpy_s(FileNamePath, MAX_PATH, FileName);

  return true;
}


bool getDesktopFile(wchar_t *fileNamePath, const wchar_t *fileName, const wchar_t *subFolder)
{
  wchar_t Path[MAX_PATH];
  wchar_t AppPath[MAX_PATH];

  if (SHGetSpecialFolderPath(hWndMain, Path, CSIDL_DESKTOPDIRECTORY, 1)!=NOERROR) {
    int i=wcslen(Path);
    if (Path[i-1]!='\\')
      wcscat_s(Path, MAX_PATH, L"\\");

    wcscpy_s(AppPath, MAX_PATH, Path);
    wcscat_s(AppPath, MAX_PATH, L"Meos\\");

    CreateDirectory(AppPath, NULL);

    if (subFolder) {
      wcscat_s(AppPath, MAX_PATH, subFolder);
      wcscat_s(AppPath, MAX_PATH, L"\\");
      CreateDirectory(AppPath, NULL);
    }

    wcscpy_s(fileNamePath, MAX_PATH, AppPath);
    wcscat_s(fileNamePath, MAX_PATH, fileName);
  }
  else wcscpy_s(fileNamePath, MAX_PATH, fileName);

  return true;
}

static set<wstring> tempFiles;
static wstring tempPath;

wstring getTempPath() {
  wchar_t tempFile[MAX_PATH];
  if (tempPath.empty()) {
    wchar_t path[MAX_PATH];
    GetTempPath(MAX_PATH, path);
    GetTempFileName(path, L"meos", 0, tempFile);
    DeleteFile(tempFile);
    if (CreateDirectory(tempFile, NULL))
      tempPath = tempFile;
    else
      throw std::exception("Failed to create temporary file.");
  }
  return tempPath;
}

void registerTempFile(const wstring &tempFile) {
  tempFiles.insert(tempFile);
}

wstring getTempFile() {
  getTempPath();

  wchar_t tempFile[MAX_PATH];
  if (GetTempFileName(tempPath.c_str(), L"ix", 0, tempFile)) {
    tempFiles.insert(tempFile);
    return tempFile;
  }
  else
    throw std::exception("Failed to create temporary file.");
}

void removeTempFile(const wstring &file) {
  DeleteFile(file.c_str());
  tempFiles.erase(file);
}

void removeTempFiles() {
  vector<wstring> dir;
  for (set<wstring>::iterator it = tempFiles.begin(); it!= tempFiles.end(); ++it) {
    wchar_t c = *it->rbegin();
    if (c == '/' || c == '\\')
      dir.push_back(*it);
    else
      DeleteFile(it->c_str());
  }
  tempFiles.clear();
  bool removed = true;
  while (removed) {
    removed = false;
    for (size_t k = 0; k<dir.size(); k++) {
      if (!dir[k].empty() && RemoveDirectory(dir[k].c_str()) != 0) {
        removed = true;
        dir[k].clear();
      }
    }
  }

  if (!tempPath.empty()) {
    RemoveDirectory(tempPath.c_str());
    tempPath.clear();
  }
}
