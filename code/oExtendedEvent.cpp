#include "stdafx.h"
#include "oExtendedEvent.h"
#include "oSSSQuickStart.h"
#include "gdioutput.h"
#include "meos_util.h"
#include "localizer.h"
#include "gdifonts.h"
#include "Download.h"
#include "progress.h"
#include "csvparser.h"


oExtendedEvent::oExtendedEvent(gdioutput &gdi) : oEvent(gdi)
{
	IsSydneySummerSeries = 0;
  SssEventNum = 0;
  SssSeriesPrefix = L"sss";
	LoadedCards = false;
}

oExtendedEvent::~oExtendedEvent(void)
{
}

bool oExtendedEvent::SSSQuickStart(gdioutput &gdi)
{
  oSSSQuickStart qs(*this);
	if (qs.ConfigureEvent(gdi)){
		IsSydneySummerSeries = true;
		return true;
	}
	return false;
}

void oExtendedEvent::exportCourseOrderedIOFSplits(IOFVersion version, const wchar_t *file, bool oldStylePatrolExport, const set<int> &classes, int leg)
{
	// Create new classes names after courses
	std::map<int, int> courseNewClassXref;
	std::map<int, int> runnerOldClassXref;
	std::set<std::wstring> usedNames;

	for (oClassList::iterator j = Classes.begin(); j != Classes.end(); j++) {
		usedNames.insert(j->getName());
	}

	for (oCourseList::iterator j = Courses.begin(); j != Courses.end(); j++) {			
		std::wstring name = j->getName();
		if (usedNames.find(name) != usedNames.end()) {
				name = lang.tl("Course ") + name;
			}
		while (usedNames.find(name) != usedNames.end()) {
				name = name + L"_";
			}

		usedNames.insert(name);
		pClass newClass = addClass(name, j->getId());
		courseNewClassXref[j->getId()] = newClass->getId();
	}
	
	// Reassign all runners to new classes, saving old ones
	for (oRunnerList::iterator j = Runners.begin(); j != Runners.end(); j++) {
		runnerOldClassXref[j->getId()] = j->getClassId(false);	
		int id = j->getCourse(false)->getId();
		j->setClassId(courseNewClassXref[id], true);
	}

	// Do the export
	oEvent::exportIOFSplits(version, file, oldStylePatrolExport, /*USe UTC*/false, classes, leg, true, true, true, false);

	// Reassign all runners back to original classes
	for (oRunnerList::iterator j = Runners.begin(); j != Runners.end(); j++) {
		j->setClassId(runnerOldClassXref[j->getId()], true);
	}

	// Delete temporary classes
	for (std::map<int, int>::iterator j = courseNewClassXref.begin(); j != courseNewClassXref.end(); j++) {
		removeClass(j->second);
	}
}

void oEvent::calculateCourseRogainingResults()
{
	sortRunners(CoursePoints);
	oRunnerList::iterator it;

	int cCourseId=-1;
  int cPlace = 0;
	int vPlace = 0;
	int cTime = numeric_limits<int>::min();;
  int cDuplicateLeg=0;
  bool useResults = false;
  bool isRogaining = false;
  bool invalidClass = false;
	int cCourseOfClassId=-1;

	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;

    if ((it->getCourseId()!=cCourseId && (cCourseOfClassId <= 0 || it->getCourseId()!=cCourseOfClassId)) || 
			it->tDuplicateLeg!=cDuplicateLeg) {
			cCourseId = it->getCourseId();
      useResults = it->Class ? !it->Class->getNoTiming() : false;
			cPlace = 0;
			vPlace = 0;
			cTime = numeric_limits<int>::min();
      cDuplicateLeg = it->tDuplicateLeg;
      isRogaining = it->Class ? it->Class->isRogaining() : false;
      invalidClass = it->Class ? it->Class->getClassStatus() != oClass::Normal : false;
		}

		if (cCourseId == 0 && it->Class->getCourseId() > 0)
			cCourseOfClassId = it->Class->getCourseId();
		else
			cCourseOfClassId = -1;
	
    if (!isRogaining)
      continue;

    if (invalidClass) {
      it->tTotalPlace = 0;
      it->tPlace = 0;
    }
    else if(it->status==StatusOK) {
			cPlace++;

      int cmpRes = 3600 * 24 * 7 * it->tRogainingPoints - it->getRunningTime();

      if(cmpRes != cTime)
				vPlace = cPlace;

			cTime = cmpRes;

      if (useResults)
			  it->tPlace = vPlace;
      else
        it->tPlace = 0;
		}
		else
			it->tPlace = 99000 + it->status;
	}
}


void oExtendedEvent::uploadSss(gdioutput &gdi)
{
	static int s_counter(0);
	wstring url = gdi.getText("SssServer");
	SssEventNum = gdi.getTextNo("SssEventNum", false);
  SssSeriesPrefix = gdi.getText("SssSeriesPrefix",false);
	if (SssEventNum == 0) {
		gdi.alert(L"Invalid event number :" + gdi.getText("SssEventNum"));
		return;
		}

	wstring resultCsv = getTempFile();
	ProgressWindow pw(gdibase.getHWNDTarget());
	exportOrCSV(resultCsv.c_str(), false);
	wstring data = (loadCsvToString(resultCsv));
	data = string_replace(data, L"&",L"and");
	data = L"Name=" + SssSeriesPrefix + to_wstring(SssEventNum) + L"&Title=" + Name + L"&Subtitle=" + SssSeriesPrefix + to_wstring(SssEventNum) + L"&Data=" + data + L"&Serial=" + to_wstring(s_counter++);
	Download dwl;
  dwl.initInternet();
	string result;
  try {
		dwl.postData(url, data, pw);
  }
  catch (std::exception &) {
    removeTempFile(resultCsv);
    throw;
  }

  dwl.createDownloadThread();
  while (dwl.isWorking()) {
    Sleep(100);
	}
	setProperty("SssServer",url);
	gdi.alert(L"Completed upload of results to " + url);
}

void oExtendedEvent::writeExtraXml(xmlparser &xml)
{
	xml.write("IsSydneySummerSeries", IsSydneySummerSeries);
	xml.write("SssEventNum", SssEventNum);
  xml.write("SssSeriesPrefix", SssSeriesPrefix);
}

void oExtendedEvent::readExtraXml(const xmlparser &xml)
{
 	xmlobject xo;
 
	xo=xml.getObject("IsSydneySummerSeries");
	if(xo) IsSydneySummerSeries=!!xo.getInt();

	xo=xml.getObject("SssEventNum");
	if(xo) SssEventNum=xo.getInt();

	xo=xml.getObject("SssSeriesPrefix");
	if(xo) SssSeriesPrefix=xo.getw();
}

wstring oExtendedEvent::loadCsvToString(wstring file)
{
	wstring result;
	std::wifstream fin;
	fin.open(file.c_str());

	if(!fin.good())
		return wstring(L"");

	wchar_t bf[1024];
	while (!fin.eof()) {	
		fin.getline(bf, 1024);
		wstring temp(bf);
		result += temp + L"\n";
	}
	
	return result.substr(0, result.size()-1);
}

wstring oExtendedEvent::string_replace(wstring src, wstring const& target, wstring const& repl)
{
    // handle error situations/trivial cases

    if (target.length() == 0) {
        // searching for a match to the empty string will result in 
        //  an infinite loop
        //  it might make sense to throw an exception for this case
        return src;
    }

    if (src.length() == 0) {
        return src;  // nothing to match against
    }

    size_t idx = 0;

    for (;;) {
        idx = src.find( target, idx);
        if (idx == string::npos)  break;

        src.replace( idx, target.length(), repl);
        idx += repl.length();
    }

    return src;
}

int MyConvertStatusToOE(int i)
{
	switch(i)
	{
	    case StatusOK:
			return 0;	    	
	    case StatusDNS:  // Ej start
			return 1;	    	
	    case StatusDNF:  // Utg.
			return 2;	    	
	    case StatusMP:  // Felst.
			return 3;
	    case StatusDQ: //Disk
			return 4;  	    	
	    case StatusMAX: //Maxtid 
			return 5;
	} 
	return 1;//Ej start...?!
}

string my_conv_is(int i)
{
	char bf[256];
	 if(_itoa_s(i, bf, 10)==0)
		return bf;
	 return "";
}

string formatOeCsvTime(int rt)
{
  if(rt>0 && rt<3600*48) {
		char bf[16];
		sprintf_s(bf, 16, "%02d:%02d", (rt/60), rt%60);
		return bf;
	}
	return "-";
}

string ws2s(wstring ws)
	{
	string s;
	wide2String(ws, s);
	return s;
	}


bool oExtendedEvent::exportOrCSV(const wchar_t *file, bool byClass)
{
	csvparser csv;

	if(!csv.openOutput(file))
		return false;
	
	if (byClass)
		calculateResults(RTClassResult);
	else
		calculateResults(RTCourseResult);

	if (IsSydneySummerSeries)
		calculateCourseRogainingResults();

	oRunnerList::iterator it;

	string s;
	
	csv.outputRow(ws2s(lang.tl("Startnr;Bricka;Databas nr.;Efternamn;Förnamn;År;K;Block;ut;Start;Mål;Tid;Status;Klubb nr.;Namn;Ort;Land;Klass nr.;Kort;Lång;Num1;Num2;Num3;Text1;Text2;Text3;Adr. namn;Gata;Rad 2;Post nr.;Ort;Tel;Fax;E-post;Id/Club;Hyrd;Startavgift;Betalt;Bana nr.;Bana;km;Hm;Bana kontroller;Pl")));
	
	char bf[256];
	for(it=Runners.begin(); it != Runners.end(); ++it){	
		vector<string> row;
		row.resize(200);
		oDataInterface di=it->getDI();

		row[0]=my_conv_is(it->getId());
		row[1]=my_conv_is(it->getCardNo());
    row[2]=my_conv_is(int(it->getExtIdentifier()));

		row[3]=ws2s(it->getFamilyName());
		row[4]=ws2s(it->getGivenName());
		row[5]=my_conv_is(di.getInt("BirthYear") % 100);
		row[6]=ws2s(di.getString("Sex"));
		row[8] = "0";  // non-comp
		row[9]=ws2s(it->getStartTimeS());
		if(row[9]=="-") row[9]="";

		row[10]=ws2s(it->getFinishTimeS());
		if(row[10]=="-") row[10]="";

		row[11]= formatOeCsvTime(it->getRunningTime());
		if(row[11]=="-") row[11]="";

		row[12]=my_conv_is(MyConvertStatusToOE(it->getStatus()));
		if (MyConvertStatusToOE(it->getStatus()) == 1) //DNS
			continue;
		row[13]=my_conv_is(it->getClubId());
		
		row[15]=ws2s(it->getClub());
		row[16]=ws2s(di.getString("Nationality"));
		row[17]=IsSydneySummerSeries && !byClass ? "1" : my_conv_is(it->getClassId(false));
		row[18]=ws2s(it->getClass(false));
		row[19]=ws2s(it->getClass(false));
		row[20]=my_conv_is(it->getRogainingPoints(false));
		row[21]=my_conv_is(it->getRogainingReduction()+it->getRogainingPoints(false));
		row[22]=my_conv_is(it->getRogainingReduction());
		row[23]=ws2s(it->getClass(false));

		row[35]=my_conv_is(di.getInt("CardFee"));
		row[36]=my_conv_is(di.getInt("Fee"));
		row[37]=my_conv_is(di.getInt("Paid"));
		
		pCourse pc=it->getCourse(false);
		if(pc){
			row[38]=my_conv_is(pc->getId());
			row[24]=ws2s(pc->getName());
			row[18]=ws2s(pc->getName());
			row[39]=ws2s(pc->getName());
			if(pc->getLength()>0){
				sprintf_s(bf, "%d.%d", pc->getLength()/1000, pc->getLength()%1000);
				row[40]=bf;
			}
			else
				row[40] = "0";
			row[41]=my_conv_is(pc->getDI().getInt("Climb"));
			row[42]=my_conv_is(pc->getNumControls());
		}
  row[43] = ws2s(it->getPlaceS());
	row[44] = ws2s(it->getStartTimeS());
	if(row[44]=="-") row[44]="";
	row[45]=ws2s(it->getFinishTimeS());
	if(row[45]=="-") row[45]="";


// Get punches
  if (it->getCard()) 
		{
		int j(0);
    for (int i = 0; i < it->getCard()->getNumPunches(); i++)
			{
			oPunch* punch = it->getCard()->getPunchByIndex(i);
			if (punch->getControlNumber() > 0)
				{
				int st = it->getStartTime();
				int pt = punch->getAdjustedTime();
				if (st>0 && pt>0 && pt>st) 
          row [47 + j*2] = formatOeCsvTime(pt-st);
				row[46 + j*2] = my_conv_is(punch->getControlNumber());
				j++;
				}
      }
		row[42]=my_conv_is(j);
		}
	csv.outputRow(row);
	}

	csv.closeOutput();

	return true;
}

void oExtendedEvent::loadRentedCardNumbers()
{
		RentedCards.clear();
		vector<string> strings;
		string input = getPropertyString("RentedCards","");
		istringstream f(input);
		string s;    
		while (getline(f, s, ',')) 
				strings.push_back(s);
		
		for (unsigned int i = 0; i < strings.size(); i++)
		{
			istringstream f(strings.at(i));
			vector<string> parts; // should be 1 or 2 parts 
			string part;
			while (getline(f, part, '-')) 
				parts.push_back(part);

			if (parts.size() == 1)
			{
				int card = atoi(trim(parts.at(0)).c_str());
				RentedCards.push_back(card);
			}

			if (parts.size() == 2)
			{
				int start = atoi(trim(parts.at(0)).c_str());
				int end = atoi(trim(parts.at(1)).c_str());
				if (start < end)
				{
					for (int card = start; card <= end; card++)
						RentedCards.push_back(card);
				}
			}
		}
}


bool oExtendedEvent::isRentedCard(int card)
{
	if (!LoadedCards)
			loadRentedCardNumbers();
	LoadedCards = true;
	for (unsigned int i = 0; i < RentedCards.size(); i++)
		if (RentedCards.at(i) == card)
			return true;
	return false;
}
