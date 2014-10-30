#pragma once

#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "commongrammarinterpretation.h"
#include "multiwordcreator.h"
#include "filtercheck.h"
#include "dictsholder.h"

class CTomitaChainSearcher
{
public:
    CTomitaChainSearcher(CWordVector& Words, CReferenceSearcher* pReferenceSearcher, CMultiWordCreator& MultiWordCreator);

    void RunFactGrammar(const Stroka& gramName, const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences, bool allowAmbiguity=false);
    void RunFactGrammar(const Stroka& gramName, const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences,
                        const CWordsPair& wp, bool addToWordSequences = true, bool allowAmbiguity=false);

    void RunGrammar(const article_t* pArt, const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences);
    void RunGrammar(const TGztArticle& pArt, yvector<CWordSequence*>& newWordSequences);

    bool m_bProcessedByGrammar;

protected:
    bool Check10FiosLimmitMacros();
    //void HideKWTypes(const yset<SArtPointer>&  kw_types, const yvector<SWordHomonymNum>& WordIndexes) const;
    void RunDefaultGrammar(const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences);
    void ClearAllTerminalSymbolInterpretation();
    void BuildWordsInterpretation(bool bAddEndSymbol, const CWorkGrammar& grammar, const CWordsPair& wpSent);
    void ArchiveAutomatSymbolInterpetationUnions(yvector<TerminalSymbolType>& vecUnionsArchive);
    void UnarchiveAutomatSymbolInterpetationUnions(const yvector<TerminalSymbolType>& vecUnionsArchive);
    void RunDateGrammar(const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences);

protected:
    CWordVector& m_Words;
    CWordSequence::TSharedVector& m_wordSequences;
    CReferenceSearcher* m_pReferenceSearcher;
    CMultiWordCreator& m_MultiWordCreator;
    CFilterCheck m_FilterCheck;
};
