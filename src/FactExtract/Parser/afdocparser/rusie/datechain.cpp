#include <util/string/cast.h>

#include "datechain.h"
#include "referencesearcher.h"

bool CDateChain::b_SpoolDateLog = false;

int ExtractDigits(i64 iDate, int from, int count)
{
    int iSign = 1;
    if (iDate < 0)
        iSign = 1;
    iDate = __abs64(iDate);
    i64 i1 = iDate / (ui64)pow((double)10, 16 - from - count);
    i64 i2 = i1 / (ui64)pow((double)10, count);
    i2 *= (ui64)pow((double)10, count);
    return (int)(iSign*(i1 - i2));

}

int GetWeekDay(i64 iDate)
{
    return ExtractDigits(iDate, 5, 1);
}

int GetDay(i64 iDate)
{
    return ExtractDigits(iDate, 6, 3);
}

int GetWeek(i64 iDate)
{
    return ExtractDigits(iDate, 9, 2);
}

int GetMonth(i64 iDate)
{
    return ExtractDigits(iDate, 11, 2);
}

int GetYear(i64 iDate)
{
    return ExtractDigits(iDate, 13, 3);
}

int GetHour(i64 iDate)
{
    return ExtractDigits(iDate, 3, 2);
}

int GetMinute(i64 iDate)
{
    return ExtractDigits(iDate, 1, 2);
}

CDateChain::CDateChain(const CWordVector& Words, const CWorkGrammar& gram, yvector<CWordSequence*>& foundFPCs,
                       yvector<SWordHomonymNum>& clauseWords, CReferenceSearcher* RefSearcher)
    : CTomitaItemsHolder(Words, gram, clauseWords)
    , CFPCInterpretations(foundFPCs, RefSearcher)
{
}

CInputItem* CDateChain::CreateNewItem(size_t SymbolNo, size_t iFirstItemNo)
{
    const CWord* pW = &m_Words.GetWord(m_FDOChainWords[iFirstItemNo + m_iCurStartWord]);

    THolder<CFloatingPrimitiveDateGroup> pNewGroup(
        new CFloatingPrimitiveDateGroup(pW, iFirstItemNo + m_iCurStartWord,
        m_GLRgrammar.m_UniqueGrammarItems.at(SymbolNo).m_vReplaceInterp));
    pNewGroup->SetActiveSymbol(SymbolNo);

    CheckSelfInterpretation(pNewGroup.Get());
    if (CheckAvailableDateItem(pNewGroup.Get()))
        return pNewGroup.Release();
    else
        return NULL;
}

void CDateChain::CheckSelfInterpretation(CFloatingPrimitiveDateGroup* pNewGroup)
{
    for (size_t i = 0; i < pNewGroup->m_vDateInterpretation.size(); i++)
        if (ITSELF_INTERP == pNewGroup->m_vDateInterpretation[i]) {
            int ifract = 0;
            const CHomonym* pH = pNewGroup->GetFirstMainHomonym();
            if (pH->HasKWType(GlobalDictsHolder->BuiltinKWTypes().Number, KW_DICT)) {
                CNumber* pNumb = (CNumber*)(pH->GetSourceWordSequence());
                for (size_t j = 0; j < pNumb->m_Numbers.size(); j++) {
                    double tt = 0;
                    modf(pNumb->m_Numbers[j], &tt);
                    pNewGroup->m_vDateInterpretation.push_back((int)tt);
                }
                continue;
            }

            for (CWord::SHomIt it = pNewGroup->GetMainWord()->IterHomonyms(); it.Ok(); ++it)
                if (GlobalGramInfo->FindCardinalNumber(it->GetLemma(), (ui64&)pNewGroup->m_vDateInterpretation[i], ifract) ||
                    GlobalGramInfo->FindOrdinalNumber(it->GetLemma(), (ui64&)pNewGroup->m_vDateInterpretation[i], ifract))
                    break;
        }
    for (int i = (int)pNewGroup->m_vDateInterpretation.size()-1; 0 <= i; i--)
        if (ITSELF_INTERP == pNewGroup->m_vDateInterpretation[i])
            pNewGroup->m_vDateInterpretation.erase(pNewGroup->m_vDateInterpretation.begin()+i);
}

CInputItem* CDateChain::CreateNewItem(size_t SymbolNo, const CRuleAgreement& agreements,
                                      size_t iFirstItemNo, size_t iLastItemNo,
                                      size_t iSynMainItemNo, yvector<CInputItem*>& children)
{
    if (iSynMainItemNo > children.size())
        ythrow yexception() << "Bad iSynMainItemNo in \"CFDOChain::CreateNewItem\"";

    SReduceConstraints constr(agreements, children, SymbolNo, iFirstItemNo, iSynMainItemNo, iLastItemNo - iFirstItemNo + 1);
    if (!ReduceCheckConstraintSet(&constr))
        return NULL;

    THolder<CFloatingSynDateGroup> pNewGroup(new CFloatingSynDateGroup((CGroup*)children[iSynMainItemNo], constr.Grammems, (int)SymbolNo));

    pNewGroup->SetPair(iFirstItemNo + m_iCurStartWord, iLastItemNo + m_iCurStartWord - 1);
    pNewGroup->m_Children = children;
    pNewGroup->AppendChildHomonymsFrom(children);

    pNewGroup->SaveCheckedAgreements(constr.Get().m_RulesAgrs, iSynMainItemNo, children);
    pNewGroup->SetCheckedConstraints(constr.Get());

    InitKwtypesCount(pNewGroup.Get(), children, constr.Get());
    InitUserWeight(pNewGroup.Get(), children, constr.Get());

    for (size_t i = 0; i < children.size(); i++) {
        if (((CGroup*)children[i])->IsPrimitive())
            MergeFloatingDates(pNewGroup->m_vDateInterpretation, ((CFloatingPrimitiveDateGroup*)children[i])->m_vDateInterpretation,
                                                                   IsComplexPeriod(SymbolNo));
        else
            MergeFloatingDates(pNewGroup->m_vDateInterpretation, ((CFloatingSynDateGroup*)children[i])->m_vDateInterpretation,
                                                                   IsComplexPeriod(SymbolNo));
    }

    return pNewGroup.Release();
}

void CDateChain::MergeFloatingDates(yvector<i64>& FlDateF, const yvector<i64>& FlDateS, bool bComplexPeriod)
{
    if ((0 == FlDateF.size() && 0 == FlDateS.size()) || (0 == FlDateS.size())) return;
    if (0 == FlDateF.size()) {
        FlDateF = FlDateS;  return;
    }

    yvector<i64> FlDateRes;
    for (size_t i = 0; i < FlDateF.size(); i++) {
        for (size_t j = 0; j < FlDateS.size(); j++) {
            if (FloatingDateIntersection(FlDateF[i], FlDateS[j]) || bComplexPeriod) {
                if (FlDateRes.end() == std::find(FlDateRes.begin(), FlDateRes.end(), FlDateF[i]))
                    FlDateRes.push_back(FlDateF[i]);
                if (FlDateRes.end() == std::find(FlDateRes.begin(), FlDateRes.end(), FlDateS[j]))
                    FlDateRes.push_back(FlDateS[j]);
            } else {
                i64 iD = CalculateFloatingDate(FlDateF[i], FlDateS[j]);
                if (ITSELF_INTERP == iD) {
                    if (FlDateRes.end() == std::find(FlDateRes.begin(), FlDateRes.end(), FlDateF[i]))
                        FlDateRes.push_back(FlDateF[i]);
                    if (FlDateRes.end() == std::find(FlDateRes.begin(), FlDateRes.end(), FlDateS[j]))
                        FlDateRes.push_back(FlDateS[j]);
                } else if (0 != iD) {
                    if (FlDateRes.end() == std::find(FlDateRes.begin(), FlDateRes.end(), iD))
                        FlDateRes.push_back(iD);
                }
            }
        }
    }

    FlDateF.clear();
    FlDateF = FlDateRes;
}

i64 CDateChain::CalculateFloatingDate(i64 iDateF, i64 iDateS)
{

    if (-1 == iDateF || -1 == iDateS) return (iDateF * iDateS);
    if (0 == iDateF || 0 == iDateS) return (iDateF * iDateS);
    if (ITSELF_INTERP < __abs64(iDateF) && ITSELF_INTERP < __abs64(iDateS)) {
        i64 res = ((__abs64(iDateF)+__abs64(iDateS))-ITSELF_INTERP);
        if (0 > iDateF || 0 > iDateS) return ((-1)*res); else return res;
    }
    if (ITSELF_INTERP > __abs64(iDateF) && ITSELF_INTERP > __abs64(iDateS)) return ITSELF_INTERP;

    if (ITSELF_INTERP < __abs64(iDateF)) return MultiplyFloatingDate(iDateS, iDateF);
    if (ITSELF_INTERP < __abs64(iDateS)) return MultiplyFloatingDate(iDateF, iDateS);

    return iDateF;
}

bool CDateChain::FloatingDateIntersection(i64 iDateF, i64 iDateS)
{
    if (ITSELF_INTERP > __abs64(iDateF) || ITSELF_INTERP > __abs64(iDateS)) return false;

    //bool bHasWeekDayF = GetWeekDay(iDateF)  != 0;
    //bool bHasWeekF = GetWeek(iDateF)  != 0;

    bool bHasTimeF = (GetHour(iDateF)  != 0) || (GetMinute(iDateF)  != 0);
    bool bHasDayF = GetDay(iDateF)  != 0;
    bool bHasMonthF = GetMonth(iDateF)  != 0;
    bool bHasYearF = GetYear(iDateF)  != 0;

    //bool bHasWeekDayS = GetWeekDay(iDateS)  != 0;
    //bool bHasWeekS = GetWeek(iDateS)  != 0;

    bool bHasTimeS = (GetHour(iDateS)  != 0) || (GetMinute(iDateS)  != 0);
    bool bHasDayS = GetDay(iDateS)  != 0;
    bool bHasMonthS = GetMonth(iDateS)  != 0;
    bool bHasYearS = GetYear(iDateS)  != 0;

    if ((bHasTimeF && bHasTimeS) ||
        (bHasDayF && bHasDayS) ||
        (bHasMonthF && bHasMonthS) ||
        (bHasYearF && bHasYearS))
        return true;

    return false;
}

i64 CDateChain::MultiplyFloatingDate(i64 iCoeff, i64 iDate)
{
    i64 res = 0;
    int i_pow = get_rank(__abs64(iDate)-ITSELF_INTERP);
    if (11 == i_pow) return iDate;
    if ((i_pow <= 5 && i_pow > 3)) {
        if (0 == ((__abs64(iDate)-ITSELF_INTERP)-ten_pow(13,3))) {
            if (iCoeff < 0 || iDate < 0)
                res = m_pReferenceSearcher->GetDocDate().GetMonth()-__abs64(iCoeff);
            else
                res = m_pReferenceSearcher->GetDocDate().GetMonth()+__abs64(iCoeff);
            if (12 < res)
                res = ten_pow((res-12),3)+ITSELF_INTERP+1;
            else if (0 > res)
                res = ten_pow((12+res),3)+ITSELF_INTERP+1;
            else if (0 == res)
                res = ten_pow(1,3)+ITSELF_INTERP;
            else
                res = ten_pow(res,3)+ITSELF_INTERP;
        } else
            res = __abs64(ten_pow(iCoeff, 7))+__abs64(iDate);
    } else
        res = (__abs64(iCoeff)*(__abs64(iDate)-ITSELF_INTERP))+ITSELF_INTERP;

    return (0 > iCoeff || 0 > iDate) ? ((-1)*res) : res;
}

CTimeLong CDateChain::CalculateExactDate(i64 iDate, bool bPeriod, int iNewYear, int iNewMonth, int iNewDay)
{
    if (0 == iDate || ITSELF_INTERP >= __abs64(iDate)
         || 0 == m_pReferenceSearcher->GetDocDate().GetTime()) return CTimeLong(m_pReferenceSearcher->GetDocDate());

    bool bMinus = (0 > iDate);
    int iDays = 0;

    try {
        Stroka sFlDate = ::ToString(__abs64(iDate));
        int iWeekDay = GetWeekDay(iDate);
        int iDay = GetDay(iDate);
        int iWeek = GetWeek(iDate);
        int iMonth = GetMonth(iDate);
        int iYear = GetYear(iDate);

        iDays += iWeek*7;
        if (14 == iMonth)
            iMonth = m_pReferenceSearcher->GetDocDate().GetMonth();
        if (13 == iMonth) {
            if (bMinus) {
                iMonth = m_pReferenceSearcher->GetDocDate().GetMonth()-1;
                if (0 == iMonth) { iMonth = 12; iYear++; }
            } else {
                iMonth = m_pReferenceSearcher->GetDocDate().GetMonth()+1;
                if (12 < iMonth) { iMonth = 1; iYear++; }
            }
        }
        if ((0 < iWeekDay)) {
            int iW = m_pReferenceSearcher->GetDocDate().GetDayOfWeek();
            if (bMinus) {
                if (iW - iWeekDay >= 0)
                    iDays += iW - iWeekDay;
                else
                    iDays += 7 - iWeekDay + iW;
            } else {
                if (iWeekDay - iW >= 0)
                    iDays += iWeekDay - iW;
                else
                    iDays += 7 - iW + iWeekDay;
            }
        }

        if ((iDay > 0) && (0 == iDays) && (0 == iMonth) && (0 == iYear) && (iNewDay != 0)) {
            iDays += iDay; iDay = 0;
        }
        if (bMinus && bPeriod && (0 == iDay) && (0 == iDays) && (0 < iMonth) && (0 == iYear))
            iYear++;

        if ((0 != iDays) && (iNewDay != 0)) {
            CTimeSpan tspan(iDays, 0, 0, 0);
            CTime tt(iNewYear, iNewMonth, iNewDay, 0, 0, 0);
            if (bMinus)
                tt -= tspan;
            else
                tt += tspan;
            iNewDay = tt.GetDay();
            iNewMonth = tt.GetMonth();
            iNewYear = tt.GetYear();
        }

        if (0 == iDay) iDay = iNewDay;
        if (0 == iMonth) iMonth = iNewMonth;
        if (0 == iYear) iYear = iNewYear;
        else {
            if (bMinus)
                iYear = iNewYear - iYear;
            else
                iYear = iNewYear + iYear;
        }

        int iHour = FromString<int>(TStringBuf(sFlDate).SubStr(3,2));
        int iMin = FromString<int>(TStringBuf(sFlDate).SubStr(1,2));
        CheckDate(iMonth, iDay, iHour, iMin);

        int iYDif = 0;
        iYear = CTimeLong::CalculateYDif(iYear, iYDif);
        return CTimeLong(iYear,iMonth, iDay, iHour, iMin, 0, iYDif);
    } catch (...) {
        //printf( "An error has occured in CDateChain::CalculateExactDate!\n" );
        return CTimeLong(m_pReferenceSearcher->GetDocDate());
    }
}

void CDateChain::Run()
{
    CInputSequenceGLR InputSequenceGLR(this);
    yvector< COccurrence > occurrences;
    RunParser(InputSequenceGLR, occurrences, false);

    for (size_t i = 0; i < occurrences.size(); i++) {
        SWordHomonymNum whStart = m_FDOChainWords[occurrences[i].first];

        int iLast = occurrences[i].second - 1;
        if (m_Words.GetWord(m_FDOChainWords[iLast]).IsEndOfStreamWord())
            iLast--;

        SWordHomonymNum whEnd = m_FDOChainWords[iLast];

        //РЅРµ Р±РµСЂРµРј РґР°С‚Сѓ РІ РєР°РІС‹С‡РєР°С… - СЃ С‡РµРіРѕ СЌС‚Рѕ РµРµ РІ РєР°РІС‹С‡РєР°С… РЅР°РїРёСЃР°Р»Рё?
        if (m_Words.GetWord(whStart).HasOpenQuote() &&
            m_Words.GetWord(whEnd).HasCloseQuote())
            continue;
        THolder<CDateGroup> pDateGroup(new CDateGroup());
        pDateGroup->PutWSType(DateTimeWS);
        pDateGroup->SetPair(m_Words.GetWord(whStart).GetSourcePair().FirstWord() , m_Words.GetWord(whEnd).GetSourcePair().LastWord());
        Wtroka lemma;
        lemma = m_Words.ToString(*pDateGroup);
        pDateGroup->SetCommonLemmaString(lemma);
        BuildSpans(occurrences[i], pDateGroup.Get());
        if (!pDateGroup->m_SpanInterps.empty())
            m_FoundFPCs.push_back(pDateGroup.Release());
    }

    FreeParser(InputSequenceGLR);
}

bool CDateChain::InterpPeriod(CFloatingSynDateGroup* pFloatingDate, CDateGroup* pDateGroup)
{
    if (pFloatingDate->m_vDateInterpretation.size() != 2)
        return false;
    int iYear1 = GetYear(pFloatingDate->m_vDateInterpretation[0]);
    int iYear2 = GetYear(pFloatingDate->m_vDateInterpretation[1]);
    CTime dateFromPeriod = m_pReferenceSearcher->GetDocDate();

    CTimeLong timeSpanInterp1;
    CTimeLong timeSpanInterp2;

    if ((iYear1 != 0) && (iYear2 == 0)) {
        int iMonth = GetMonth(pFloatingDate->m_vDateInterpretation[0]);
        int iYear = iYear1 + m_pReferenceSearcher->GetDocDate().GetYear();
        if (iMonth != 0)
            dateFromPeriod = CTime(iYear, iMonth, 1,0,0,0);
        timeSpanInterp1 = CalculateExactDate(pFloatingDate->m_vDateInterpretation[0], (pFloatingDate->m_vDateInterpretation.size()>1), m_pReferenceSearcher->GetDocDate().GetYear(), m_pReferenceSearcher->GetDocDate().GetMonth(), m_pReferenceSearcher->GetDocDate().GetDay());
        timeSpanInterp2 = CalculateExactDate(pFloatingDate->m_vDateInterpretation[1], (pFloatingDate->m_vDateInterpretation.size()>1), dateFromPeriod.GetYear(), dateFromPeriod.GetMonth(), 0);
    } else if ((iYear1 == 0) && (iYear2 != 0)) {
            int iMonth = GetMonth(pFloatingDate->m_vDateInterpretation[1]);
            int iYear = iYear2 + m_pReferenceSearcher->GetDocDate().GetYear();
            if (iMonth != 0)
                dateFromPeriod = CTime(iYear, iMonth, 1,0,0,0);
            timeSpanInterp1 = CalculateExactDate(pFloatingDate->m_vDateInterpretation[0], (pFloatingDate->m_vDateInterpretation.size()>1), dateFromPeriod.GetYear(), dateFromPeriod.GetMonth(), 0);
            timeSpanInterp2 = CalculateExactDate(pFloatingDate->m_vDateInterpretation[1], (pFloatingDate->m_vDateInterpretation.size()>1), m_pReferenceSearcher->GetDocDate().GetYear(), m_pReferenceSearcher->GetDocDate().GetMonth(), m_pReferenceSearcher->GetDocDate().GetDay());

        } else {
                timeSpanInterp1 = CalculateExactDate(pFloatingDate->m_vDateInterpretation[0], (pFloatingDate->m_vDateInterpretation.size()>1), m_pReferenceSearcher->GetDocDate().GetYear(), m_pReferenceSearcher->GetDocDate().GetMonth(), m_pReferenceSearcher->GetDocDate().GetDay());
                timeSpanInterp2 = CalculateExactDate(pFloatingDate->m_vDateInterpretation[1], (pFloatingDate->m_vDateInterpretation.size()>1), m_pReferenceSearcher->GetDocDate().GetYear(), m_pReferenceSearcher->GetDocDate().GetMonth(), m_pReferenceSearcher->GetDocDate().GetDay());
            }
    if (timeSpanInterp1.GetTime() != 0 && timeSpanInterp2.GetTime() != 0) {
        pDateGroup->m_SpanInterps.insert(timeSpanInterp1);
        pDateGroup->m_SpanInterps.insert(timeSpanInterp2);
    }
    return true;
}

void CDateChain::BuildSpans(const COccurrence& occur, CDateGroup* pDateGroup)
{
    //const yvector<CSymbolNode>& Output = ((CGLRParser*)occur.m_pFinalNode)->m_SymbolNodes;
    BuildFinalCommonGroup(occur, (CWordSequence*)pDateGroup);
    //С‡С‚РѕР±С‹ РЅРµ РїСЂРёРїРёСЃС‹РІР°С‚СЊ С†РµРїРѕС‡РєРµ "14-РіРѕ СЃРµРЅС‚СЏР±СЂСЏ" РїРѕСЂСЏРґРєРѕРІРѕРµ С‡РёСЃР»РёС‚РµР»СЊРЅРѕРµ, РІ РєР°С‡РµСЃС‚РІРµ
    //Р§Р  РґР»СЏ multiword
    pDateGroup->SetGrammems(THomonymGrammems());
    CFloatingSynDateGroup* pFloatingDate = ((CFloatingSynDateGroup*)occur.m_pInputItem);

    if (!InterpPeriod(pFloatingDate, pDateGroup)) {
        for (size_t i = 0; i < pFloatingDate->m_vDateInterpretation.size(); i++) {
            CTimeLong timeSpanInterp = CalculateExactDate(pFloatingDate->m_vDateInterpretation[i], (pFloatingDate->m_vDateInterpretation.size()>1), m_pReferenceSearcher->GetDocDate().GetYear(), m_pReferenceSearcher->GetDocDate().GetMonth(), m_pReferenceSearcher->GetDocDate().GetDay());
            if (0 != timeSpanInterp.GetTime())
                pDateGroup->m_SpanInterps.insert(timeSpanInterp);
        }
    }

    ClonePeriodMonthNP(pFloatingDate, pDateGroup->m_SpanInterps);
    CheckPeriods(pFloatingDate, pDateGroup->m_SpanInterps);

    if (0 == pFloatingDate->m_vDateInterpretation.size())
        CheckAnalyticYearDateGroup(pFloatingDate, pDateGroup);

}

void CDateChain::CheckPeriods(const CFloatingSynDateGroup* pFloatingDate, yset<CTimeLong>& r_spans)
{
    try {
        if (2 != pFloatingDate->m_vDateInterpretation.size() || 2 != r_spans.size()) return;

        //char buffer[65];
        Stroka sFDate = ::ToString(__abs64(pFloatingDate->m_vDateInterpretation[0]));
        Stroka sSDate = ::ToString(__abs64(pFloatingDate->m_vDateInterpretation[1]));

        int iFMonth = FromString<int>(TStringBuf(sFDate).SubStr(11,2));
        int iSMonth = FromString<int>(TStringBuf(sSDate).SubStr(11,2));

        int iFDay = FromString<int>(TStringBuf(sFDate).SubStr(6,3));
        int iSDay = FromString<int>(TStringBuf(sSDate).SubStr(6,3));

        if (!(iFMonth > iSMonth)
             && ((iFDay != 0) || (iSDay != 0))) return;

        CTimeLong tFDate, tSDate;
        yset<CTimeLong>::iterator it = r_spans.begin();
        if (r_spans.begin()->GetMonth() == iFMonth) {
            tFDate = *(it); it++;
            tSDate = *(it);
        } else {
            tSDate = *(it); it++;
            tFDate = *(it);
        }
        r_spans.clear();

        int iFYear = tFDate.GetRealYear();
        int iSYear = tSDate.GetRealYear();
        if (iFMonth > iSMonth && iFYear == iSYear) iFYear--; //iSYear++;
        if (iFDay == 0 && iSDay == 0 && iSMonth != 0 && iFMonth != 0 && tFDate.GetDay() == m_pReferenceSearcher->GetDocDate().GetDay()) {
            iFDay = 1;
            iSDay = MaxDayNumInMonth(tSDate.GetMonth());
        } else {
            iFDay = tFDate.GetDay();
            iSDay = tSDate.GetDay();
        }

        int iYDif = 0;
        iFYear = CTimeLong::CalculateYDif(iFYear, iYDif);
        r_spans.insert(CTimeLong(iFYear, tFDate.GetMonth(), iFDay, tFDate.GetHour(), tFDate.GetMinute(),0, iYDif));
        iYDif = 0;
        iSYear = CTimeLong::CalculateYDif(iSYear, iYDif);
        r_spans.insert(CTimeLong(iSYear, tSDate.GetMonth(), iSDay, tSDate.GetHour(), tSDate.GetMinute(),0, iYDif));
    } catch (...) {

    }
}

void CDateChain::ClonePeriodMonthNP(const CFloatingSynDateGroup* pFloatingDate, yset<CTimeLong>& r_spans)
{
    if (1 != r_spans.size() || 1 != pFloatingDate->m_vDateInterpretation.size()) return;

    Stroka sDate = ::ToString(__abs64(pFloatingDate->m_vDateInterpretation[0]));
    int iMonth = FromString<int>(TStringBuf(sDate).SubStr(11,2));
    int iDay = FromString<int>(TStringBuf(sDate).SubStr(6,3));
    int iWeekDay = FromString<int>(TStringBuf(sDate).SubStr(5,1));

    if (iMonth > 0 && 0 == iDay && 0 == iWeekDay) {
        CTimeLong tDate = *(r_spans.begin());
        r_spans.clear();
        int iYDif = 0;
        int iYear = CTimeLong::CalculateYDif(tDate.GetRealYear(), iYDif);
        r_spans.insert(CTimeLong(iYear, tDate.GetMonth(), 1, tDate.GetHour(), tDate.GetMinute(),0, iYDif));
        r_spans.insert(CTimeLong(iYear, tDate.GetMonth(), MaxDayNumInMonth(tDate.GetMonth()), tDate.GetHour(), tDate.GetMinute(),0, iYDif));
    }
}

int CDateChain::MaxDayNumInMonth(int iMonth) const
{
    switch (iMonth) {
        case 1: return 31;
        case 2: return 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
    }

    return 0;
}

void CDateChain::CheckDate(int& iMonth, int& iDay, int& iHour, int& iMin)
{
    if (iMonth > 12) iMonth = 12;
    if (iMonth < 0) iMonth = 0;
    if (iHour < 0) iHour = 0;
    if (iHour > 23) iHour = 23;
    if (iMin < 0) iMin = 0;
    if (iMin > 59) iMin = 59;
    if (iMonth == 0) return;
    if (iDay > MaxDayNumInMonth(iMonth)) iDay = MaxDayNumInMonth(iMonth);
};

bool CDateChain::IsComplexPeriod(int iSymbNo) const
{
    return ("ComplexPeriodUnion" == m_GLRgrammar.m_UniqueGrammarItems[iSymbNo].m_ItemStrId);
}

bool CDateChain::CheckAvailableDateItem(CFloatingPrimitiveDateGroup* pPrimDateGroup)
{
    if (m_GLRgrammar.m_UniqueGrammarTerminalIDs.find(pPrimDateGroup->GetActiveSymbol())->second != T_DATE)
        return true;

    const CWordSequence* pWordSeq = pPrimDateGroup->GetFirstMainHomonym()->GetSourceWordSequence();
    if (DateTimeWS != pWordSeq->GetWSType())
        return false;

    const CDateGroup* pDateGroup = dynamic_cast< const CDateGroup* >(pWordSeq);
    if (!pDateGroup)
        return false;

    Wtroka title;
    if (pWordSeq->HasAuxArticle())
        title = GlobalDictsHolder->GetAuxArticle(pWordSeq->GetAuxArticleIndex())->get_title();
    else if (pWordSeq->HasGztArticle())
        title = pWordSeq->GetGztArticle().GetTitle();

    static const Wtroka kWeakTime = UTF8ToWide("_недовремя");
    static const Wtroka kWeakDate = UTF8ToWide("_недодата");

    if (title == kWeakTime) {
        if (!pDateGroup->IsMH())
            return false;
        i64 iHour = pDateGroup->m_iHour[0];
        iHour = ten_pow(iHour, 11);
        i64 iMinute = pDateGroup->m_iMinute[0];
        iMinute = ten_pow(iMinute, 13);
        i64 iTime = iHour + iMinute;
        iTime += ITSELF_INTERP;
        pPrimDateGroup->m_vDateInterpretation.push_back(iTime);
        return true;
    } else if (pWordSeq->GetKWType() == GlobalDictsHolder->BuiltinKWTypes().Date) {
        pPrimDateGroup->m_vDateInterpretation.clear();
        if (pDateGroup->IsDMY()) {
            if (pDateGroup->m_iMonth.size() == pDateGroup->m_iYear.size() && pDateGroup->m_iMonth.size() == pDateGroup->m_iDay.size())
                for (size_t i = 0; i < pDateGroup->m_iYear.size(); ++i) {
                    i64 iYDif = m_pReferenceSearcher->GetDocDate().GetYear()-pDateGroup->m_iYear[i];
                    int sign = (iYDif > 0)? -1 : 1;
                    iYDif = __abs64(iYDif) + ITSELF_INTERP;
                    iYDif += ten_pow(pDateGroup->m_iMonth[i], 3);
                    iYDif += ten_pow(pDateGroup->m_iDay[i], 7);
                    iYDif *= sign;
                    pPrimDateGroup->m_vDateInterpretation.push_back(iYDif);
                }
            return true;
        } else if (pDateGroup->IsY()) {
            for (size_t i = 0; i < pDateGroup->m_iYear.size(); ++i) {
                i64 iYDif = m_pReferenceSearcher->GetDocDate().GetYear()-pDateGroup->m_iYear[i];
                if (0 == iYDif)
                    continue;
                if (iYDif < 0)
                    iYDif = __abs64(iYDif) + ITSELF_INTERP;
                else
                    iYDif = -1*(iYDif + ITSELF_INTERP);
                pPrimDateGroup->m_vDateInterpretation.push_back(iYDif);
            }
            return true;
        }
    } else if (title == kWeakDate && pDateGroup->IsDM()) {
        i64 iDate = 0;
        iDate += ten_pow(pDateGroup->m_iMonth[0], 3);
        iDate += ten_pow(pDateGroup->m_iDay[0], 7);
        iDate += ITSELF_INTERP;
        pPrimDateGroup->m_vDateInterpretation.push_back(iDate);
        return true;
    }

    return false;
}

bool CDateChain::CheckAnalyticYearDateGroup(CFloatingSynDateGroup* pFloatingDate, CDateGroup* pDateGroup)
{
    CWordSequence* pWordSeq = pFloatingDate->GetFirstMainHomonym()->GetSourceWordSequence();

    if (!pWordSeq)
        return false;

    if (DateTimeWS != pWordSeq->GetWSType())
        return false;
    CDateGroup* pAnalyticDateGroup = dynamic_cast< CDateGroup* >(pWordSeq);
    if (NULL == pAnalyticDateGroup)
        return false;

    *pDateGroup = *pAnalyticDateGroup;

    ConvertFromDMY2CTime(pAnalyticDateGroup, pDateGroup->m_SpanInterps);

    *pAnalyticDateGroup = *pDateGroup;

    return true;
}

void CDateChain::ConvertFromDMY2CTime(const CDateGroup* pDateGroup, yset<CTimeLong>& r_spans)
{
    int iYDif, iYear;
    if (pDateGroup->IsDMY()) {
        if (pDateGroup->m_iMonth.size() == pDateGroup->m_iYear.size() && pDateGroup->m_iMonth.size() == pDateGroup->m_iDay.size())
            for (size_t i = 0; i < pDateGroup->m_iYear.size(); i++) {
                iYDif = 0;
                iYear = CTimeLong::CalculateYDif(pDateGroup->m_iYear[i], iYDif);
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[i], pDateGroup->m_iDay[i], 0, 0, 0, iYDif));
            } else if (pDateGroup->m_iMonth.size() == pDateGroup->m_iDay.size()) {
            for (size_t i = 0; i < pDateGroup->m_iMonth.size(); i++) {
                iYDif = 0;
                iYear = CTimeLong::CalculateYDif(pDateGroup->m_iYear[0], iYDif);
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[i], pDateGroup->m_iDay[i], 0, 0, 0, iYDif));
            }
        } else {
            for (size_t i = 0; i < pDateGroup->m_iDay.size(); i++) {
                iYDif = 0;
                iYear = CTimeLong::CalculateYDif(pDateGroup->m_iYear[0], iYDif);
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[0], pDateGroup->m_iDay[i], 0, 0, 0, iYDif));
            }
        }
    } else if (pDateGroup->IsMY()) {
        if (pDateGroup->m_iMonth.size() == pDateGroup->m_iYear.size())
            for (size_t i = 0; i < pDateGroup->m_iYear.size(); i++) {
                iYDif = 0;
                iYear = CTimeLong::CalculateYDif(pDateGroup->m_iYear[i], iYDif);
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[i], 1, 0, 0, 0, iYDif));
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[i], MaxDayNumInMonth(pDateGroup->m_iMonth[i]), 0, 0, 0, iYDif));
            } else {
            for (size_t i = 0; i < pDateGroup->m_iMonth.size(); i++) {
                iYDif = 0;
                iYear = CTimeLong::CalculateYDif(pDateGroup->m_iYear[0], iYDif);
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[i], 1, 0, 0, 0, iYDif));
                r_spans.insert(CTimeLong(iYear, pDateGroup->m_iMonth[i], MaxDayNumInMonth(pDateGroup->m_iMonth[i]), 0, 0, 0, iYDif));
            }
        }
    } else if (pDateGroup->IsY()) {
        for (size_t i = 0; i < pDateGroup->m_iYear.size(); i++) {
            iYDif = 0;
            iYear = CTimeLong::CalculateYDif(pDateGroup->m_iYear[i], iYDif);
            r_spans.insert(CTimeLong(iYear, 1, 1, 0, 0, 0, iYDif));
            r_spans.insert(CTimeLong(iYear, 12, 31, 0, 0, 0, iYDif));
        }
    }
}
