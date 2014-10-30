#include "dateparser.h"

void CDateParser::FillMaps(void)
{
  m_mWeekdayNames.clear();
  m_mWeekdayNames["mon"] = "1";
  m_mWeekdayNames["tue"] = "2";
  m_mWeekdayNames["wed"] = "3";
  m_mWeekdayNames["thu"] = "4";
  m_mWeekdayNames["fri"] = "5";
  m_mWeekdayNames["sat"] = "6";
  m_mWeekdayNames["sun"] = "7";

  m_mWeekdayNames["monday"] = "1";
  m_mWeekdayNames["tuesday"] = "2";
  m_mWeekdayNames["wednesday"] = "3";
  m_mWeekdayNames["thursday"] = "4";
  m_mWeekdayNames["friday"] = "5";
  m_mWeekdayNames["saturday"] = "6";
  m_mWeekdayNames["sunday"] = "7";

  m_mWeekdayNames["понедельник"] = "1";
  m_mWeekdayNames["вторник"] = "2";
  m_mWeekdayNames["среда"] = "3";
  m_mWeekdayNames["четверг"] = "4";
  m_mWeekdayNames["пятница"] = "5";
  m_mWeekdayNames["суббота"] = "6";
  m_mWeekdayNames["воскресенье"] = "7";

  m_mWeekdayNames["пн"] = "1";
  m_mWeekdayNames["вт"] = "2";
  m_mWeekdayNames["ср"] = "3";
  m_mWeekdayNames["чт"] = "4";
  m_mWeekdayNames["пт"] = "5";
  m_mWeekdayNames["сб"] = "6";
  m_mWeekdayNames["вс"] = "7";

  m_mMonthNames.clear();
  m_mMonthNames["jan"] = "1";
  m_mMonthNames["feb"] = "2";
  m_mMonthNames["mar"] = "3";
  m_mMonthNames["apr"] = "4";
  m_mMonthNames["may"] = "5";
  m_mMonthNames["jun"] = "6";
  m_mMonthNames["jul"] = "7";
  m_mMonthNames["aug"] = "8";
  m_mMonthNames["sep"] = "9";
  m_mMonthNames["oct"] = "10";
  m_mMonthNames["nov"] = "11";
  m_mMonthNames["dec"] = "12";

  m_mMonthNames["january"] = "1";
  m_mMonthNames["february"] = "2";
  m_mMonthNames["march"] = "3";
  m_mMonthNames["april"] = "4";
  m_mMonthNames["may"] = "5";
  m_mMonthNames["june"] = "6";
  m_mMonthNames["jule"] = "7";
  m_mMonthNames["august"] = "8";
  m_mMonthNames["september"] = "9";
  m_mMonthNames["october"] = "10";
  m_mMonthNames["november"] = "11";
  m_mMonthNames["december"] = "12";

  m_mMonthNames["янв"] = "1";
  m_mMonthNames["фев"] = "2";
  m_mMonthNames["мар"] = "3";
  m_mMonthNames["апр"] = "4";
  m_mMonthNames["май"] = "5";
  m_mMonthNames["июн"] = "6";
  m_mMonthNames["июл"] = "7";
  m_mMonthNames["авг"] = "8";
  m_mMonthNames["сен"] = "9";
  m_mMonthNames["окт"] = "10";
  m_mMonthNames["ноя"] = "11";
  m_mMonthNames["дек"] = "12";

  m_mMonthNames["январь"] = "1";
  m_mMonthNames["февраль"] = "2";
  m_mMonthNames["март"] = "3";
  m_mMonthNames["апрель"] = "4";
  m_mMonthNames["май"] = "5";
  m_mMonthNames["июнь"] = "6";
  m_mMonthNames["июль"] = "7";
  m_mMonthNames["август"] = "8";
  m_mMonthNames["сентябрь"] = "9";
  m_mMonthNames["октябрь"] = "10";
  m_mMonthNames["ноябрь"] = "11";
  m_mMonthNames["декабрь"] = "12";

  m_mMonthNames["января"] = "1";
  m_mMonthNames["февраля"] = "2";
  m_mMonthNames["марта"] = "3";
  m_mMonthNames["апреля"] = "4";
  m_mMonthNames["мая"] = "5";
  m_mMonthNames["июня"] = "6";
  m_mMonthNames["июля"] = "7";
  m_mMonthNames["августа"] = "8";
  m_mMonthNames["сентября"] = "9";
  m_mMonthNames["октября"] = "10";
  m_mMonthNames["ноября"] = "11";
  m_mMonthNames["декабря"] = "12";
}

CDateParser::CDateParser()
{
  FillMaps();
}

CDateParser::~CDateParser()
{
  m_mWeekdayNames.clear();
  m_mMonthNames.clear();
}

bool CDateParser::IsWhiteSpace(char ch)
{
  unsigned char chu = (unsigned char)ch;
  if (chu<=32) return true;
  return false;
}

bool CDateParser::IsTermSymbol(char ch)
{
  if (ch == '%') return true;
  if ((ch>='0') && (ch<='9')) return true;
  if ((ch>='a') && (ch<='z')) return true;
  if ((ch>='A') && (ch<='Z')) return true;
  if ((ch>='а') && (ch<='я')) return true;
  if ((ch>='А') && (ch<='Я')) return true;
  return false;
}

Stroka CDateParser::GetTerm(Stroka & s, int & pos)
{
  int nLength = s.size();
  while ((pos<nLength) && IsWhiteSpace(s[pos])) pos++;
  if (pos>=nLength) return "";
  int posstart = pos;
  while ((pos<nLength) && IsTermSymbol(s[pos])) pos++;
  return s.substr(posstart, pos-posstart);
}

Stroka CDateParser::GetSymbols(Stroka & s, int & pos)
{
  int nLength = s.size();
  Stroka sSymb = "";
  while ((pos<nLength) && !IsTermSymbol(s[pos])) {
    if (!IsWhiteSpace(s[pos])) sSymb+=s[pos];
    pos++;
  }
  return sSymb;
}

bool CDateParser::MatchNumber(Stroka s, int nMin, int nMax, int & nRes)
{
  int nNumb = 0;
  int pos = 0;
  int nLength = s.size();
  if (nLength<=0) return false;
  while (pos<nLength) {
    unsigned char chu = (unsigned char)s[pos];
    if ((chu<'0') || (chu>'9')) return false;
    nNumb = nNumb*10+(chu-'0');
    pos++;
  }
  if (nNumb<nMin) return false;
  if (nNumb>nMax) return false;
  nRes = nNumb;
  return true;
}

bool CDateParser::MatchDayOfWeek(Stroka s, int & nRes)
{
  if (s.find_first_of("0123456789") != Stroka::npos) return false;
  s.to_lower();
  S2S::iterator i = m_mWeekdayNames.find(s);
  if (i == m_mWeekdayNames.end())
    return false;
  Stroka sNumber = i->second;
  return MatchNumber(sNumber, 1, 7, nRes);
}

bool CDateParser::MatchYear(Stroka s, int & nRes)
{
  int n;
  bool bResult = MatchNumber(s, 1000, 9999, n);
  if (bResult) {
    nRes = n;
    return true;
  }
  bResult = MatchNumber(s, 40, 99, n);
  if (bResult) {
    nRes = n + 1900;
    return true;
  }
  bResult = MatchNumber(s, 00, 38, n);
  if (bResult) {
    nRes = n + 2000;
    return true;
  }
  return false;
}

bool CDateParser::MatchMonth(Stroka s, int & nRes)
{
  if (s.find_first_of("0123456789") != Stroka::npos)
     return MatchNumber(s, 1, 12, nRes);

  s.to_lower();
  S2S::iterator i = m_mMonthNames.find(s);
  if (i == m_mMonthNames.end())
    return false;
  Stroka sNumber = i->second;
  return MatchNumber(sNumber, 1, 12, nRes);
}

bool CDateParser::MatchDay(Stroka s, int & nRes)
{
  return MatchNumber(s, 1, 31, nRes);
}

bool CDateParser::MatchHour(Stroka s, int & nRes)
{
  return MatchNumber(s, 0, 23, nRes);
}

bool CDateParser::MatchMinute(Stroka s, int & nRes)
{
  return MatchNumber(s, 0, 59, nRes);
}

bool CDateParser::MatchSecond(Stroka s, int & nRes)
{
  return MatchNumber(s, 0, 59, nRes);
}

bool CDateParser::MatchHex(Stroka s, unsigned int & nHex)
{
  unsigned int n = 0;
  int i;
  if (s.size()!=8) return false;
  for (i=0;i<8;i++) {
    unsigned char chu = (unsigned char)s[i];
    int d = -1;
    if ((chu>='0') && (chu<='9')) d = chu-'0';
    if ((chu>='a') && (chu<='f')) d = chu-'a'+10;
    if ((chu>='A') && (chu<='F')) d = chu-'A'+10;
    if (d<0) return false;
    n = n * 16+d;
  }
  nHex = n;
  if (nHex>=0x80000000)
    return false;
  return true;
}

//  387c73a5                      %x
//  Fri, 01 Jan 1999 00:00:01     %w, %d %M %y %h:%n:%s
//  21.02.2000 23:33              %d.%m.%y %h:%n
//  21.02.2000 23:33:00           %d.%m.%y %h:%n:%s
//  21.02.2000,23:33              %d.%m.%y,%h:%n
//  21.02.2000,23:33:32           %d.%m.%y,%h:%n:%s
//  02/23/1999 23:00:00           %m/%d/%y %h:%n:%s
//  10-01-00   23:00:23           %m-%d-%y %h:%n:%s
//  10-01-00   23:00              %m-%d-%y %h:%n

bool CDateParser::MatchPattern(Stroka s, Stroka sPattern, tm & t)
{
  int posS = 0;
  int posP = 0;
  int nYear = -1;
  int nMonth = -1;
  int nDay = -1;
  int nDayOfWeek = -1;
  int nHour = -1;
  int nMinute = -1;
  int nSecond = -1;
  unsigned int nHex = 0;

  while (true) {
    Stroka sSymS = GetSymbols(s, posS);
    Stroka sSymP = GetSymbols(sPattern, posP);
    if (sSymS!=sSymP) return false;
    Stroka sTermS = GetTerm(s, posS);
    sTermS.to_lower();
    Stroka sTermP = GetTerm(sPattern, posP);
    sTermP.to_lower();
    if (sTermS.empty()) {
      if (sTermP.empty())
        break;
      else
        return false;
    } else if (sTermP.empty()) return false;
    if (sTermP == "%w") {
      if (!MatchDayOfWeek(sTermS, nDayOfWeek)) return false;
    } else if (sTermP == "%x") {
      if (!MatchHex(sTermS, nHex)) return false;
    } else if (sTermP == "%d") {
      if (!MatchDay(sTermS, nDay)) return false;
    } else if (sTermP == "%m") {
      if (!MatchMonth(sTermS, nMonth)) return false;
    } else if (sTermP == "%y") {
      if (!MatchYear(sTermS, nYear)) return false;
    } else if (sTermP == "%h") {
      if (!MatchHour(sTermS, nHour)) return false;
    } else if (sTermP == "%n") {
      if (!MatchMinute(sTermS, nMinute)) return false;
    } else if (sTermP == "%s") {
      if (!MatchMinute(sTermS, nSecond)) return false;
    } else if (sTermP != sTermS) return false;
  }
  if (nHex!=0) {
    try {
      //Р­С‚РѕС‚ РєСѓСЃРѕРє РєРѕРґР° РёР· 16-СЂРёС‡РЅРѕРіРѕ С‡РёСЃР»Р° nHex Р·Р°РїРѕР»РЅСЏРµС‚ РїРµСЂРµРјРµРЅРЅС‹Рµ
      //РіРѕРґР°, РјРµСЃСЏС†Р° Рё С‚.Рґ.
      time_t tHex = (time_t)nHex;
      struct tm * ptm = localtime(&tHex);
      nYear = ptm->tm_year+1900;
      nMonth = ptm->tm_mon + 1;
      nDay = ptm->tm_mday;
      nHour = ptm->tm_hour;
      nMinute = ptm->tm_min;
      nSecond = ptm->tm_sec;
    } catch (...) {
      return false;
    }
  }
  if ((nYear>0) && (nMonth<0) && (nDay<0)) {
    nMonth = 1;
    nDay = 1;
  }

  if ((nYear>0) && (nMonth>0) && (nDay<0)) {
    nDay = 1;
  }
  if ((nYear<=0) || (nMonth<=0) || (nDay<=0)) return false;
  if (nHour<0) {
    if ((nMinute>=0) || (nSecond>=0)) return false;
    nHour = 0;
    nMinute = 0;
    nSecond = 0;
  } else {
    if (nMinute<0) return false;
    if (nSecond<0) nSecond = 0;
  }
  try {
    //Р—Р°РїРѕР»РЅСЏРµРј СЃС‚СЂСѓРєС‚СѓСЂСѓ t РёР· РїРµСЂРµРјРµРЅРЅС‹С… nYear, nMonth, ....
    t.tm_hour = nHour;
    t.tm_isdst = -1;//_daylight;  //Р’С‹РєР»СЋС‡Р°РµРј Р°РІС‚РѕРїРµСЂРµСЃС‡РµС‚ С‚Р°Р№РјР·РѕРЅ Рё daylight saving
    t.tm_mday = nDay;
    t.tm_min = nMinute;
    t.tm_mon = nMonth - 1;
    t.tm_sec = nSecond;
    t.tm_year = nYear-1900;
    /*tm t2 = t;
    t2.tm_hour = 12;*/
    mktime(&t);
    /*t.tm_wday = t2.tm_wday;
    t.tm_yday = t2.tm_yday;*/

    //Р’СЃРµ РїРѕСЃС‡РёС‚Р°Р»Рё, РІ СЃС‚СЂСѓРєС‚СѓСЂРµ t РЅР°С…РѕРґРёС‚СЃСЏ РІСЂРµРјСЏ, РєРѕС‚РѕСЂС‹Рµ РјС‹ РІРѕР·РІСЂР°С‰Р°РµРј
    //Р•СЃР»Рё Сѓ РЅР°СЃ Р±С‹Р»Рё СѓСЃС‚Р°РЅРѕРІР»РµРЅС‹ РїРµСЂРµРјРµРЅРЅС‹Рµ С‚РёРїР° nDayOfWeek, nHex, nYear, nMonth Рё С‚.Рґ.
    //РїСЂРѕРІРµСЂСЏРµРј, СЃРѕРІРїР°РґР°РµС‚ Р»Рё РёС… Р·РЅР°С‡РµРЅРёРµ СЃ СЂРµР·СѓР»СЊС‚Р°С‚РѕРј РІ СЃС‚СѓРєС‚СѓСЂРµ t
    if (nDayOfWeek>=1) {
      int nDOW = t.tm_wday;
      if (nDOW<=0) nDOW = 7;
      if (nDayOfWeek != nDOW) return false;
    }
    if (nHex>0) {
      if ((time_t)nHex != (time_t)mktime(&t)) return false;
    }
    if ((nYear>0) && (t.tm_year+1900!=nYear)) return false;
    if ((nMonth>0) && (t.tm_mon + 1!=nMonth)) return false;
    if ((nDay>0) && (t.tm_mday!=nDay)) return false;
    if ((nHour>0) && (t.tm_hour!=nHour)) return false;
    if ((nMinute>0) && (t.tm_min!=nMinute)) return false;
    if ((nSecond>0) && (t.tm_sec!=nSecond)) return false;
  } catch (...) {
    return false;
  }
  return true;
}

bool CDateParser::MatchPattern(Stroka s, Stroka sPattern, CTime & t)
{
  tm myt;
  if (!MatchPattern(s,  sPattern, myt))
    return false;
  t = mktime(&myt);
  //РџСЂРѕРІРµСЂСЏРµРј, РІР»РµР·Р»Рѕ Р»Рё Р·РЅР°С‡РµРЅРёРµ РёР· tm РІ С‚РёРї CTime
  if (t == (time_t)-1)
    return false;
  return true;
}

int CDateParser::ParseDate(Stroka s, tm & t)
{
  if (MatchPattern(s, "%x", t)) return IT_DATETIME;
  if (MatchPattern(s, "%w, %d %M %y %h:%n:%s", t)) return IT_DATETIME;
  if (MatchPattern(s, "%w, %d %M %y %h:%n:%s msk", t)) return IT_DATETIME;
  if (MatchPattern(s, "%w, %d %M %y %h:%n:%s gmt", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d %M %y %h:%n:%s msk", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d %M %y %h:%n:%s gmt", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d.%m.%y %h:%n:%s", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d.%m.%y %h:%n", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d.%m.%y,%h:%n:%s", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d.%m.%y,%h:%n", t)) return IT_DATETIME;
  if (MatchPattern(s, "%d.%m.%y", t)) return IT_DATE;
  if (MatchPattern(s, "%m.%y", t)) return IT_YEARMONTH;
  if (MatchPattern(s, "%y", t)) return IT_YEAR;
  if (MatchPattern(s, "%m/%d/%y %h:%n:%s", t)) return IT_DATETIME;
  if (MatchPattern(s, "%m-%d-%y %h:%n:%s", t)) return IT_DATETIME;
  if (MatchPattern(s, "%m-%d-%y %h:%n", t)) return IT_DATETIME;
  if (MatchPattern(s, "%m-%d-%y", t)) return IT_DATE;
  if (MatchPattern(s, "%w %M %d %h:%n:%s %y", t)) return IT_DATETIME;
  return IT_ERROR;
}

int CDateParser::ParseDate(Stroka s, CTime & t)
{
  tm myt;
  int nReturn = ParseDate(s,  myt);
  if (nReturn == IT_ERROR)
    return IT_ERROR;
  t = mktime(&myt);
  //РџСЂРѕРІРµСЂСЏРµРј, РІР»РµР·Р»Рѕ Р»Рё Р·РЅР°С‡РµРЅРёРµ РёР· tm РІ С‚РёРї CTime
  if (t == (time_t)-1)
    return IT_ERROR;
  return nReturn;
}

//=======================================================
// fInt = 1 - РЅР°С‡Р°Р»Рѕ РёРЅС‚РµСЂРІР°Р»Р°, fInt = 2 - РєРѕРЅРµС† РёРЅС‚РµСЂРІР°Р»Р°
//bool CDateParser::GetLtm(Stroka s, CTime& t, int fInt)
//{
//  int r = ParseDate(s, t);
//  switch( r )
//  {
//  case IT_ERROR:
//    return false;
//  case IT_YEAR:
//    if( fInt == 2 ) //РљРѕРЅРµС† РёРЅС‚РµСЂРІР°Р»Р°
//    {
//      t = CTime(t.GetYear() + 1, 1, 1, 0, 0, 0);
//      t -= CTimeSpan(0, 0, 0, 1);
//    }
//    break;
//  case IT_YEARMONTH:
//    if( fInt == 2 ) //РљРѕРЅРµС† РёРЅС‚РµСЂРІР°Р»Р°
//    {
//      if( t.GetMonth() == 12 )
//        t = CTime(t.GetYear() + 1, 1, 1, 0, 0, 0);
//      else
//        t = CTime(t.GetYear(), t.GetMonth() + 1, 1, 0, 0, 0);
//      t -= CTimeSpan(0, 0, 0, 1);
//    }
//    break;
//  case IT_DATE:
//    if( fInt == 2 ) //РљРѕРЅРµС† РёРЅС‚РµСЂРІР°Р»Р°
//    {
//      t += CTimeSpan(1, 0, 0, 0);
//      t -= CTimeSpan(0, 0, 0, 1);
//    }
//    break;
//  case IT_DATETIME:
//    break;
//  }
//  return true;
//}

CDateParser dateParser;
