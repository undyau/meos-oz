#pragma once
#include "oevent.h"

class oExtendedEvent :
  public oEvent
{
public:
  oExtendedEvent(gdioutput &gdi);
  ~oExtendedEvent(void);


  bool SSSQuickStart(gdioutput &gdi);
  void exportCourseOrderedIOFSplits(IOFVersion version, const wchar_t *file, bool oldStylePatrolExport, const set<int> &classes, int leg);
  void uploadSss(gdioutput &gdi, bool automate);
  void uploadSssUnattended();
  int incUploadCounter();
  void prepData4SssUpload(wstring& data);
  virtual void writeExtraXml(xmlparser &xml);
  virtual void readExtraXml(const xmlparser &xml);
  int getSssEventNum() {return SssEventNum;};
  wstring getSssAltName() { return SssAltName; }
  wstring getSssSeriesPrefix() {return SssSeriesPrefix;};
  int getIsSydneySummerSeries() {return IsSydneySummerSeries;};
  bool exportOrCSV(const wchar_t  *file, bool byClass);
  bool getAutoUploadSss();
  time_t getLastSssUploadTime() { return LastAutoUploadSssTime; }
  bool setAutoUploadSss(bool automatic);
  void checkForPeriodicEvents();
  void loadHireCards();
  bool preserveExistingRunnersAsIs(bool preserve);
  bool addXmlRunner(gdioutput & gdi, xmlobject & xo);
  bool getPreserveExistingRunnersAsIs() { return PreserveExistingRunnersAsIs; }
  void importXML_SeasonTickets(gdioutput &gdi, const wstring &competitorFile, const wstring& classesFile);
  wstring NormaliseClassName(wstring name);
  void AddClassNameNormalisation(wstring oldName, wstring newName);

private:
  wstring loadCsvToString(wstring file);
  wstring string_replace(wstring src, wstring const& target, wstring const& repl);
  bool IsSydneySummerSeries;
  int SssEventNum;
  wstring SssSeriesPrefix;
  wstring SssAltName;
  bool AutoUploadSss;
  bool PreserveExistingRunnersAsIs;
  time_t LastAutoUploadSssTime;
  time_t AutoUploadSssInterval;
  std::map<wstring, wstring> RenamedClasses; // Use on import so we can rename from long class names to short ones
};
