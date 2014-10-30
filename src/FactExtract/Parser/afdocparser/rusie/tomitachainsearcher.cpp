#include "tomitachainsearcher.h"
#include "datechain.h"
#include "terminalsassigner.h"

CTomitaChainSearcher::CTomitaChainSearcher(CWordVector& Words, CReferenceSearcher* pReferenceSearcher, CMultiWordCreator& MultiWordCreator)
    : m_bProcessedByGrammar(false)
    , m_Words(Words)
    , m_wordSequences(MultiWordCreator.m_wordSequences)
    , m_pReferenceSearcher(pReferenceSearcher)
    , m_MultiWordCreator(MultiWordCreator)
    , m_FilterCheck(MultiWordCreator)
{
}

void CTomitaChainSearcher::RunDateGrammar(const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences)
{
    if (!m_FilterCheck.CheckFilters(grammar.m_FilterStore)) {
        if (NULL != CGramInfo::s_PrintGLRLogStream)
            *CGramInfo::s_PrintGLRLogStream
                << "********************************** finished src-date.cxx"
                << " (m_FilterCheck.CheckFilters == false) **********************************\n"
                << Endl;
        return;
    }

    yvector<SWordHomonymNum> WordIndexes;
    m_MultiWordCreator.CreateWordIndexes(grammar.m_kwArticlePointer, WordIndexes, true);

    ClearAllTerminalSymbolInterpretation();
    BuildWordsInterpretation(true, grammar, CWordsPair(0, m_Words.OriginalWordsCount()));

    CDateChain dateChain(m_Words, grammar, newWordSequences ,WordIndexes, m_pReferenceSearcher);
    dateChain.Run();
    m_wordSequences.insert(m_wordSequences.begin(), newWordSequences.begin() , newWordSequences.end());
    ClearAllTerminalSymbolInterpretation();
}

void CTomitaChainSearcher::RunFactGrammar(const Stroka& gramName, const CWorkGrammar& grammar, yvector<CWordSequence*>& newFactsWS, bool allowAmbiguity)
{
    RunFactGrammar(gramName, grammar, newFactsWS, CWordsPair(0, m_Words.OriginalWordsCount()), true, allowAmbiguity);
}

bool CTomitaChainSearcher::Check10FiosLimmitMacros()
{
    yvector<SWordHomonymNum>* pWHs = m_MultiWordCreator.GetFoundWords(GlobalDictsHolder->BuiltinKWTypes().Fio, KW_DICT);
    if (pWHs == 0)
        return true;
    if (pWHs->size() >= 10)
        return false;
    if (pWHs->size() >= 5) {
        SWordHomonymNum& whPrev = pWHs->at(0);
        int iPrevWord = m_Words.GetWord(whPrev).GetSourcePair().LastWord();
        for (size_t i = 1; i < pWHs->size(); i++) {
            SWordHomonymNum& wh = pWHs->at(i);
            int iWord = m_Words.GetWord(wh).GetSourcePair().FirstWord();
            bool bPunctFound = false;
            int j;
            for (j = iPrevWord + 1; j < iWord; j++) {
                const CWord& w = m_Words.GetOriginalWord(j);
                if (w.IsPunct()) {
                    if (bPunctFound)
                        break;
                    else
                        bPunctFound = true;
                }
            }
            if (j < iWord)
                return true;
            iPrevWord = m_Words.GetWord(wh).GetSourcePair().LastWord();
        }
    }
    return true;
}

void CTomitaChainSearcher::RunFactGrammar(const Stroka& gramName, const CWorkGrammar& grammar, yvector<CWordSequence*>& newFactsWS, const CWordsPair& wp, bool addToWordSequences/*=true*/, bool allowAmbiguity)
{
    if (NULL != CGramInfo::s_PrintGLRLogStream)
            *CGramInfo::s_PrintGLRLogStream
                << "\n\n========================================== processing "
                << gramName << " ==========================================\n" << Endl;

    if (grammar.m_ExternalGrammarMacros.m_b10FiosLimit) {
        if (!Check10FiosLimmitMacros()) {
            if (NULL != CGramInfo::s_PrintGLRLogStream)
                *CGramInfo::s_PrintGLRLogStream
                    << "********************************** finished " << gramName
                    << " (Check10FiosLimmitMacros == false) **********************************\n" << Endl;
            return;
        }
    }

    m_MultiWordCreator.FindKWWords(grammar.m_kwArticlePointer);

    if (!m_FilterCheck.CheckFilters(grammar.m_FilterStore)) {
        if (NULL != CGramInfo::s_PrintGLRLogStream)
            *CGramInfo::s_PrintGLRLogStream
                << "********************************** finished " << gramName
                << " (m_FilterCheck.CheckFilters == false) **********************************\n" << Endl;
        return;
    }

    m_bProcessedByGrammar = true;
    yvector<SWordHomonymNum> WordIndexes;
    m_MultiWordCreator.CreateWordIndexes(grammar.m_kwArticlePointer, WordIndexes, true);

    ClearAllTerminalSymbolInterpretation();
    BuildWordsInterpretation(true, grammar, wp);

    //if( wp.FirstWord() != -1 )
    {
        size_t i = 0;
        for (; i < WordIndexes.size(); i++) {
            if (m_Words.GetWord(WordIndexes[i]).GetSourcePair().FirstWord() == wp.FirstWord())
                break;
        }
        if (i < WordIndexes.size())
            WordIndexes.erase(WordIndexes.begin(), WordIndexes.begin() + i);
    }

    //if( wp.LastWord() != -1 )
    {
        int i = WordIndexes.size() - 1;
        for (; i >= 0; i--) {
            if (m_Words.GetWord(WordIndexes[i]).GetSourcePair().LastWord() == wp.LastWord())
                break;
        }
        if (i >= 0)
            WordIndexes.erase(WordIndexes.begin() + i + 1, WordIndexes.end());

    }

    CCommonGrammarInterpretation CommonGrammarInterpretation(m_Words, grammar, newFactsWS, WordIndexes, m_pReferenceSearcher, CGramInfo::s_maxFactsCount);
    CommonGrammarInterpretation.m_GramFileName = gramName;

    CommonGrammarInterpretation.Run(allowAmbiguity);

    if (addToWordSequences)
        m_wordSequences.insert(m_wordSequences.begin(), newFactsWS.begin() , newFactsWS.end());
    ClearAllTerminalSymbolInterpretation();

    if (NULL != CGramInfo::s_PrintGLRLogStream)
        *CGramInfo::s_PrintGLRLogStream
            << "********************************** finished "
            << gramName << " **********************************\n" << Endl;
}

void CTomitaChainSearcher::RunGrammar(const article_t* pArt, const CWorkGrammar& grammar, yvector<CWordSequence*>& newWordSequences)
{
    if (pArt->get_kw_type() == GlobalDictsHolder->BuiltinKWTypes().DateChain)
        RunDateGrammar(grammar, newWordSequences);
    else
        RunFactGrammar(pArt->get_gram_file_name(), grammar, newWordSequences, pArt->allow_ambiguity());
}

void CTomitaChainSearcher::RunGrammar(const TGztArticle& pArt, yvector<CWordSequence*>& newWordSequences)
{
    typedef yhash<Stroka, const CWorkGrammar*> TGrammars;
    TGrammars grammars;

    const CDictsHolder& dicts = *GlobalDictsHolder;
    for (NGzt::TCustomKeyIterator it(pArt, ::GetTomitaPrefix()); it.Ok(); ++it)
        grammars[*it] = &dicts.GetGrammarOrRegister(*it);

    if (pArt.GetType() == GlobalDictsHolder->BuiltinKWTypes().DateChain)
        for (TGrammars::const_iterator it = grammars.begin(); it != grammars.end(); ++it)
            RunDateGrammar(*(it->second), newWordSequences);
    else
        for (TGrammars::const_iterator it = grammars.begin(); it != grammars.end(); ++it)
            RunFactGrammar(it->first, *(it->second), newWordSequences);
}

void CTomitaChainSearcher::ArchiveAutomatSymbolInterpetationUnions(yvector<TerminalSymbolType>& vecUnionsArchive)
{
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i)
        m_Words.GetAnyWord(i).ArchiveAutomatSymbolInterpetationUnions(vecUnionsArchive);
}

void CTomitaChainSearcher::BuildWordsInterpretation(bool bAddEndSymbol, const CWorkGrammar& grammar, const CWordsPair& wpSent)
{
    CTerminalsAssigner TerminalsAssigner(grammar, wpSent);
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& w = m_Words.GetAnyWord(i);
        TerminalsAssigner.BuildTerminalSymbols(w);
        if (bAddEndSymbol)
            w.m_AutomatSymbolInterpetationUnion.insert(grammar.GetEndOfStreamSymbol());
    }
}

void CTomitaChainSearcher::UnarchiveAutomatSymbolInterpetationUnions(const yvector<TerminalSymbolType>& vecUnionsArchive)
{
    if (0 == vecUnionsArchive.size()) return;

    int iLength = 0;
    size_t iIter = 0;
    ClearAllTerminalSymbolInterpretation();

    for (size_t j = 0; j < m_Words.GetAllWordsCount(); ++j) {
        assert(iIter < vecUnionsArchive.size());
        iLength = vecUnionsArchive[iIter]; iIter++;
        while (0 < iLength) {
            m_Words.GetAnyWord(j).AddTerminalSymbol(vecUnionsArchive[iIter]);
            iIter++; iLength--;
        }

        for (CWord::SHomIt it = m_Words.GetAnyWord(j).IterHomonyms(); it.Ok(); ++it) {
            assert(iIter < vecUnionsArchive.size());
            iLength = vecUnionsArchive[iIter]; iIter++;
            while (0 < iLength) {
                m_Words.GetAnyWord(j).GetRusHomonym(it.GetID()).AddTerminalSymbol(vecUnionsArchive[iIter]);
                iIter++; iLength--;
            }
        }
    }
}

void CTomitaChainSearcher::ClearAllTerminalSymbolInterpretation()
{
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& w = m_Words.GetAnyWord(i);
        w.DeleteAllTerminalSymbols();
    }
}

