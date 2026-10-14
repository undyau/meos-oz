#pragma once
#include "oevent.h"

class oExtendedEvent :
  public oEvent
{
public:
  oExtendedEvent(gdioutput &gdi);
  ~oExtendedEvent(void);


  bool SSSQuickStart(gdioutput &gdi);
  virtual void writeExtraXml(xmlparser &xml);
  virtual void readExtraXml(const xmlparser &xml);
  int getIsSydneySummerSeries() {return IsSydneySummerSeries;};
  bool preserveExistingRunnersAsIs(bool preserve);
  bool addXmlRunner(gdioutput & gdi, xmlobject & xo);
  bool getPreserveExistingRunnersAsIs() { return PreserveExistingRunnersAsIs; }
  void importXML_SeasonTickets(gdioutput &gdi, const wstring &competitorFile, const wstring& classesFile);
  wstring NormaliseClassName(wstring name);
  wstring GetShortSssName(wstring& className);
  void AddClassNameNormalisation(wstring oldName, wstring newName);

private:
  bool IsSydneySummerSeries;
  bool PreserveExistingRunnersAsIs;
  std::map<wstring, wstring> RenamedClasses; // Use on import so we can rename from long class names to short ones
};
