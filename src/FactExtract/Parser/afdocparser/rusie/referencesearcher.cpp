#include "referencesearcher.h"
#include "textprocessor.h"

//==============   CReferenceSearcher  ==========================

CReferenceSearcher::CReferenceSearcher(NGzt::TCachingGazetteer& gazetteer, CTextProcessor& textProcessor, CTime& dateFromDateFiled)
    : GztCache(gazetteer)
    , TextProcessor(textProcessor)
    , m_vecSentence(TextProcessor.m_vecSentence)
    , m_dateFromDateFiled(dateFromDateFiled)
{
    m_iCurSentence = 0;
}

CSentenceRusProcessor* CReferenceSearcher::GetSentenceProcessor(int i) const {
    return CheckedCast<CSentenceRusProcessor*>(m_vecSentence[i]);
}

Wtroka CReferenceSearcher::GetSentenceString(int iSentNum)
{
    Wtroka str;
    CSentence *ptrSentenceRus = (CSentence*)m_vecSentence[iSentNum];
    ptrSentenceRus->ToString(str);
    return str;
}

void CReferenceSearcher::BuildTextIndex()
{
    if (0 < m_TextIndex.size()) return;
    for (int i = 0; (size_t)i < m_vecSentence.size(); i++) {
        CSentence *ptrSentenceRus = (CSentence*)m_vecSentence[i];
        const CWordVector& rRusWords = ptrSentenceRus->GetRusWords();
        for (size_t j = 0; j < rRusWords.OriginalWordsCount(); ++j) {
            const CWord& w = rRusWords.GetOriginalWord(j);

            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it) {
                Wtroka s_lem = it->GetLemma();
                if (s_lem.empty()) continue;

                long l_ind = GetThreeLetterIndex(s_lem);

                _text_ind_it tid = m_TextIndex.find(l_ind);
                if (m_TextIndex.end() == tid) {
                    m_TextIndex[l_ind] = yset< TPair<int,int> > ();
                    m_TextIndex.find(l_ind)->second.insert(TPair<int,int>(i, j));
                } else
                    tid->second.insert(TPair<int,int>(i, j));
            }
        }
    }

};

long CReferenceSearcher::GetThreeLetterIndex(const Wtroka& s_lem) const
{
    long l_ind = 0;
    l_ind = (int)s_lem[0]*1000;
    if (1 < s_lem.size())
        l_ind += (int)s_lem[1];
    l_ind *= 1000;
    if (2 < s_lem.size())
    l_ind += (int)s_lem[2];

    return l_ind;
};

//начало: интерфейс доступа к m_DocNonGrammarFacts
void CReferenceSearcher::PushArtificialFacts(int iSentNum, CFactsWS* pFactWS)
{
    artificial_names_it ArtNamesIt = m_DocNonGrammarFacts.m_DocArtificialExtractedFacts.find(iSentNum);
    if (ArtNamesIt != m_DocNonGrammarFacts.m_DocArtificialExtractedFacts.end())
        ArtNamesIt->second.push_back(pFactWS);
    else
        m_DocNonGrammarFacts.m_DocArtificialExtractedFacts[iSentNum] = yvector<CWordSequence*>(1, pFactWS);
}

void CReferenceSearcher::GetArtificialFacts(int iSentNum, yvector<CWordSequence*>& rFoundFPCs)
{
    artificial_names_it ArtNamesIt = m_DocNonGrammarFacts.m_DocArtificialExtractedFacts.find(iSentNum);
    if (ArtNamesIt != m_DocNonGrammarFacts.m_DocArtificialExtractedFacts.end())
        rFoundFPCs = ArtNamesIt->second;
}

void CReferenceSearcher::SetArtificialFactProcessed(EArticleTextAlg eTextAlg)
{
    m_DocNonGrammarFacts.m_ProcessedTextAlgs.insert(eTextAlg);
}

bool CReferenceSearcher::IsArtificialFactProcessed(EArticleTextAlg eTextAlg) const
{
    return (m_DocNonGrammarFacts.m_ProcessedTextAlgs.find(eTextAlg) != m_DocNonGrammarFacts.m_ProcessedTextAlgs.end());
}
//конец: интерфейс доступа к m_DocNonGrammarFacts
