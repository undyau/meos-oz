#include "stdafx.h"
#include "oSSSQuickStart.h"
#include "Download.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "meos_util.h"
#include "metalist.h"
#include <ctime>
#include "MeOSFeatures.h"

oSSSQuickStart::oSSSQuickStart(oExtendedEvent& a_Event):m_Event(a_Event)
{
}

oSSSQuickStart::~oSSSQuickStart(void)
{
}

bool oSSSQuickStart::ConfigureEvent(gdioutput &gdi)
{
// retrieve competition from install or web
wstring file = getTempFile();
if (!GetEventTemplateFromInstall(file))
	if (!GetEventTemplateFromWeb(file))
		return false;

// If the competition already exists (say from Eventor) then just add course, controls etc
if (!m_Event.empty())
  {
  // Make sure that we have courses, controls and class->course mapping
  m_Event.getMeOSFeatures().useFeature(MeOSFeatures::Rogaining, true, m_Event);
  std::vector<pControl> controls;
  m_Event.getControls(controls, false);
  if (controls.size() == 0)
    if (!LoadControlsFromFile(file))
      {
      removeTempFile(file);
      return false;
      }

  std::vector<pCourse> courses;
  m_Event.getCourses(courses);
  if (courses.size() == 0)
    if (!LoadCoursesFromFile(file))
      {
      removeTempFile(file);
      return false;
      }

  vector<pClass> allCls;
  m_Event.getClasses(allCls, false);
  if (allCls.size() == 0)
    if (!LoadClassesFromFile(file))
      {
      removeTempFile(file);
      return false;
      }

  m_Event.getClasses(allCls, false);
  m_Event.getCourses(courses);
  for (size_t k = 0; k < allCls.size(); k++) 
    {
    if (courses.size() && allCls[k]->getCourse(false) == 0)
      allCls[k]->setCourse(courses.at(0));
    allCls[k]->setAllowQuickEntry(true);
    }

  }
else
  {
  gdi.setWaitCursor(true);
  if(m_Event.open(file, true)) 
	  {
	  m_Event.updateTabs();
	  gdi.setWindowTitle(m_Event.getTitleName());
    removeTempFile(file);
	  }
  else
	  {
	  removeTempFile(file);
	  return false;
	  }
  }

SYSTEMTIME st;
GetSystemTime(&st);

m_Event.setDate(convertSystemDate(st));
CustomiseClasses();

AddMeosOzCustomList(wstring(L"SSS Receipt Results.xml"));
AddMeosOzCustomList(wstring(L"SSS Results.xml"));

return true;
}

void oSSSQuickStart::AddMeosOzCustomList(wstring a_ReportDef)
{
	wchar_t path[MAX_PATH];
	if (getUserFile(path, a_ReportDef.c_str()))
		{
		wstring file(path);
		if (!fileExist(path))
			{
			wchar_t exepath[MAX_PATH];
			if (GetModuleFileName(NULL, exepath, MAX_PATH))
				{
				for (int i = wcslen(exepath) - 1; i > 1; i--)
					if (exepath[i-1] == '\\')
						{
						exepath[i] = '\0';
						break;
						}
				wcscat(exepath,a_ReportDef.c_str());
				if (fileExist(exepath))
					CopyFile(exepath, path, true);
				}
			}
		}
	if (fileExist(path))
		{
		xmlparser xml;
		xml.read(path);
		xmlobject xlist = xml.getObject(0);

// Check that we don't have the list already
// Do nothing if we have that list already
		wstring listName;
		xlist.getObjectString("ListName", listName);
		MetaListContainer &lc = m_Event.getListContainer();
		if (lc.getNumLists(MetaListContainer::ExternalList) > 0) 
		for (int k = 0; k < lc.getNumLists(); k++) 
			{
			if (lc.isExternal(k)) 
				{
				MetaList &mc = lc.getList(k);
				if (mc.getListName() == listName)
					return;
				}
			}

		m_Event.synchronize();
		m_Event.getListContainer().load(MetaListContainer::ExternalList, xlist, false);
		m_Event.synchronize(true);
		}
}

bool oSSSQuickStart::GetEventTemplateFromInstall(wstring& a_File)
{
	  TCHAR ownPth[MAX_PATH]; 

    // Will contain exe path
    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule != NULL) {
      GetModuleFileName(hModule,ownPth, (sizeof(ownPth)));
			int pos = wcslen(ownPth) - 1;
			while (ownPth[pos] != '\\' && pos > 0)
				--pos;
			if (pos == 0)
				return false;
			ownPth[pos] = '\0';

			wstring templateFile(ownPth);
			templateFile += L"\\sss201230.xml";
			if (!fileExist(templateFile.c_str()))
				return false;
			else
				return !!CopyFile(templateFile.c_str(), a_File.c_str(), FALSE);
		}
		else
			return false;
}

bool oSSSQuickStart::GetEventTemplateFromWeb(wstring& a_File)
{
  wstring url = L"http://sportident.itsdamp.com/sss201230.xml";

  Download dwl;
  dwl.initInternet();
  std::vector<pair<wstring,wstring>> headers;

  try {
    dwl.downloadFile(url, a_File, headers);
  }
  catch (std::exception &) {
    removeTempFile(a_File);
    throw;
  }

  dwl.createDownloadThread();
  while (dwl.isWorking()) {
    Sleep(100);
  }

  return true;
}

void oSSSQuickStart::CustomiseClasses()
{
	// Set age/gender details for each class
  oClassList::iterator it;
  for (it=m_Event.Classes.begin();it!=m_Event.Classes.end();++it) 
		{
		if (it->getName().size() < 4)
			{
			wstring gender = it->getName().substr(it->getName().size()-1,1);
			if (gender == L"W" || gender == L"M")
					it->setSex(gender == L"W" ? sFemale : sMale);

			int s(0);
			time_t t = time(0);   // get time now
			struct tm * now = localtime( & t );

			if (now->tm_mon + 1 <=6)
				s = 1; // for second half of season split over a year boundary, use last year's age
			if (it->getName().size() == 2)
				{
				switch (it->getName().at(0))
					{
					case 'J' : it->setAgeLimit(0 + s,20 + s); break;
					case 'O' : it->setAgeLimit(21 + s,34 + s); break;
					case 'M' : it->setAgeLimit(35 + s, 44 + s); break;
					case 'V' : it->setAgeLimit(45 + s, 54 +s); break;
					case 'L' : it->setAgeLimit(65 + s, 74 + s); break;
					case 'I' : it->setAgeLimit(74 + s, 200 + s);  break;
					}
				}
			else if (it->getName().substr(0,2) == L"SV")
				it->setAgeLimit(55 + s,64 + s);
			}
		}
}


bool oSSSQuickStart::LoadCoursesFromFile(wstring file)
{
  xmlparser xml;
  xml.read(file);
  xmlobject xo;

  //Get courses
  xo=xml.getObject("CourseList");
  if (xo){
    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;
    set<int> knownCourse;
    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Course")){
        oCourse c(&m_Event);
        c.Set(&*it);
        if (c.getId()>0 && knownCourse.count(c.getId()) == 0) {
          m_Event.addCourse(c);
          knownCourse.insert(c.getId());
        }
      }
    }
  }
  else
    return false;

  return true;
}

bool oSSSQuickStart::LoadControlsFromFile(wstring file)
{
  xmlparser xml;
  xml.read(file);
  xmlobject xo;

  xo = xml.getObject("ControlList");
  if (xo){
    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;
    set<int> knownControls;
    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Control")){
        oControl c(&m_Event);
        c.set(&*it);

        if (c.getId()>0 && knownControls.count(c.getId()) == 0) {
          m_Event.Controls.push_back(c);
          knownControls.insert(c.getId());
        }
      }
    }
  }
  else
    return false;

  return true;
}

bool oSSSQuickStart::LoadClassesFromFile(wstring file)
{
  xmlparser xml;
  xml.read(file);
  xmlobject xo;

  xo = xml.getObject("ClassList");
  if (xo){
    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;
    set<int> knownClass;
    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Class")){
        oClass c(&m_Event);
        c.Set(&*it);
        if (c.getId()>0 && knownClass.count(c.getId()) == 0) {
          m_Event.Classes.push_back(c);
          knownClass.insert(c.getId());
        }
      }
    }
  }
  else
    return false;

  return true;
}