﻿/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2022 Melin Software HB

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

// TimeStamp.cpp: implementation of the TimeStamp class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "meos.h"
#include "TimeStamp.h"
#include <algorithm>
#include "meos_util.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

constexpr __int64 minYearConstant = 2014 - 1601;

TimeStamp::TimeStamp()
{
  Time=0;
  //Update();
}

TimeStamp::~TimeStamp()
{

}

void TimeStamp::update(TimeStamp &ts)
{
  Time=max(Time, ts.Time);
}

void TimeStamp::update()
{
  SYSTEMTIME st;
  GetLocalTime(&st);

  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  __int64 &currenttime=*(__int64*)&ft;

  Time=unsigned((currenttime/10000000L) - minYearConstant*365*24*3600);
}

int TimeStamp::getAge() const
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);
  __int64 &currenttime=*(__int64*)&ft;

  int CTime=int((currenttime/10000000)-minYearConstant*365*24*3600);

  return CTime-Time;
}

const string &TimeStamp::getStamp() const
{
  if (stampCodeTime == Time)
    return stampCode;
  
  stampCodeTime = Time;
  __int64 ft64=(__int64(Time)+minYearConstant*365*24*3600)*10000000;
  FILETIME &ft=*(FILETIME*)&ft64;
  SYSTEMTIME st;
  FileTimeToSystemTime(&ft, &st);
  char bf[64];
  sprintf_s(bf, 32, "%d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  stampCode = bf;

  return stampCode;
}

const string &TimeStamp::getStamp(const string &sqlStampIn) const {
  stampCode.resize(8 + 6 + 1);
  int outIx = 0;
  for (char c : sqlStampIn) {
    if (c >= '0' && c <= '9' && outIx < 8 + 6)
      stampCode[outIx++] = c;
  }
  return stampCode;
}

wstring TimeStamp::getStampString() const
{
  __int64 ft64=(__int64(Time)+minYearConstant*365*24*3600)*10000000;
  FILETIME &ft=*(FILETIME*)&ft64;
  SYSTEMTIME st;
  FileTimeToSystemTime(&ft, &st);

  wchar_t bf[32];
  swprintf_s(bf, L"%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

  return bf;
}

string TimeStamp::getStampStringN() const
{
  __int64 ft64 = (__int64(Time) + minYearConstant * 365 * 24 * 3600) * 10000000;
  FILETIME &ft = *(FILETIME*)&ft64;
  SYSTEMTIME st;
  FileTimeToSystemTime(&ft, &st);
  int y = getThisYear();
  if (st.wYear > y || st.wYear < 2009) {
    st.wYear = y;
    st.wDay = 1;
    st.wMonth = 1;
    st.wHour = 2;
    st.wMinute = 0;
    st.wSecond = 0;
  }
  
  char bf[32];
  sprintf_s(bf, "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

  return bf;
}

void TimeStamp::setStamp(const string &s)
{
  if (s.size()<14)
    return;
  SYSTEMTIME st;
  memset(&st, 0, sizeof(st));

  //const char *ptr=s.c_str();
  //sscanf(s.c_str(), "%4hd%2hd%2hd%2hd%2hd%2hd", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  st.wYear=atoi(s.substr(0, 4).c_str());
  st.wMonth=atoi(s.substr(4, 2).c_str());
  st.wDay=atoi(s.substr(6, 2).c_str());
  st.wHour=atoi(s.substr(8, 2).c_str());
  st.wMinute=atoi(s.substr(10, 2).c_str());
  st.wSecond=atoi(s.substr(12, 2).c_str());

  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  __int64 &currenttime=*(__int64*)&ft;

  Time = unsigned((currenttime/10000000)-minYearConstant*365*24*3600);
}
