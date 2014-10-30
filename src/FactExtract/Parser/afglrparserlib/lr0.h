#pragma once

#include <util/generic/vector.h>
#include "commontypes.h"

struct CWorkGrammar;

struct CTableItem
{
    int                    m_GotoState;
    const CWorkRule*    m_pReduceRule;
    CTableItem ()
    {
        m_pReduceRule = 0;
        m_GotoState = -1;
    };
};

class CLR0Table
{
public:
    const CWorkGrammar* m_pWorkGrammar;
    yvector< yvector <CTableItem> > m_ParsingTable;

    CLR0Table();

    bool BuildLRTable();

};

enum ParserStepResultEnum {psReduce, psShift, psError};

struct CInputPeriod {
    size_t m_Start;
    size_t m_End;
};

struct CLR0OutputItem  : public CInputPeriod
{
    const CWorkRule*    m_pRule;
};

struct CLR0StackItem : public CInputPeriod
{
    size_t m_StateNo;
};

struct CLR0Parser
{
    yvector<CLR0StackItem>        m_StateStack;
    yvector<CLR0OutputItem>        m_Output;

    CLR0Parser();
    CLR0Parser(size_t Offset);

    bool    ShouldReduce(const CLR0Table& LR0Table) const;
    bool    ShiftAction(size_t InputSymbolOffset, int InputSymbol, const CLR0Table& LR0Table);
    void    ReduceAction(const CLR0Table& ParsingTable);
};
