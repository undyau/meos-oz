﻿/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2018 Melin Software HB

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
#include "random.h"

static int _rbp=0;
static int _rbinterval=1;

const int PrimeBits=16381;
unsigned short _rbsource[];

void InitRanom(int offset, int interval)
{
  if (offset<0) offset=-offset;
  if (interval<0) interval=-interval;

  _rbp=offset%PrimeBits;
  _rbinterval=interval%PrimeBits;

  if (_rbinterval==0) _rbinterval=1;
}

bool GetRandomBit()
{
  int p1=_rbp/16;
  int p2=_rbp%16;

  int mask=1<<p2;
  _rbp=(_rbp+_rbinterval)%PrimeBits;

  return (_rbsource[p1]&mask)!=0;
}

int GetRandomNumber(int m)
{
  DWORD r=0;
  if (m<=1) return 0;

  int bits=0;

  if (m==2)
    bits=1;
  else if (m==4)
    bits=2;
  else if (m<5)
    bits=8;
  else if (m<10)
    bits=10;
  else if (m<15)
    bits=14;
  else if (m<30)
    bits=14;
  else if (m<100)
    bits=20;
  else bits=31;

  for(int i=0; i<bits; i++)
    if (GetRandomBit())    r|=(1<<i);

  return r%m;
}

void permute(vector<int> &vec)
{
  if (vec.empty())
    return;

  permute(&vec.front(), vec.size());
}

void permute(int *array, int size)
{
  int size1=size-1;

  int m=0;

  while(m<=size1) {
    if (GetRandomBit()) {
      int t=array[size1];
      array[size1]=array[m];
      size1--;
      array[m]=t;
    }
    else m++;
  }

  int p1=m;
  int p2=size-m;

  if (p1>1) permute(array, p1);

  if (p2>1) permute(&array[p1], p2);
}

//Truely 16-bit random number
unsigned short _rbsource[1024] = {
  30728,  35457,  26975,  19859,  23519,
  5742, 33642,  14368,  7129, 26291,
  27899,  12534,  29631,  51822,  5116,
  35417,  43931,  10867,  6675, 3520,
  5327, 55783,  6163, 51872,  2022,
  7560, 52062,  25467,  49664,  48585,
  20752,  62332,  26940,  33950,  14160,
  40096,  40812,  34902,  50239,  23210,
  25706,  47651,  47192,  6217, 40301,
  49652,  52899,  28620,  53991,  57852,
  10485,  36089,  16846,  5584, 8677,
  31815,  22339,  24278,  55526,  62684,
  39735,  6753, 5384, 40822,  23192,
  10303,  25428,  51727,  26223,  30049,
  13405,  62013,  63093,  37555,  39870,
  37613,  16503,  33445,  26463,  20228,
  37242,  39552,  51257,  2660, 20339,
  32439,  31637,  60935,  53538,  29452,
  54115,  18739,  49913,  759,  57034,
  24809,  38616,  48361,  65411,  20643,
  919,  35464,  61603,  57907,  21815,
  42812,  10476,  30086,  36103,  5749,
  47644,  32016,  4855, 16566,  52890,
  34894,  36823,  45151,  24524,  42331,
  47215,  29876,  1086, 19682,  44505,
  6953, 48447,  36245,  57337,  49577,
  31192,  61198,  24040,  58801,  6090,
  8809, 34512,  50288,  43905,  52238,
  17551,  64957,  16979,  9085, 16058,
  59048,  19127,  43613,  9212, 21228,
  44774,  49355,  31526,  60870,  60021,
  33378,  1477, 26571,  41926,  22807,
  63555,  50328,  32519,  58060,  8723,
  30810,  23886,  28227,  31135,  5179,
  59446,  1779, 49455,  12293,  27635,
  17108,  27207,  52213,  11773,  48464,
  51180,  34022,  30889,  23256,  4804,
  44202,  38709,  8563, 10425,  8927,
  7202, 62358,  60199,  56370,  10997,
  63435,  45747,  16668,  19149,  47928,
  52858,  14550,  19895,  2221, 24491,
  14678,  1662, 58742,  60776,  37361,
  62283,  35482,  45578,  25345,  27396,
  64403,  10254,  63522,  10742,  16185,
  29178,  2949, 54682,  13276,  56613,
  6709, 49089,  20299,  31633,  65422,
  3376, 52106,  39859,  22744,  59562,
  40792,  28838,  40600,  49722,  26507,
  44161,  60566,  56914,  6553, 44363,
  36017,  30479,  46476,  28601,  45802,
  34504,  25227,  57970,  137,  21241,
  63779,  50860,  59660,  38459,  7041,
  57115,  65314,  59544,  51920,  12941,
  59915,  13229,  44323,  49502,  3986,
  43556,  15283,  55171,  51930,  30124,
  31918,  60884,  52450,  22785,  45804,
  42051,  59293,  22468,  21249,  64383,
  61853,  31326,  59432,  47362,  55949,
  7926, 55496,  51528,  13949,  36770,
  17244,  11081,  41718,  32293,  19623,
  12004,  55951,  19630,  44767,  23274,
  57811,  42013,  53583,  28647,  55799,
  56730,  45717,  3738, 26033,  13187,
  59853,  62262,  42172,  64605,  4250,
  16442,  61124,  9084, 42264,  3296,
  8006, 15245,  34327,  27031,  5858,
  2640, 62110,  40807,  54229,  37204,
  23300,  36879,  41222,  45144,  46438,
  29292,  2871, 43485,  31001,  4079,
  5198, 14977,  35282,  1922, 62253,
  61644,  6911, 40400,  14753,  7053,
  49294,  37683,  16316,  56099,  56827,
  22523,  48722,  12734,  22514,  44588,
  59253,  13579,  45458,  53523,  54413,
  6049, 19933,  1121, 22006,  14235,
  27043,  48798,  56043,  41070,  59482,
  12563,  18473,  17759,  42406,  10542,
  42404,  40131,  31086,  29589,  53042,
  5822, 19427,  59335,  64300,  26468,
  3068, 5974, 29452,  24493,  12050,
  1181, 36582,  40382,  62551,  24868,
  61656,  10073,  2633, 57140,  34621,
  64676,  35919,  56547,  60366,  2179,
  7602, 28768,  21862,  18931,  58015,
  44539,  58886,  31334,  25534,  5831,
  51614,  63768,  41067,  44091,  28948,
  8307, 32583,  10276,  34526,  20340,
  8295, 58427,  63975,  63752,  2230,
  53748,  44164,  53573,  48047,  22021,
  26565,  58103,  36556,  17837,  24933,
  54433,  65420,  7682, 48337,  9110,
  52001,  25769,  16743,  17,   46490,
  54901,  23878,  19348,  22829,  25907,
  28096,  28793,  63691,  54362,  12952,
  58876,  27746,  38542,  29933,  19216,
  32032,  63839,  14429,  17431,  2445,
  45532,  60914,  4649, 22265,  45104,
  9763, 20512,  45049,  25378,  39744,
  10554,  50373,  12992,  8499, 46343,
  16933,  63691,  19219,  28607,  59929,
  7281, 11164,  43687,  12709,  25653,
  45176,  60319,  23950,  20644,  61539,
  61287,  45953,  46149,  33619,  46006,
  65365,  64492,  33842,  36026,  26790,
  16052,  38306,  17781,  35129,  50098,
  52022,  56984,  60295,  29876,  6632,
  47592,  51948,  25687,  25095,  59006,
  35267,  31056,  63081,  6230, 28892,
  48117,  11070,  44274,  38068,  60870,
  3673, 62138,  47151,  217,  55073,
  27930,  48784,  2407, 48362,  8969,
  46317,  63707,  2025, 31577,  42668,
  31790,  43951,  61656,  25632,  48904,
  8734, 2545, 63348,  39723,  21321,
  12448,  11362,  30383,  10485,  42111,
  57866,  6588, 17846,  2188, 59079,
  43620,  241,  27575,  51373,  3111,
  6068, 10372,  64345,  25249,  36913,
  15072,  36880,  22920,  3230, 5081,
  32958,  50358,  7878, 18166,  18909,
  56552,  5527, 43930,  20479,  65480,
  12692,  4868, 39170,  28755,  5191,
  30292,  42759,  26030,  21496,  40897,
  1843, 62971,  61260,  47438,  834,
  17987,  29647,  53149,  62821,  26611,
  28046,  8382, 60773,  7552, 54779,
  54428,  40227,  53776,  57859,  23402,
  35420,  32850,  65448,  40732,  36860,
  25794,  15241,  50379,  4355, 52229,
  46451,  15505,  39115,  46520,  56540,
  11529,  47030,  59898,  35162,  58059,
  53564,  39315,  1535, 25155,  49810,
  8846, 32669,  26626,  53269,  1825,
  64357,  49070,  3600, 33692,  7987,
  61,   24243,  17754,  20137,  39901,
  38795,  28199,  27046,  28004,  33510,
  39150,  48320,  20705,  45764,  40176,
  38714,  1184, 31962,  16213,  51209,
  38278,  24801,  56759,  17018,  60382,
  42066,  47030,  49315,  19948,  31687,
  24178,  59609,  4277, 63742,  39745,
  60057,  39751,  17252,  63402,  39614,
  28135,  12750,  48062,  32869,  42756,
  41237,  44458,  63489,  34621,  24869,
  31908,  65428,  43373,  14052,  19853,
  48763,  14635,  3569, 21064,  51917,
  23082,  54170,  44780,  65368,  1372,
  48083,  42008,  6122, 8996, 34452,
  3303, 54818,  54847,  19141,  6746,
  31631,  36252,  40732,  24195,  32248,
  13508,  43545,  33603,  28745,  41316,
  58808,  24214,  51529,  20705,  2866,
  57107,  51665,  51911,  17662,  28622,
  29578,  50017,  21376,  30333,  2214,
  10489,  48075,  46746,  10809,  1111,
  26559,  29457,  1360, 6469, 10200,
  41521,  34678,  13069,  10882,  366,
  36085,  14816,  9457, 10243,  19000,
  44036,  30160,  22295,  21307,  8242,
  20545,  56386,  27901,  21702,  35236,
  57935,  60688,  6163, 27038,  11634,
  4041, 1011, 61094,  6802, 63312,
  5986, 27700,  13123,  11982,  51424,
  48425,  7091, 52297,  51808,  9253,
  17501,  56772,  18246,  2506, 18773,
  11147,  35416,  47385,  36956,  21263,
  16701,  13794,  52756,  2047, 4534,
  17257,  6853, 19991,  29008,  5273,
  42814,  4589, 56285,  49199,  46228,
  46271,  60096,  53203,  3942, 30218,
  24090,  41675,  51153,  42911,  52293,
  64212,  50287,  41345,  60489,  28006,
  52367,  56447,  43286,  62528,  5916,
  30864,  44523,  58971,  55724,  3494,
  43906,  28694,  21250,  27093,  57804,
  12233,  33150,  2100, 12811,  5052,
  31241,  549,  29819,  59432,  19016,
  48020,  53766,  43348,  13634,  24657,
  17549,  11930,  37716,  18918,  45007,
  21662,  1561, 60785,  57279,  5679,
  26879,  54071,  3220, 6437, 7772,
  64372,  5764, 11973,  1872, 11652,
  29010,  4287, 11284,  7966, 53008,
  44764,  43943,  58067,  6842, 20447,
  49644,  176,  25075,  26989,  54655,
  52950,  47510,  428,  31441,  26850,
  32390,  6361, 41971,  42033,  45355,
  6334, 58392,  44757,  41528,  9593,
  7362, 52409,  16207,  5459, 32215,
  6116, 24159,  37266,  18989,  61404,
  56066,  59647,  51229,  63369,  16396,
  51858,  48111,  49537,  26123,  14758,
  8304, 5843, 20037,  19705,  34137,
  21841,  20916,  48812,  33837,  40442,
  32173,  56091,  15816,  60267,  53482,
  9015, 61795,  41219,  23273,  30874,
  50797,  52328,  50702,  54431,  21615,
  43481,  3443, 47978,  56058,  48777,
  51551,  10601,  21927,  22034,  41042,
  22472,  37855,  64711,  27822,  37580,
  34908,  31200,  55035,  57455,  12870,
  41220,  3053, 16491,  54672,  41618,
  45874,  62483,  10430,  49167,  27787,
  42797,  17295,  57933,  64205,  23999,
  24788,  26660,  23722,  6042
};
