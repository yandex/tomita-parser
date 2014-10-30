// AhoKorasick.cpp : Defines the entry point for the console application.

#include <util/system/defaults.h>

#include "ahokorasick.h"
#include "simplegrammar.h"


CTrieNode::CTrieNode()
{
    m_FailureFunction = -1;
    m_Parent = -1;
    m_IncomingSymbol = -1;
    m_GrammarRuleNo = -1;
    m_Depth = 0;
    m_ChildrenIndex = 0;
};

void CTrieNode::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_FailureFunction);
    ::Save(buffer, m_Parent);
    ::Save(buffer, m_IncomingSymbol);
    ::Save(buffer, m_GrammarRuleNo);
    ::Save(buffer, m_ChildrenIndex);
    ::Save(buffer, m_Depth);
}

void CTrieNode::Load(TInputStream* buffer)
{
    ::Load(buffer, m_FailureFunction);
    ::Load(buffer, m_Parent);
    ::Load(buffer, m_IncomingSymbol);
    ::Load(buffer, m_GrammarRuleNo);
    ::Load(buffer, m_ChildrenIndex);
    ::Load(buffer, m_Depth);
}

void CTrieHolder::PrintChildren(size_t NodeNo) const
{
    printf("%" PRISZT, NodeNo);

    if (m_Nodes[NodeNo].m_FailureFunction != -1)
        printf(" failure(%i) ", m_Nodes[NodeNo].m_FailureFunction);
    printf(" -> ");

    size_t i=0;
    size_t Count =  GetChildrenCount(NodeNo);
    for (; i<Count; i++) {
        const CTrieRelation& p = GetChildren(NodeNo)[i];
        SymbolInformationType::const_iterator it = m_pSymbolInformation->find(p.m_RelationChar);
        assert (it != m_pSymbolInformation->end());
        printf("%i %s,", p.m_ChildNo, it->second.c_str());
    };

    printf("\n");

    for (; i<Count; i++)
        PrintChildren(GetChildren(NodeNo)[i].m_ChildNo);
};

int CTrieHolder::FindChild(int NodeNo, TerminalSymbolType RelationChar) const
{
    size_t Count =  GetChildrenCount(NodeNo);
    for (size_t i=0; i<Count; i++) {
        const CTrieRelation& p = GetChildren(NodeNo)[i];
        if (p.m_RelationChar == RelationChar)
            return p.m_ChildNo;
    };
    return -1;
};

int  CTrieHolder::GetTerminatedPeriodNext (int NodeNo) const
{
    if (NodeNo == 0) return -1;

    TerminalSymbolType RelationChar = m_Nodes[NodeNo].m_IncomingSymbol;
    int r =  m_Nodes[m_Nodes[NodeNo].m_Parent].m_FailureFunction;
    while (r != -1) {
        while ((r != -1) &&  (FindChild(r,RelationChar) == -1))
                    r = m_Nodes[r].m_FailureFunction;

        if (r != -1)
            return FindChild(r,RelationChar);
    };

    return -1;
};

void CTrieHolder::InitFailureFunction()
{
    assert (!m_Nodes.empty());

    yvector<size_t> BFS;
    BFS.push_back(0);

    //  going breadth-first search
    for (size_t i=0; i < BFS.size(); i++) {
        const CTrieNode& Parent = m_Nodes[BFS[i]];
        size_t ChildrenCount =  GetChildrenCount(BFS[i]);
        for (size_t j=0; j<ChildrenCount; j++) {
            const CTrieRelation& p = GetChildren(BFS[i])[j];

            //  going upside using following failure function in order to find the first node which
            //  has a way downward with symbol RelationChar
            int r =  Parent.m_FailureFunction;
            while ((r != -1) &&  (FindChild(r,p.m_RelationChar) == -1))
                r = m_Nodes[r].m_FailureFunction;

            if (r != -1) {
                //  the way is found
                m_Nodes[p.m_ChildNo].m_FailureFunction = FindChild(r,p.m_RelationChar);
            } else
                // no way is found, should start from the beginning
                m_Nodes[p.m_ChildNo].m_FailureFunction = 0;

            BFS.push_back(p.m_ChildNo);
        };
    };
};

void CTrieHolder::AddNode(const CTrieNode& T)
{
    m_Nodes.push_back(T);
    m_ChildrenAux.insert(m_ChildrenAux.end(),  GetAlphabetSize(), -1);
};

//#pragma optimize( "", off )
void CTrieHolder::CreateChildrenSequence(CTSI begin, CTSI end, size_t ParentNo, size_t WorkRuleNo)
{
    assert (begin < end);

    //  creating a child
    CTrieNode T;
    T.m_Parent = ParentNo;
    T.m_Depth =  m_Nodes[ParentNo].m_Depth+1;
    T.m_IncomingSymbol = *begin;
    assert (T.m_IncomingSymbol < (int)GetAlphabetSize());
    AddNode(T);

    //  registering this child
    size_t ChildNo = m_Nodes.size() - 1;
    assert (GetChildrenAux(ParentNo)[T.m_IncomingSymbol] == -1);
    GetChildrenAux(ParentNo)[T.m_IncomingSymbol] = ChildNo;

    //  inserting the next child
    if (end - begin > 1)
        CreateChildrenSequence(begin+1, end, ChildNo, WorkRuleNo);
    else
        m_Nodes[ChildNo].m_GrammarRuleNo = WorkRuleNo;
};

void CTrieHolder::ConvertAuxChildrenToNormal()
{
    m_Children.clear();
    m_Children.reserve(m_ChildrenAux.size());
    for (size_t NodeNo=0; NodeNo < m_Nodes.size(); NodeNo++) {
        m_Nodes[NodeNo].m_ChildrenIndex = m_Children.size();
        for (size_t i=0; i<GetAlphabetSize(); i++)
            if (GetChildrenAux(NodeNo)[i] != -1) {
                CTrieRelation R;
                R.m_ChildNo = GetChildrenAux(NodeNo)[i];
                R.m_RelationChar = i;
                m_Children.push_back(R);
            };
    };
    m_ChildrenAux.clear();
};

void CTrieHolder::CreateTrie(const yset< CWorkRule >& Patterns)
{
    assert(!Patterns.empty());
    m_Nodes.clear();
    m_ChildrenAux.clear();
    m_Nodes.reserve(2*Patterns.size());
    m_ChildrenAux.reserve(2*Patterns.size()*GetAlphabetSize());

    //  inserting root
    AddNode(CTrieNode ());

    yset< CWorkRule >::const_iterator iter, prev_iter;
    iter = prev_iter = Patterns.begin();
    size_t RuleNo = 0;
    CreateChildrenSequence(iter->m_RightPart.m_Items.begin(), iter->m_RightPart.m_Items.end(),  0, RuleNo);
    RuleNo++;

    for (iter++; iter != Patterns.end(); iter++, RuleNo++) {
        const CWorkRule& P = *iter;
        assert (!P.m_RightPart.m_Items.empty());

        //  Starter should be the node of the previous pattern, from which we should start
        //  current sequence.

        //  Example1:
        //  Previous = abcd
        //  Current  = abd
        //  We have graph (1) -a-> (2) -b-> (3) -c-> (4) -d-> (5)
        //  Starter should be pointed to node 3.

        //  Example2:
        //  Previous = abc
        //  Current  = abcd
        //  We have graph (1) -a-> (2) -b-> (3) -c-> (4)
        //  Starter should be pointed to node 4.

        size_t Starter = 0;
        size_t CharNo =0;
        for (; CharNo < P.m_RightPart.m_Items.size(); CharNo++) {
            if  ((CharNo == prev_iter->m_RightPart.m_Items.size())
                    ||  (P.m_RightPart.m_Items[CharNo] !=  (*prev_iter).m_RightPart.m_Items[CharNo])
                )
            break;

            Starter = GetChildrenAux(Starter)[P.m_RightPart.m_Items[CharNo]];
            assert ((int)Starter != -1);
        };

        if  (CharNo < P.m_RightPart.m_Items.size()) {

            CreateChildrenSequence(P.m_RightPart.m_Items.begin()+CharNo, P.m_RightPart.m_Items.end(),   Starter, RuleNo);
        } else {
            assert (P.m_RightPart.m_Items.size() ==  prev_iter->m_RightPart.m_Items.size());
            // a grammar can has structural ambiguity, which causes dublicates  in patterns
            //ErrorMessage( "a dublicate is found");
        };

        prev_iter = iter;
    };

    ConvertAuxChildrenToNormal();
};

//#pragma optimize( "", on )
void CTrieHolder::Create(const yset< CWorkRule >& Patterns, const SymbolInformationType* Info)
{
    m_pSymbolInformation = Info;
    m_AlphabetSize = m_pSymbolInformation->size();

    CreateTrie(Patterns);
    InitFailureFunction();
};

void CTrieHolder::UpdatePossibleOutputSymbolsbyOnState(size_t NodeNo,  yvector<bool>& PossibleOutputSymbols) const
{
    size_t Count =  GetChildrenCount(NodeNo);
    for (size_t i=0; i<Count; i++) {
        const CTrieRelation& p = GetChildren(NodeNo)[i];
        PossibleOutputSymbols[p.m_RelationChar] = true;
    };
};

void CTrieHolder::UpdatePossibleOutputSymbols (const yset<size_t>& CurrentStates, yvector<bool>& PossibleOutputSymbols) const
{
    PossibleOutputSymbols.resize(GetAlphabetSize(), false);

    for (yset<size_t>::const_iterator it = CurrentStates.begin();
            it != CurrentStates.end();
            it++
        ) {
        UpdatePossibleOutputSymbolsbyOnState(*it, PossibleOutputSymbols);

        for (int r = m_Nodes[(*it)].m_FailureFunction; r != -1; r = m_Nodes[r].m_FailureFunction)
            UpdatePossibleOutputSymbolsbyOnState(r, PossibleOutputSymbols);
    };

};

int CTrieHolder::NextState (int State, size_t TextPositionNo, TerminalSymbolType Symbol, yvector< COccurrence >& Occurrences) const
{
    while (State!=-1 &&  (FindChild(State,Symbol) == -1))
        State =  m_Nodes[State].m_FailureFunction;

    if (State == -1)
        State  = 0;
    else
        State = FindChild(State,Symbol);

    for (int r = State; r != -1; r = GetTerminatedPeriodNext(r)) {
        if (m_Nodes[r].m_GrammarRuleNo != -1) {
            COccurrence C;
            C.first = TextPositionNo - m_Nodes[r].m_Depth + 1;
            C.second =  TextPositionNo + 1;
            //C.m_pFinalNode = (void*)&(m_Nodes[State]);
            C.m_pInputItem = NULL;
            C.m_GrammarRuleNo = m_Nodes[r].m_GrammarRuleNo;
            C.m_bAmbiguous = false;
            Occurrences.push_back(C);
        }
    };

    return State;
};

void CTrieHolder::GetOccurrences (const TerminalString& Text, yvector< COccurrence >& Occurrences) const
{
    int r = 0;
    Occurrences.clear();
    for (size_t i=0; i<Text.size(); i++) {
        r = NextState(r,i,Text[i],Occurrences);
    };
};

void CTrieHolder::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_Nodes);
    ::Save(buffer, m_Children);
}

void CTrieHolder::Load(TInputStream* buffer)
{
    ::Load(buffer, m_Nodes);
    ::Load(buffer, m_Children);
}

void CTrieHolder::SetSymbolInformation(const SymbolInformationType* Info)
{
    m_pSymbolInformation = Info;
    m_AlphabetSize = m_pSymbolInformation->size();
}
