#pragma once

#include <util/generic/vector.h>
#include <util/generic/list.h>

#include <FactExtract/Parser/common/inputitem.h>
#include "simplegrammar.h"


class CInputSequence
{
public:
    CInputSequence(void);
    virtual ~CInputSequence(void);
    void AddTerminal(const CInputItem* pTerm);
    virtual void Process (const CWorkGrammar& G, bool bDumpOccurs,
                          yvector< COccurrence >& Occurrences, bool allowAmbiguity=false) = 0;
    void    FindOccurrencesWithTrie (const CWorkGrammar& G, yvector< COccurrence >& Occurrences);

protected:
    yvector<const CInputItem*> m_InputTerminals;
};

class CInputItemFactory;

struct CInputSequenceGLR : public CInputSequence
{
    // this slot should be updated during processing the whole text
    //  it contains all words which can refer a named entity
    yset<Stroka>            m_AbrigedReferences;

    ylist<CGLRParser>        m_Parsers;

    CInputSequenceGLR(CInputItemFactory* pInputItemFactory)
    {
        m_pInputItemFactory = pInputItemFactory;
    };

    void    Process (const CWorkGrammar& G, bool bDumpOccurs,  yvector< COccurrence >& Occurrences, bool allowAmbiguity=false);
    void    FreeParsers();
private:
    size_t    ApplyGLR_Parsing (const CWorkGrammar& G, const yset<size_t>& StartPoints, yvector< COccurrence >& Occurrences);
    CInputItemFactory* m_pInputItemFactory;
};

struct CSynAnInputSequence : public CInputSequenceGLR
{
    CSynAnInputSequence(const CWorkGrammar& G, CInputItemFactory* pInputItemFactory);

    using CInputSequenceGLR::Process;    //to avoid hiding
    void Process ();

    CGLRParser    m_Parser;
    const CWorkGrammar& m_Grammar;
};
