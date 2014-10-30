#include <util/system/defaults.h>

#include  "lr0.h"
#include  "lr-items.h"
#include "simplegrammar.h"

bool InitReduceActions(CLR0Table& P, const yset<CLRItem>& ItemSet, yvector <CTableItem>& NewState)
{
    size_t  Count =0;
    for (yset<CLRItem>::const_iterator it = ItemSet.begin(); it != ItemSet.end(); it++)
        if    (it->IsFinished()
                &&    it->m_pRule->m_LeftPart+1 != P.m_pWorkGrammar->m_UniqueGrammarItems.size()
            ) {
            for (size_t i=0; i < NewState.size(); i++) {
                if (NewState[i].m_GotoState != -1) {
                    // a grammar contains a shift/reduce conflict
                    return false;
                };
                NewState[i].m_pReduceRule = it->m_pRule;
            };
            Count++;

        };

    return true;
};

CLR0Table::CLR0Table()
{
    m_pWorkGrammar = 0;

};

bool CLR0Table::BuildLRTable()
{
    assert (m_pWorkGrammar != 0);
    CLRCollectionSet S;
    S.Compute(m_pWorkGrammar);

    m_ParsingTable.clear();

    for (size_t i=0;  i<S.m_ItemSets.size(); i++) {
        //  add a new line in the table (all symbols + non-existing terminal)
        yvector <CTableItem> NewState (m_pWorkGrammar->m_UniqueGrammarItems.size());

        //  going  through all symbols, except the last one which is a symbol for non-existing terminal
        for (size_t SymbolNo = 0; SymbolNo<m_pWorkGrammar->m_UniqueGrammarItems.size(); SymbolNo++) {
            size_t NextState;
            if (!S.GetGotoValue(SymbolNo,i, NextState)) continue;

            if (NewState[SymbolNo].m_GotoState != -1) {
                fprintf (stderr, "the input grammar contains a reduce/reduce conflict\n");
                return false;
            };
            NewState[SymbolNo].m_GotoState = NextState;

        };

        if (!InitReduceActions(*this, S.m_ItemSets[i], NewState)) {
            fprintf (stderr, "the input grammar contains a shift/reduce  conflict\n");
            return false;
        };

        m_ParsingTable.push_back(NewState);

    };
    return true;
};

//========================================================================
//====================     CLR0Parser    =================================
//========================================================================
CLR0Parser::CLR0Parser()
{
    CLR0StackItem I;
    I.m_StateNo = 0;
    I.m_Start = 0;
    I.m_End = 0;
    m_StateStack.push_back(I);
};

CLR0Parser::CLR0Parser(size_t Offset)
{
    CLR0StackItem I;
    I.m_StateNo = 0;
    I.m_Start = Offset;
    I.m_End = Offset;
    m_StateStack.push_back(I);

};

bool CLR0Parser::ShiftAction(size_t InputSymbolOffset, int InputSymbol, const CLR0Table& LR0Table)
{
    const yvector< yvector <CTableItem> > & ParsingTable = LR0Table.m_ParsingTable;
    assert (!m_StateStack.empty());
    int State = m_StateStack.back().m_StateNo;
    assert (State < (int)ParsingTable.size());
    assert (InputSymbol < (int)ParsingTable[State].size());
    const CTableItem& T = ParsingTable[State][InputSymbol];
    if (T.m_GotoState == -1) return false;
    // a shift action
    CLR0StackItem I;
    I.m_StateNo = T.m_GotoState;
    I.m_Start = InputSymbolOffset;
    I.m_End = InputSymbolOffset+1;
    m_StateStack.push_back(I);
    return true;
};

void CLR0Parser::ReduceAction(const CLR0Table& LR0Table)
{
    const yvector< yvector <CTableItem> > & ParsingTable = LR0Table.m_ParsingTable;
    size_t InputSymbolOffset = m_StateStack.back().m_End;

    assert (!m_StateStack.empty());
    int State = m_StateStack.back().m_StateNo;
    assert (State < (int)ParsingTable.size());
    assert (!ParsingTable[State].empty());
    const CTableItem& T = ParsingTable[State][0];
    if (T.m_pReduceRule == 0) {
        assert (false);
        return;
    };

    //  delete a projection of nonterminal from the stack
    size_t RuleSize = T.m_pReduceRule->m_RightPart.m_Items.size();
    assert (m_StateStack.size() > RuleSize);
    m_StateStack.erase(m_StateStack.end() - RuleSize, m_StateStack.end());
    assert (!m_StateStack.empty());

    {
        // update output
        CLR0OutputItem OutputItem;
        OutputItem.m_pRule = T.m_pReduceRule;
        OutputItem.m_End = InputSymbolOffset;
        OutputItem.m_Start = m_StateStack.back().m_End;
        m_Output.push_back(OutputItem);
    }

    {
        // push a new state to stack
        const CTableItem& T1 = ParsingTable[m_StateStack.back().m_StateNo][T.m_pReduceRule->m_LeftPart];
        assert (T1.m_GotoState != -1);

        CLR0StackItem I;
        I.m_Start = m_StateStack.back().m_End;
        I.m_End = InputSymbolOffset;
        I.m_StateNo = T1.m_GotoState;
        m_StateStack.push_back(I);
    }

};

bool CLR0Parser::ShouldReduce (const CLR0Table& LR0Table) const
{
    const yvector< yvector <CTableItem> > & ParsingTable = LR0Table.m_ParsingTable;
    int State = m_StateStack.back().m_StateNo;
    assert (!ParsingTable[State].empty());
    const CTableItem& T = ParsingTable[State][0];
    return (T.m_pReduceRule != 0);
};

