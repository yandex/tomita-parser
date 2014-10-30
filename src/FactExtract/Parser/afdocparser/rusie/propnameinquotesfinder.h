#pragma once

#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include "dictsholder.h"

class CTomitaChainSearcher;
class CWordVector;

class CPropNameInQuotesFinder
{
public:
    CPropNameInQuotesFinder(CWordVector& words, CTomitaChainSearcher& tomitaChainSearcher);

    void Run(const TArticleRef& article, yvector<CWordSequence*>& resWS);

protected:
    int BuildCompanyNameInQuotes(size_t iW);
    CWordSequence* ApplyGrammarToCompanyInQuotes(int iW1, int iW2);

protected:
    CWordVector& m_Words;
    CTomitaChainSearcher& m_TomitaChainSearcher;
};
