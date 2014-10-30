#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/timehelper.h>
#include <FactExtract/Parser/common/string_tokenizer.h>
#include "wordspair.h"
#include "primitive.h"
#include "wordsequence.h"

class CTimeLong: public CTime
{
public:
    CTimeLong(): CTime() { m_iYDif = 0; };
    CTimeLong(const CTimeLong& rTimeL): CTime((const CTime&)rTimeL) { m_iYDif = rTimeL.GetYDif(); };
    CTimeLong(const CTime& rTime): CTime(rTime) { m_iYDif = 0; };
    CTimeLong(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nYDif = 0);
    int GetRealYear() const;
    int GetRealDayOfWeek() const;
    int GetYDif() const { return m_iYDif; };
    Stroka GetDateString() const;
    Stroka GetSQLFormat() const;

protected:
    int m_iYDif;
public:
    const static int low_date = 1972;
    const static int high_date = 3000;
    static int CalculateYDif(int iYear, int& iYDif);
};

class CDateGroup : public CWordSequence
{
public:
    CDateGroup();
    virtual ~CDateGroup() {
    }

    bool BuildLemmas(bool IsDateField);
    Wtroka BuildMaxLemma(CTime &timeFromDateField);
    virtual Wtroka GetLemma() const;
    virtual Wtroka GetCapitalizedLemma() const;
    void Reset();
    bool IsY() const;
    bool IsMY() const;
    bool IsDMY() const;
    bool IsMH() const;
    Wtroka BuildDMDate();
    Wtroka BuildMYDate();
    Wtroka BuildDMYDate();
    Wtroka BuildYDate();
    //проверяем, что год укладывается в интервал datetime sql-я
    bool HasStorebaleYear() const;
    Wtroka  ToString() const;
    Wtroka  m_strIntervalDate;
    bool IsDM() const;

public:
    yvector<int>    m_iDay;
    yvector<int>    m_iMonth;
    yvector<int>    m_iYear;
    yvector<int>    m_iHour;
    yvector<int>    m_iMinute;
    int m_iMainWord;
    bool m_bDateField;
    yset<Wtroka>    m_strDateLemmas;
    yset<CTimeLong> m_SpanInterps;
    bool m_bUncertainYear;
};
