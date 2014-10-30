#pragma once

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/map.h>

#include "commontypes.h"
struct CWorkGrammar;

struct CStateAndSymbol
{
    int m_SymbolNo;
    size_t m_StateNo;

    CStateAndSymbol(int symbolNo, size_t stateNo)
        : m_SymbolNo(symbolNo),    m_StateNo(stateNo)
    {
    }

    bool operator < (const CStateAndSymbol& _X) const
    {
        if (m_SymbolNo != _X.m_SymbolNo)
            return m_SymbolNo < _X.m_SymbolNo;
        else
            return m_StateNo < _X.m_StateNo;
    }
};

struct CLRCollectionSet
{
    //  item sets  which can be generated from the grammar
    yvector< yset<CLRItem> > m_ItemSets;

    // goto function m_GotoFunction[i] = k means that from
    //  Itemset i.m_StateNo we can go to Itemset k over symbol i.m_SymbolNo.
    ymap< CStateAndSymbol, size_t> m_GotoFunction;

    void Compute(const CWorkGrammar* pWorkGrammar);
    inline bool GetGotoValue(int SymbolNo, size_t StateNo, size_t& ResultStateNo) const
    {
        ymap<CStateAndSymbol, size_t>::const_iterator it = m_GotoFunction.find(CStateAndSymbol(SymbolNo, StateNo));
        if (it == m_GotoFunction.end())
            return  false;
        ResultStateNo = it->second;
        return  true;
    }

    void SaveStateCollection(Stroka GrammarFileName, const CWorkGrammar* p_WorkGrammar) const;

};

