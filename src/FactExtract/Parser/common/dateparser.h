#pragma once

#include <util/generic/map.h>
#include <util/generic/stroka.h>

#define IT_ERROR     0
#define IT_YEAR      1
#define IT_YEARMONTH 2
#define IT_DATE      3
#define IT_DATETIME  4


#include "timehelper.h"


class CDateParser
{
public:
    CDateParser();
    virtual ~CDateParser();
  int ParseDate(Stroka s, CTime & t);
  int ParseDate(Stroka s, tm & t);
  bool MatchPattern(Stroka s, Stroka sPattern, tm & t);
  bool MatchPattern(Stroka s, Stroka sPattern, CTime & t);
  //bool GetLtm(Stroka s, CTime& t, int fInt);
protected:
  void FillMaps(void);
  typedef ymap<Stroka, Stroka> S2S;
    S2S m_mWeekdayNames;
    S2S m_mMonthNames;
  bool IsWhiteSpace(char ch);
  bool IsTermSymbol(char ch);
  Stroka GetTerm(Stroka & s, int & pos);
  Stroka GetSymbols(Stroka & s, int & pos);
  bool MatchNumber(Stroka s, int nMin, int nMax, int & nRes);
  bool MatchDayOfWeek(Stroka s, int & nRes);
  bool MatchYear(Stroka s, int & nRes);
  bool MatchMonth(Stroka s, int & nRes);
  bool MatchDay(Stroka s, int & nRes);
  bool MatchHour(Stroka s, int & nRes);
  bool MatchMinute(Stroka s, int & nRes);
  bool MatchSecond(Stroka s, int & nRes);
  bool MatchHex(Stroka s, unsigned int & nHex);
};

extern CDateParser dateParser;
