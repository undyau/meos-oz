﻿/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2019 Melin Software HB

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
    Eksoppsvägen 16, SE-75646 UPPSALA, Sweden

************************************************************************/
#include "stdafx.h"
#include <vector>
#include "meos_util.h"

//ABCDEFGHIJKLMNO
//V2: ABCDEFGHIHJKMN
//V31: a
//V33: abcde
//V35: abcdef
//V36: abcdef
int getMeosBuild() {
  string revision("$Rev: 915 $");
  return 174 + atoi(revision.substr(5, string::npos).c_str());
}

//ABCDEFGHIJKILMNOPQRSTUVXYZabcdefghijklmnopqrstuvxyz
//V2: abcdefgh
//V3: abcdefghijklmnopqrstuvxyz
//V31: abcde
//V32: abcdefgh
//V33: abcdefghij
//V34: abcdfge
wstring getMeosDate() {
  wstring date(L"$Date: 2019-07-30 07:05:51 +0200 (ti, 30 jul 2019) $");
  return date.substr(7,10);
}

wstring getBuildType() {
  return L"Update 2"; // No parantheses (...)
}

wstring getMajorVersion() {
  return L"3.6";
}

int getMinorVersion() {   // Used by MEOS-OZ only
	return 6;
	}

wstring getMeosFullVersion() {
  wchar_t bf[256];
  wstring maj = getMajorVersion();
  if (getBuildType().empty())
    swprintf_s(bf, L"Version X#%s.%d, %s (Meos-Oz %s.%d.%d)", maj.c_str(), getMeosBuild(), getMeosDate().c_str(), maj.c_str(), getMeosBuild(), getMinorVersion());
  else
    swprintf_s(bf, L"Version X#%s.%d, %s, %s (Meos-Oz %s.%d.%d)", maj.c_str(), getMeosBuild(), getBuildType().c_str(), getMeosDate().c_str(), maj.c_str(), getMeosBuild(), getMinorVersion());
  return bf;
}

wstring getMeosCompectVersion() {
  if (getBuildType().empty())
    return getMajorVersion() + L"." + itow(getMeosBuild());
  else
    return getMajorVersion() + L"." + itow(getMeosBuild()) + L" (" + getBuildType() + L")";
}

void getSupporters(vector<wstring> &supp, vector<wstring> &developSupp)
{
  supp.emplace_back(L"Sergio Yañez, ABC TRAIL");
  supp.emplace_back(L"Western Race Services");
  supp.emplace_back(L"IK Gandvik, Skara");
  supp.emplace_back(L"IK Stern");
  supp.emplace_back(L"OK Roslagen");
  supp.emplace_back(L"TSV Malente");
  supp.emplace_back(L"Emmaboda Verda OK");
  supp.emplace_back(L"KOB ATU Košice");
  supp.emplace_back(L"Gävle OK");
  supp.emplace_back(L"Kenneth Gattmalm, Jönköpings OK");
  supp.emplace_back(L"Søllerød OK");
  supp.emplace_back(L"Bengt Bengtsson");
  supp.emplace_back(L"OK Landehof");
  supp.emplace_back(L"OK Orinto");
  supp.emplace_back(L"Bredaryds SOK");
  supp.emplace_back(L"Thore Nilsson, Uddevalla OK");
  supp.emplace_back(L"Timrå SOK");
  supp.emplace_back(L"Åke Larsson, OK Hedströmmen");
  supp.emplace_back(L"Avesta OK");
  supp.emplace_back(L"Motionsorientering Göteborg");
  supp.emplace_back(L"OK Måsen");
  supp.emplace_back(L"IF Thor");
  supp.emplace_back(L"SOS Jindřichův Hradec");
  supp.emplace_back(L"Mats Holmberg, OK Gränsen");
  supp.emplace_back(L"Christoffer Ohlsson, Uddevalla OK");
  supp.emplace_back(L"KOB ATU Košice");
  supp.emplace_back(L"O-Ringen AB");
  supp.emplace_back(L"Hans Carlstedt, Sävedalens AIK");
  supp.emplace_back(L"IFK Mora OK");
  supp.emplace_back(L"Attunda OK");
  supp.emplace_back(L"OK Tyr, Karlstad");
  supp.emplace_back(L"Siguldas Takas, Latvia");
  supp.emplace_back(L"Eric Teutsch, Ottawa Orienteering Club, Canada");
  supp.emplace_back(L"Silkeborg OK, Denmark");
  supp.emplace_back(L"Erik Ivarsson Sandberg");
  supp.emplace_back(L"Stenungsunds OK");
  supp.emplace_back(L"OK Leipzig");
  supp.emplace_back(L"Degerfors OK");
  supp.emplace_back(L"OK Tjärnen");
  supp.emplace_back(L"Leksands OK");  
  supp.emplace_back(L"O-Travel");
  supp.emplace_back(L"Kamil Pipek, OK Lokomotiva Pardubice");
  developSupp.emplace_back(L"KOB Kysak");
  supp.emplace_back(L"Richard HEYRIES");
  supp.emplace_back(L"Ingemar Carlsson");
  supp.emplace_back(L"Tolereds AIK");
  supp.emplace_back(L"OK Snab");
  supp.emplace_back(L"OK 73");
  supp.emplace_back(L"Helsingborgs SOK");
  supp.emplace_back(L"Sala OK");
  supp.emplace_back(L"OK Roskilde");
  developSupp.emplace_back(L"Almby IK, Örebro");
  supp.emplace_back(L"Ligue PACA");
  supp.emplace_back(L"SC vebr-sport");
  supp.emplace_back(L"IP Skogen Göteborg");
  supp.emplace_back(L"Smedjebackens Orientering");
  supp.emplace_back(L"Gudhems IF");
  supp.emplace_back(L"Kexholm SK");
  supp.emplace_back(L"Utby IK");
  supp.emplace_back(L"JWOC 2019");
  developSupp.emplace_back(L"OK Nackhe");
  supp.emplace_back(L"OK Rodhen");

  reverse(supp.begin(), supp.end());
}
