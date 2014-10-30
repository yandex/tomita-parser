#pragma once

#include <util/generic/vector.h>
#include <util/generic/map.h>

#include <FactExtract/Parser/common/serializers.h>
#include <FactExtract/Parser/common/utilit.h>

#include "commontypes.h"

typedef yvector<TerminalSymbolType> TerminalString;
typedef  TerminalString::const_iterator CTSI;
typedef  TerminalString::iterator TPI;
typedef     ymap<TerminalSymbolType, Stroka> SymbolInformationType;

struct COccurrence : public TPair<size_t, size_t>
{
    // a copy from   CPattern::m_GrammarRuleNo
    size_t                            m_GrammarRuleNo;

    bool                            m_bAmbiguous;
    const CInputItem*                m_pInputItem;
};

struct CTrieNode
{
    int                                m_FailureFunction;
    int                                m_Parent;
    TerminalSymbolType                m_IncomingSymbol;
    int                                m_GrammarRuleNo;
    ui32                            m_ChildrenIndex;
    ui8                                m_Depth;
    CTrieNode();

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

};

struct CTrieRelation
{
    TerminalSymbolType    m_RelationChar;
    int                    m_ChildNo;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_RelationChar);
        ::Save(buffer, m_ChildNo);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_RelationChar);
        ::Load(buffer, m_ChildNo);
    }

};

class CTrieHolder
{
    size_t m_AlphabetSize;
    const SymbolInformationType* m_pSymbolInformation;
public:
    yvector<CTrieNode> m_Nodes;

    void InitFailureFunction();
    void GetOccurrences (const TerminalString& Text, yvector< COccurrence >& Occurrences) const;
    void Create(const yset< CWorkRule >& Patterns, const SymbolInformationType* Info);
    void PrintChildren(size_t NodeNo) const;
    void UpdatePossibleOutputSymbols (const yset<size_t>& CurrentStates, yvector<bool>& PossibleOutputSymbols) const;
    int     NextState (int State, size_t TextPositionNo, TerminalSymbolType Symbol, yvector< COccurrence >& Occurrences) const;
    size_t GetAlphabetSize() const    { return m_AlphabetSize; };

    size_t  GetChildrenCount(size_t NodeNo)  const
    {  if (NodeNo+1 == m_Nodes.size())
                return m_Children.size() - m_Nodes[NodeNo].m_ChildrenIndex;
        else
                return m_Nodes[NodeNo+1].m_ChildrenIndex - m_Nodes[NodeNo].m_ChildrenIndex;
    };
    yvector<CTrieRelation>::const_iterator  GetChildren(size_t NodeNo) const { return m_Children.begin() + m_Nodes[NodeNo].m_ChildrenIndex; };
    int        FindChild(int NodeNo, TerminalSymbolType Child) const;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    void SetSymbolInformation(const SymbolInformationType* Info);

protected:
    yvector<int>                                    m_ChildrenAux;
    yvector<CTrieRelation >    m_Children;

    yvector<int>::iterator  GetChildrenAux(size_t NodeNo) { return m_ChildrenAux.begin() + NodeNo*GetAlphabetSize(); };
    yvector<int>::const_iterator  GetChildrenAux(size_t NodeNo) const { return m_ChildrenAux.begin() + NodeNo*GetAlphabetSize(); };

    void CreateChildrenSequence(CTSI begin, CTSI end, size_t ParentNo, size_t WorkRuleNo);
    void ConvertAuxChildrenToNormal();
    int  GetTerminatedPeriodNext (int NodeNo) const;
    void UpdatePossibleOutputSymbolsbyOnState(size_t NodeNo,  yvector<bool>& PossibleOutputSymbols) const;
    void CreateTrie(const yset< CWorkRule >& Patterns);
    void AddNode(const CTrieNode& T);
};
