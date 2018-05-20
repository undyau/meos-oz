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
	void uploadSss(gdioutput &gdi);
	virtual void writeExtraXml(xmlparser &xml);
	virtual void readExtraXml(const xmlparser &xml);
	int getSssEventNum() {return SssEventNum;};
  wstring getSssSeriesPrefix() {return SssSeriesPrefix;};
	int getIsSydneySummerSeries() {return IsSydneySummerSeries;};
	bool exportOrCSV(const wchar_t  *file, bool byClass);
	bool isRentedCard(int card);
	void loadRentedCardNumbers();

private:
	wstring loadCsvToString(wstring file);
	wstring string_replace(wstring src, wstring const& target, wstring const& repl);
	bool IsSydneySummerSeries;
	int SssEventNum;
  wstring SssSeriesPrefix;
	std::vector<int> RentedCards;
	bool LoadedCards;
};
