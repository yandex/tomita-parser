#pragma once

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>

class CPeriodSubTree : public CWordsPair
{

public:
    typedef yset<CWordsPair*, SPeriodOrder>::iterator period_iter;
    typedef yset<CWordsPair*, SPeriodOrder>::reverse_iterator period_iter_rev;

    CPeriodSubTree(CWordsPair* pPeriod);
    CPeriodSubTree();
    virtual ~CPeriodSubTree(void);
    virtual CPeriodSubTree* CloneBase();
    void Free();
    void AddPeriod(CWordsPair* pPeriod);
    CWordsPair* GetMainPeriodBase()
    {
        if (m_Periods.size() == 0)
            ythrow yexception() << "Failed \"CClauseSubTree::GetMainClause\" ( m_Clauses is empty)";
        return *(--m_Periods.end());
    }
    void AddSubPeriods(CPeriodSubTree* pSubTree, CWordsPair* pPeriod);
    void EraseMainPeriod();

    CWordsPair* GetIncludedClauseRBase()
    {
        if (m_Periods.size() == 1)
            return NULL;
        period_iter main = m_Periods.end();
        --main;
        period_iter prev = main;
        --prev;
        if ((**main).LastWord() == (**prev).LastWord())
            return *prev;
        return NULL;
    }

    //все периоды, которые непосредственно включаются в главный
    void GetIncludedPeriodsBase(yvector<CWordsPair*>& periods);

    CWordsPair* GetIncludedClauseLBase()
    {
        if (m_Periods.size() == 1)
            return NULL;
        period_iter main = --m_Periods.end();
        period_iter prev = m_Periods.begin();

        if ((**main).FirstWord() == (**prev).FirstWord())
            return *prev;
        return NULL;
    }

    period_iter GetPeriodIterBegin()
    {
        return m_Periods.begin();
    }

    period_iter GetPeriodIterEnd()
    {
        return m_Periods.end();
    }

protected:
    virtual CPeriodSubTree* CreateNew() = 0;

protected:
    yset<CWordsPair*, SPeriodOrder> m_Periods;

};
