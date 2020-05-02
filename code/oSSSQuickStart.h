#pragma once
#include "oExtendedEvent.h"
#pragma warning (disable: 4512)
class oSSSQuickStart
{
public:
  oSSSQuickStart(oExtendedEvent& a_Event);
  bool ConfigureEvent(gdioutput &gdi);
	int ImportCount() {return m_ImportCount;};
  ~oSSSQuickStart(void);

private:
  oExtendedEvent& m_Event;
  bool GetEventTemplateFromWeb(wstring& a_File);
	bool GetEventTemplateFromInstall(wstring& a_File);
	void AddMeosOzCustomList(wstring a_ReportDef);
	void CustomiseClasses(); // Set age limits, gender for each class
  bool LoadCoursesFromFile(wstring file);
  bool LoadControlsFromFile(wstring file);
  bool LoadClassesFromFile(wstring file);
	int m_ImportCount;
};
#pragma warning (default: 4512)
