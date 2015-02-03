#include "inputsequence.h"
#include "inputitemfactory.h"


CInputSequence::CInputSequence(void)
{
}

CInputSequence::~CInputSequence(void)
{
}

void CInputSequence::AddTerminal(const CInputItem* pTerm)
{
    m_InputTerminals.push_back(pTerm);
}

void CInputSequence::FindOccurrencesWithTrie (const CWorkGrammar& G, yvector< COccurrence >& Occurrences)
{
    yset<size_t> CurrentStates, NextStates;
    CurrentStates.insert(0);
    Occurrences.clear();

    for (size_t WordNo=0; WordNo < m_InputTerminals.size(); WordNo++) {
        const CInputItem& W = *m_InputTerminals[WordNo];

        yset<size_t>::const_iterator it = CurrentStates.begin();
        for (; it != CurrentStates.end(); it++) {
               yset<TerminalSymbolType>::const_iterator a_it = W.m_AutomatSymbolInterpetationUnion.begin();
               for (; a_it != W.m_AutomatSymbolInterpetationUnion.end(); a_it++) {
                    int q = G.m_TrieHolder.NextState(*it, WordNo,*a_it, Occurrences);
                    NextStates.insert(q);
               }
        }
        CurrentStates.swap(NextStates);
        NextStates.clear();

        if (CurrentStates.empty())
            CurrentStates.insert(0);
    }
}

/**********************CInputSequenceGLR********************************/

size_t CInputSequenceGLR::ApplyGLR_Parsing (const CWorkGrammar& G, const yset<size_t>& StartPoints, yvector< COccurrence >& Occurrences)
{
    if (StartPoints.empty()) return 0;
    m_Parsers.push_back(CGLRParser(m_pInputItemFactory));
    CGLRParser& Parser = m_Parsers.back();
    Parser.m_bRobust = true;
    Parser.InitGLRParser(&G.m_GLRTable);

    for (size_t CurrWordNo = 0; CurrWordNo < m_InputTerminals.size(); CurrWordNo++) {
        const CInputItem& W = *m_InputTerminals[CurrWordNo];
        //size_t size = Parser.m_SymbolNodes.size();

        if (Parser.ParseSymbol(W.m_AutomatSymbolInterpetationUnion, StartPoints.find(CurrWordNo)!=StartPoints.end())) {
            //  if it is the last word of the sentence and the lst symbol was parsed successfully
            //  then we should add an end of stream symbol to the input stream and finish parsing
            if    (CurrWordNo+1 == m_InputTerminals.size()) {
                yset<int> EndOfSentenceSet;
                EndOfSentenceSet.insert(G.GetEndOfStreamSymbol());
                Parser.ParseSymbol(EndOfSentenceSet, false);
                break;
            };
        } else
            break;
    };

    bool bFound = false;
    const yvector<CSymbolNode>& Output = Parser.m_SymbolNodes;
    for (size_t i=0; i<Output.size(); i++) {
        if    (G.m_UniqueGrammarItems[Output[i].m_SymbolNo].m_bGrammarRoot
                &&    !Output[i].m_bDisabled
                &&    (Output[i].m_SymbolNo != (int)G.GetEndOfStreamSymbol())
            ) {
            COccurrence C(Output[i].m_InputStart, Output[i].m_InputEnd, i, false, Output[i].m_pItem);
            Occurrences.push_back(C);
            bFound = true;
        };
    };
    if (!bFound) {
        Parser.Free();
        m_Parsers.pop_back();
        return 0;
    }
    return Parser.m_SymbolNodes.size();
}

void    CInputSequenceGLR::FreeParsers()
{
    ylist<CGLRParser>::iterator it = m_Parsers.begin();
    for (; it != m_Parsers.end(); it++) {
        CGLRParser& parser = *it;
        parser.Free();
    }
    m_Parsers.clear();
}

void CInputSequenceGLR::Process (const CWorkGrammar& G, bool /*bDumpOccurs*/, yvector< COccurrence >& Occurrences, bool /*allowAmbiguity*/)
{
    // first, we should  find all possible occurrences of prefixes in this sentence
    // since FindOccurrencesWithTrie works at O(n) then we should try to build from each
    // found occcurrence of a prefix a full occurrence of the grammar root
    yvector< COccurrence > Prefixes;
    FindOccurrencesWithTrie(G, Prefixes);

    yset<size_t> StartPoints;
    for (size_t OccurNo =0; OccurNo < Prefixes.size(); OccurNo++)
        StartPoints.insert(Prefixes[OccurNo].first);

    m_pInputItemFactory->SetCurStartWord(0);
    ApplyGLR_Parsing(G, StartPoints, Occurrences);
};

/**********************CInputSequenceGLR********************************/
CSynAnInputSequence::CSynAnInputSequence(const CWorkGrammar& G, CInputItemFactory* pInputItemFactory):
    CInputSequenceGLR(pInputItemFactory),
    m_Parser(pInputItemFactory),
    m_Grammar(G)

{
    m_Parser.InitGLRParser(&G.m_GLRTable);
}

void    CSynAnInputSequence::Process ()
{
    yset<int> EndOfSentenceSet;
    EndOfSentenceSet.insert(m_Grammar.GetEndOfStreamSymbol());

    for (size_t WordNo = 0; WordNo < m_InputTerminals.size(); WordNo++) {
        const CInputItem& W = *m_InputTerminals[WordNo];
        if (!m_Parser.ParseSymbol(W.m_AutomatSymbolInterpetationUnion, false))
            ythrow yexception() << "Can't parse symbol!";
        if    (WordNo+1 == m_InputTerminals.size()) {
            m_Parser.ParseSymbol(EndOfSentenceSet, false);
        };
    };
}
