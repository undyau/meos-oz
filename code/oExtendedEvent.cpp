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
  SssSeriesPrefix = "sss";
	LoadedCards = false;
	setShortClubNames(false);   // default to behaving like vanilla MEOS
}

oExtendedEvent::~oExtendedEvent(void)
{
}

bool oExtendedEvent::SSSQuickStart(gdioutput &gdi)
{
  oSSSQuickStart qs(*this);
	if (qs.ConfigureEvent(gdi)){
		IsSydneySummerSeries = true;
		setShortClubNames(true);
		return true;
	}
	return false;
}

void oExtendedEvent::exportCourseOrderedIOFSplits(IOFVersion version, const char *file, bool oldStylePatrolExport, const set<int> &classes, int leg)
{
	// Create new classes names after courses
	std::map<int, int> courseNewClassXref;
	std::map<int, int> runnerOldClassXref;
	std::set<std::string> usedNames;

	for (oClassList::iterator j = Classes.begin(); j != Classes.end(); j++) {
		usedNames.insert(j->getName());
	}

	for (oCourseList::iterator j = Courses.begin(); j != Courses.end(); j++) {			
		std::string name = j->getName();
		if (usedNames.find(name) != usedNames.end()) {
				name = lang.tl("Course ") + name;
			}
		while (usedNames.find(name) != usedNames.end()) {
				name = name + "_";
			}

		usedNames.insert(name);
		pClass newClass = addClass(name, j->getId());
		courseNewClassXref[j->getId()] = newClass->getId();
	}
	
	// Reassign all runners to new classes, saving old ones
	for (oRunnerList::iterator j = Runners.begin(); j != Runners.end(); j++) {
		runnerOldClassXref[j->getId()] = j->getClassId();	
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


void oEvent::setShortClubNames(bool shorten)
{
  eventProperties["DoShortenClubNames"] = shorten ? "1" : "0";
}

string oEvent::shortenName(string name)
{
  if (eventProperties["DoShortenClubNames"] != "1")
    return name;

	std::map<string,string> clubs;
	clubs["ShortNameFor Abominable O-Men ACT"] = "AO.A";
	clubs["ShortNameFor Albury-Wodonga Orienteering Club"] = "AW.V";
	clubs["ShortNameFor Auckland"] = "AK.NZL";
	clubs["ShortNameFor Australian Bushrangers"] = "AUB.AUS";
	clubs["ShortNameFor Australian Challenge team"] = "AUC.AUS";
	clubs["ShortNameFor Australopers"] = "AL.T";
	clubs["ShortNameFor Bayside Kangaroos Orienteers"] = "BK.V";
	clubs["ShortNameFor Bendigo Orienteers"] = "BG.V";
	clubs["ShortNameFor Bennelong Northside Orienteers"] = "BN.N";
	clubs["ShortNameFor Bibbulmun Orienteers"] = "BO.W";
	clubs["ShortNameFor Big Foot Orienteers"] = "BF.N";
	clubs["ShortNameFor Brumby Orienteering Club"] = "BR.V";
	clubs["ShortNameFor Bullecourt Boulder Bounders"] = "BB.Q";
	clubs["ShortNameFor Bundaberg United Scrub Harriers"] = "BU.Q";
	clubs["ShortNameFor Bush n Beach Orienteering Club"] = "BB.N";
	clubs["ShortNameFor Bushflyers ACT"] = "BS.A";
	clubs["ShortNameFor Canberra Cockatoos"] = "CC.A";
	clubs["ShortNameFor Canberra Forest Riders ACT"] = "CF.A";
	clubs["ShortNameFor Central Coast Orienteers"] = "CC.N";
	clubs["ShortNameFor Central Highlands Orienteering Club"] = "CH.V";
	clubs["ShortNameFor Counties Manakau"] = "CM.NZL";
	clubs["ShortNameFor Dandenong Ranges Orienteering Club"] = "DR.V";
	clubs["ShortNameFor Dunedin"] = "DN.NZL";
	clubs["ShortNameFor Enoggeroos"] = "EN.Q";
	clubs["ShortNameFor Esk Valley"] = "EV.T";
	clubs["ShortNameFor Eureka Orienteers"] = "EU.V";
	clubs["ShortNameFor Far North Orienteers"] = "FN.Q";
	clubs["ShortNameFor Far North Orienteers"] = "NT.Q";
	clubs["ShortNameFor Fraser Region Orienteering Group"] = "FR.Q";
	clubs["ShortNameFor Garingal Orienteers"] = "GO.N";
	clubs["ShortNameFor Girls Grammar ACT"] = "GG.A";
	clubs["ShortNameFor Goldseekers Orienteers"] = "GS.N";
	clubs["ShortNameFor Grammar Junior ACT"] = "GJ.A";
	clubs["ShortNameFor Grammar Senior ACT"] = "GS.A";
	clubs["ShortNameFor Hamilton"] = "HA.NZL";
	clubs["ShortNameFor Hawkes Bay"] = "HB.NZL";
	clubs["ShortNameFor Hutt Valley"] = "HV.NZL";
	clubs["ShortNameFor Illawarra Kareelah Orienteers"] = "IK.N";
	clubs["ShortNameFor Kulgun 225 Orienteers"] = "KO.W";
	clubs["ShortNameFor LOST"] = "LO.W";
	clubs["ShortNameFor Lincoln Orienteers"] = "LI.S";
	clubs["ShortNameFor Malborough"] = "MB.NZL";
	clubs["ShortNameFor Melbourne Forest Racers"] = "MF.V";
	clubs["ShortNameFor Mountain Devils MTB Orienteering Club"] = "MD.N";
	clubs["ShortNameFor Multi Terrain Bike Orienteers"] = "MT.Q";
	clubs["ShortNameFor NSW Stingers"] = "ST.N";
	clubs["ShortNameFor Nelson"] = "NL.NZL";
	clubs["ShortNameFor New Zealand Challenge team"] = "NZC.NZL";
	clubs["ShortNameFor New Zealand Pinestars"] = "NZP.NZL";
	clubs["ShortNameFor Newcastle Orienteering Club"] = "NC.N";
	clubs["ShortNameFor Nillumbik Emus Orienteering Club"] = "NE.V";
	clubs["ShortNameFor No club membership"] = "NONE";
	clubs["ShortNameFor North West"] = "NW.NZL";
	clubs["ShortNameFor Northern Rivers Orienteering Club"] = "NR.N";
	clubs["ShortNameFor Northern Tablelands Orienteering Club"] = "NT.N";
	clubs["ShortNameFor Onkaparinga Hills Orienteering Club"] = "OH.S";
	clubs["ShortNameFor Orienteering ACT"] = "NONE";
	clubs["ShortNameFor Paradise Lost Orienteers"] = "PL.Q";
	clubs["ShortNameFor Parawanga Orienteers ACT"] = "PO.A";
	clubs["ShortNameFor Pathfinders"] = "PF.T";
	clubs["ShortNameFor Peninsula and Plains"] = "PP.NZL";
	clubs["ShortNameFor Queensland Cyclones"] = "QC.Q";
	clubs["ShortNameFor Range Runners Orienteering Club"] = "RR.Q";
	clubs["ShortNameFor Red Kiwi"] = "RK.NZL";
	clubs["ShortNameFor Red Roos ACT"] = "RR.A";
	clubs["ShortNameFor Rotorua"] = "RO.NZL";
	clubs["ShortNameFor Saltbush Orienteers"] = "SB.S";
	clubs["ShortNameFor South-East Orienteering Club"] = "SE.S";
	clubs["ShortNameFor South-West Orienteering Trekkers"] = "SW.W";
	clubs["ShortNameFor Southern Arrows"] = "SW.S";
	clubs["ShortNameFor Southern Highlands Occasional Orienteers"] = "SH.N";
	clubs["ShortNameFor Southland"] = "SD.NZL";
	clubs["ShortNameFor St Edmunds ACT"] = "SE.A";
	clubs["ShortNameFor Sunshine Orienteers Club"] = "SO.Q";
	clubs["ShortNameFor Taranaki"] = "TA.NZL";
	clubs["ShortNameFor Tasmanian Foresters"] = "TAS.A";
	clubs["ShortNameFor Taupo"] = "TP.NZL";
	clubs["ShortNameFor Tintookies Orienteers"] = "TT.S";
	clubs["ShortNameFor Tjuringa Orienteers"] = "TJ.S";
	clubs["ShortNameFor Toohey Forest Orienteers"] = "TF.Q";
	clubs["ShortNameFor Top End Orienteers"] = "TE.NT";
	clubs["ShortNameFor Townsville Thuringowa Orienteering Club"] = "TT.Q";
	clubs["ShortNameFor Tuckonie Orienteering Club"] = "TK.V";
	clubs["ShortNameFor Ugly Gully Orienteers"] = "UG.Q";
	clubs["ShortNameFor University of Queensland"] = "UQ.Q";
	clubs["ShortNameFor Uringa Orienteers"] = "UR.N";
	clubs["ShortNameFor Victorian ARDF Group"] = "AR.V";
	clubs["ShortNameFor Victorian Nuggets"] = "VN.V";
	clubs["ShortNameFor Wagga and Riverina Occasional Orienteers"] = "WR.N";
	clubs["ShortNameFor Wairarapa"] = "WA.NZL";
	clubs["ShortNameFor Wallaringa Orienteering Club"] = "WA.S";
	clubs["ShortNameFor Wellington Ranges"] = "WR.T";
	clubs["ShortNameFor Wellington"] = "WN.NZL";
	clubs["ShortNameFor Western Nomads"] = "WN.W";
	clubs["ShortNameFor Western Plains Orienteers"] = "WP.N";
	clubs["ShortNameFor Western and Hills Orienteers"] = "WH.N";
	clubs["ShortNameFor Weston Emus ACT"] = "WE.A";
	clubs["ShortNameFor Wullundigong Orienteers of the West"] = "WO.W";
	clubs["ShortNameFor Yalanga Orienteers"] = "YA.S";
	clubs["ShortNameFor Yarra Valley Orienteering Club"] = "YV.V";

	// Check for presence in map of shortened names
	string key("ShortNameFor " + name);
	
	if (eventProperties.find(key) != eventProperties.end() && eventProperties[key].length() > 0)
		return eventProperties[key];
	
	if (clubs.find(key) != clubs.end())
		{
		eventProperties[key] = clubs[key];
		return clubs[key];
		}

	// Shorten mechanically
	std::vector<string> words;
	unsigned int pos = 0;
	while ((pos = name.find(" ", pos)) != string::npos)
		{
		string temp = name.substr(0,pos - 1);
		temp = trim(temp);
		if (temp.length() > 0)
			words.push_back(temp);
		if (pos < name.length())
			name = name.substr(pos + 1);
		else
			pos = string::npos;
		}
	if (words.size() > 1)
		{
		if (stringMatch(words[words.size() -1].substr(0,10), "orienteers"))
			words.pop_back();
		string club;
		for (unsigned int i = 0; i < words.size(); i++)
			if (words[i] == "OC" || words[i] == "OK")
				club += words[i];
			else
				club += toupper(words[i][0]);
		return club;
		}
	else
		return name.substr(0,5);
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
	string url = gdi.getText("SssServer");
	SssEventNum = gdi.getTextNo("SssEventNum", false);
  SssSeriesPrefix = gdi.getText("SssSeriesPrefix",false);
	if (SssEventNum == 0) {
		gdi.alert("Invalid event number :" + gdi.getText("SssEventNum"));
		return;
		}

	string resultCsv = getTempFile();
	ProgressWindow pw(hWnd());
	exportOrCSV(resultCsv.c_str(), false);
	string data = _T(loadCsvToString(resultCsv));
	data = string_replace(data, "&","and");
	data = "Name=" + SssSeriesPrefix + itos(SssEventNum) + "&Title=" + Name + "&Subtitle=" + SssSeriesPrefix + itos(SssEventNum) + "&Data=" + data + "&Serial=" + itos(s_counter++);
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
	gdi.alert("Completed upload of results to " + url);
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
	if(xo) SssSeriesPrefix=xo.get();
}

string oExtendedEvent::loadCsvToString(string file)
{
	string result;
	std::ifstream fin;
	fin.open(file.c_str());

	if(!fin.good())
		return string("");

	char bf[1024];
	while (!fin.eof()) {	
		fin.getline(bf, 1024);
		string temp(bf);
		result += temp + "\n";
	}
	
	return result.substr(0, result.size()-1);
}

string oExtendedEvent::string_replace(string src, string const& target, string const& repl)
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

bool oExtendedEvent::exportOrCSV(const char *file, bool byClass)
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

	csv.OutputRow(lang.tl("Startnr;Bricka;Databas nr.;Efternamn;Förnamn;År;K;Block;ut;Start;Mål;Tid;Status;Klubb nr.;Namn;Ort;Land;Klass nr.;Kort;Lång;Num1;Num2;Num3;Text1;Text2;Text3;Adr. namn;Gata;Rad 2;Post nr.;Ort;Tel;Fax;E-post;Id/Club;Hyrd;Startavgift;Betalt;Bana nr.;Bana;km;Hm;Bana kontroller;Pl"));
	
	char bf[256];
	for(it=Runners.begin(); it != Runners.end(); ++it){	
		vector<string> row;
		row.resize(200);
		oDataInterface di=it->getDI();

		row[0]=my_conv_is(it->getId());
		row[1]=my_conv_is(it->getCardNo());
    row[2]=my_conv_is(int(it->getExtIdentifier()));
		row[3]=it->getFamilyName();
		row[4]=it->getGivenName();
		row[5]=my_conv_is(di.getInt("BirthYear") % 100);
		row[6]=di.getString("Sex");
		row[8] = "0";  // non-comp
		row[9]=it->getStartTimeS();
		if(row[9]=="-") row[9]="";

		row[10]=it->getFinishTimeS();
		if(row[10]=="-") row[10]="";

		row[11]= formatOeCsvTime(it->getRunningTime());
		if(row[11]=="-") row[11]="";

		row[12]=my_conv_is(MyConvertStatusToOE(it->getStatus()));
		if (MyConvertStatusToOE(it->getStatus()) == 1) //DNS
			continue;
		row[13]=my_conv_is(it->getClubId());
		
		row[15]=it->getClub();
		row[16]=di.getString("Nationality");
		row[17]=IsSydneySummerSeries && !byClass ? "1" : my_conv_is(it->getClassId());
		row[18]=it->getClass();
		row[19]=it->getClass();
		row[20]=my_conv_is(it->getRogainingPoints(false));
		row[21]=my_conv_is(it->getRogainingReduction()+it->getRogainingPoints(false));
		row[22]=my_conv_is(it->getRogainingReduction());
		row[23]=it->getClass();

		row[35]=my_conv_is(di.getInt("CardFee"));
		row[36]=my_conv_is(di.getInt("Fee"));
		row[37]=my_conv_is(di.getInt("Paid"));
		
		pCourse pc=it->getCourse(false);
		if(pc){
			row[38]=my_conv_is(pc->getId());
			row[24]=pc->getName();
			row[18]=pc->getName();
			row[39]=pc->getName();
			if(pc->getLength()>0){
				sprintf_s(bf, "%d.%d", pc->getLength()/1000, pc->getLength()%1000);
				row[40]=bf;
			}
			else
				row[40] = "0";
			row[41]=my_conv_is(pc->getDI().getInt("Climb"));
			row[42]=my_conv_is(pc->getNumControls());
		}
  row[43] = it->getPlaceS();
	row[44] = it->getStartTimeS();
	if(row[44]=="-") row[44]="";
	row[45]=it->getFinishTimeS();
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
	csv.OutputRow(row);
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
