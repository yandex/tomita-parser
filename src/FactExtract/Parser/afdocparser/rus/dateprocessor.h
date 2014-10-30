#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/common/datetime.h>
#include "wordvector.h"


class CDateProcessor
{
public:
    CDateProcessor(CWordVector& words, CWordSequence::TSharedVector& wordSequences)
        : m_Words(words)
        , m_wordSequences(wordSequences)
    {
    }

    virtual ~CDateProcessor(void);
    virtual void Run();

    yvector<CDateGroup> m_Dates;

protected:
    bool OnlyOneDigit(size_t w);
    bool BuildDigitDate(size_t w, CDateGroup& date);
    bool BuildDate(size_t from, CDateGroup& date);
    bool BuildTime(size_t from, CDateGroup& date);
    bool BuildDMYDate(size_t from, CDateGroup& date);
    bool BuildMYDate(size_t from, CDateGroup& date);
    bool BuildDMDate(size_t from, CDateGroup& date);
    bool BuildDigitalDMDate(size_t from, CDateGroup& date);
    int  GetMohth(const Wtroka& str);
    bool IsYearWord(size_t w);
    bool IsYearNumber(ui64& val);
    bool IsYear(size_t w, CDateGroup& date, bool& bHasYearWord, Wtroka& strPostfix,  int& iOriginalYear);
    bool CheckYearWithoutYearWord(size_t from, CDateGroup& date, Wtroka strPostfix);
    bool IsDay(size_t w, CDateGroup& date);
    bool IsMonth(size_t w, CDateGroup& date);
    bool BuildYDate(size_t from, CDateGroup& date);
    void FindPrevMDates(size_t iDate);
    bool FindPrevDMDates(size_t iDate);
    void FindPrevDDates(size_t iDate);
    void BuildDatesIntervals();
    void UniteDates();
    bool TryToBuildDMYDateInterval(size_t i);
    bool TryToBuildDDateInterval(yvector<CDateGroup>& new_dates, size_t i);
    bool TryToBuildMDateInterval(yvector<CDateGroup>& new_dates, size_t i);
    bool TryToBuildDMDateInterval(size_t i);
    bool TryToBuildYDateInterval(yvector<CDateGroup>& new_dates, size_t i);
    bool HasStorableYearNumber(CDateGroup& date);
    void AddToWordSequences();
    bool HasTomuNazad(size_t  from);
    int FindNeighbourDate(int iD);

protected:
    CWordVector& m_Words;
    CWordSequence::TSharedVector& m_wordSequences;
};
