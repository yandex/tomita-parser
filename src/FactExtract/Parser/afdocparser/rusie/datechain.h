#pragma once

#include <util/generic/vector.h>

#include "tomitaitemsholder.h"
#include "floatingdategroup.h"
#include "dictsholder.h"

class CDateChain : public CTomitaItemsHolder,
                   public CFPCInterpretations
{
public:
    CDateChain(const CWordVector& Words, const CWorkGrammar& gram, yvector<CWordSequence*>& foundFPCs,
               yvector<SWordHomonymNum>& clauseWords, CReferenceSearcher* RefSearcher);

    virtual ~CDateChain() {};

    void Run();

    virtual CInputItem* CreateNewItem(size_t SymbolNo, const CRuleAgreement& agreements,
                                        size_t iFirstItemNo, size_t iLastItemNo,
                                        size_t iSynMainItemNo, yvector<CInputItem*>& children);

    virtual CInputItem* CreateNewItem(size_t SymbolNo, size_t iFirstItemNo);

protected:
    void CheckSelfInterpretation(CFloatingPrimitiveDateGroup* pNewGroup);
    void MergeFloatingDates(yvector<i64>& FlDateF, const yvector<i64>& FlDateS, bool bComplexPeriod);
    i64 CalculateFloatingDate(i64 iDateF, i64 iDateS);
    i64 MultiplyFloatingDate(i64 iDateF, i64 iDateS);
    bool FloatingDateIntersection(i64 iCoeff, i64 iDate);
    CTimeLong CalculateExactDate(i64 iDate, bool bPeriod, int iNewYear, int iNewMonth, int iNewDay);
    bool InterpPeriod(CFloatingSynDateGroup* pFloatingDate);
    void BuildSpans(const COccurrence& occur, CDateGroup* pDateGroup);
    bool InterpPeriod(CFloatingSynDateGroup* pFloatingDate, CDateGroup* pDateGroup);
    void CheckPeriods(const CFloatingSynDateGroup* pFloatingDate, yset<CTimeLong>& r_spans);
    void ClonePeriodMonthNP(const CFloatingSynDateGroup* pFloatingDate, yset<CTimeLong>& r_spans);
    int MaxDayNumInMonth(int iMonth) const;
    void CheckDate(int& iMonth, int& iDay, int& iHour, int& iMin);
    bool IsComplexPeriod(int iSymbNo) const;
    //void SpoolDateToLogFile(const CFloatingSynDateGroup* pFloatingDate, const yset<CTimeLong>& r_spans);
    bool CheckAvailableDateItem(CFloatingPrimitiveDateGroup* pPrimDateGroup);
    bool CheckAnalyticYearDateGroup(CFloatingSynDateGroup* pFloatingDate, CDateGroup* pDateGroup);
    void ConvertFromDMY2CTime(const CDateGroup* pDateGroup, yset<CTimeLong>& r_spans);

public:
    static bool b_SpoolDateLog;
};
