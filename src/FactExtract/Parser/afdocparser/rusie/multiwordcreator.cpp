#include "multiwordcreator.h"
#include "tomitachainsearcher.h"
#include "referencesearcher.h"
#include "normalization.h"
#include "quotesfinder.h"
#include "interview_fact_creator.h"
#include "extractedfieldsearcher.h"
#include "propnameinquotesfinder.h"
#include "keywordsfinder.h"
#include "factfields.h"

#include "textprocessor.h"


CMultiWordCreator::CMultiWordCreator(CWordVector& _Words, CWordSequence::TSharedVector& wordSequences)
    : m_GztArticlesFound(false)
    , m_pReferenceSearcher(NULL)
    , m_wordSequences(wordSequences)
    , m_Words(_Words)
    , m_bProcessedByGrammar(false)
{
    for (int i = 0; i < DICTYPE_COUNT; ++i)
        m_TextKWFound[i] = false;
}

void CMultiWordCreator::CollectArticleInfo(const SArtPointer& artP, EDicType dicType,
                                           yvector<TGztArticle>& gztArticles,
                                           yvector<SDictIndex>& auxArticles,
                                           bool hasTextKeys) const
{
    hasTextKeys = false;

    const CDictsHolder& dicts = *GlobalDictsHolder;
    const TGazetteer& gzt = *dicts.Gazetteer();
    const CIndexedDictionary& auxDict = dicts.GetDict(dicType);

    YASSERT(artP.IsValid());

    if (artP.HasStrType()) {
        NGzt::TArticlePtr art;
        if (gzt.ArticlePool().FindCustomArticleByName(artP.GetStrType(), art))
            gztArticles.push_back(TGztArticle(art));
        else {
            int iArt = auxDict.get_article_index(artP.GetStrType());
            if (iArt != -1)
                auxArticles.push_back(SDictIndex(dicType, iArt));
        }
    } else if (artP.GetKWType().GetDescriptor() == TGztArticle::AuxDicArticleDescriptor()) {
        // do not search gzt articles by TAuxDictArticle kw-type
    } else {
        yhash<TKeyWordType, SKWTypeInfo>::const_iterator kwtype_info = auxDict.m_KWType2Articles.find(artP.GetKWType());
        if (kwtype_info == auxDict.m_KWType2Articles.end() || kwtype_info->second.m_bOnlyTextContent)
            hasTextKeys = true;
        else
            for (size_t i = 0; i < kwtype_info->second.m_Articles.size(); ++i)
                auxArticles.push_back(SDictIndex(dicType, kwtype_info->second.m_Articles[i]));

        const yset<ui32>* custom_offsets = gzt.ArticlePool().FindCustomArticleOffsetsByType(artP.GetKWType().GetDescriptor());
        if (custom_offsets != NULL)
            for (yset<ui32>::const_iterator it = custom_offsets->begin(); it != custom_offsets->end(); ++it)
                gztArticles.push_back(TGztArticle(*it, gzt.ArticlePool()));
    }

    if (!hasTextKeys)
        for (size_t i = 0; i < gztArticles.size(); ++i)
            if (gztArticles[i].HasNonCustomKey()) {
                hasTextKeys = true;
                break;
            }
}

bool CMultiWordCreator::FindKWWords(const SArtPointer& artP, EDicType dicType)
{
    if (!artP.IsValid() || m_FoundArticles.find(artP) != m_FoundArticles.end())
        return false;

    yvector<TGztArticle> gztArticles;
    yvector<SDictIndex> auxArticles;
    bool hasTextKeys = false;

    CollectArticleInfo(artP, dicType, gztArticles, auxArticles, hasTextKeys);

    if (hasTextKeys)
        FindKeyWords(dicType);

    for (size_t i = 0; i < gztArticles.size(); ++i) {
        SArtPointer sub_art(gztArticles[i].GetTitle());
        if (!(sub_art == artP) && m_FoundArticles.find(sub_art) != m_FoundArticles.end())
            continue;

        // the single article could contain keys of both 'tomita:' or 'alg:' types
        ProcessTomitaGztArticle(gztArticles[i]);
        ProcessAlgArticle(gztArticles[i]);
        ProcessTaggerGztArticle(gztArticles[i]);    // and 'tagger:' as well

        m_FoundArticles.insert(sub_art);
    }

    FindAuxKWWords(auxArticles, dicType);

    m_FoundArticles.insert(artP);

    return true;
}

void CMultiWordCreator::FindKWWords(const yset<SArtPointer>& kw_types)
{
    yset<SArtPointer>::const_iterator it = kw_types.begin();
    for (; it != kw_types.end(); ++it)
        FindKWWords(*it, KW_DICT);
}

void CMultiWordCreator::FindAuxKWWords(yvector<SDictIndex>& indexes, EDicType dicType)
{
    if (dicType == DICTYPE_COUNT)
        return;

    bool bHasTextKW = false;
    yvector<SDictIndex> tomitaKWArticles;
    SDictIndex algArticleIndex;//считатеся, что у одного kw_type не больше одного alg
    for (size_t i = 0; i < indexes.size(); ++i) {
        SDictIndex index = indexes[i];
        YASSERT(dicType == index.m_DicType);    //словарь должен быть для всех одинаковый

        const article_t* pArt = GlobalDictsHolder->GetAuxArticle(index);
        if (m_FoundArticles.find(SArtPointer(pArt->get_title())) != m_FoundArticles.end())
            continue;

        if (pArt->has_text_content()) // значит поле СОСТАВ состоит из обычных слов
            bHasTextKW = true;
        else if (pArt->has_gram_file_name())
            tomitaKWArticles.push_back(index);
        else if (pArt->has_alg())
            algArticleIndex = index;
    }

    if (bHasTextKW || dicType == KW_DICT)
        FindKeyWords(dicType);

    for (size_t i = 0; i < tomitaKWArticles.size(); ++i)
        ProcessTomitaAuxArticle(tomitaKWArticles[i]);

    if (algArticleIndex.IsValid())
        ProcessAlgArticle(TArticleRef(algArticleIndex));
}

void CMultiWordCreator::AddFoundArticle(TKeyWordType article_type, const Wtroka& article_title, SWordHomonymNum& word)
{
    AddFoundArticle(article_type, article_title, word, CWordsPair());
}

void CMultiWordCreator::AddFoundArticle(TKeyWordType article_type, const Wtroka& article_title,
                                        const SWordHomonymNum& word, const CWordsPair& searchAreaWP)
{
    if (searchAreaWP.IsValid()) {
        m_FoundWordsByPeriod[searchAreaWP].push_back(word);
        return;
    }

    //m_Words[word].Lock();
    if (article_type != NULL)
        m_KW2Words[article_type].push_back(word);

    m_Title2Words[article_title].push_back(word);
};

void CMultiWordCreator::ProcessTomitaAuxArticle(SDictIndex& dicIndex)
{
    CTomitaChainSearcher TomitaChainSearcher(m_Words, m_pReferenceSearcher, *this);
    const article_t* pArt = GlobalDictsHolder->GetAuxArticle(dicIndex);
    yvector<CWordSequence*> newWordSequences;
    TomitaChainSearcher.RunGrammar(pArt, GlobalDictsHolder->GetGrammarOrRegister(pArt->get_gram_file_name()), newWordSequences);
    m_bProcessedByGrammar = m_bProcessedByGrammar || TomitaChainSearcher.m_bProcessedByGrammar;

    for (size_t i = 0; i < newWordSequences.size(); ++i) {
        newWordSequences[i]->PutAuxArticle(dicIndex);
        AddMultiWord(newWordSequences[i]);
    }
}

void CMultiWordCreator::ProcessTomitaGztArticle(const TGztArticle& article)
{
    CTomitaChainSearcher TomitaChainSearcher(m_Words,m_pReferenceSearcher, *this);
    yvector<CWordSequence*> newWordSequences;
    TomitaChainSearcher.RunGrammar(article, newWordSequences);
    m_bProcessedByGrammar = m_bProcessedByGrammar || TomitaChainSearcher.m_bProcessedByGrammar;

    for (size_t i = 0; i < newWordSequences.size(); ++i) {
        newWordSequences[i]->PutGztArticle(article);
        AddMultiWord(newWordSequences[i]);
    }
}

inline void CMultiWordCreator::ProcessTaggerGztArticle(const TGztArticle& article) {
    // each tagger runs on whole text once, not sentence by sentence.
    m_pReferenceSearcher->TextProcessor.RunTagger(article);
}

void CMultiWordCreator::FindExtractedNamesInText(const TArticleRef& article, EArticleTextAlg eAlg)
{
    CExtractedFieldSearcher ExtractedNameSearcher(m_pReferenceSearcher, eAlg);
    yvector<CWordSequence*> newWordSequences;
    ExtractedNameSearcher.SearchExtractedNamesInText(m_pReferenceSearcher->GetCurSentence(), newWordSequences);

    for (size_t i = 0; i < newWordSequences.size(); ++i) {
        newWordSequences[i]->PutArticle(article);
        AddMultiWord(newWordSequences[i]);
    }
    m_wordSequences.insert(m_wordSequences.begin(), newWordSequences.begin(), newWordSequences.end());
}

void CMultiWordCreator::ProcessAlgArticle(const TArticleRef& article)
{
    if (article.AuxDic().IsValid())
        ProcessAlgArticle(article, GlobalDictsHolder->GetAuxArticle(article.AuxDic())->get_alg());
    else {
        YASSERT(!article.Gzt().Empty());
        yset<Stroka> algs;
        for (NGzt::TCustomKeyIterator it(article.Gzt(), ::GetAlgPrefix()); it.Ok(); ++it)
            algs.insert(*it);

        for (yset<Stroka>::const_iterator it = algs.begin(); it != algs.end(); ++it)
            ProcessAlgArticle(article, *it);
    }
}

void CMultiWordCreator::ProcessAlgArticle(const TArticleRef& article, const Stroka& strAlg)
{
    static const Stroka WORDS_IN_QUOTES = "words_in_quotes";
    static const Stroka COMP_IN_TEXT = "comp_in_text";
    static const Stroka GEO_IN_TEXT = "geo_in_text";
    static const Stroka VERB_COMMINIC = "verb_communic";
    static const Stroka INTERVIEW = "interview";
    static const Stroka FIO = "fio";
    static const Stroka FIO_WITHOUT_SURNAME = "fio_without_surname";
    static const Stroka DATE = "date";
    static const Stroka DIGITAL_TIME = "digital_time";
    static const Stroka QUASI_DATE = "quasi_date";
    static const Stroka NUMBER = "number";

    if (strAlg == WORDS_IN_QUOTES) {
        FindCompanyNamesInQuotes(article);
        return;
    } else if (strAlg == COMP_IN_TEXT) {
        FindExtractedNamesInText(article, CompInTextAlg);
        return;
    } else if (strAlg == GEO_IN_TEXT) {
        FindExtractedNamesInText(article, GeoInTextAlg);
        return;
    } else if (strAlg == VERB_COMMINIC) {
        YASSERT(m_pReferenceSearcher != NULL);
        CQuotesFinder QuotesFinder(m_pReferenceSearcher->m_vecSentence, article);
        QuotesFinder.FindQuotesFacts(m_pReferenceSearcher->GetCurSentence());
        return;
      } else if (strAlg == INTERVIEW) {
          YASSERT(m_pReferenceSearcher != NULL);
          CInterviewFactCreator interviewFactCreator(m_pReferenceSearcher->m_vecSentence, article);
          interviewFactCreator.CreateFact(m_pReferenceSearcher->m_DocumentAttribtes.m_strInterviewFio,
                                        m_pReferenceSearcher->m_DocumentAttribtes.m_TitleSents.second);
        return;
    }

    for (size_t i = 0; i < m_wordSequences.size(); ++i) {
        TIntrusivePtr<CWordSequence> pWS = m_wordSequences[i];
        switch (pWS->GetWSType()) {
            case FioWS:
                if (strAlg == FIO)
                    AddFIOMultiWord(*(CFioWordSequence*)(pWS.Get()), article, false);
                if (strAlg == FIO_WITHOUT_SURNAME)
                    AddFIOMultiWord(*(CFioWordSequence*)(pWS.Get()), article, true);
                break;
            case DateTimeWS:
                {
                    CDateGroup* pDateWS = dynamic_cast<CDateGroup*>(pWS.Get());
                    if (pDateWS) {
                        if ((strAlg == DATE && (pDateWS->IsDMY() || pDateWS->IsMY() || pDateWS->IsY())) ||
                            (strAlg == DIGITAL_TIME && pDateWS->IsMH()) ||
                            (strAlg == QUASI_DATE && pDateWS->IsDM())) {
                                pWS->PutArticle(article);
                                AddMultiWord(pWS.Get());
                            }
                    }
                }
                break;
            case NumberWS:
                if (strAlg == NUMBER) {
                    pWS->PutArticle(article);
                    TGramBitSet newPos(gNumeral);
                    //если это однословное число
                    //и оно записано буквами - то сохраним ту часть речи,
                    //которая приписана морфологией
                    if (pWS->Size() == 1) {
                        const CWord& w = m_Words[pWS->FirstWord()];
                        if (!w.HasUnknownPOS())
                            newPos = TGramBitSet();
                    }
                    AddMultiWord(pWS.Get(), newPos);
                }
                break;
            default:
                break;
        }
    }
}

void CMultiWordCreator::InitWordIndexes(yvector<SWordHomonymNum>& WordIndexes)
{
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); i++)
        WordIndexes.push_back(SWordHomonymNum(i, -1));
}

void CMultiWordCreator::CreateWordIndexes(const yset<SArtPointer>&  artPointers, yvector<SWordHomonymNum>& WordIndexes, bool bAddEndOfStreamSymbol)
{
    for (yset<SArtPointer>::const_iterator it = artPointers.begin(); it != artPointers.end(); ++it)
        FindKWWords(*it, KW_DICT);

    InitWordIndexes(WordIndexes);
    //использую алгоритм SolveAmbiguity и соответственно структуру COccurrence
    //COccurrence.m_GrammarRuleNo - номер multiWord
    yvector<COccurrence> multiWordsOccurrences;
    for (size_t i = 0; i < m_Words.GetMultiWordsCount(); ++i) {
        const CWord& pW = m_Words.GetMultiWord(i);

        yset<SArtPointer>::const_iterator it;
        for (it = artPointers.begin(); it != artPointers.end(); ++it)
            if (pW.HasArticle(*it))
                break;

        if (it == artPointers.end())
            continue;

        COccurrence o(pW.GetSourcePair().FirstWord(), pW.GetSourcePair().LastWord() + 1, i);
        multiWordsOccurrences.push_back(o);
    }

    SolveAmbiguity(multiWordsOccurrences);
    yvector<SWordHomonymNum> multiWords;
    for (size_t i = 0; i < multiWordsOccurrences.size(); i++) {
        SWordHomonymNum tmp(multiWordsOccurrences[i].m_GrammarRuleNo,-1, false);
        SubstituteByMultiWord(tmp, WordIndexes);
    }
    if (bAddEndOfStreamSymbol)
        WordIndexes.push_back(m_Words.GetEndWordWH());
}

void CMultiWordCreator::SubstituteByMultiWord(SWordHomonymNum& WordHomonymNum, yvector<SWordHomonymNum>& WordIndexes)
{
    const CWord& multi_word = m_Words.GetWord(WordHomonymNum);

    int k1 = -1, k2 = -1;
    for (size_t i = 0; i < WordIndexes.size(); i++) {
        const CWord& w = m_Words.GetWord(WordIndexes[i]);
        if ((w.IsOriginalWord()) &&
            (w.GetSourcePair().FirstWord() == multi_word.GetSourcePair().FirstWord()))
            k1 = i;

        if (k1 != -1) {
            if (!w.IsOriginalWord())
                break;

            if (w.GetSourcePair().FirstWord() == multi_word.GetSourcePair().LastWord()) {
                k2 = i;
                break;
            }
        }
    }

    if ((k1 != -1) && (k2 != -1))
        for (int j = k2; j >= k1; j--)
            WordIndexes.erase(WordIndexes.begin() + j);

    if (k1 != -1)
        WordIndexes.insert(WordIndexes.begin() + k1, WordHomonymNum);
}

void CMultiWordCreator::AddQuoteMultiWord(CWordSequence& ws, const TArticleRef& article)
{
    SWordHomonymNum wh;
    Wtroka str;
    CWord* pNewWord = GetWordForMultiWord(ws, str, wh);
    if (pNewWord->m_SourceWords.Size() == 1 && pNewWord->HasOnlyUnknownPOS()) {
        size_t firstId = pNewWord->IterHomonyms().GetID();
        CHomonym& h = pNewWord->GetRusHomonym(firstId);
        h.SetSourceWordSequence(&ws);
        h.PutArticle(article);
        wh.m_HomNum = firstId;
    } else {
        pNewWord->m_SourceWords.SetPair(ws.FirstWord(), ws.LastWord());
        if (str.size() == 0)
            str = pNewWord->m_txt;
        TMorph::ToLower(str);
        CHomonym* pNewHom = new CHomonym(TMorph::GetMainLanguage(), str);
        pNewHom->SetSourceWordSequence(&ws);
        pNewHom->PutArticle(article);
        wh.m_HomNum = pNewWord->AddRusHomonym(pNewHom);
    }

    if (article.AuxDic().IsValid()) {
        const article_t* pArt =  GlobalDictsHolder->GetAuxArticle(article.AuxDic());
        YASSERT(pArt != NULL);
        AddFoundArticle(pArt->get_kw_type(), pArt->get_title(), wh);
    } else {
        YASSERT(!article.Gzt().Empty());
        AddFoundArticle(article.Gzt().GetType(), article.Gzt().GetTitle(), wh);
    }

    m_wordSequences.push_back(&ws);
}

void CMultiWordCreator::AddFIOMultiWord(CFioWordSequence& wordsPair, const TArticleRef& article, bool bWithoutSurname)
{
    if (bWithoutSurname == wordsPair.m_NameMembers[Surname].IsValid())
        return;
    wordsPair.PutArticle(article);
    wordsPair.GetFio().MakeUpper();
    AddMultiWord(&wordsPair);
}

yvector<SWordHomonymNum>* CMultiWordCreator::GetFoundWords(TKeyWordType type, EDicType dicType, bool bTryToFind/*=true*/)
{
    if (bTryToFind)
          FindKWWords(type, dicType);
    ymap<TKeyWordType, yvector<SWordHomonymNum> >::iterator it = m_KW2Words.find(type);
    return (it != m_KW2Words.end())? &it->second : NULL;
}

int CMultiWordCreator::GetFoundWordsCount(TKeyWordType type, EDicType dicType)
{
    yvector<SWordHomonymNum>* res = GetFoundWords(type, dicType);
    return (res != NULL)? res->size() : 0;
}

yvector<SWordHomonymNum>* CMultiWordCreator::GetFoundWords(const SArtPointer& artP, EDicType dicType, bool bTryToFind/*=true*/)
{
    if (artP.HasKWType())
        return GetFoundWords(artP.GetKWType(), dicType, bTryToFind);
    else if (artP.HasStrType())
        return GetFoundWords(artP.GetStrType(), dicType, bTryToFind);
    return NULL;
}

yvector<SWordHomonymNum>* CMultiWordCreator::GetFoundWords(const Wtroka& strTitle, EDicType dicType, bool bTryToFind /* = true*/)
{
    if (bTryToFind)
        FindKWWords(strTitle, dicType);
    ymap<Wtroka, yvector<SWordHomonymNum> >::iterator it = m_Title2Words.find(strTitle);
    if (it == m_Title2Words.end())
        return NULL;
    else
        return &it->second;
}

CWord* CMultiWordCreator::GetWordForMultiWord(CWordSequence& ws, Wtroka& str, SWordHomonymNum& newWH)
{
    CWord* pNewWord;
    if (ws.Size() == 1) {
        pNewWord = &m_Words.GetOriginalWord(ws.FirstWord());
        newWH.m_bOriginalWord = true;
        newWH.m_WordNum = ws.FirstWord();
    } else {
        for (int i = ws.FirstWord(); i <= ws.LastWord(); ++i) {
            str += m_Words.GetOriginalWord(i).GetOriginalText();
            if (i < ws.LastWord())
                str += '_';
        }

        size_t k;
        for (k = 0; k < m_Words.GetMultiWordsCount(); ++k) {
            const CWord& w = m_Words.GetMultiWord(k);
            if (w.GetSourcePair() == ws)
                break;
        }

        if (k < m_Words.GetMultiWordsCount()) {
            pNewWord = &m_Words.GetMultiWord(k);
            newWH.m_WordNum = k;
        } else {
            pNewWord = new CMultiWord(str, false);
            pNewWord->m_bUp = m_Words.GetOriginalWord(ws.FirstWord()).m_bUp;
            m_Words.AddMultiWord(pNewWord);
            newWH.m_WordNum = m_Words.GetMultiWordsCount()-1;
        }

        newWH.m_bOriginalWord = false;

    }

    return pNewWord;
}

static void MergeGrammems(THomonymGrammems& dst, const TGramBitSet& art_grammems, const TGramBitSet& newPos)
{
    // first, reset Part Of Speech if any
    if (newPos.HasAny(TMorph::AllPOS()))
        dst.SetPOS(newPos);
    else if (art_grammems.HasAny(TMorph::AllPOS()))
        dst.SetPOS(art_grammems);

    // take other grammems from @art_grammems if any
    TGramBitSet other = art_grammems & ~TMorph::AllPOS();
    if (other.any()) {
        // if there is a form with such grammems - just leave it alone and drop the rest ones
        bool found = false;
        for (THomonymGrammems::TFormIter it = dst.IterForms(); it.Ok(); ++it)
            if (it->HasAll(other)) {
                dst.ResetSingleForm(*it);
                found = true;
            }

        if (!found) {
            // otherwise merge all forms and replace grammems by classes
            TGramBitSet newForm = dst.All();
            newForm.ReplaceByMaskIfAny(art_grammems, NSpike::AllCases);
            newForm.ReplaceByMaskIfAny(art_grammems, NSpike::AllGenders);
            newForm.ReplaceByMaskIfAny(art_grammems, NSpike::AllNumbers);
            const TGramBitSet anim(gAnimated, gInanimated);
            newForm.ReplaceByMaskIfAny(art_grammems, anim);
            newForm.ReplaceByMaskIfAny(art_grammems, NSpike::AllTimes);
            newForm.ReplaceByMaskIfAny(art_grammems, NSpike::AllPersons);
            // just add the rest non-classified grammems
            static const TGramBitSet nonclassified = ~(NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers |
                                                       anim | NSpike::AllTimes | NSpike::AllPersons);
            newForm |= art_grammems & nonclassified;
            dst.Reset(newForm);
        }
    }

    // if we still do not known POS, apply some workarounds:
    if (dst.GetPOS().none()) {
        dst.SetPOS(TGramBitSet(gSubstantive));
        if (!dst.HasAny(NSpike::AllCases))
            dst.Add(NSpike::AllCases);
        if (!dst.HasAny(NSpike::AllGenders))
            dst.Add(NSpike::AllGenders);
        if (!dst.HasAny(NSpike::AllNumbers))
            dst.Add(NSpike::AllNumbers);
        }

    // set a noun or adj without additional grammem as indeclinable
    if (!dst.HasAny(~TMorph::AllPOS()) &&
        (art_grammems.Has(gSubstantive) || TMorph::IsFullAdjective(art_grammems)))
        dst.Add(NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers);

}

bool CMultiWordCreator::HasToAddGrammemsFromMainWord(const CWordSequence& ws)
{
    TKeyWordType type = ws.GetKWType();
    const TBuiltinKWTypes& builtin = GlobalDictsHolder->BuiltinKWTypes();
    return type != builtin.CompanyInQuotes &&
           type != builtin.Date &&
           type != builtin.DateChain;
}

static bool FindBestHomonym(const CWord& word, TWtringBuf lemma, TKeyWordType kwtype, THomonymGrammems grammems, int& homId) {
    // search if we already have a multiword homonym with such text
    bool found = false;
    bool foundSameKwType = false;
    for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it)
        if (it->CHomonymBase::GetLemma() == lemma) {
            if (it->HasKWType(kwtype, KW_DICT)) {
                if (it->Grammems == grammems) {
                    homId = it.GetID();     // prefer homonyms with same kwtype and same grammems
                    return true;
                } else if (!foundSameKwType) {
                    homId = it.GetID(); // prefer homonyms with same kwtype
                    found = true;
                    foundSameKwType = true;
                }
            } else  if (!found) {
                homId = it.GetID();   // otherwise, return the first one having @lemma text
                found = true;
            }
        }

    return found;
}

// return found (or newly made) homonym id
static int FindOrMakeMultiwordHomonym(const CWordSequence& ws, CWord& word, TKeyWordType kwtype, THomonymGrammems grammems, THomonymPtr& res) {
    // use the content of CWordSequence.m_Lemmas as new homonym lemma
    Wtroka str = ws.GetLemma();
    if (!str)
        str = word.GetLowerText();
    TMorph::ToLower(str);

    int homId = -1;
    // search if we already have a multiword homonym with such text
    if (!FindBestHomonym(word, str, kwtype, grammems, homId)) {
        // create new homonym, if there is no ready one
        res = new CHomonym(TMorph::GetMainLanguage(), str);
        if (ws.HasAuxArticle())
            res->PutAuxArticle(ws.GetAuxArticleIndex());
        else
            res->PutGztArticle(ws.GetGztArticle());
        homId = word.AddRusHomonym(res);
    }
    return homId;
}

SWordHomonymNum CMultiWordCreator::AddMultiWordInt(CWordSequence* ws, bool takeOnwership,
                                                   const TGramBitSet& newPos, const CWordsPair& searchAreaWP)
{
    SWordHomonymNum wh = ws->GetMainWord();
    Wtroka stmp;
    SWordHomonymNum newWH;
    CWord* pNewWord = GetWordForMultiWord(*ws, stmp, newWH);

    pNewWord->m_SourceWords.SetPair(ws->FirstWord(), ws->LastWord());

    TGramBitSet art_grammems;      // output grammems of article
    Wtroka article_title;
    TKeyWordType article_type = NULL;

    if (ws->HasGztArticle()) {
        const TGztArticle& gzt_article = ws->GetGztArticle();
        article_title = gzt_article.GetTitle();
        article_type = gzt_article.GetType();
        const NGzt::TMessage* lemma = gzt_article.GetLemmaInfo();
        if (lemma != NULL)
            art_grammems = gzt_article.GetLemmaOutputGrammems(*lemma);
    } else if (ws->HasAuxArticle()) {
        const article_t* pArt = GlobalDictsHolder->GetAuxArticle(ws->GetAuxArticleIndex());
        art_grammems = pArt->get_new_pos();
        article_title = pArt->get_title();
        article_type = pArt->get_kw_type();
    }

    THomonymGrammems newGram;
    if (!ws->GetGrammems().Empty()) {
        newGram = ws->GetGrammems();
        if (!newGram.HasForms() && wh.IsValid())
            newGram.SetPOS(m_Words[wh].Grammems.GetPOS());
    } else if (wh.IsValid() && HasToAddGrammemsFromMainWord(*ws))
        newGram = m_Words[wh].Grammems;
    MergeGrammems(newGram, art_grammems, newPos);

    THomonymPtr pNewHom;
    if (pNewWord->IsMultiWord() && (pNewWord->GetSourcePair().Size() != 1 || !wh.IsValid())) {
        newWH.m_HomNum = FindOrMakeMultiwordHomonym(*ws, *pNewWord, article_type, newGram, pNewHom);
        YASSERT(newWH.IsValid());
    }

    if (pNewHom.Get() == NULL) {
        if (!pNewWord->IsMultiWord()) {
            if (wh.IsValid())
                newWH = wh;
            else {
                // just take the first homonym
                newWH.m_bOriginalWord = true;
                newWH.m_WordNum = pNewWord->GetSourcePair().FirstWord();
                newWH.m_HomNum = pNewWord->IterHomonyms().GetID();
            }
        }
        YASSERT(newWH.IsValid());
        //часто бывает ситуация, когда мы вынуждены клонировать абсолютно одинаковые
        //омонимы, различающиеся только приписанными статьями из aux_dic,
        //в случае с geo_thesaurus.cxx это чревато порождением огромного количества омонимов
        //(боле 50 для "Петров"), тогда если статьи не отличаются друг от друга полем СОСТАВ
        //приписываемыми граммемами, ЧР и KWType, то мы омонимы не клонируем а дополнительные статьи
        //записываем в CHomonym::m_KWtype2Articles. Это происходит в CWord::PutArticleIndex.
        //если мы считаем, что найденные статьи для одного и того же омонима ничем не отличаются,
        //то главное слово для неотличающихся стаей у ws одно и то же и ему приписана
        //первая попавшаяся среди неразличимы статья
        //например статьи "_петрова_2" и "_петрова_3" для нас одинаковы (отличаются только ГЕО_ЧАСТЬ
        //а это неважно для парсера) и незачем плодить омонимы
        bool bCloneAnyway = (!newGram.Empty() && !(m_Words[newWH].Grammems == newGram)) ||
                            !GlobalDictsHolder->BuiltinKWTypes().IsGeo(article_type);

        if (ws->HasAuxArticle())
            newWH.m_HomNum = m_Words.GetWord(newWH).PutAuxArticle(newWH.m_HomNum, ws->GetAuxArticleIndex(), bCloneAnyway);
        else
            newWH.m_HomNum = m_Words.GetWord(newWH).PutGztArticle(newWH.m_HomNum, ws->GetGztArticle(), bCloneAnyway);
    }
    YASSERT(newWH.IsValid());

    AddFoundArticle(article_type, article_title, newWH, searchAreaWP);
    CHomonym& h = m_Words[newWH];
    h.SetSourceWordSequence(ws);
    if (!newGram.Empty())
        h.SetGrammems(newGram);

    if (takeOnwership) {
        if (!ws->HasLemmas())
            NormalizeMultiWordHomonym(pNewWord, &h);
        m_wordSequences.push_back(ws);
    }

    return newWH;
}

void CMultiWordCreator::NormalizeMultiWordHomonym(CWord* pW,  CHomonym* pH)
{
    CWordSequence* pWS = pH->GetSourceWordSequence();
    pWS->ClearLemmas();

    Wtroka lemma_text;
    if (pWS->HasAuxArticle()) {
        const article_t* pArt = GlobalDictsHolder->GetAuxArticle(pWS->GetAuxArticleIndex());
        if (pArt->has_lemma())
            lemma_text = pArt->get_lemma();
    } else if (pWS->HasGztArticle()) {
        const TGztArticle& gzt_article = pWS->GetGztArticle();
        const NGzt::TMessage* lemma_info = gzt_article.GetLemmaInfo();
        if (lemma_info != NULL)
            lemma_text = gzt_article.GetLemmaText(*lemma_info);
    }

    if (!lemma_text.empty()) {
        //если есть лемма, указанная в статье
        Wtroka capitalizedLemma = CNormalization::GetCapitalizedLemma(*pW, lemma_text);
        pWS->AddLemma(SWordSequenceLemma(lemma_text, capitalizedLemma));
    } else {
        if (pWS->Size() == 1) {
            // for single-word sequences - just take a lemma of corresponding word
            pWS->AddLemma(SWordSequenceLemma(m_Words[pWS->GetMainWord()].GetLemma()), true);
        } else for (int i = pWS->FirstWord(); i <= pWS->LastWord(); ++i) {
            // not normalized now, will be normalized lazily on request
            pWS->AddLemma(SWordSequenceLemma(m_Words[i].GetText()), false);
        }
    }

}

void CMultiWordCreator::FindKeyWordsAfterAnalyticFormBuilder(EDicType dicType)
{
    //если еще не вызывали общую ф-цию FindKeyWords
    //то и пытаться найти статью для инфинитивов, ставших Г, еще не надо
    if (!m_TextKWFound[dicType])
        return;
    CKeyWordsFinder keyWordsFinder(*this);
    keyWordsFinder.FindKeyWordsAfterAnalyticFormBuilder(dicType);
}

bool CMultiWordCreator::FindKeyWords(EDicType dicType)
{
    if (m_TextKWFound[dicType])
        return false;

    CKeyWordsFinder keyWordsFinder(*this);
    if (dicType == KW_DICT && !m_GztArticlesFound) {
        keyWordsFinder.CollectGztArticles(m_pReferenceSearcher->GztCache);
        m_GztArticlesFound = true;
    }
    keyWordsFinder.Run(dicType);
    m_TextKWFound[dicType] = true;
    return true;
}

void CMultiWordCreator::FindCompanyNamesInQuotes(const TArticleRef& article)
{
    CTomitaChainSearcher tmp(m_Words, m_pReferenceSearcher, *this);
    CPropNameInQuotesFinder propNameInQuotesFinder(m_Words, tmp);

    yvector<CWordSequence*> newWS;
    propNameInQuotesFinder.Run(article, newWS);
    for (size_t i = 0; i < newWS.size(); ++i) {
        newWS[i]->SetCommonLemmaString(m_Words.ToString(*newWS[i]));
        //не будем вызывать стандартную AddMultiWord, всегда будем добавлять
        //новый омоним с пустыми граммемками, даже для одиночных слов.
        //считаем, что слово в кавычках - это что-то особенное, и просто переносить грам. инф. не стоит пока
        AddQuoteMultiWord(*newWS[i], article);
    }

}

//выдает найденные слова на данном отрезке
//эта функция не запускает поиск !!!, она только выдает уже найденное ранее,
//так что перед ней должна быть вызвана FindWordsInPeriod

void CMultiWordCreator::GetFoundWordsInPeriod(const SArtPointer& artP, yvector<SWordHomonymNum>& foundWords,
                                              const CWordsPair& wp) const
{
    foundWords.clear();

    //сначала пытаемся найти среди KwType, которые уже искались на всем предложении, те, которые
    //входят в наш отрезок

    if (artP.HasKWType()) {
        ymap<TKeyWordType, yvector<SWordHomonymNum> >::const_iterator it = m_KW2Words.find(artP.GetKWType());
        if (it != m_KW2Words.end()) {
            const yvector<SWordHomonymNum>& whs = it->second;
            for (size_t i = 0; i < whs.size(); i++) {
                CWord& w = m_Words.GetWord(whs[i]);
                if (wp.Includes(w.GetSourcePair()))
                    foundWords.push_back(whs[i]);
            }
        }
    }

    if (!foundWords.empty())
        return;

    //пытаемся найти среди StrType, которые уже искались на всем предложении, те, которые
    //входят в наш отрезок
    if (artP.HasStrType()) {
        ymap<Wtroka, yvector<SWordHomonymNum> >::const_iterator it = m_Title2Words.find(artP.GetStrType());
        if (it != m_Title2Words.end()) {
            const yvector<SWordHomonymNum>& whs = it->second;
            for (size_t i = 0; i < whs.size(); ++i) {
                CWord& w = m_Words.GetWord(whs[i]);
                if (wp.Includes(w.GetSourcePair()))
                    foundWords.push_back(whs[i]);
            }
        }
    }

    if (!foundWords.empty())
        return;

    //теперь проходим по запомненным отрезкам, для которых хранятся все найденные MultiWords
    ymap<CWordsPair, yvector<SWordHomonymNum> >::const_iterator it = m_FoundWordsByPeriod.find(wp);
    if (it != m_FoundWordsByPeriod.end()) {
        const yvector<SWordHomonymNum>& whs = it->second;
        for (size_t i = 0; i < whs.size(); i++) {
            const CHomonym& h = m_Words[whs[i]];
            if (h.HasArticle(artP, KW_DICT))
                foundWords.push_back(whs[i]);
        }
    }
}

//ищет статьи на определенном (wp) отрезке слов (используется, например, для поиска в ссылках)
//так как повторять всю бодягу с кэшами неохота и накладно, то ленивости здесь почти не будет
//при вызове ф-ции FindWordsInPeriod второй раз на одних и тех же данных, лишнего ничего найдено не будет,
//но и, если ничего не было найдено, это нигде не запоминается и второй раз будем пытаться искать
void CMultiWordCreator::FindWordsInPeriod(const SArtPointer& artP, const CWordsPair& wp)
{
    yvector<SWordHomonymNum> foundWords;
    GetFoundWordsInPeriod(artP, foundWords, wp);

    if (!foundWords.empty())
        return;

    yvector<TGztArticle> gztArticles;
    yvector<SDictIndex> auxArticles;
    bool hasTextKeys = false;
    CollectArticleInfo(artP, KW_DICT, gztArticles, auxArticles, hasTextKeys);

    if (hasTextKeys)
        FindKeyWords(KW_DICT);

    yvector<CWordSequence*> newWordSequences;

    // nasty copy-pasting...
    for (size_t i = 0; i < gztArticles.size(); ++i) {
        for (NGzt::TCustomKeyIterator it(gztArticles[i], ::GetTomitaPrefix()); it.Ok(); ++it) {
            CTomitaChainSearcher TomitaChainSearcher(m_Words, m_pReferenceSearcher, *this);
            newWordSequences.clear();
            TomitaChainSearcher.RunFactGrammar(*it, GlobalDictsHolder->GetGrammarOrRegister(*it), newWordSequences, wp);
            if (TomitaChainSearcher.m_bProcessedByGrammar)
                m_bProcessedByGrammar = true;

            for (size_t j = 0; j < newWordSequences.size(); ++j) {
                newWordSequences[j]->PutGztArticle(gztArticles[i]);
                AddMultiWordInt(newWordSequences[j], false, TGramBitSet(), wp);
            }
        }
    }

    for (size_t i = 0; i < auxArticles.size(); ++i) {
        const article_t* pArt = GlobalDictsHolder->GetAuxArticle(auxArticles[i]);
        //если в поле СОСТАВ просто перечислены нужные слова, то не будем выпендриваться и искать их только в данном отрезке
        //все равно ф-ция FindKeyWords рано или поздно вызовется на всем предложении
        if (pArt->has_text_content()) {
            if (!hasTextKeys)
                FindKWWords(pArt->get_title(), KW_DICT);
        } else if (pArt->has_gram_file_name()) {
             CTomitaChainSearcher TomitaChainSearcher(m_Words, m_pReferenceSearcher, *this);
            newWordSequences.clear();
            const Stroka& gramfile = pArt->get_gram_file_name();
            TomitaChainSearcher.RunFactGrammar(gramfile, GlobalDictsHolder->GetGrammarOrRegister(gramfile), newWordSequences, wp);
            if (TomitaChainSearcher.m_bProcessedByGrammar)
                m_bProcessedByGrammar = true;

            for (size_t j = 0; j < newWordSequences.size(); ++j) {
                newWordSequences[j]->PutAuxArticle(auxArticles[i]);
                AddMultiWordInt(newWordSequences[j], false, TGramBitSet(), wp);
            }
        }
    }
}

void CMultiWordCreator::PrintWordsFeatures(TOutputStream &stream, ECharset encoding) const
{
    ymap< Wtroka, yset<TKeyWordType> > kwtypes;
    ymap< Wtroka, yset<Wtroka> > titles;

    for (ymap<TKeyWordType, yvector<SWordHomonymNum> >::const_iterator it = m_KW2Words.begin(); it != m_KW2Words.end(); ++it)
        for (size_t i = 0; i < it->second.size(); ++i)
            kwtypes[m_Words.GetWord(it->second[i]).m_txt].insert(it->first);

    for (ymap<Wtroka, yvector<SWordHomonymNum> >::const_iterator it = m_Title2Words.begin(); it != m_Title2Words.end(); ++it)
        for (size_t i = 0; i < it->second.size(); ++i)
            titles[m_Words.GetWord(it->second[i]).m_txt].insert(it->first);

    stream << "==================== multiwords ====================\n\n";
    for (ymap<Wtroka, yset<TKeyWordType> >::const_iterator it = kwtypes.begin(); it != kwtypes.end(); ++it) {
        const Wtroka& word = it->first;
        stream << WideToChar(word, encoding) << ": ";
        for (yset<TKeyWordType>::const_iterator kwt = it->second.begin(); kwt != it->second.end(); ++kwt)
            stream << (*kwt)->name() << ", ";
        stream << "\n\t";

        ymap<Wtroka, yset<Wtroka> >::iterator titles_it = titles.find(word);

        if (titles_it != titles.end()) {
            const yset<Wtroka>& articles = titles_it->second;
            for (yset<Wtroka>::const_iterator art = articles.begin(); art != articles.end(); ++art)
                stream << WideToChar(*art, encoding) << ", ";
            titles.erase(titles_it);
        }
        stream << "\n";
    }

    ymap<Wtroka, yset<Wtroka> >::iterator titles_it = titles.begin();
    for (; titles_it != titles.end(); ++titles_it) {
        stream << WideToChar(titles_it->first, encoding) << ": ";
        const yset<Wtroka>& articles = titles_it->second;
        for (yset<Wtroka>::const_iterator art = articles.begin(); art != articles.end(); ++art)
            stream << WideToChar(*art, encoding) << ", ";
        stream << "\n";
    }

    stream << "\n" << Endl;
}

