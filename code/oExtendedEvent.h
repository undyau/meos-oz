#pragma once
#include "oevent.h"

class oExtendedEvent :
  public oEvent
{
public:
  oExtendedEvent(gdioutput &gdi);
  ~oExtendedEvent(void);


	bool SSSQuickStart(gdioutput &gdi);
	void exportCourseOrderedIOFSplits(IOFVersion version, const char *file, bool oldStylePatrolExport, const set<int> &classes, int leg);
	void uploadSss(gdioutput &gdi);
	virtual void writeExtraXml(xmlparser &xml);
	virtual void readExtraXml(const xmlparser &xml);
	int getSssEventNum() {return SssEventNum;};
  string getSssSeriesPrefix() {return SssSeriesPrefix;};
	int getIsSydneySummerSeries() {return IsSydneySummerSeries;};
	bool exportOrCSV(const char *file, bool byClass);
	bool isRentedCard(int card);
	void loadRentedCardNumbers();

private:
	string loadCsvToString(string file);
	string string_replace(string src, string const& target, string const& repl);
	bool IsSydneySummerSeries;
	int SssEventNum;
  string SssSeriesPrefix;
	std::vector<int> RentedCards;
	bool LoadedCards;
};
