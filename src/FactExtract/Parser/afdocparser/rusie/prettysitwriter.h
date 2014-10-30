#pragma once

#include <util/generic/stroka.h>

#include "prettywriter.h"
#include <FactExtract/Parser/afdocparser/rusie/situationssearcher.h>

class CPrettySitWriter: public CPrettyWriter
{
public:
    CPrettySitWriter(ECharset encoding, const CParserOptions& parserOptions)
        : CPrettyWriter("AfDump.xsl", encoding, parserOptions)
    {
    }

protected:
    virtual TXmlNodePtrBase AddSentence(CSentenceRusProcessor* pSent, TXmlNodePtrBase piPar, int iSent);
    void AddSituationTable(TXmlNodePtrBase piSent, CSentence* pSent, const SValenciesValues& val);
};
