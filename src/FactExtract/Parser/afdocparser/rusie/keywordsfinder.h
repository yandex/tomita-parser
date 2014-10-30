#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/common/gztarticle.h>
#include "dictsholder.h"


class CWordVector;
class CMultiWordCreator;
class CTextWS;

class CKeyWordsFinder
{
public:
    CKeyWordsFinder(CMultiWordCreator& m_MultiWordCreator);

    void Run(EDicType dicType);
    void CollectGztArticles(NGzt::TCachingGazetteer& gzt);

    void FindKeyWordsAfterAnalyticFormBuilder(EDicType dicType);

    bool FindAuxWordSequenceByLemma(int iStart, const CArticleContents& wsDic, int& iLast, SWordHomonymNum& mainWord,
                                    ymap<int, yset<int> >& mapHom2Art, EDicType dicType);

protected:
    //bool CorrectPredictedHomonym(const NGzt::TPhraseWords<CWordVector>& article_words, size_t first_word_pos, size_t& corrected_id);

    void RestorePredictedHomonyms(yhash<size_t, int>& predicted_good);

    int FindKeyWordsFromWordByWord(EDicType dicType, int iW);
    int FindKeyWordsFromWordByLemma(EDicType dicType, int iW);
    bool CheckAgreements(ymap<int, yset<int> >& mapHom2Art, int iW1);
    bool CheckAgreements(int iArt, int iW1);
    bool CompWithLemmas(SContent2ArticleID& content2ArticleID, int iStartW, int& iLastW, EDicType dicType,
                        ymap<int, yset<int> >& mapHom2Art, SWordHomonymNum& mainWord);
    int CompWithWords(yvector<Wtroka>& word, int iStartWord);
    bool FindWordSequenceByWord(int iStart, const CArticleContents& wsDic, int& iLast, yset<int>& artIDs);
    bool FindClusterByHomonym(const CArticleContents& wsDic, const CWord& w, const CHomonym& h, yvector<CContentsCluster*>&  foundClusters);
    bool AddOneWordKW(const yvector<long>& arts, EDicType dicType, CHomonym& h, const CWord& w, int iHomID,
                      ymap<int, yset<int> >& mapHom2Art);

    bool CheckGrammInfoFromArticle(int iArt, EDicType dicType, CHomonym& h, const CWord& firstWord); //@h could be modified inside
    bool HasGoodGraphmatType(CWord* pW);
    void AddWordsInfo(CTextWS* pWS);

protected:
    CMultiWordCreator& m_MultiWordCreator;
    CWordVector& m_Words;
};
