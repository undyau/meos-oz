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
#include "xmlparser.h"
#include <time.h>
#include "iof30interface.h"


oExtendedEvent::oExtendedEvent(gdioutput &gdi) : oEvent(gdi)
{
  IsSydneySummerSeries = false;
  PreserveExistingRunnersAsIs = false;
}

oExtendedEvent::~oExtendedEvent(void)
{
}

bool oExtendedEvent::preserveExistingRunnersAsIs(bool preserve) {
  return PreserveExistingRunnersAsIs = preserve;
  }

bool oExtendedEvent::addXmlRunner(gdioutput & gdi, xmlobject& xo)
{
  xmlobject xPers = xo.getObject("Person");
  if (!xPers)
    return false;

  wstring name;
  xmlobject xname = xPers.getObject("Name");

  if (!xname) {
    gdi.addString("", 0, "Entry missing name");
    return false;
    }

  wstring given, family;
  name = xname.getObjectString("Given", given) + L" " +  xname.getObjectString("Family", family);

  wstring club;
  xmlobject xclub = xo.getObject("Organisation");
  if (xclub)
    xclub.getObjectString("Name", club);

  wstring className;
  xmlobject xclass = xo.getObject("Class");
  if (!xclass)
    {
    gdi.addString("", 0, L"Entry for " + name + L" missing class - entry ignored");
    return false;
    }
  xclass.getObjectString("Name", className);
  className = NormaliseClassName(className);
  pClass cl = getClass(className);
  if (!cl)
    {
    className = GetShortSssName(className);
    cl = getClass(className);
    if (!cl)
      {
      gdi.addString("", 0, L"Entry for " + name + L" has unknown class " + className + L" - entry ignored");
      return false;
      }
    }

  const int cardNo = xo.getObjectInt("ControlCard");
  if (cardNo == 0)
    gdi.addString("", 0, L"No control card number for " + name);

  wstring birthDate;
  xPers.getObjectString("BirthDate", birthDate);
  if (birthDate.empty())
    birthDate = L"1998-01-01";  // Force to be a senior at least

  // Don't over-write existing
  pRunner existing = getRunnerByName(name);
  if (existing && (existing->getCardNo() == cardNo || cardNo == 0))
    {
    gdi.addString("", 0, L"Entry for " + name + L" already seems to exist - entry ignored");
    return false;
    }

  existing = getRunnerByCardNo(cardNo, 0, oEvent::CardLookupProperty::Any);
  if (existing && existing->getName() == name)
    {
    gdi.addString("", 0, L"Entry for " + name + L" already seems to exist - entry ignored");
    return false;
    }

  return !!addRunner(name, club, cl->getId(), cardNo, birthDate, true);
}

void oExtendedEvent::importXML_SeasonTickets(gdioutput & gdi, const wstring & competitorFile, const wstring& classesFile)
{
// Read the classes file to determine any short names which can then be applied to the competitor import
  wstring name, shortName, longName;
  xmlparser xmlCl;
  xmlCl.read(classesFile);
  xmlobject xo = xmlCl.getObject("ClassList");
  xmlList cClass;
  if (xo)
    xo.getObjects("Class", cClass);
  for (size_t k = 0; k < cClass.size(); k++) {
    xmlobject &xclass = cClass[k];
    xclass.getObjectString("Name", name);
    xclass.getObjectString("ShortName", shortName);
    if (name.size() > 0 && shortName.size() > 0 && shortName.size() < name.size())
      AddClassNameNormalisation(name, shortName);
    }

// Only handles the simplest case - single event, single competitor per entry
  int ent = 0, fail = 0;

  xmlparser xml;
  xml.read(competitorFile);

  xo = xml.getObject("EntryList");

  if (xo) {
    gdi.addString("", 0, "Importerar anmälningar (IOF, xml)");
    gdi.refreshFast();

    xmlList pEntries;

    xo.getObjects("PersonEntry", pEntries);
    for (size_t k = 0; k < pEntries.size(); k++) {
      if (addXmlRunner(gdi, pEntries[k]))
        ++ent;
      else
        ++fail;
      }
    }
    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(ent));
    if (fail>0)
      gdi.addString("", 0, "Antal som inte importerades: X#" + itos(fail)).setColor(colorRed);
    gdi.dropLine();
    gdi.refreshFast();
}

wstring oExtendedEvent::NormaliseClassName(wstring name)
  {
  if (RenamedClasses.find(name) != RenamedClasses.end())
    return RenamedClasses[name];
  else
    return name;
  }

wstring oExtendedEvent::GetShortSssName(wstring & className)
{
  // Get initials of each word if its two words long and not "Walking..."
  vector<wstring> parts;
  split(className, L" ", parts);
  if (parts.size() > 1)
  {
    if (parts[0].substr(0, 2) == wstring(L"Wa"))
      return L"Wa" + parts[1].substr(0, 1);
    else
    {
      wstring retVal;
      for (auto i = 0; i < parts.size(); i++)
        retVal += parts[i].substr(0, 1);
      return retVal;
    }
  }

  return className;

}

void oExtendedEvent::AddClassNameNormalisation(wstring oldName, wstring newName)
  {
  RenamedClasses[oldName] = newName;
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

void oExtendedEvent::writeExtraXml(xmlparser &xml)
{
  xml.write("IsSydneySummerSeries", IsSydneySummerSeries);
}

void oExtendedEvent::readExtraXml(const xmlparser &xml)
{
   xmlobject xo;
 
  xo=xml.getObject("IsSydneySummerSeries");
  if(xo) IsSydneySummerSeries=!!xo.getInt();

}
