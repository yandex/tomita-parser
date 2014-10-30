#pragma once

#include <util/generic/vector.h>
#include <util/ysaveload.h>

#include <FactExtract/Parser/common/inputitem.h>

struct CWorkRightRulePart
{
    yvector<TerminalSymbolType> m_Items;

    //  calculated by CGrammarItem::m_SynMain in  CParseGrammar::EncodeGrammar
    size_t m_SynMainItemNo;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_Items);
        ::Save(buffer, m_SynMainItemNo);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_Items);
        ::Load(buffer, m_SynMainItemNo);
    }

    inline bool operator == (const CWorkRightRulePart& _X) const
    {
        return m_Items == _X.m_Items && m_SynMainItemNo == _X.m_SynMainItemNo;
    }

    bool operator < (const CWorkRightRulePart& _X) const
    {
        if (m_Items != _X.m_Items)
            return m_Items < _X.m_Items;
        else
            return m_SynMainItemNo < _X.m_SynMainItemNo;
    }
};

struct CWorkRule
{
    size_t m_OriginalRuleNo;
    size_t m_LeftPart;
    CWorkRightRulePart m_RightPart;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_OriginalRuleNo);
        ::Save(buffer, m_LeftPart);
        ::Save(buffer, m_RightPart);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_OriginalRuleNo);
        ::Load(buffer, m_LeftPart);
        ::Load(buffer, m_RightPart);
    }

    inline bool operator == (const CWorkRule& _X) const
    {
        return m_LeftPart == _X.m_LeftPart && m_RightPart == _X.m_RightPart;
    }

    inline bool operator < (const CWorkRule& _X) const
    {
        //  First we should sort by the right part in order to make Aho-Korasick work;
        // for Aho-Korasick all patterns should be sorted lexicographically
        if (!(m_RightPart == _X.m_RightPart))
            return m_RightPart < _X.m_RightPart;
        else
            return m_LeftPart < _X.m_LeftPart;
    }
};

struct CLRItem
{
    const CWorkRule* m_pRule;
    size_t m_DotSymbolNo;

    inline CLRItem(const CWorkRule* rule, size_t dotSymbolNo)
        : m_pRule(rule), m_DotSymbolNo(dotSymbolNo)
    {
    }

    inline bool operator < (const CLRItem& _X) const
    {
        if (m_DotSymbolNo != _X.m_DotSymbolNo)
            return m_DotSymbolNo < _X.m_DotSymbolNo;
        else
            return m_pRule->m_OriginalRuleNo < _X.m_pRule->m_OriginalRuleNo;
    }

    inline bool operator == (const CLRItem& _X) const
    {
        return m_DotSymbolNo == _X.m_DotSymbolNo
            && m_pRule->m_OriginalRuleNo == _X.m_pRule->m_OriginalRuleNo;
    }

    inline bool IsFinished() const
    {
        return m_pRule->m_RightPart.m_Items.size() == m_DotSymbolNo;
    }

    inline int GetSymbolNo() const
    {
        return m_pRule->m_RightPart.m_Items[m_DotSymbolNo];
    }
};

