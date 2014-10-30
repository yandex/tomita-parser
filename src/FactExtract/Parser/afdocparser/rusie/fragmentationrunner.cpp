#include <FactExtract/Parser/common/libxmlutil.h>
#include "fragmentationrunner.h"
#include <FactExtract/Parser/afdocparser/rus/sentence.h>
#include "sentencerusprocessor.h"
#include "terminalsassigner.h"
#include "parseroptions.h"


CFragmentationRunner::CFragmentationRunner(CSentenceRusProcessor* pSent)
    : m_pSent(pSent)
    , m_ClauseGraph(pSent->m_Words)
    , m_Words(pSent->m_Words)
    , m_bFreeClauseVars(true)
    , m_wordSequences(pSent->m_wordSequences)
    , m_MultiWordCreator(pSent->GetMultiWordCreator())
    , m_words(pSent->m_words)
    , m_pErrorStream(pSent->m_pErrorStream)
    , m_EmptyClauseVar(pSent->m_Words)
{
}

void   CFragmentationRunner::freeFragmResults()
{
    m_ClauseGraph.Free();

    if (m_bFreeClauseVars) {
        for (size_t i = 0; i < m_ClauseVars.size(); i++)
            m_ClauseVars[i].Reset();
    }
    m_ClauseVars.clear();
}

CClauseVariant& CFragmentationRunner::GetFirstBestVar()
{
    if (m_ClauseVars.size() == 0) {
        (*m_pErrorStream) << "No clause variants after fragmentation!";
        return m_EmptyClauseVar;
    }
    return m_ClauseVars[0];
}

void CFragmentationRunner::InitWordIndexes(const SFragmOptions* opt)
{
    const NGzt::TProtoPool& pool = GlobalDictsHolder->Gazetteer()->ProtoPool();
    yset<SArtPointer> kwTypes;

    yset<TUnresolvedArtPointer> unresolved;
    if (opt != NULL) {
        if (opt->m_MultiWordsToFind.size() > 0)
            unresolved.insert(opt->m_MultiWordsToFind.begin(), opt->m_MultiWordsToFind.end());
    } else {
        SFragmOptions default_opt;
        unresolved.swap(default_opt.m_MultiWordsToFind);
    }

    // resolve
    for (yset<TUnresolvedArtPointer>::const_iterator it = unresolved.begin(); it != unresolved.end(); ++it)
        kwTypes.insert(SArtPointer(*it, pool));

    if (GlobalDictsHolder->m_SynAnGrammar.m_kwArticlePointer.size() > 0)
        kwTypes.insert(GlobalDictsHolder->m_SynAnGrammar.m_kwArticlePointer.begin(), GlobalDictsHolder->m_SynAnGrammar.m_kwArticlePointer.end());
    m_MultiWordCreator.CreateWordIndexes(kwTypes, m_WordIndexes, false);
}

bool CFragmentationRunner::Run(const SFragmOptions* fragmOptions)
{
    GetMutableGlobalDictsHolder()->InitFragmDics();

    InitWordIndexes(fragmOptions);
    if (!BuildInitialTopClauses())
        return false;
    m_ClauseGraph.RunRules(true);
    m_ClauseGraph.BuildClauseVariants(m_ClauseVars);
    //if( !BuildInitialClauses() )
    //  return false;
    //RunClauseRules();
    std::sort(m_ClauseVars.begin(), m_ClauseVars.end(), ClauseVariantsPred);
    m_bFreeClauseVars = false;
    return true;
}

bool CFragmentationRunner::BuildInitialTopClauses()
{
    if (!BuildInitialClauses())
        return false;
    m_ClauseGraph.AddClauseVariants(m_ClauseVars);
    m_ClauseVars.clear();
    return true;
}

void CFragmentationRunner::BuildWordsInterpretation(bool bAddEndSymbol, const CWorkGrammar& grammar)
{
    CTerminalsAssigner TerminalsAssigner(grammar, CWordsPair(0, m_Words.OriginalWordsCount()));
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& w = m_Words.GetAnyWord(i);
        TerminalsAssigner.BuildTerminalSymbols(w);
        if (bAddEndSymbol)
            w.m_AutomatSymbolInterpetationUnion.insert(grammar.GetEndOfStreamSymbol());
    }
}

void CFragmentationRunner::ClearAllTerminalSymbolInterpretation()
{
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& w = m_Words.GetAnyWord(i);
        w.DeleteAllTerminalSymbols();
    }
}

bool CFragmentationRunner::BuildInitialClauses()
{
        m_MultiWordCreator.GetFoundWords(GlobalDictsHolder->BuiltinKWTypes().Parenthesis, KW_DICT);
        DelAdjShortAdverbHomonym();
        FindPredics();
        //FindParenthesis();
        FindTemplateArticles();
        FindConjs();
        FindPreps();
        //m_Words.SortMultiWords();
        ClearAllTerminalSymbolInterpretation();
        if (GlobalDictsHolder->m_SynAnGrammar.m_UniqueGrammarItems.size() > 0)
            BuildWordsInterpretation(false, GlobalDictsHolder->m_SynAnGrammar);
        CClause* pClause = CClause::Create(m_Words);
        m_ClauseVars.clear();
        pClause->SetPair(0, 0);

        m_ClauseVars.push_back(CClauseVariant(m_Words));

        EInitialClauseSearchState state = SearchComma;

        CClause* pPrevClause = NULL;
        int iWordIndex = 0;
        while (ReadNextInitialClause(*pClause, state, iWordIndex)) {
            CWordsPair saveClause = *pClause;
            //pClause->InitClauseWords();
            bool bNewClause = pClause->TryToFindMultiWordConjs();

            pClause->Init();

            CClause* clonedClause = NULL;
            if (bNewClause && (m_ClauseVars.size() < (size_t)g_iMaxVarsCount)) {
                clonedClause = CClause::Create(m_Words);
                clonedClause->SetPair(saveClause.FirstWord(), saveClause.LastWord());
                clonedClause->Init(false);
            }

            yvector<CClause*> clonedClausesByConjs;
            yvector<CClause*> clonedClausesByMultiConjs;

            if (pClause->HasConjs() &&
                (!pPrevClause || !pPrevClause->HasDash()) &&
                (m_ClauseVars.size() < (size_t)g_iMaxVarsCount))
                CloneClausesByConjs(clonedClausesByConjs, *pClause);

            if (clonedClause &&
                clonedClause->HasConjs() &&
                (m_ClauseVars.size() < (size_t)g_iMaxVarsCount))
                    CloneClausesByConjs(clonedClausesByConjs, *clonedClause);

            int ClVarsCount = m_ClauseVars.size();

            for (int i = 0; i < ClVarsCount; i++) {
                if (clonedClause) {
                    if (i > 0)
                        clonedClause = clonedClause->Clone();
                    CClauseVariant ClauseVar(m_Words);
                    m_ClauseVars[i].Clone(ClauseVar);
                    ClauseVar.AddInitialClause(clonedClause);
                    m_ClauseVars.push_back(ClauseVar);
                }

                for (size_t j = 0; j < clonedClausesByConjs.size(); j++) {
                    CClauseVariant ClauseVar(m_Words);
                    m_ClauseVars[i].Clone(ClauseVar);
                    CClause* pCl = clonedClausesByConjs[j];
                    if (i > 0)
                        pCl = pCl->Clone();
                    ClauseVar.AddInitialClause(pCl);
                    m_ClauseVars.push_back(ClauseVar);
                }

                for (size_t j = 0; j < clonedClausesByMultiConjs.size(); j++) {
                    CClauseVariant ClauseVar(m_Words);
                    m_ClauseVars[i].Clone(ClauseVar);
                    CClause* pCl = clonedClausesByMultiConjs[j];
                    if (i > 0)
                        pCl = pCl->Clone();
                    ClauseVar.AddInitialClause(pCl);
                    m_ClauseVars.push_back(ClauseVar);
                }

                CClause* pClauseToAdd = pClause;
                if (ClVarsCount > 1 && (i < ClVarsCount - 1))
                    pClause = pClauseToAdd->Clone();
                m_ClauseVars[i].AddInitialClause(pClauseToAdd);
            }

            pPrevClause = pClause;
            pClause = CClause::Create(m_Words);
            pClause->SetPair(pPrevClause->LastWord()+1, pPrevClause->LastWord()+1);
        }
        delete pClause;
        //если построилась какая-нибудь аналитическая форма, то часть речи может измениться
        //тогда нужно для этих слов заново попытаться поискать их в KWDict
        m_MultiWordCreator.FindKeyWordsAfterAnalyticFormBuilder(KW_DICT);
        return true;
}

int CFragmentationRunner::SkipPuncts(int i)
{
    //for(int j = i+1 ; (j < m_Words.OriginalWordsCount()) ; j++)
    int j;
    for (j = i+1; j < (int)m_WordIndexes.size(); j++) {
        CWord& word = GetWordFromWordIndexes(j);
        if (!word.IsPunct())
            break;
    }
    return j-1;
}

void CFragmentationRunner::CloneClausesByConjs(yvector<CClause*>& clonedClausesByConjs, CClause& clause)
{
    CClause* clonedClause = CClause::Create(m_Words);
    clonedClause->SetPair(clause.FirstWord(), clause.LastWord());
    clonedClause->InitClauseWords(clause);

    while (clonedClause->InitConjs()) {
        clonedClause->Init(false);
        clonedClausesByConjs.push_back(clonedClause);

        clonedClause = CClause::Create(m_Words);
        clonedClause->SetPair(clause.FirstWord(), clause.LastWord());
        clonedClause->InitClauseWords(clause);
        if (clonedClausesByConjs.size() > (size_t)g_iMaxVarsCount)
            break;
    }

    delete clonedClause;
}

CWord& CFragmentationRunner::GetWordFromWordIndexes(int i)
{
    return m_Words.GetWord(m_WordIndexes[i]);
}

void CFragmentationRunner::InitClauseWords(CClause& clause, int iStartWodIndex, int iLastWordIndex)
{
    CWord& w = GetWordFromWordIndexes(iLastWordIndex);
    clause.SetLastWord(w.GetSourcePair().LastWord());
    for (int i = iStartWodIndex; i <= iLastWordIndex; i++)
        clause.m_ClauseWords.push_back(m_WordIndexes[i]);
}

bool CFragmentationRunner::ReadNextInitialClause(CClause& clause, EInitialClauseSearchState& state, int& iStartWordIndex)
{
    if (iStartWordIndex >= (int)m_WordIndexes.size())
        return false;

    //for(int i = clause.FirstWord() ; i < m_Words.OriginalWordsCount() ; i++)
    for (int i = iStartWordIndex; i < (int)m_WordIndexes.size(); i++) {
        //CWord& word = m_Words[i];
        CWord& word = GetWordFromWordIndexes(i);

        if ((state == SearchComma) && word.IsOpenBracket() && (i < ((int)m_WordIndexes.size() - 1)) && (i > iStartWordIndex)) {
            InitClauseWords(clause, iStartWordIndex,  i-1);
            state = SearchCloseBracket;
            iStartWordIndex = i;
            return true;
        }

        if (state == SearchCloseBracket) {
            if (word.IsCloseBracket()) {
                int ii = SkipPuncts(i);
                InitClauseWords(clause, iStartWordIndex,  ii);
                iStartWordIndex = ii + 1;
                state = SearchComma;
                return true;
            } else if (i == (int)m_WordIndexes.size() - 1) {
                    InitClauseWords(clause, iStartWordIndex,  i);
                    iStartWordIndex = i + 1;
                    return true;
                } else
                    continue;
        }

        if (state == SearchCloseBracket)
            continue;

        if (GlobalGramInfo->HasSimConjNotWithCommaObligatory(word) && i > iStartWordIndex) {
            InitClauseWords(clause, iStartWordIndex,  i - 1);
            iStartWordIndex = i;
            return true;
        }

        if (i == (int)m_WordIndexes.size() - 1) {
            InitClauseWords(clause, iStartWordIndex, i);
            iStartWordIndex = i + 1;
            return true;
        }

        if (IsLastPunct(m_WordIndexes[i])) {
            if (word.IsPoint())
                continue;
            int ii = SkipPuncts(i);
            InitClauseWords(clause, iStartWordIndex, ii);
            iStartWordIndex = ii + 1;
            return true;
        }
    }
    return false;
}

bool CFragmentationRunner::IsLastPunct(SWordHomonymNum& wh)
{
    return m_Words.GetOriginalWord(m_Words.GetWord(wh).GetSourcePair().LastWord()).IsPunct();
}

void CFragmentationRunner::RunClauseRules()
{
    bool bClone = true; //разрешать ли клонирование
    for (size_t i = 0; i < m_ClauseVars.size(); i++) {
        CClauseVariant& ClauseVar = m_ClauseVars[i];
        ClauseVar.RunRules(bClone);
        ClauseVar.CalculateBadWeight();
        if (ClauseVar.GetBadWeight() == 0) {
            for (size_t j = 0; j < ClauseVar.m_newClauseVariants.size(); j++)
                ClauseVar.m_newClauseVariants[j].Reset();
            ClauseVar.m_newClauseVariants.clear();
            break;
        }
        m_ClauseVars.insert(m_ClauseVars.end(), ClauseVar.m_newClauseVariants.begin(), ClauseVar.m_newClauseVariants.end());
        bClone = (m_ClauseVars.size() < (size_t)g_iMaxVarsCount);
    }
}

bool CFragmentationRunner::FindCorr(const Wtroka& strCorr, int iWordPos)
{
    size_t i = 0;
    for (; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& word = m_Words.GetAnyWord(i);
        if (word.GetSourcePair().LastWord() >= iWordPos)
            return false;
        int iH = word.HasPOSi(gCorrelateConj);
        if (iH == -1)
            continue;
        const CHomonym& hom = word.GetRusHomonym(iH);
        if (hom.GetLemma() != strCorr)
            continue;
        if (word.GetSourcePair().LastWord() < iWordPos)
            break;
    }

    return i < m_Words.GetAllWordsCount();
}

bool CFragmentationRunner::FindConjForCorr(Wtroka strCorr, int iWordPos)
{
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& word = m_Words.GetAnyWord(i);
        if (word.GetSourcePair().FirstWord() <= iWordPos)
            continue;
        int iH = word.HasConjHomonym_i();
        if (iH == -1)
            continue;
        const CHomonym& hom = word.GetRusHomonym(iH);
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, CONJ_DICT);
        int ic = piArt->get_corrs_count();
        for (int j = 0; j < ic; ++j)
            if (strCorr == piArt->get_corr(j))
                return true;
    }

    return false;
}

void CFragmentationRunner::CheckCorrForSimConj()
{
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& word = m_Words.GetAnyWord(i);
        int iH = word.HasPOSi(gCorrelateConj);
        if (iH == -1)
            continue;
        if (!FindConjForCorr(GlobalDictsHolder->GetAuxArticle(word.GetRusHomonym(iH), CONJ_DICT)->get_title(),
                             word.GetSourcePair().LastWord())) {
            if (!word.IsOriginalWord() && word.GetRusHomonymsCount() == 1) {
                //m_Words.DeleteAnyWord(i);
                //i--;
            } else {
                static const TGramBitSet corrconj(gCorrelateConj);
                word.DeleteHomonymWithPOS(corrconj);
            }
        }
    }

    //заплта для "сколько", который может быть и подч. союзом,
    //и соч. союзом "не столько, ... сколько"
    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& word = m_Words.GetAnyWord(i);
        int k = word.HasSimConjHomonym_i();
        if (k == -1)
            continue;
        CHomonym& hom = word.GetRusHomonym(k);
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, CONJ_DICT);
        int iCorrsCount = piArt->get_corrs_count();
        int j;
        for (j = 0; j < iCorrsCount; j++)
            if (FindCorr(piArt->get_corr(j), word.GetSourcePair().FirstWord()))
                break;

        if (j >= iCorrsCount)
            continue;
        static const TGramBitSet subconj(gSubConj), pronconj(gPronounConj);
        word.DeleteHomonymWithPOS(subconj);
        word.DeleteHomonymWithPOS(pronconj);
    }
}

void CFragmentationRunner::FindConjs()
{
    //должна вызываться в самом начале
    //ASSERT(m_Words.GetMultiWordsCount() == 0);

    int iW2 = -1;
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        CContentsCluster* Cont = GlobalDictsHolder->FindCluster(m_Words[i].GetLowerText(), CONJ_DICT, CompByWord);

        if (Cont != NULL) {
            if (Cont->m_IDs.size() != 0)
                //нашли однословный союз
                for (size_t k = 0; k < Cont->m_IDs.size(); ++k)
                    InitConj(Cont->m_IDs[k], i, i, -1);

            //если iW2 != -1, значит до iW2 мы уже нашли союз
            if ((int)i <= iW2)
                continue;

            for (size_t j = 0; j < Cont->size(); ++j) {
                if (Cont->at(j).m_Content.size() == 1)
                    break; //значит многословных больше нет

                //long ID = Cont->at(j).m_ID;
                iW2 = CompWithWords(Cont->at(j).m_Content, i);
                if (iW2 != -1) {
                    for (size_t k = 0; k < Cont->at(j).m_IDs.size(); ++k)
                        InitConj(Cont->at(j).m_IDs[k], i, iW2, -1);
                    break;
                }
            }
        }

        //должны искать по лемме для "мест_союз", а "что - подч_союз" совпадает
        //с "что - мест_союз"
        {
            //предпологаем пока, что многословные по лемме не ищутся
            CWord& word = m_Words[i];
            CWord::SHomIt it = word.IterHomonyms();
            if (word.m_variant.size() != 0 || word.m_bHasUnusefulPostfix)
                it = CWord::SHomIt();

            for (; it.Ok(); ++it) {
                Cont = GlobalDictsHolder->FindCluster(it->GetLemma(), CONJ_DICT, CompByLemma);
                if (Cont && Cont->m_IDs.size() != 0) {
                    for (size_t k = 0; k < Cont->m_IDs.size(); ++k)
                        InitConj(Cont->m_IDs[k], i, i, it.GetID());
                    break;
                }
            }
        }
    }

    CheckCorrForSimConj();
}

void CFragmentationRunner::FindPreps()
{
    int iW2 = -1;
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        CWord& w = m_Words[i];
        CContentsCluster* Cont = GlobalDictsHolder->FindCluster(m_Words[i].GetLowerText(), PREP_DICT, CompByWord);
        if (Cont != NULL) {
            if (Cont->m_IDs.size() != 0)
                //нашли однословный предлог
                for (size_t k = 0; k < Cont->m_IDs.size(); k++)
                    InitPrep(Cont->m_IDs[k], i, i, w.HasPOSi(gPreposition));

            //если iW2 != -1, значит до iW2 мы уже нашли союз
            if ((int)i <= iW2)
                continue;

            for (size_t j = 0; j < Cont->size(); j++) {
                if (Cont->at(j).m_Content.size() == 1)
                    break; //значит многословных больше нет

                iW2 = CompWithWords(Cont->at(j).m_Content, i);
                if (iW2 != -1) {
                    for (size_t k = 0; k < Cont->at(j).m_IDs.size(); k++)
                        InitPrep(Cont->at(j).m_IDs[k], i, iW2, -1);
                    break;
                }
            }
        }
    }
}

void CFragmentationRunner::FindTemplateArticles()
{
    //должна вызываться в самом начале

    for (size_t i = 0; i < m_Words.GetAllWordsCount(); ++i) {
        CWord& word = m_Words.GetAnyWord(i);

        for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
            long ArtID = GlobalDictsHolder->FindTemplateArticle(*it);
            if (ArtID != -1)
                word.PutAuxArticle(it.GetID(), SDictIndex(TEMPLATE_DICT, ArtID), true);
        }
    }
}

void CFragmentationRunner::FindPredics()
{
    //должна вызываться в самом начале
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        CContentsCluster* Cont = GlobalDictsHolder->FindCluster(m_Words[i].GetLowerText(), PREDIC_DICT, CompByWord);

        if (Cont != NULL && Cont->m_IDs.size() != 0)
            for (size_t k = 0; k < Cont->m_IDs.size(); ++k)
                InitPredic(Cont->m_IDs[k], i);
    }
}

int CFragmentationRunner::CompWithWords(yvector<Wtroka>& word, int iStartWord)
{
    size_t i = 0;
    size_t j = 0;

    if (!word.size())
        return -1;

    for (i = iStartWord; i < m_Words.OriginalWordsCount(); ++i) {
        if (j >= word.size())
            break;

        if (m_Words[i].GetLowerText() != word[j++])
            return -1;
    }

    if (j >= word.size())
        return (int)i - 1;

    return -1;
}

void CFragmentationRunner::InitPrep(long ArtID, int iW1, int iW2, int iH)
{
    article_t* piArt = GlobalDictsHolder->GetDict(PREP_DICT)[ArtID];

    THomonymPtr pNewHom;
    TGramBitSet pos;

    if (iW1 == iW2) {
        if (iH == -1)
            return;
        CWord& word = m_Words[iW1];
        pNewHom = word.m_Homonyms[iH];
        pos = pNewHom->Grammems.GetPOS();
        if (piArt->get_del_homonym())
            word.DeleteAllHomonyms();
        word.PutAuxArticle(iH, SDictIndex(PREP_DICT, ArtID), true);
    } else {
        pos = piArt->get_new_pos();
        if (pos.none())
            return;

        int iW = m_Words.FindMultiWord(iW1, iW2);
        if (iW == -1) {
            if (m_Words.HasMultiWord(iW1))
                return;

            CWord* pNewWord = new CWord(piArt->get_title(), false);
            pNewWord->m_SourceWords.SetPair(iW1, iW2);
            pNewHom = new CHomonym(TMorph::GetMainLanguage(), pNewWord->m_txt);
            pNewHom->PutAuxArticle(SDictIndex(/*ConjDic - mistyped? */ PREP_DICT, ArtID));
            pNewWord->AddRusHomonym(pNewHom);
            m_Words.AddMultiWord(pNewWord);
        } else {
            CHomonym* pNewHom = new CHomonym(TMorph::GetMainLanguage(), piArt->get_title());
            pNewHom->PutAuxArticle(SDictIndex(/*ConjDic - mistyped? */ PREP_DICT, ArtID));

            //вызываем именно CWord::AddRusHomonym
            //так как в переопределенной у MultiWord ф-ции AddRusHomonym
            //проверяется, чтобы был обязательно CWordSequence
            m_Words.GetMultiWord(iW).CWord::AddRusHomonym(pNewHom);
        }
    }

    if (pNewHom.Get() != NULL) {
        for (size_t i = 0; i < piArt->get_valencies().size(); ++i) {
            const or_valencie_list_t& vals = piArt->get_valencies().at(i);
            for (size_t j = 0; j < vals.size(); ++j) {
                const valencie_t& val = vals[j];
                if (val.m_morph_scale.size() == 1)
                    pNewHom->Grammems.Reset(*(val.m_morph_scale.begin()));  // complete replacement of previous grammems???
            }
        }
        pNewHom->Grammems.SetPOS(pos);
    }
}

void CFragmentationRunner::InitConj(long ArtID, int iW1, int iW2, int iH)
{
    article_t* piArt = GlobalDictsHolder->GetDict(CONJ_DICT)[ArtID];

    if (iW1 == iW2) {
        CWord& word = m_Words[iW1];

        if (piArt->get_new_pos().any()) {
            THomonymPtr pNewHom;

            if (iH != -1)
                pNewHom = word.m_Homonyms[iH]->Clone();
            else
                pNewHom = word.IterHomonyms()->Clone();

            pNewHom->PutAuxArticle(SDictIndex(CONJ_DICT, ArtID));
            pNewHom->Grammems.SetPOS(piArt->get_new_pos());

            if (piArt->get_del_homonym())
                word.DeleteAllHomonyms();
            else {
                if (pNewHom->HasPOS(gPronounConj))
                    word.DeleteHomonymWithPOS(TGramBitSet(gSubstPronoun));
                else
                    word.DeleteHomonymWithPOS(TGramBitSet(gConjunction));
            }

            word.AddRusHomonym(pNewHom);
        } else {
            if (iH != -1) {

                if (piArt->get_del_homonym()) {
                    THomonymPtr pNewHom = word.m_Homonyms[iH]->Clone();
                    word.DeleteAllHomonyms();
                    pNewHom->PutAuxArticle(SDictIndex(CONJ_DICT, ArtID));
                    word.AddRusHomonym(pNewHom);
                } else
                    word.PutAuxArticle(iH, SDictIndex(CONJ_DICT, ArtID), true);
            } else {
                CWord::SHomIt it = word.IterHomonyms();
                for (; it.Ok(); ++it)
                    if (it->HasPOS(piArt->get_pos()))
                        break;

                if (it.Ok()) {
                    if (piArt->get_del_homonym()) {
                        THomonymPtr pNewHom = it->Clone();
                        word.DeleteAllHomonyms();
                        pNewHom->PutAuxArticle(SDictIndex(CONJ_DICT, ArtID));
                        word.AddRusHomonym(pNewHom);
                    } else
                        word.PutAuxArticle(it.GetID(), SDictIndex(CONJ_DICT, ArtID), true);
                }
            }
        }
    } else {
        if (piArt->get_new_pos().none())
            return;

        int iW = m_Words.FindMultiWord(iW1, iW2);

        if (iW == -1) {
            CWord* pNewWord = new CWord(piArt->get_title(), false);
            pNewWord->m_SourceWords.SetPair(iW1, iW2);
            CHomonym* pNewHom = new CHomonym(TMorph::GetMainLanguage(), pNewWord->m_txt);
            pNewHom->Grammems.SetPOS(piArt->get_new_pos());
            pNewHom->PutAuxArticle(SDictIndex(CONJ_DICT, ArtID));
            pNewWord->AddRusHomonym(pNewHom);
            m_Words.AddMultiWord(pNewWord);
        } else {
            CHomonym* pNewHom = new CHomonym(TMorph::GetMainLanguage(), piArt->get_title());
            pNewHom->Grammems.SetPOS(piArt->get_new_pos());
            pNewHom->PutAuxArticle(SDictIndex(CONJ_DICT, ArtID));
            CWord& word = m_Words.GetMultiWord(iW);
            //вызываем именно CWord::AddRusHomonym
            //так как в переопределенной у MultiWord ф-ции AddRusHomonym
            //проверяется, чтобы был обязательно CWordSequence
            word.CWord::AddRusHomonym(pNewHom);
        }
    }
}

bool CFragmentationRunner::InitPredic(long ArtID, int iW)
{
    article_t* piArt = GlobalDictsHolder->GetDict(PREDIC_DICT)[ArtID];
    TGramBitSet POS = piArt->get_pos();
    TGramBitSet newPOS = piArt->get_new_pos();

    CWord& word = m_Words[iW];
    int iH = word.HasPOSi(POS);

    if (iH == -1)
        iH = word.HasPOSi(newPOS);

    if (iH != -1) {
        if (piArt->get_del_homonym()) {
            THomonymPtr pHom = word.GetRusHomonym(iH).Clone();
            pHom->PutAuxArticle(SDictIndex(PREDIC_DICT, ArtID));
            word.DeleteAllHomonyms();
            word.AddRusHomonym(pHom);
        } else
            word.PutAuxArticle(iH, SDictIndex(PREDIC_DICT, ArtID), true);
        return true;
    }

    if (newPOS.any()) {
        THomonymPtr pNewHom = word.IterHomonyms()->Clone();
        pNewHom->PutAuxArticle(SDictIndex(PREDIC_DICT, ArtID));
        pNewHom->Grammems.SetPOS(piArt->get_new_pos());
        if (piArt->get_del_homonym())
            word.DeleteAllHomonyms();
        word.AddRusHomonym(pNewHom);
        return true;
    }

    return false;
}

void CFragmentationRunner::DelAdjShortAdverbHomonym()
{
    static const TGramBitSet AdjectiveShort(gAdjective, gShort);
    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        CWord& word = m_Words[i];
        int iHom = word.HasPOSi(AdjectiveShort);
        if (iHom != -1) {
            const CHomonym& hom = word.GetRusHomonym(iHom);
            if (hom.HasGrammem(gNeuter)) {
                size_t j = 0;
                for (; j < m_Words.OriginalWordsCount(); ++j) {
                    const CWord& word1 = m_Words[j];
                    CWord::SHomIt it = word1.IterHomonyms();

                    for (; it.Ok(); ++it) {
                        const CHomonym& hom = *it;
                        int ii;
                        if (CClause::CanBeSubject(hom, ii)) {
                            static const TGramBitSet mask(gNeuter, gNominative);
                            if (hom.Grammems.HasAll(mask))
                                break;
                        }
                    }
                    if (it.Ok())
                        break;
                }

                if (j >= m_Words.OriginalWordsCount())
                    word.DeleteHomonymWithPOS(AdjectiveShort);
            }
        }
    }
}
/*
void CFragmentationRunner::AddDebugVar(xmlNodePtr piSentEl, int iVar)
{
    for (size_t i = 0; i < m_ClauseVars[iVar].m_DebugVars.size(); i++) {
        xmlNodePtr piVarEl = xmlNewNode(NULL, (const xmlChar*)"variant");
        Stroka strVarNum;
        if (i + 1 == m_ClauseVars[iVar].m_DebugVars.size())
            strVarNum = Substitute("Variant $0 (After rule  - $1, Bad weight - $2):",
                                   iVar+1, m_ClauseVars[iVar].m_DebugVars[i].m_strDescr,
                                   m_ClauseVars[iVar].m_DebugVars[i].m_pVariant->GetBadWeight());
        else
            strVarNum = Substitute("Variant $0 (After rule  - $1):",
                                   iVar+1, m_ClauseVars[iVar].m_DebugVars[i].m_strDescr);
        AddAsciiAttr(piVarEl, "variant_num", strVarNum);

        m_ClauseVars[iVar].m_DebugVars[i].m_pVariant->PrintToXML(piVarEl);
        m_ClauseVars[iVar].m_DebugVars[i].m_pVariant->Reset();
        delete m_ClauseVars[iVar].m_DebugVars[i].m_pVariant;
        xmlAddChild(piSentEl, piVarEl);
    }
}

void CFragmentationRunner::PrintToXML(xmlNodePtr piSentEl, int iSentNum)
{
    AddAsciiAttr(piSentEl, "sent_num", Substitute("Sentence $0: ($1 variants) ", iSentNum, m_ClauseVars.size()));

    for (size_t i = 0; i < m_Words.OriginalWordsCount(); ++i) {
        const CWord& word = m_Words[i];
        xmlAddChild(piSentEl, CreateWordTagAndPrint(word));
    }

    for (size_t i = 0; i < m_ClauseVars.size(); i++) {
        if (CGramInfo::s_bFramentationDebug)
            AddDebugVar(piSentEl, i);

        if (!CGramInfo::s_bFramentationDebug) {
            xmlNodePtr piVarEl = xmlNewNode(NULL,(const xmlChar*)"variant");
            AddAsciiAttr(piVarEl, "variant_num",
                         Substitute("Variant $0 (Bad weight - $1):", i+1, m_ClauseVars[i].GetBadWeight()));

            m_ClauseVars[i].PrintToXML(piVarEl);
            xmlAddChild(piSentEl, piVarEl);
        }
    }
}

xmlNodePtr CFragmentationRunner::CreateWordTagAndPrint(const CWord& word) const
{
    xmlNodePtr piWordEl = xmlNewNode(NULL, (const xmlChar*)"w");
    AddAttr(piWordEl, "str", word.GetText());

    for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
        TStringStream ss;
        it->Print(ss, CODES_UTF8);
        xmlNodePtr piHomEl = xmlNewNode(NULL, (const xmlChar*)"h");
        AddUtf8Attr(piHomEl, "str", ss);
        xmlAddChild(piWordEl, piHomEl);
    }
    return piWordEl;
}
*/
