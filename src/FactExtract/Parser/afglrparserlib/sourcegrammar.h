#pragma once

#include <util/generic/vector.h>
#include "grammaritem.h"

struct CWorkGrammar;

struct CParseLeftPart : yvector<CGrammarItem>
{
    size_t    m_SourceLineNo;
    Stroka  m_SourceLine;
};

struct CParseRule
{
    CGrammarItem                m_LeftPart;
    yvector< CParseLeftPart >     m_RightParts;

    bool    IsValid(Stroka& ErrorMsg) const;
    Stroka    toString() const;
    void    Print() const;
};

struct CParseGrammar
{
    yvector<CGrammarItem>    m_Roots;
    yvector<CParseRule>        m_Rules;
    MorphLanguageEnum    m_Language;

    CParseGrammar();
    bool    CheckCoherence() const;
    void    EncodeGrammar(CWorkGrammar& Result) const;
    void    Print();
    bool    AssignSynMainItemIfNotDefined();
    void    Clear();
};
