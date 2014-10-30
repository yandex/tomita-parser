#pragma once

#include <util/generic/vector.h>

#include "glrtable.h"

const size_t MaxSymbolNodesCount = 10000;

class CInputItemFactory;

struct CParsingChildrenSet
{
    const CGLRRuleInfo*    m_ReduceRule;
    yvector<size_t>        m_Items;
};

struct CSymbolNode
{
    CSymbolNode()
    {
        m_bDisabled = false;
        m_pItem = NULL;
    }

    void AddInputItem(CInputItem* pItem);

    //  a reference to a grammar
    int                m_SymbolNo;
    CInputItem*        m_pItem;
    void Free();

    TPair<int,int>        m_PathTop;

    bool m_bDisabled;

    yvector<CInputItem*> m_OtherItemVariants;

    //  a reference to all state nodes which are successors of this symbol node
    yvector<size_t>  m_NodeChildren;

    // m_ParseChildren[i] is one possible set of children of this symbol node in the parse tree
    // if m_ParseChildren.size ()>1, then  "local ambiguity packing" has occurred.
    // if m_ParseChildren.empty(), then this node is a leaf in the parsing tree.
    yvector<CParsingChildrenSet>    m_ParseChildren;

    bool IsLeaf() const
    {
        return m_ParseChildren.size() == 0;
    }

    // the period of the input stream, which is occupied by this symbol
    size_t    m_InputStart;
    size_t    m_InputEnd;

    // m_RecurseParentNodeNo is a redundant slot which poits to the parent of this symbol node
    // it can be calculated by the CStateNodem_SymbolChildren

    int        FindChild(size_t NodeNo) const
    {
        yvector<size_t>::const_iterator it = std::find (m_NodeChildren.begin(),m_NodeChildren.end(), NodeNo);
        if (it == m_NodeChildren.end()) return -1;
        return  it - m_NodeChildren.begin();
    };

    //все терминалы, которые участвовали в построении этой группы
    yvector<size_t> m_Terminals;
};

struct CStateNode
{
    CStateNode ()
        : m_StateNo(0)
        , m_bDisabled(false)
    {

    }

    int                            m_StateNo;
    yvector<size_t>                m_SymbolChildren;

    bool m_bDisabled;

    yset<size_t>                    m_ReducedTo;
};

struct CPendingReduction
{
    size_t                m_SymbolNodeNo;
    size_t                m_LastStateNodeNo;
    const CGLRRuleInfo* m_pReduceRule;

    CPendingReduction(const CGLRRuleInfo* reduceRule, size_t symbol, size_t lastState)
        : m_SymbolNodeNo(symbol)
        , m_LastStateNodeNo(lastState)
        , m_pReduceRule(reduceRule)
    {
    }

    bool operator < (const CPendingReduction& _X) const
    {
        if (m_SymbolNodeNo != _X.m_SymbolNodeNo)
            return m_SymbolNodeNo < _X.m_SymbolNodeNo;
        else if (m_LastStateNodeNo != _X.m_LastStateNodeNo)
            return m_LastStateNodeNo < _X.m_LastStateNodeNo;
        else
            return m_pReduceRule < _X.m_pReduceRule;
    }

};

struct CPendingShift
{

    int        m_SymbolNo;
    size_t    m_StateNodeNo;
    size_t    m_NextStateNo;

    CPendingShift(size_t state, size_t nextState, int symbol)
        : m_SymbolNo(symbol)
        , m_StateNodeNo(state)
        , m_NextStateNo(nextState)
    {
    }

    bool operator < (const CPendingShift& _X) const
    {
        if (m_StateNodeNo != _X.m_StateNodeNo)
            return m_StateNodeNo < _X.m_StateNodeNo;

        if (m_SymbolNo !=  _X.m_SymbolNo)
            return m_SymbolNo < _X.m_SymbolNo;

        return m_NextStateNo < _X.m_NextStateNo;
    }
};

class CGLRParser
{

    bool m_bDisableWORDSStateNodes;
    yset< TPair<size_t,size_t> > m_BuiltGROUPs;
    bool m_bSynAnOptions;

    bool TryToDisableSymbolNode(CSymbolNode& node, int iStart);

    // m_InputLevels[i] is the upper border of m_StateNodes , so that all nodes
    // [0..m_InputLevels[i]) from m_StateNodes were created for the input sequence till the i-th symbol
    // we created on the step i for the symbol i.
    yvector< size_t >        m_InputLevels;

    // m_PendingReductions is a set of pending reduction
    yset<CPendingReduction>    m_PendingReductions;

    // m_PendingShifts is a set of pending shifts
    yset<CPendingShift>        m_PendingShifts;

    // m_ActiveNodes is a set of active state nodes
    yset<size_t>                m_ActiveNodes;

    // Graph Structured Stack
    yvector<CStateNode>        m_StateNodes;

    int m_SENTNonTerminal;
    int m_WORDTerminal;
    int m_GROUPNonTerminal;
    TPair<int, int> m_MaxLastGroup;

    void    Actor(const yset<int>& Symbols);
    void    Reducer(const yset<int>& Symbols);
    void    Shifter(const yset<int>& Symbols);
    void    ReduceOnePath(const yset<int>& Symbols, const CPendingReduction& Reduction, const yvector<size_t>& Path);

    bool    CreateOrReuseNode(int StateNo, size_t& NewNodeNo);
    bool    HasPathOfLengthTwo(CInputItem* pNewItem,  int SourceNodeNo, int TargetNodeNo, size_t& SymbolNodeNo, const CSymbolNode& BetweenNode) const;
    void    UpdatePendingReductions(const CSLRCell& C);
    size_t    GetFirstNodeNoOfCurrentClosure() const;

    void FillRightPartItems(yvector<CInputItem*>& rightPartItems, const yvector<size_t>& Path);
    void AddTerminals(const yvector<size_t>& Path, CSymbolNode& newNode);

    void DisableWORDSSrtateNodesOld();
    void DisableWORDSSrtateNodes();

    size_t GetNumberOfClosureSet(size_t StateNodeNo) const;
    int m_iCreatedGroups;

public:
    bool    m_bRobust;

    bool    m_bBreakAfterMaxSymbolNodesCount;

    const    CGLRTable*        m_pTable;
    yvector<CSymbolNode>        m_SymbolNodes;

    ~CGLRParser() {}
    CGLRParser(CInputItemFactory* pInputItemFactory);
    CGLRParser(CInputItemFactory* factory, CGLRTable* table, bool breakaftermax, bool isrobust, int sent, int word, int group);

    void PutSynAnOptions(int SENTNonTerminal, int WORDTerminal, int GROUPNonTerminal);

    void Free();

    void InitGLRParser(const CGLRTable*    pTable);
    void CopyFrom(const CGLRParser& C);

    bool ParseSymbol(const yset<int>& Symbols, bool bStartPoint);
    bool    HasGrammarRootAtStart() const;

    CInputItemFactory* m_pInputItemFactory;

};
