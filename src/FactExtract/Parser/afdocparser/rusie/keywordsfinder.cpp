#include <FactExtract/Parser/afdocparser/rusie/gztwrapper.h>
#include "keywordsfinder.h"
#include "multiwordcreator.h"
#include "textwordsequence.h"

CKeyWordsFinder::CKeyWordsFinder(CMultiWordCreator& MultiWordCreator)
    : m_MultiWordCreator(MultiWordCreator)
    , m_Words(MultiWordCreator.m_Words)
{
}

void CKeyWordsFinder::Run(EDicType dicType)
{
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        int iLast = FindKeyWordsFromWordByLemma(dicType, i);
        if (iLast != -1)
            i = iLast;
    }

    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        int iLast = FindKeyWordsFromWordByWord(dicType, i);
        if (iLast != -1)
            i = iLast;
    }
}
/*
bool CKeyWordsFinder::CorrectPredictedHomonym(const NGzt::TPhraseWords<CWordVector>& article_words, size_t first_word_pos,
                                              size_t& corrected_id)
{
    // locate first word found by predicted (non-dictionary) lemma
    size_t i = 0;
    for (; i < article_words.Size(); ++i)
        if (article_words.IsLemma(i) && article_words.GetLemma(i).GetID() == -1)
            break;

    // no such word
    if (i >= article_words.Size())
        return true;

    // allow only single-word predicted keys
    if (article_words.Size() > 1)
        return false;

    // copy predicted homonym to regular dict homonym for corresponding word
    CWord& word = m_Words.GetOriginalWord(first_word_pos + i);
    CWord::SHomIt hit = word.IterHomonyms();
    YASSERT(hit.Ok());

    THomonymPtr predicted = word.GetPredictedHomonyms()[article_words.GetLemma(i).GetPredictedIndex()];
    //clear unsafe grammems (which could not be reliable if predicted)
    static const TGramBitSet safe_mask = ~TGramBitSet(gSurname, gFirstName, gPatr, gGeo);
    TGrammarBunch safe_predicted_grammems;
    for (CHomonym::TFormIter g = predicted->Grammems.IterForms(); g.Ok(); ++g)
        safe_predicted_grammems.insert(*g & safe_mask);

    if (!hit->IsDictionary() && hit->Grammems.Empty())
        word.GetRusHomonym(hit.GetID()).Init(predicted->GetLemma(), safe_predicted_grammems, hit->IsDictionary());
    else if (hit->GetLemma() != predicted->GetLemma())
        return false;

    corrected_id = hit.GetID();
    return true;
}
*/
void CKeyWordsFinder::RestorePredictedHomonyms(yhash<size_t, int>& predicted_good)
{
    for (yhash<size_t, int>::iterator it = predicted_good.begin(); it != predicted_good.end(); ++it) {
        // copy predicted homonym to regular dict homonym for corresponding word
        CWord& word = m_Words.GetOriginalWord(it->first);
        CWord::SHomIt hom = word.IterHomonyms();
        while (hom.Ok() && (hom->IsDictionary() || !hom->Grammems.Empty()))
            ++hom;

        if (hom.Ok()) {
            THomonymPtr predicted = word.GetPredictedHomonyms()[it->second];
            //clear unsafe grammems (which could not be reliable if predicted)
            THomonymGrammems predictedGram = predicted->Grammems;
            predictedGram.Delete(TGramBitSet(gSurname, gFirstName, gPatr, gGeo));

            CHomonym& restored = word.GetRusHomonym(hom.GetID());
            restored.SetLemma(predicted->GetLemma());
            restored.SetGrammems(predictedGram);

            it->second = hom.GetID();

        } else
            it->second = -1;    // the homonym cannot be restored
    }
}

template <typename TStr>
static inline int CompareStrings(const TStr& a, const TStr& b) {
    return TCharTraits<typename TStr::char_type>::Compare(~a, +a, ~b, +b);
}

static inline int CompareGztArticles(const TGztArticle& a, const TGztArticle& b) {
    int typeCmp = CompareStrings(a.GetType()->name(), b.GetType()->name());
    return (typeCmp == 0) ? CompareStrings(a.GetTitle(), b.GetTitle()) : typeCmp;
}

typedef TIntrusivePtr<CTextWS> TTextWSPtr;

static inline bool GztWSLess(const TTextWSPtr& a, const TTextWSPtr& b) {

    // first, sort by position
    if (a->FirstWord() != b->FirstWord())
        return a->FirstWord() < b->FirstWord();

    if (a->LastWord() != b->LastWord())
        return a->LastWord() < b->LastWord();

    // next, sort by article type/title
    return CompareGztArticles(a->GetGztArticle(), b->GetGztArticle()) < 0;

}

void CKeyWordsFinder::CollectGztArticles(NGzt::TCachingGazetteer& gzt)
{
    yvector<TTextWSPtr> found_ws;      // all found articles as word sequences
    yhash<size_t, int> predicted_good;       // word index -> predicted homonym index (to restore as normal homonym)

    // Step 1: create raw word sequences for all found gzt-articles
    // As some of found articles could be found on predicted homonyms
    // we should later restore such homonyms (to get their homonym ids)

    TArticleIter<CWordVector> it;
    for (gzt.Gazetteer().IterArticles(m_Words, &it); it.Ok(); ++it) {
        TGztArticle article = gzt.GetArticle(it);
        //skip non-TAuxDicArticle articles
        if (!article.IsAuxDicArticle())
            continue;

        const NGzt::TPhraseWords<CWordVector>& article_words = it.GetWords();
        size_t iFirst = article_words.FirstWordIndex();
        size_t iLast = iFirst + article_words.Size() - 1;

        // collect information about predicted (bastard) homonyms found in gazetteer
        // (meaning that they had been predicted correctly and could be safely restored)
        for (size_t i = 0; i < article_words.Size(); ++i)
            if (article_words.IsLemma(i) && article_words.GetLemma(i).GetID() == -1)
                predicted_good[iFirst + i] = article_words.GetLemma(i).GetPredictedIndex();

        size_t iMainRel = article.GetMainWord();            // main word relative to start of article_words
        if (iMainRel >= article_words.Size())
            iMainRel = 0;

        size_t iMain = iMainRel + iFirst;                   // main word relative to start of m_Words, i.e. absolute in our context.

        int iMainHomonym;
        if (article_words.IsLemma(iMainRel))
            iMainHomonym = article_words.GetLemma(iMainRel).GetID();        // could be -1 (i.e. predicted)
        else // just take the first homonym
            iMainHomonym = m_Words.GetOriginalWord(iMain).IterHomonyms().GetID();

        THolder<CTextWS> pWS(new CTextWS());
        pWS->SetPair(iFirst, iLast);
        pWS->SetMainWord(iMain, iMainHomonym);
        // AddWordsInfo(pWS.Get()); do it later as iMainHomonym could be fixed (when corresponding predicted homonym is restored)
        pWS->PutGztArticle(article);
        pWS->PutWSType(KeyWord);

        found_ws.push_back(pWS.Release());
    }

    // Sort gzt results, as we do not want to depend on gazetteer iteration order
    //::StableSort(found_ws.begin(), found_ws.end(), GztWSLess);

    // Step 2:
    RestorePredictedHomonyms(predicted_good);

    // Step 3: fix main word homonyms if it was predicted
    // Collect one-word sequences separately (grouping by kwtype)

    // (word, homonym, kwtype) -> list of corresponding one-word articles
    typedef yvector<TTextWSPtr> TWSPtrArray;
    yhash<size_t, yhash<size_t, yhash<TKeyWordType, TWSPtrArray> > > single_word_ws;

    for (size_t i = 0; i < found_ws.size(); ++i) {
        SWordHomonymNum main = found_ws[i]->GetMainWord();
        if (main.m_HomNum == -1) {
            yhash<size_t, int>::const_iterator restored = predicted_good.find(main.m_WordNum);
            if (restored == predicted_good.end() || restored->second == -1)
                continue;   // skip this word sequence

            main.m_HomNum = restored->second;
            found_ws[i]->SetMainWord(main);
        }

        const TGztArticle& article = found_ws[i]->GetGztArticle();

        if (article.GetLightKW())
            // don't add new homonym for light_kw articles, just set a label on corresponding main homonym
            m_Words.GetOriginalWord(main.m_WordNum).GetRusHomonym(main.m_HomNum).AddLabel(article.GetTitle());
        else {
            AddWordsInfo(found_ws[i].Get());
            // for single-word articles: first collect all
            if (found_ws[i]->Size() == 1)
                single_word_ws[main.m_WordNum][main.m_HomNum][article.GetType()].push_back(found_ws[i].Release());
            else
                // add multiword articles (without any additional groupping)
                m_MultiWordCreator.TakeMultiWord(found_ws[i].Release());
        }
    }

    // Step 4: finally process remained single-word articles

    yhash<size_t, yhash<size_t, yhash<TKeyWordType, TWSPtrArray > > >::iterator it_word = single_word_ws.begin();
    for (; it_word != single_word_ws.end(); ++it_word) {
        yhash<size_t, yhash<TKeyWordType, TWSPtrArray > >::iterator it_hom = it_word->second.begin();
        for (; it_hom != it_word->second.end(); ++it_hom) {
            yhash<TKeyWordType, TWSPtrArray>::iterator it_kwtype = it_hom->second.begin();
            for (; it_kwtype != it_hom->second.end(); ++it_kwtype) {
                TWSPtrArray& articles = it_kwtype->second;
                YASSERT(articles.size() > 0);
                int newHomNum = m_MultiWordCreator.TakeMultiWord(articles[0].Release()).m_HomNum;
                for (size_t i = 1; i < articles.size(); ++i) {
                    articles[i]->SetMainWord(articles[i]->GetMainWord().m_WordNum, newHomNum);
                    m_MultiWordCreator.TakeMultiWord(articles[i].Release());
                }
            }
        }
    }
}

void CKeyWordsFinder::FindKeyWordsAfterAnalyticFormBuilder(EDicType dicType)
{
    //если еще не вызывали общую ф-цию FindKeyWords
    //то и пытаться найти статью для инфинитивов, ставших Г, еще не надо
    //if( !m_TextKWFound[dicType] )
    //  return;
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        CWord& w = m_Words.GetOriginalWord(i);
        if (!w.HasAnalyticVerbFormHomonym(gVerb))
            continue;
        FindKeyWordsFromWordByLemma(dicType, i);
    }
}

void CKeyWordsFinder::AddWordsInfo(CTextWS* pWS)
{
    SWordHomonymNum wh = pWS->GetMainWord();
    if (!wh.IsValid())
        return;
    for (int k = pWS->FirstWord(); k <= pWS->LastWord(); k++) {
        const CWord& w = m_Words[k];
        if (k == wh.m_WordNum) {
            static const Wtroka MAINWORD_MARK = CharToWide("*");
            pWS->AddWordInfo(MAINWORD_MARK + m_Words[wh].GetLemma());
        } else {
            yset<Wtroka> words;
            for (CWord::SHomIt h_it = w.IterHomonyms(); h_it.Ok(); ++h_it)
                words.insert(h_it->GetLemma());
            for (yset<Wtroka>::iterator it = words.begin(); it != words.end(); ++it)
                pWS->AddWordInfo(*it);
        }
    }
}

int CKeyWordsFinder::FindKeyWordsFromWordByLemma(EDicType dicType, int iW)
{
    //в такую структуру сохраняем информацию о том,
    //какие омонимы под какие статьи подошли (для однословных)
    //для многословных всегда в качестве номера омонима хранится 0
    ymap<int, yset<int> > mapHom2Art;
    SWordHomonymNum mainWord;
    int iLast;
    if (!FindAuxWordSequenceByLemma(iW, GlobalDictsHolder->GetDict(dicType).m_Contents, iLast, mainWord,  mapHom2Art, dicType))
        return -1;

    ymap<int, yset<int> >::iterator it = mapHom2Art.begin();
    for (; it != mapHom2Art.end(); ++it) {
        yset<int>::const_iterator art_it = it->second.begin();
        //сначала обработаем все статьи с полем ЛЕГКОЕ_КС = да
        //так как это не KW-тип и новый омоним для него никогда не создаем
        //тогда сначала припишем все label для этого омонима
        //а потом, в случае нескольких KW-типов все эти label
        //будут благополучно копироваться
        if (iLast == iW) //только для однословных
        {
            CHomonym& h = m_Words.GetOriginalWord(iW).GetRusHomonym(it->first);
            for (; art_it != it->second.end(); ++art_it) {
                const article_t* pArt = GlobalDictsHolder->GetAuxArticle(SDictIndex(dicType, *art_it));
                if (pArt->is_light_kw())
                    h.AddLabel(pArt->get_title());
            }
        }

        art_it = it->second.begin();

        //следим за тем, какие KWType (статьи) каким омонимам
        //уже были приписаны. Если очередная статья имеет KWType,
        //который уже встречался, то следим за тем чтобы у WS,
        //которые ей приписан, был тот же, что и у самого первого
        //AddMultiWord может изменить омоним, которому приписывается статья,
        //в случае клонирования (омониму уже было что-то приписано, его отклонировали и вернули
        //номер нового омонима)

        yhash<TKeyWordType, int> usedHomByKWType;
        for (; art_it != it->second.end(); ++art_it) {
            SDictIndex dictIndex(dicType, *art_it);
            const article_t* pArt = GlobalDictsHolder->GetAuxArticle(dictIndex);
            if (pArt->is_light_kw()) //уже такие обработали
                continue;

            CTextWS* pWS = new CTextWS();

            pWS->SetPair(iW,iLast);

            if (pWS->Size() > 1)
                pWS->SetMainWord(mainWord.m_WordNum, mainWord.m_HomNum);
            else
                pWS->SetMainWord(iW, it->first);
            AddWordsInfo(pWS);
            pWS->PutAuxArticle(dictIndex);
            pWS->PutWSType(KeyWord);

            TKeyWordType kwType = pArt->get_kw_type();
            if (kwType != NULL) {
                yhash<TKeyWordType, int>::const_iterator hom_it = usedHomByKWType.find(kwType);
                if (hom_it != usedHomByKWType.end() && pWS->Size() == 1) {
                    SWordHomonymNum whMain = pWS->GetMainWord();
                    whMain.m_HomNum = hom_it->second;
                    pWS->SetMainWord(whMain);
                }
            }

            int iH = m_MultiWordCreator.TakeMultiWord(pWS).m_HomNum;
            if (kwType != NULL)
                usedHomByKWType[kwType] = iH;

        }
    }
    return iLast;
}

int CKeyWordsFinder::FindKeyWordsFromWordByWord(EDicType dicType, int iW)
{
    int iLast;
    yset<int> artIDs;
    if (!FindWordSequenceByWord(iW, GlobalDictsHolder->GetDict(dicType).m_Contents, iLast, artIDs))
        return -1;

    for (yset<int>::iterator it = artIDs.begin(); it != artIDs.end(); ++it) {
        THolder<CWordSequence> pWS(new CWordSequence());
        pWS->SetPair(iW, iLast);
        pWS->PutAuxArticle(SDictIndex(dicType, *it));
        //берем первый попавшийся омоним
        SWordHomonymNum wh(iW, m_Words.GetOriginalWord(iW).IterHomonyms().GetID());
        pWS->SetMainWord(wh);
        m_MultiWordCreator.TakeMultiWord(pWS.Release());
    }
    return iLast;
}

bool CKeyWordsFinder::CheckGrammInfoFromArticle(int iArt, EDicType dicType, CHomonym& h,
                                                const CWord& /*firstWord*/)
{
    if (dicType == DICTYPE_COUNT) //если это ф-ция вызывается для geo-словаря (пока это не AuxDic)
        return true;

    const article_t* art = GlobalDictsHolder->GetAuxArticle(SDictIndex(dicType, iArt));
    if (art->get_pos().any()) {
        //if we have non-dictionary homonym then update its POS (!)
/*        if (!h.IsIndexed() && h.HasUnknownPOS())
            h.PutPOS(art->get_pos());
        else */
        if (h.IsDictionary() || !h.HasUnknownPOS())
            if (!h.HasPOS(art->get_pos()))
                return false;
    }

    const TGrammarBunch& art_grammems = art->get_morph_info();
    for (TGrammarBunch::const_iterator it = art_grammems.begin(); it != art_grammems.end(); ++it)
        if (h.Grammems.HasAll(*it))
            return true;

    return art_grammems.empty();
}

bool CKeyWordsFinder::AddOneWordKW(const yvector<long>& arts, EDicType dicType, CHomonym& h,
                                   const CWord& w, int iHomID, ymap<int, yset<int> >& mapHom2Art)
{
    bool bRes = false;
    for (size_t i = 0; i < arts.size(); i++) {
        if (CheckGrammInfoFromArticle(arts[i], dicType, h, w)) {
            bRes = true;
            ymap<int, yset<int> >::iterator it_hom2art = mapHom2Art.find(iHomID);
            if (it_hom2art == mapHom2Art.end()) {
                yset<int> found_arts;
                found_arts.insert(arts[i]);
                mapHom2Art[iHomID] = found_arts;
            } else
                it_hom2art->second.insert(arts[i]);
        }
    }
    return bRes;
}

bool CKeyWordsFinder::FindClusterByHomonym(const CArticleContents& wsDic, const CWord& w, const CHomonym& h,
                                           yvector<CContentsCluster*>& foundClusters)
{

    if (!wsDic.FindCluster(h.GetLemma(), w.IsDictionary()? CompByLemma : CompByLemmaSimilar, foundClusters)) {
        //для предсказанного по последней части дефисному слову пытаемся еще найти просто его вторую часть
        //например, "ассоциация-гильдия"
        if (w.m_typ == Hyphen && !w.m_bUp && h.IsDictionary()) {
            size_t ii = h.GetLemma().rfind('-');
            //больше одного дефиса не берем
             if (h.GetLemma().find('-') != ii)
                 return NULL;
             if (ii == Wtroka::npos)
                return NULL;
            Wtroka ss = h.GetLemma().substr(ii + 1);
            ECompType compType = CompByLemma;
            if (!TMorph::FindWord(ss))
                compType = CompByLemmaSimilar;
            wsDic.FindCluster(ss, compType, foundClusters);
        }
    }
    return foundClusters.size() > 0;
}

bool CKeyWordsFinder::HasGoodGraphmatType(CWord* pW)
{
    if ((pW->m_typ != Word) &&
        (pW->m_typ != DivWord) &&
        (pW->m_typ != Hyphen) &&
        (pW->m_typ != Abbrev) &&
        (pW->m_typ != Initial) &&
        (pW->m_typ != AltInitial) &&
        (pW->m_typ != Punct) &&
        (pW->m_typ != AltWord) &&
        (pW->m_typ != AbbAlt) &&
        (pW->m_typ != AbbEos) &&
        (pW->m_typ != AbbEosAlt) &&
        (pW->m_typ != AbbDigAlt) &&
        (pW->m_typ != AbbDig) &&
        (pW->m_typ != HypDiv))
        return false;

    return true;
}

bool CKeyWordsFinder::FindAuxWordSequenceByLemma(int iStart, const CArticleContents& wsDic, int& iLast, SWordHomonymNum& mainWord,
                                                 ymap<int, yset<int> >& mapHom2Art, EDicType dicType)
{
    CWord* pW = &m_Words[iStart];

    if (!HasGoodGraphmatType(pW))
        return false;

    for (CWord::SHomIt it = pW->IterHomonyms(); it.Ok(); ++it) {
        yvector<CContentsCluster*>  foundClusters;
        FindClusterByHomonym(wsDic, *pW, *it, foundClusters);
        for (size_t k = 0; k < foundClusters.size(); k++) {
            CContentsCluster* pItem = foundClusters[k];
            if (pItem->m_IDs.size() != 0 && (pItem->size() == 1)) //значит одно слово
            {
                if (pW->HasMorphNounWithGrammems(TGramBitSet(gAbbreviation)))
                    if (!pW->HasAtLeastTwoUpper())
                        break;

                //for non-dictionary homonyms try to restore their grammems (if morphology gives same bastard lemma)
                if (!it->IsDictionary() && it->Grammems.Empty()) {
                    TGrammarBunch predicted_grammems;
                    if (TMorph::PredictFormGrammemsByKnownLemma(it->GetLemma(), pItem->GetRepr(), predicted_grammems)) {
                        //clear unsafe grammems (which could not be reliable if predicted)
                        static const TGramBitSet safe_mask = ~TGramBitSet(gSurname, gFirstName, gPatr, gGeo);
                        TGrammarBunch safe_predicted_grammems;
                        for (TGrammarBunch::const_iterator g = predicted_grammems.begin(); g != predicted_grammems.end(); ++g)
                            safe_predicted_grammems.insert(*g & safe_mask);
                        pW->GetRusHomonym(it.GetID()).Init(pItem->GetRepr(), safe_predicted_grammems, it->IsDictionary());
                    }
                }

                if (AddOneWordKW(pItem->m_IDs, dicType, pW->GetRusHomonym(it.GetID()), *pW, it.GetID(), mapHom2Art))
                    iLast = iStart;

            } else {
                for (size_t k = 0; k < pItem->size(); k++) {
                    if (pItem->at(k).m_Content.size() > 1) {
                        ymap<int, yset<int> > mapHom2ArtSave = mapHom2Art;
                        mapHom2Art.clear();
                        if (CompWithLemmas(pItem->at(k), iStart, iLast, dicType, mapHom2Art, mainWord))
                            return true;
                        mapHom2Art = mapHom2ArtSave;
                    } else {
                        if (AddOneWordKW(pItem->at(k).m_IDs, dicType, pW->GetRusHomonym(it.GetID()), *pW, it.GetID(), mapHom2Art))
                            iLast = iStart;
                    }

                }
            }
        }
    }
    return mapHom2Art.size() > 0;
}

bool CKeyWordsFinder::FindWordSequenceByWord(int iStart, const CArticleContents& wsDic, int& iLast, yset<int>& artIDs)
{
    CWord* pW = &m_Words[iStart];

    if (!HasGoodGraphmatType(pW))
        return false;

    CContentsCluster* Cont = wsDic.FindCluster(m_Words[iStart].GetLowerText(), CompByWord);
    if (Cont != NULL) {
        for (size_t j = 0; j < Cont->size(); j++) {
            iLast = CompWithWords(Cont->at(j).m_Content, iStart);
            if (iLast != -1) {
                for (size_t k = 0; k < Cont->at(j).m_IDs.size(); k++) {
                    SWordHomonymNum wh(iStart, m_Words.GetOriginalWord(iStart).IterHomonyms().GetID());
                    artIDs.insert(Cont->at(j).m_IDs[k]);
                }

                break;
            }
        }
    }
    return artIDs.size() > 0;
}

int CKeyWordsFinder::CompWithWords(yvector<Wtroka>& word, int iStartWord)
{
    int i = 0;
    size_t j = 0;

    if (word.size() == 0)
        return -1;

    for (i = iStartWord; i < (int)m_Words.OriginalWordsCount(); ++i) {
        if (j >= word.size())
            break;

        Wtroka str_word = m_Words[i].GetLowerText();
        if (str_word != word[j++])
            return -1;
    }

    if (j >= word.size())
        return i - 1;

    return -1;
}

bool CKeyWordsFinder::CompWithLemmas(SContent2ArticleID& content2ArticleID, int iStartW, int& iLastW, EDicType dicType,
                                     ymap<int, yset<int> >& mapHom2Art, SWordHomonymNum& mainWord)
{
    yvector<Wtroka>& words = content2ArticleID.m_Content;
    SWordHomonymNum main_word;
    int i;
    for (i = iStartW; i < (int)m_Words.OriginalWordsCount() && (i - iStartW) < (int)words.size(); i++) {
        CWord* pW = &m_Words[i];
        if (!HasGoodGraphmatType(pW))
            return false;

        CWord::SHomIt it = pW->IterHomonyms();
        for (; it.Ok(); ++it) {
            if (it->GetLemma() == words[i - iStartW]) {
                bool bChecked = false;
                for (size_t k = 0; k < content2ArticleID.m_IDs.size(); k++) {
                    if (dicType != DICTYPE_COUNT) {
                        const article_t* art = GlobalDictsHolder->GetAuxArticle(SDictIndex(dicType, content2ArticleID.m_IDs[k]));
                        if (art->get_main_word() +  iStartW == i) {
                            if (CheckGrammInfoFromArticle(content2ArticleID.m_IDs[k], dicType, pW->GetRusHomonym(it.GetID()), m_Words[iStartW])) {
                                ymap<int, yset<int> >::iterator it_arts = mapHom2Art.find(0);
                                if (it_arts == mapHom2Art.end()) {
                                    yset<int> arts;
                                    arts.insert(content2ArticleID.m_IDs[k]);
                                    mapHom2Art[0] = arts;
                                } else
                                    it_arts->second.insert(content2ArticleID.m_IDs[k]);
                                mainWord.m_HomNum = it.GetID();
                                mainWord.m_WordNum = i;
                                bChecked = true;
                            }
                        } else {
                            bChecked = true;
                            //break;
                        }
                    } else {
                        //если вызывается для поля СОСТАВ из валентности
                        yset<int> arts;
                        arts.insert(content2ArticleID.m_IDs[k]);
                        mapHom2Art[0] = arts;

                        bChecked = true;
                    }
                }
                if (bChecked)
                    break;
            }
        }
        if (!it.Ok())
            return false;
    }
    if ((i - iStartW) == (int)words.size()) {
        iLastW = i - 1;
        if (!CheckAgreements(mapHom2Art, iStartW))
            return false;
        return true;
    }
    return false;
}

//проверим согласования из статьи iArt
//в цепочке слов предложения, начинающейся со слова iW1
bool CKeyWordsFinder::CheckAgreements(int iArt, int iW1)
{
    const article_t* art = GlobalDictsHolder->GetAuxArticle(SDictIndex(KW_DICT, iArt));
    if (!art->has_agreements())
        return true;
    for (int i = 0; i < art->get_agreements_count(); i++) {
        const SAgreement& agr = art->get_agreement(i);
        const CWord& w1 = m_Words[iW1 + agr.m_iW1];
        const CWord& w2 = m_Words[iW1 + agr.m_iW2];
        CWord::SHomIt it1 = w1.IterHomonyms();
        for (; it1.Ok(); ++it1) {
            CWord::SHomIt it2 =  w2.IterHomonyms();
            for (; it2.Ok(); ++it2)
                if (NGleiche::Gleiche(*it1, *it2, agr.m_agr))
                    break;

            if (it2.Ok())
                break;
        }
        if (!it1.Ok())
            return false;
    }
    return true;
}

bool CKeyWordsFinder::CheckAgreements(ymap<int, yset<int> >& mapHom2Art, int iW1)
{
    //берем нулевой омоним, так как если есть согласования
    //то значит это уже multiword, а значит нет омонима,
    //которому была приписана статья (или статьи)
    ymap<int, yset<int> >::iterator it_arts = mapHom2Art.find(0);
    if (it_arts == mapHom2Art.end())
        return false;
    yset<int>& arts = it_arts->second;
    yset<int> newArts;
    yset<int>::iterator it = arts.begin();
    for (; it != arts.end(); it++) {
        if (CheckAgreements(*it, iW1))
            newArts.insert(*it);
    }
    it_arts->second = newArts;
    return newArts.size() > 0;
}
