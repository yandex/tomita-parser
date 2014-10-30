#include "primitivegroup.h"
#include "normalization.h"
#include <FactExtract/Parser/afdocparser/rus/numberprocessor.h>
#include <FactExtract/Parser/afdocparser/rus/namecluster.h>


namespace NImpl {

// Getting article lemma info - moved from CDictsHolder

bool HasDictionaryLemma(const CHomonym& h, EDicType dic) {
    if (dic == KW_DICT && h.HasGztArticle() && h.GetGztArticle().GetLemmaInfo() != NULL)
        return true;

    const article_t* piArt = GlobalDictsHolder->GetAuxArticle(h, dic);
    return piArt != NULL && piArt->has_lemma();
}

const SArticleLemmaInfo& GetLemmaInfo(const CHomonym& h, EDicType dic, Wtroka& lemma_text, SArticleLemmaInfo& lemma_info_scratch) {
    if (dic == KW_DICT && h.HasGztArticle() &&
        h.GetGztArticle().FillSArticleLemmaInfo(lemma_info_scratch, lemma_text))
            return lemma_info_scratch;
    else {
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(h, dic);
        if (piArt == NULL) {
            lemma_info_scratch.Reset();
            lemma_text.clear();
            return lemma_info_scratch;
        }
        lemma_text = piArt->get_lemma();
        return piArt->get_lemma_info();
    }
}

Wtroka BuildLemmaForm(const SArticleLemmaInfo& lemmaInfo, const TGramBitSet& grammems) {
    Wtroka s = lemmaInfo.m_Lemmas[lemmaInfo.m_iMainWord];

    CWordVector rusWords;
    CNormalization normalize(rusWords);

    // создадим CWord для главного слова
    CWord w(s, false);
    w.CreateHomonyms();

    // найдем первое попавшееся сущ. совпадающее с леммой
    int iH = -1;
    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it) {
        const CHomonym& h = *it;
        if (h.HasPOS(gSubstantive) && h.GetLemma() == s) {
            iH = it.GetID();
            break;
        }
    }

    if (iH == -1)
        return Wtroka();

    const CHomonym& mainH = w.GetRusHomonym(iH);
    TGramBitSet genders_numbers_cases = TMorph::GuessNormalGenderNumberCase(mainH.Grammems.All(), grammems);

    TGramBitSet nounGrammems = genders_numbers_cases | (mainH.Grammems.All() & TGramBitSet(gAnimated, gInanimated));
    Wtroka sMainLemma = normalize.GetFormWithGrammems(w, mainH, nounGrammems);
    if (sMainLemma.empty())
        sMainLemma = w.GetText();

    Wtroka res;
    const TGramBitSet active_passive(gActive, gPassive);
    for (size_t i = 0; i < lemmaInfo.m_Lemmas.size(); ++i) {
        res += ' ';
        if ((int)i == lemmaInfo.m_iMainWord)
            res += sMainLemma;
        else if ((int)i < lemmaInfo.m_iMainWord) {
                CWord w(lemmaInfo.m_Lemmas[i], false);
                w.CreateHomonyms();
                int iAdjH = w.HasMorphAdj_i();
                if (iAdjH == -1) {
                    res += lemmaInfo.m_Lemmas[i];
                    continue;
                }

                const CHomonym& adjHom = w.GetRusHomonym(iAdjH);
                TGramBitSet adj_grammems = genders_numbers_cases;
                if (adjHom.IsFullParticiple()) {
                    adj_grammems |= mainH.Grammems.All() & NSpike::AllTimes;
                    adj_grammems |= mainH.Grammems.All() & active_passive;
                }

                Wtroka s = normalize.GetFormWithGrammems(w, adjHom , adj_grammems);
                if (s.empty())
                    res += lemmaInfo.m_Lemmas[i];
                else
                    res += s;
            } else
                res += lemmaInfo.m_Lemmas[i];
    }
    return StripString(res);
}

void SurroundWithQuotes(Wtroka& text, const CWord& w) {
    if (w.HasOpenQuote())
        text = Wtroka((wchar16)'\"') + text;
    if (w.HasCloseQuote())
        text += '\"';
}

Wtroka GetDictionaryLemma(const CWord& w, const CHomonym& h, EDicType dic) {
    Wtroka res = GlobalDictsHolder->GetUnquotedDictionaryLemma(h, dic);
    if (!res.empty())
        SurroundWithQuotes(res, w);
    return  res;

}

Wtroka GetDictionaryLemma(const CWord& w, const CHomonym& h, const TGramBitSet& grammems, EDicType dic) {
    Wtroka res;
    SArticleLemmaInfo gzt_lemma_info;
    const SArticleLemmaInfo& lemma_info = GetLemmaInfo(h, dic, res, gzt_lemma_info);

    if (!lemma_info.IsEmpty() && !lemma_info.m_bIndeclinable) {
        res = BuildLemmaForm(lemma_info, grammems);
        if (!res)
            res = GetDictionaryLemma(w, h, dic);
        if (!res.empty())
            SurroundWithQuotes(res, w);
    }
    return res;
}

// выдает сточку с разделенными ";" леммами (нужно, когда для некоторого
// поля факта хранятся все леммы из него - для сливалки)]
void BuildCommaSeparatedLemmas(const CHomonym& h, bool bMainWords, yvector<Wtroka>& res, EDicType dic) {
    Wtroka lemma_text;
    SArticleLemmaInfo gzt_lemma_info;
    const SArticleLemmaInfo& lemma_info = GetLemmaInfo(h, dic, lemma_text, gzt_lemma_info);

    if (lemma_info.IsEmpty())
        return;

    // если ЛЕММА.НЕИЗМ = да
    if (lemma_info.m_bIndeclinable) {
        for (size_t i = 0; i < lemma_info.m_Lemmas.size(); ++i)
            if (bMainWords && (int)i == lemma_info.m_iMainWord)
                res.push_back(Wtroka(wchar16('*')) + lemma_info.m_Lemmas[i]);
            else
                res.push_back(lemma_info.m_Lemmas[i]);
        return;
    }

    for (size_t i = 0; i < lemma_info.m_Lemmas.size(); ++i) {
        if (bMainWords && (int)i == lemma_info.m_iMainWord) {
            res.push_back(Wtroka(wchar16('*')) + lemma_info.m_Lemmas[i]);
            continue;
        }

        CWord w(lemma_info.m_Lemmas[i], false);
        w.CreateHomonyms();
        CWord::SHomIt it = w.IterHomonyms();
        for (; it.Ok(); ++it) {
            const CHomonym& h = *it;
            if (h.GetLemma() == lemma_info.m_Lemmas[i]) {
                res.push_back(h.GetLemma());
                break;
            }
        }
        // значит не нашли омонима с леммой
        // совпадающей с очередным словом из поля ЛЕММА
        if (!it.Ok()) {
            int iH = -1;
            if ((int)i < lemma_info.m_iMainWord)
                iH = w.HasMorphAdj_i();
            else
                iH = w.HasMorphNoun_i();
            if (iH == -1) {
                YASSERT(w.GetHomonymsCount() > 0);
                res.push_back(w.IterHomonyms()->GetLemma());
            } else
                res.push_back(w.GetRusHomonym(iH).GetLemma());
        }
    }
}

// grammems-filter functinoids
struct TGramFilter {
    TGramBitSet m_required_grammems;
    TGramFilter(const TGramBitSet& required_grammems)
        : m_required_grammems(required_grammems)
    {
    }

    bool operator()(const TGramBitSet& checked_grammems) const {
        return checked_grammems.HasAll(m_required_grammems);
    }
};

struct TGramFilterFullAdj: public TGramFilter {
    TGramFilterFullAdj(const TGramBitSet& required_grammems)
        : TGramFilter(required_grammems)
    {
    }

    bool operator()(const TGramBitSet& checked_grammems) const {
        return TMorph::IsFullAdjective(checked_grammems)
            && checked_grammems.HasAll(m_required_grammems);
    }
};

} // namespace NImpl

CNormalization::CNormalization(const CWordVector& rWords, const CWorkGrammar* pGram)
    : m_Words(rWords)
    , m_pGLRgrammar(pGram)
{
}

void CNormalization::UpdateFioFactField(const CFactSynGroup* pParentGroup, const yvector<SWordHomonymNum>& FdoChainWords, CFioWS& newFioWS) const
{
    const CFactSynGroup* pGroup = GetChildGroupByWordSequence(FdoChainWords, pParentGroup, &newFioWS);
    if (!pGroup) // если не смогли найти нужную группу, то значит поле ФИО пришло из другого факта, а значит
        return;  // и обновлять нечего
    CWordSequence* pWsq = pGroup->GetFirstMainHomonym()->GetSourceWordSequence();
    const CFioWordSequence* pFioWS = dynamic_cast<const CFioWordSequence*>(pWsq);
    if (!pFioWS)
        return;
    newFioWS.m_Fio = pFioWS->m_Fio;
    memcpy(newFioWS.m_NameMembers,pFioWS->m_NameMembers, sizeof(pFioWS->m_NameMembers));
    int iMainWord = pGroup->GetMainPrimitiveGroup()->FirstWord();;
    SWordHomonymNum mainWH = FdoChainWords[iMainWord];
    mainWH.m_HomNum = GetFirstHomonymInTomitaGroup(FdoChainWords, pGroup, iMainWord);
    newFioWS.SetMainWord(mainWH);
}

void CNormalization::Normalize(CFactFields& fact, const yvector<SWordHomonymNum>& FdoChainWords,
                               const CFactSynGroup* pGroup) const
{
    const fact_type_t* fact_type = &GlobalDictsHolder->RequireFactType(fact.GetFactName());
    for (size_t i = 0; i < fact_type->m_Fields.size(); i++) {
        const fact_field_descr_t& fact_field_descr = fact_type->m_Fields.at(i);

        switch (fact_field_descr.m_Field_type) {
        case FioField:
            {
                CFioWS* fioWS = fact.GetFioValue(fact_field_descr.m_strFieldName);
                if (!fioWS)
                    break;
                UpdateFioFactField(pGroup, FdoChainWords, *fioWS);
                break;
            }
        case TextField:
            {
                TGramBitSet caseForNorm = TGramBitSet(gNominative);
                CTextWS* pWS = fact.GetTextValue(fact_field_descr.m_strFieldName);
                if (pWS != NULL) {
                    caseForNorm = pWS->m_iNormGrammems;
                    if (fact_field_descr.m_options.find("gennorm") != fact_field_descr.m_options.end())
                        caseForNorm.ReplaceByMask(TGramBitSet(gGenitive), NSpike::AllCases);
                    NormalizeWordSequence(FdoChainWords, pGroup,  pWS, caseForNorm);
                }
                break;
            }
        case DateField:
            {
                DateNormalize(fact.GetDateValue(fact_field_descr.m_strFieldName));
                break;
            }
        default:
            /* nothing to do? */
            break;
        }
    }
    // нужно будет протащить через язык алгоритм для определения коротких имен: ф-ция CheckCompanyShortName
    // перенести в правила грамматики условия ф-ции IsEnoughForLemma
}

size_t GetUnionSize(const TGramBitSet& d1, const TGramBitSet& d2)
{
    return (d1 & d2).count();
};

int CNormalization::GetHomonymByTerminalWithTheseGrammems(const CWord& w, TerminalSymbolType s, const TGramBitSet& grammems) const
{
    CWord::SHomIt it = w.IterHomonyms();
    CWord::SHomIt best_which_has_this_terminal;
    CWord::SHomIt best_which_has_this_terminal_and_grammems;
    size_t MaxUnionSize = 0;

    for (; it.Ok(); ++it) {
        const CHomonym& hom = *it;
        if (it->ContainTerminal(s)) {
            if (!best_which_has_this_terminal.Ok() || hom.HasGrammem(gMasculine))
                best_which_has_this_terminal = it;

            if (hom.Grammems.HasAll(grammems)) {
                if (!best_which_has_this_terminal_and_grammems.Ok() || hom.HasGrammem(gMasculine))
                    best_which_has_this_terminal_and_grammems = it;
            } else {
                size_t UnionSize = GetUnionSize(grammems, hom.Grammems.All());
                if (UnionSize > MaxUnionSize) {
                    best_which_has_this_terminal = it;
                    MaxUnionSize = UnionSize;
                }
            }
        }
    }

    if (best_which_has_this_terminal_and_grammems.Ok())
        return best_which_has_this_terminal_and_grammems.GetID();

    if (best_which_has_this_terminal.Ok())
        return best_which_has_this_terminal.GetID();

    return -1;
}

// normalize words of word-sequence which is retrieved not from grammar but just from aux_dic
// without any syntactic relations
void CNormalization::NormalizeSimpleWordSequence(const CWordsPair& wp, yvector<SWordSequenceLemma>& lemmas,
                                                 SWordHomonymNum mainWH, const TGramBitSet& caseForNorm) const
{
    const CHomonym& nounHom = m_Words[mainWH];
    TGramBitSet genders_numbers_cases(gSingular);
    static const TGramBitSet active_passive_mask(gActive, gPassive),
                             perfect_imperfect_mask(gPerfect, gImperfect);

    // main word could be either adjective or participle
    // for example (translit) "ispolnyauschiy obyazannosti"
    if (nounHom.IsMorphAdj()) {
        if (nounHom.HasGrammem(gMasculine))
            genders_numbers_cases.Set(gMasculine);
        else
            genders_numbers_cases.Set(gFeminine);
    } else
        genders_numbers_cases |= nounHom.Grammems.All() & NSpike::AllGenders;

    genders_numbers_cases |= (caseForNorm & NSpike::AllCases);

    for (int i = wp.FirstWord(); i < m_Words.GetWord(mainWH).GetSourcePair().FirstWord(); i++) {
        const CWord& pW = m_Words[i];
        int iH = -1;
        iH = pW.HasMorphAdj_i();
        TGramBitSet grammems = genders_numbers_cases;

        if (iH == -1) {
            lemmas.push_back(SWordSequenceLemma(pW.GetText()));
            continue;
        }
        const CHomonym& adjHom = m_Words[i].GetRusHomonym(iH);

        if (adjHom.IsFullParticiple()) {
            grammems |= nounHom.Grammems.All() & NSpike::AllTimes;
            grammems |= nounHom.Grammems.All() & active_passive_mask;
            grammems |= nounHom.Grammems.All() & perfect_imperfect_mask;
        }
        Wtroka s = GetFormWithGrammems(pW, adjHom, grammems);
        if (s.empty())
            s = m_Words.GetWord(mainWH).GetText();
        else
            s = GetCapitalizedLemma(pW, s);
        lemmas.push_back(SWordSequenceLemma(s));
    }

    TGramBitSet main_word_grammems = genders_numbers_cases;
    if (nounHom.IsFullParticiple()) {
        main_word_grammems |= nounHom.Grammems.All() & NSpike::AllTimes;
        main_word_grammems |= nounHom.Grammems.All() & active_passive_mask;
        main_word_grammems |= nounHom.Grammems.All() & perfect_imperfect_mask;
    }

    if (!nounHom.IsMorphAdj() && !nounHom.IsMorphNoun())
        main_word_grammems.Reset();

    Wtroka s = GetFormWithGrammems(m_Words.GetWord(mainWH), nounHom, main_word_grammems);
    if (s.empty())
        s = m_Words.GetWord(mainWH).GetText();
    else
        s = GetCapitalizedLemma(m_Words.GetWord(mainWH), s);

    lemmas.push_back(SWordSequenceLemma(s));

    for (int i = m_Words.GetWord(mainWH).GetSourcePair().LastWord() + 1; i <=wp.LastWord(); i++) {
        const CWord& pW = m_Words[i];
        lemmas.push_back(SWordSequenceLemma(pW.GetText()));
    }
}

Wtroka CNormalization::GetArtificialLemma(const SWordHomonymNum& Word, const TGramBitSet& grammems) const
{
    const CHomonym& H = m_Words[Word];
    Wtroka res;
    yvector<SWordSequenceLemma> tmpLemmas;

    CWordSequence* pWS = H.GetSourceWordSequence();

    if (H.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Number, KW_DICT)) {
        CNumber* pNumber = (CNumber*)(pWS);
        if (pNumber && !pNumber->m_Numbers.empty())
            return CharToWide(ToString(static_cast<int>(pNumber->m_Numbers[0])));

    } else if (H.HasArtificialLemma()) {
        TGramBitSet cases = grammems & NSpike::AllCases;
        if (cases.none() || cases == TGramBitSet(gNominative)) {
            if (!pWS->IsNormalizedLemma()) {
                // lazy normalization
                NormalizeSimpleWordSequence(*pWS, tmpLemmas, pWS->GetMainWord(), cases);
                pWS->ResetLemmas(tmpLemmas, true);
            }
            res = pWS->GetCapitalizedLemma();

        } else {
            NormalizeSimpleWordSequence(*pWS, tmpLemmas, pWS->GetMainWord(), grammems);
            CWordSequence tmp;
            tmp.ResetLemmas(tmpLemmas, true);
            res = tmp.GetCapitalizedLemma();
        }
    }

    if (res.empty() && pWS->Size() > 1)
        res = m_Words.ToString(*pWS);

    return res;
}

const CFactSynGroup* CNormalization::GetChildGroupByWordSequence(const yvector<SWordHomonymNum>& FDOChainWords,
                                                                 const CFactSynGroup* pGroup, const CWordSequence* pWs) const
{
    CWordsPair P = ConvertToTomitaCoordinates(FDOChainWords, *pWs);
    if (!P.IsValid())
        return NULL;
    const CFactSynGroup* pParent = pGroup;
    pGroup = pParent->GetChildByWordPair(P);
    if (!pGroup && P.LastWord() + 1 == pParent->LastWord() &&
        m_Words.GetWord(FDOChainWords[pParent->LastWord()]).IsEndOfStreamWord()) {
        P.SetPair(P.FirstWord(), P.LastWord()+1);
        pGroup = pParent->GetChildByWordPair(P);
    }
    return pGroup;
}

static bool IsAlwaysReplace(const CHomonym& h) {
    if (h.HasGztArticle()) {
        const NGzt::TMessage* lemma_info = h.GetGztArticle().GetLemmaInfo();
        if (lemma_info != NULL)
            return h.GetGztArticle().GetLemmaAlways(*lemma_info);
    } else if (h.HasAuxArticle(KW_DICT))
        return GlobalDictsHolder->GetAuxArticle(h, KW_DICT)->get_lemma_info().m_bAlwaysReplace;

    return false;
}

//traverse syntax tree and see if the article from which we are going to take lemma has been checked
//since we can check this only for main word, traverse only those groups where our word is main.
//pGroup - a group containing normalized wh
//wpFromFDOChainWords - wh in FDOChainWords system of coordinates
bool CNormalization::HomonymCheckedByKWType(const CFactSynGroup* pGroup, SWordHomonymNum wh, CWordsPair wpFromFDOChainWords) const
{
    const CHomonym& h = m_Words[wh];
    if (!h.HasGztArticle() && !h.HasAuxArticle(KW_DICT))
        return false;
    if (IsAlwaysReplace(h))
        return true;

    yvector<const CGroup*> groups;
    GetBranch(groups, wpFromFDOChainWords, pGroup);

    YASSERT(m_pGLRgrammar != NULL);
    for (size_t i = 0; i < groups.size(); i++) {
        int iSymb = groups[i]->GetActiveSymbol();
        const CGrammarItem& grammarItem = m_pGLRgrammar->m_UniqueGrammarItems[iSymb];
        if (GlobalDictsHolder->HasArticle(h, grammarItem.GetPostfixKWType(DICT_KWTYPE), KW_DICT))
            return true;
    }
    return false;
}

//"cut" labels processed, traverse all the tree from top to bottom
// If some node has label @m_exclude_from_normalization then add all leaves (word numbers)
// of this node's sub-tree to @excludedWords and afterwards remove them from normalization
void CNormalization::GetExcludedWords(const CGroup* pCurGroup, yset<int>& excludedWords) const
{
    YASSERT(pCurGroup != NULL);
    YASSERT(m_pGLRgrammar != NULL);
    int iSymb = pCurGroup->GetActiveSymbol();
    const CGrammarItem& grammarItem = m_pGLRgrammar->m_UniqueGrammarItems[iSymb];
    if (grammarItem.m_PostfixStore.m_exclude_from_normalization == YEs) {
        for (int i = pCurGroup->FirstWord(); i <= pCurGroup->LastWord(); i++)
            excludedWords.insert(i);
        return;
    }

    if (!pCurGroup->IsPrimitive()) {
        const CFactSynGroup* pSynGroup = CheckedCast<const CFactSynGroup*>(pCurGroup);
        for (size_t i = 0; i < pSynGroup->m_Children.size(); i++)
            GetExcludedWords(CheckedCast<const CGroup*>(pSynGroup->m_Children[i]),
                             excludedWords);
    }
}

// Traverse tree @pCurGroup putting into @groups those groups
// which main words (primitive groups) match with @wp
void CNormalization::GetBranch(yvector<const CGroup*>& groups, const CWordsPair& wp, const CGroup* pCurGroup)const
{
    if (*(pCurGroup->GetMainPrimitiveGroup()) == wp)
        groups.push_back(pCurGroup);

    if (pCurGroup->IsPrimitive())
        return;

    const CFactSynGroup* pSynGroup = CheckedCast<const CFactSynGroup*>(pCurGroup);
    for (size_t i = 0; i < pSynGroup->m_Children.size(); i++) {
        const CGroup* pChildGroup = CheckedCast<const CGroup*>(pSynGroup->m_Children[i]);
        if (pChildGroup->Includes(wp)) {
            GetBranch(groups, wp, pChildGroup);
            break;
        }
    }
}

Wtroka CNormalization::GetDictionaryLemma(SWordHomonymNum wh, const TGramBitSet& grammems, const CWordsPair& pWs) const
{
    const CWord& w = m_Words.GetWord(wh);
    const CHomonym& h = m_Words[wh];

    DECLARE_STATIC_RUS_WORD(kRespublika, "республика");
    DECLARE_STATIC_RUS_WORD(kGorod, "город");
    DECLARE_STATIC_RUS_WORD(kG, "г.");

    // Ugly workaround for geography - for republics and cities normalization.
    // For example, input "Tatarskiy" (translit) linked to lemma "Tatarstan" in the dictionary.
    // The grammar will extract sequence "Tatarskaya respublika" with word "respublika" having a "cut" label
    // And we still need to transform "Tatarskaya" to "Tatarstan".
    // The same with city, e.g. "gorod Moskva" should be transformed to just "Moskva" and normalized too - while the main word is "gorod"

    //if( h.HasKWType(GeoAdm_KW) || h.HasKWType(GeoCity_KW) || h.HasKWType(GeoHauptstadt_KW) || h.HasKWType(GeoCountry_KW))
    if (GlobalDictsHolder->BuiltinKWTypes().IsGeo(GlobalDictsHolder->GetKWType(h, KW_DICT))) {
        int ii = w.GetSourcePair().FirstWord();
        if (ii > 0) {
            CWord& wPrev = m_Words.GetOriginalWord(ii-1);
            //if the sequence being normalized is larger than GEO, e.g. [translit] "mer (administratsii goroda Moskvy) Y.Luzhkov"
            if (pWs.Includes(wPrev.GetSourcePair()) && (pWs.FirstWord() < wPrev.GetSourcePair().FirstWord())) {
                int iH = wPrev.GetHomonymIndex(kRespublika);
                if (iH == -1)
                    iH = wPrev.GetHomonymIndex(kGorod);
                if (iH != -1) {
                    const CHomonym& respH = wPrev.GetRusHomonym(iH);
                    if (respH.HasGrammem(gGenitive)) {
                        Wtroka lemma = NImpl::GetDictionaryLemma(w, m_Words[wh],
                            (grammems & ~NSpike::AllCases & ~NSpike::AllGenders) | TGramBitSet(gGenitive), KW_DICT);
                        return GetCapitalizedLemma(w, lemma);
                    }
                }
            }

            //if the sequence being normalized is equal to GEO, e.g. [translit] "mer (goroda Moskvy) Y.Luzhkov"
            else if (pWs.FirstWord() == wPrev.GetSourcePair().FirstWord() && pWs.LastWord() == w.GetSourcePair().FirstWord()) {
                if (wPrev.HasHomonym(kGorod) || wPrev.HasHomonym(kG)) {
                    Wtroka lemma = NImpl::GetDictionaryLemma(w, m_Words[wh],
                        (grammems & ~NSpike::AllCases & ~NSpike::AllGenders) | TGramBitSet(gNominative), KW_DICT);
                    return GetCapitalizedLemma(w, lemma);
                }
            }

        }
    }
    //another spike for GEO: if geography is a Genetive, make a lemma also Genetive
    //since genetive case for geography is not checked in most cases
    Wtroka dict_lemma;
    if (GlobalDictsHolder->BuiltinKWTypes().IsGeo(GlobalDictsHolder->GetKWType(h, KW_DICT))) {
        TGramBitSet cases = grammems & NSpike::AllCases;
        if (grammems.none() || cases.count() > 1)       //2009-07-29, mowgli: "several cases" condition added
        {
            if (h.HasGrammem(gGenitive)) {
                TGramBitSet req_grams = grammems.none() ?
                    TGramBitSet(gGenitive, gSingular) : ((grammems & ~NSpike::AllCases) | TGramBitSet(gGenitive));
                dict_lemma = NImpl::GetDictionaryLemma(w, h, req_grams, KW_DICT);
            } else dict_lemma = NImpl::GetDictionaryLemma(w, h, KW_DICT);
        } else dict_lemma = NImpl::GetDictionaryLemma(w, h, grammems, KW_DICT);
    } else if (h.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Number, KW_DICT)) {
            CNumber* pNumber = (CNumber*)(h.GetSourceWordSequence());
            if (!pNumber->m_Numbers.empty())
                dict_lemma = CharToWide(ToString(static_cast<int>(pNumber->m_Numbers[0])));
        } else
            dict_lemma = NImpl::GetDictionaryLemma(w, m_Words[wh], grammems, KW_DICT);
    return GetCapitalizedLemma(w, dict_lemma);
}

//if original word is fully uppercased then capitalize whole @strLemma too
//if only first letter is uppercased and the word is not the first in sentence, then capitalize only this letter.
Wtroka CNormalization::GetCapitalizedLemmaSimple(const CWord& w, Wtroka strLemma)
{
    if (strLemma.empty())
        return Wtroka();
    if (w.HasAtLeastTwoUpper())
        TMorph::ToUpper(strLemma);
    else if (w.m_bUp && w.GetSourcePair().FirstWord() > 0)
        NStr::ToFirstUpper(strLemma);

    return strLemma;
}

Wtroka CNormalization::GetCapitalizedLemma(const CWord& w, const Wtroka& strLemma)
{
    if (strLemma.empty())
        return Wtroka();
    Wtroka res;
    bool bFirstWord = (w.GetSourcePair().FirstWord() == 0);
    if (!w.IsMultiWord()) {
        const Wtroka& strWordLow = w.GetLowerText();

        //do not uppercase the first word until this is declared mandatory in fact_types
        if (bFirstWord && !w.HasAtLeastTwoUpper())
            res = strLemma;
        else if (strWordLow == strLemma)
            res = w.GetText();
        else {
            bool bAllIsUpper = true;
            bool bFindDash = false;

            //идем до первой буквы, различающейся в лемме и исходном слове,
            //и меняем регистр в лемме
            //если есть регистр, то после различающейся буквы идем до дефиса,
            //а затем опять до первой различающейся
            size_t i, j;
            for (i = 0, j = 0; i < strWordLow.size() && j < strLemma.size();) {
                if (NStr::IsLangAlpha(w.GetText()[i], TMorph::GetMainLanguage()))
                    bAllIsUpper = bAllIsUpper && ::IsUpper(w.GetText()[i]);

                if (!bFindDash) {
                    if (strWordLow[i] == strLemma[j]) {
                        res.append(w.GetText()[i]);
                        i++;
                        j++;
                    } else {
                        bFindDash = true;
                    }
                } else {
                    if (strchr("- ",strWordLow[i]) && strchr("- ", strLemma[j])) {
                        res.append(strLemma[j]);
                        i++;
                        j++;
                        bFindDash = false;
                    } else if (!strchr("- ", strWordLow[i]) && strchr("- ",strLemma[j])) {
                        i++;
                    } else if (strchr("- ",strWordLow[i]) && !strchr("- ",strLemma[j])) {
                        res.append(strLemma[j]);
                        j++;
                    } else {
                        // break if the first letters differ
                        if ((i == 0) || (j == 0))
                            break;
                        //otherwise go to hyphen (if any)
                        res.append(strLemma[j]);
                        i++;
                        j++;
                        bFindDash = true;
                    }
                }
            }
            if (i == 0 || j == 0)
                res = GetCapitalizedLemmaSimple(w, strLemma);
            else {
                if (j < strLemma.size())
                    res.append(~strLemma + j, +strLemma - j);
                //to avoid turning "RF" into "Rossia" [translit]
                int dif = strWordLow.size() - strLemma.size();
                if (dif < 0)
                    dif *= -1;
                if (bAllIsUpper && dif <= 2)
                    TMorph::ToUpper(res);
            }
        }
    } else {
        static const Wtroka delims = CharToWide("_ ");
        StringTokenizer<Wtroka> words(w.GetText(), delims);
        bool bFirst = true;
        bool bHReg1 = false;
        bool bHReg2 = false;
        TWtringBuf tok;
        while (words.NextToken(tok)) {
            if (NStr::IsLangAlpha(tok[0], TMorph::GetMainLanguage()) && ::IsUpper(tok[0])) {
                if (bFirst) {
                    bHReg1 = true;
                    bFirst = false;
                }
                bHReg2 = true;
            } else {
                bHReg2 = false;
                break;
            }
        }
        res = strLemma;
        if (bHReg2)
            CWordSequence::CapitalizeFirst2(res);
        else if (bHReg1)
            CWordSequence::CapitalizeFirst1(res);
    }

    return res;
}

void CNormalization::NormalizeWordSequence(const yvector<SWordHomonymNum>& FDOChainWords,
                                           const CFactSynGroup* pGroup, CTextWS* pWs, const TGramBitSet& caseForNorm) const
{
    yset<int> excludedWords;
    NormalizeWordSequence(FDOChainWords, pGroup, pWs, excludedWords, caseForNorm);
}

void CNormalization::NormalizeWordSequenceOnlyWithDictionariesLemmas(const yvector<SWordHomonymNum>& FDOChainWords,
                                                                     const CFactSynGroup* pParentGroup, CTextWS* pWs) const
{
    const CFactSynGroup* pGroup = GetChildGroupByWordSequence(FDOChainWords, pParentGroup, pWs);

    for (int i = pGroup->FirstWord(); i <= pGroup->LastWord(); i++) {
        Wtroka s = GetOriginalWordStrings(FDOChainWords, i, pGroup, *pWs, true);
        pWs->AddLemma(SWordSequenceLemma(s));

        SWordHomonymNum wh = FDOChainWords[i];
        wh.m_HomNum = GetFirstHomonymInTomitaGroup(FDOChainWords, pGroup, i);
        pWs->AddWordInfo(m_Words[wh].GetLemma());
    }
}

// функция нормализует pWs, которая была построена в группе pGroup.
// сначала главное слово нормализуется в свою собственную лемму, затем устанавливаются
// формы всех подчиненных  слов. Слово Х подчинено главному слову, если существует
// цепочка отношений согласований,прописанных в грамматике и  связывающих Х и главное слово,
// caseForNorm - падеж, в который ставить главное слово(и согласованные с ним)
// если caseForNorm == AllGrammems, то главное слово не трогаем, это нужно, чтобы
// заменять исходные слова на лемму из aux_dict
// excludedWords - передаются слова, которые нужно исключить принормализации из-зи пометы trim
void CNormalization::NormalizeWordSequence(const yvector<SWordHomonymNum>& FDOChainWords,
                                           const CFactSynGroup* pParentGroup, CTextWS* pWs , yset<int>& excludedWords,
                                           TGramBitSet caseForNorm /*= gNominative */) const
{
    if (caseForNorm.none())
        caseForNorm.Set(gNominative);

    if (!pWs || pWs->HasLemmas())
        return;

    const TBuiltinKWTypes& builtins = GlobalDictsHolder->BuiltinKWTypes();

    pWs->ClearWordsInfos();
    yvector<TGramBitSet> NewGrammems(FDOChainWords.size(), TGramBitSet());
    yvector<Wtroka> Norms(FDOChainWords.size(), Wtroka());
    yvector<bool> bMainWords(FDOChainWords.size(),false);

    const CFactSynGroup* pGroup = GetChildGroupByWordSequence(FDOChainWords, pParentGroup, pWs);

    if (!pGroup) {
        Stroka S = Substitute("CNormalization::GetChildGroupByWordSequence: cannot locate wordsequence [$0,$1] in group [$2,$3]",
            pWs->FirstWord(), pWs->LastWord(), pParentGroup->FirstWord(), pParentGroup->LastWord());
        ythrow yexception() << S;
    };

    //get words excluded from normalization with "cut" label
    GetExcludedWords(pGroup, excludedWords);

    // skipFirstNumeralsCount - specifies how many words could be omitted starting from the beginning of group
    // these words are numerals or construction "odin iz" [translit]
    size_t skipFirstNumeralsCount = 0;
    bool isMainWordNoun = true;

    int mainWordNo = -1;
    // normalization of main word = converting it to nomivative case, singular number
    if (caseForNorm != NSpike::AllGrammems) {
        mainWordNo = pGroup->GetMainPrimitiveGroup()->FirstWord();
        int mainWordHomNo = 0;
        {
            const CWord* pMainWord = &m_Words.GetWord(FDOChainWords[mainWordNo]);
            long homIDs = pGroup->GetChildHomonyms()[mainWordNo - pGroup->FirstWord()];
            mainWordHomNo = pMainWord->HasFirstHomonymByHomIDs_i(homIDs);
        }
        ShiftMainWordIfOdinIz(FDOChainWords, pGroup, mainWordNo, mainWordHomNo, skipFirstNumeralsCount);

        SWordHomonymNum word = FDOChainWords[mainWordNo];
        word.m_HomNum = mainWordHomNo;

        const CWord* pMainWord = &m_Words.GetWord(FDOChainWords[mainWordNo]);
        const CHomonym* pHom = &pMainWord->GetRusHomonym(mainWordHomNo);
        isMainWordNoun = pHom->IsMorphNoun();

        TGramBitSet Grammems;

        // Participle is normalized to its own normal form, not verb infinitive
        //if (pHom->IsFullParticiple())
        if (!pHom->HasUnknownPOS() && pHom->Grammems.HasAny(NSpike::AllCases))     //just use grammems of word, when possible
            Grammems = pHom->Grammems.All() & ~NSpike::AllCases;
        else
            Grammems = GetLemmaGrammems(*pHom, pMainWord->m_typ);                   //otherwise ask morph

        //if @Grammems is empty, take gender and number from homonym if any
        if (Grammems.none())
            Grammems = pHom->Grammems.All() & (NSpike::AllNumbers | NSpike::AllGenders);

        //set Nominative if homonym has some cases
        if (pHom->Grammems.HasAny(NSpike::AllCases))
            Grammems.Set(gNominative);

        // when normalizing a name, set Nominative and remove other cases
        if (Grammems.HasAny(NSpike::AllCases)) {
            if (caseForNorm.HasAny(NSpike::AllCases))
                Grammems.ReplaceByMask(caseForNorm, NSpike::AllCases);
            else
                Grammems.ReplaceByMask(TGramBitSet(gNominative), NSpike::AllCases);

            //Grammems = (Grammems & ~NSpike::AllCases) | (caseForNorm & NSpike::AllCases);
            if (caseForNorm.HasAny(NSpike::AllGenders) && Grammems.HasAny(NSpike::AllGenders))
                Grammems.ReplaceByMask(caseForNorm, NSpike::AllGenders);

            //use Singular!
            if (Grammems.HasAny(NSpike::AllNumbers))
                Grammems.ReplaceByMask((caseForNorm.HasAny(NSpike::AllNumbers))? caseForNorm : TGramBitSet(gSingular), NSpike::AllNumbers);

            //remove any animated/inanimated flags (not required to get normal form)
            Grammems &= ~TGramBitSet(gAnimated, gInanimated);

        }
        //nim : 23.12.05
        //when word is not found in morphology and predicted as adjective, transfer grammems of expected form
        if (Grammems.none())
            Grammems = caseForNorm;

        // if the lemma has all genders leave only Masculine to enable agreement of the rest part with it.
        // +adjectives for words like [translit] "kollega" (i.e. Masculine+Feminine) will always be normalized to Masculine
        if (Grammems.HasAny(TGramBitSet(gMasculine, gMasFem)))
            Grammems.ReplaceByMask(TGramBitSet(gMasculine), NSpike::AllGenders);

        // if lemma has simultaniously a singular and plural numbers (e.g. "kolli" [translit])
        // then leave only singular to enable agreement of the rest words with it
        if (Grammems.HasAll(NSpike::AllNumbers))
            Grammems.ReplaceByMask(TGramBitSet(gSingular), NSpike::AllNumbers);

        NewGrammems[mainWordNo] = Grammems;
        if (pMainWord->IsMultiWord() || pHom->HasKWType(builtins.Fio, KW_DICT) ||
                (pHom->HasKWType(builtins.Number, KW_DICT)
                 && HomonymCheckedByKWType(pGroup, word, CWordsPair(mainWordNo))))
            Norms[mainWordNo] = GetArtificialLemma(word, Grammems);
        else if (NImpl::HasDictionaryLemma(*pHom, KW_DICT) && HomonymCheckedByKWType(pGroup, word, CWordsPair(mainWordNo)))
            Norms[mainWordNo] = GetCapitalizedLemma(*pMainWord, NImpl::GetDictionaryLemma(*pMainWord, *pHom, KW_DICT));

        if (Norms[mainWordNo].empty())
            Norms[mainWordNo] = GetFormWithGrammemsAndCheck(*pMainWord, *pHom, Grammems);

        YASSERT(!Norms[mainWordNo].empty());
        bMainWords[mainWordNo] = true;
    };

    bool bUpdated = true;
    while (bUpdated && caseForNorm != NSpike::AllGrammems) {
        bUpdated = false;
        for (size_t i=0; i<pGroup->m_CheckedAgrs.size(); i++) {
            const CCheckedAgreement& A = pGroup->m_CheckedAgrs[i];
            int OldWordNo,NewWordNo;
            {
                // if the word to be argeed is out of the specified sequence of words
                // then ignore such agreement
                if (!pWs->Includes(m_Words.GetWord(FDOChainWords[A.m_WordNo1]).GetSourcePair())) continue;
                if (!pWs->Includes(m_Words.GetWord(FDOChainWords[A.m_WordNo2]).GetSourcePair())) continue;

                OldWordNo = A.m_WordNo1;
                NewWordNo = A.m_WordNo2;

                // если оба слова не были сгенерированы, либо оба слова уже были сгенерированы,
                // тогда пропускаем эту функцию согласования
                if (Norms[OldWordNo].empty() == Norms[NewWordNo].empty())
                    continue;

                // calculation of "new" and "old" words
                if (Norms[OldWordNo].empty()) {
                    std::swap (OldWordNo, NewWordNo);
                };

                // if group with this word has a "cut" label
                if (excludedWords.find(NewWordNo) != excludedWords.end())
                    continue;

            }
            bUpdated = true;

            SWordHomonymNum Old = FDOChainWords[OldWordNo];
            SWordHomonymNum New = FDOChainWords[NewWordNo];

            // find out which grammems the new form should have
            Old.m_HomNum = GetFirstHomonymInTomitaGroup(FDOChainWords, pGroup, OldWordNo);
            New.m_HomNum = GetFirstHomonymInTomitaGroup(FDOChainWords, pGroup, NewWordNo);
            const CHomonym* NewHom = &m_Words.GetWord(New).GetRusHomonym(New.m_HomNum);

            TGramBitSet DelGrammems;
            switch (A.m_AgreementType.e_AgrProcedure) {
                case CoordFuncCount:    assert(false); break;
                case GendreNumberCase   : DelGrammems = NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers; break;
                case GendreNumber       : DelGrammems = NSpike::AllGenders | NSpike::AllNumbers; break;
                case GendreCase         : DelGrammems = NSpike::AllGenders | NSpike::AllCases; break;
                case NumberCaseAgr      : DelGrammems = NSpike::AllCases | NSpike::AllNumbers; break;
                case CaseAgr            : DelGrammems = NSpike::AllCases; break;
                case FeminCaseAgr       : DelGrammems = NSpike::AllCases; break;
                case NumberAgr          : DelGrammems = NSpike::AllNumbers; break;
                default                 : /* do nothing? */ break;
            };
            // Здесь нет проверки для "gnc" на то, что  первый элемент - существительное, а второе прилагательное ,
            // это должно случиться само собой, поскольку существительное - главное слово
            TGramBitSet Grammems = (NewHom->Grammems.All() & ~DelGrammems) | (NewGrammems[OldWordNo] & DelGrammems);
            //normalize numbers
            if (Grammems.HasAll(NSpike::AllNumbers))
                Grammems.ReplaceByMask(TGramBitSet(gSingular), NSpike::AllNumbers);
            //remove any animated/inanimated flags (not required to get normal form)
            Grammems &= ~TGramBitSet(gAnimated, gInanimated);

            NewGrammems[NewWordNo] =  Grammems;
            const CWord& NewWord = m_Words.GetWord(New);
            if (NewWord.IsMultiWord() || NewHom->HasKWType(builtins.Fio, KW_DICT) ||
                (NewHom->HasKWType(builtins.Number, KW_DICT) && HomonymCheckedByKWType(pGroup,New, CWordsPair(mainWordNo)))) {
                assert (Norms[NewWordNo].empty());
                Norms[NewWordNo] = GetArtificialLemma(New, Grammems);
                // если не удалось нормализовать и номализуется одиночное слово,
                // тогда пробуем его нормализовать как одиночное слово
                if (Norms[NewWordNo].empty() &&  NewWord.GetSourcePair().Size() == 1)
                    Norms[NewWordNo] = GetFormWithGrammemsAndCheck(NewWord, *NewHom, Grammems);

                YASSERT(!Norms[NewWordNo].empty());
            } else {
                if (NImpl::HasDictionaryLemma(*NewHom, KW_DICT) && HomonymCheckedByKWType(pGroup,New, CWordsPair(NewWordNo)))
                    Norms[NewWordNo] = GetDictionaryLemma(New, Grammems, *pWs);
                if (Norms[NewWordNo].empty())
                    Norms[NewWordNo] = GetFormWithGrammemsAndCheck(NewWord, *NewHom, Grammems);
            }

            if (NewHom->IsMorphNoun())
                bMainWords[NewWordNo] = true;
        };
    };

    for (size_t i=0; i<pGroup->m_CheckedAgrs.size(); i++) {
        const CCheckedAgreement& A = pGroup->m_CheckedAgrs[i];
        // if this is unary agreement (just grammems)
        // then set the word to this grammems if if has dictionary lemma
        if (A.IsUnary() && A.m_WordNo1 != mainWordNo) {
            if (!Norms[A.m_WordNo1].empty())
                continue;

            //if group containing this word has a "cut" label
            if (excludedWords.find(A.m_WordNo1) != excludedWords.end())
                continue;

            SWordHomonymNum wh =    FDOChainWords[A.m_WordNo1];
            if (!pWs->Includes(m_Words.GetWord(wh).GetSourcePair())) continue;
            wh.m_HomNum = GetFirstHomonymInTomitaGroup(FDOChainWords, pGroup, A.m_WordNo1);
            const CHomonym& h = m_Words[wh];
            if (NImpl::HasDictionaryLemma(h, KW_DICT) && HomonymCheckedByKWType(pGroup,wh, CWordsPair(A.m_WordNo1))) {
                TGramBitSet grams = A.m_AgreementType.m_Grammems;

                //take required missing grammems from the very word
                if (!grams.HasAny(NSpike::AllCases))
                    grams |= h.Grammems.All() & NSpike::AllCases;

                if (!grams.HasAny(NSpike::AllNumbers))
                    grams |= h.Grammems.All() & NSpike::AllNumbers;
                if (grams.HasAll(NSpike::AllNumbers))
                    grams.Reset(gPlural);

                if (!grams.HasAny(NSpike::AllGenders))
                    grams |= h.Grammems.All() & NSpike::AllGenders;
                if (grams.HasAny(TGramBitSet(gMasculine, gMasFem)))
                    grams.ReplaceByMask(TGramBitSet(gMasculine), NSpike::AllGenders);

                Wtroka s = GetDictionaryLemma(wh,grams, *pWs);
                if (!s.empty())
                    Norms[A.m_WordNo1] = s;
            }
        }
    }

    // generating m_Lemmas
    pWs->ClearLemmas();

    //nim: 20.06.05
    int iMax = pGroup->LastWord();
    if (m_Words.GetWord(FDOChainWords[iMax]).IsEndOfStreamWord())
        iMax--;

    //adding special labels
    if (!isMainWordNoun)
        pWs->SetAdj();

    for (int i = pGroup->FirstWord(); i <= iMax; i++) {
        assert (i < (int)FDOChainWords.size());
        const CWord& w = m_Words.GetWord(FDOChainWords[i]);
        const CWordsPair& Pair = w.GetSourcePair();

        //skip leading numerals
        assert(pWs->Includes(Pair));
        if (Pair.FirstWord() < pWs->FirstWord()+(int)skipFirstNumeralsCount) continue;

        //if group containing this word has a "cut" label
        if (excludedWords.find(i) != excludedWords.end())
            continue;

        Wtroka s = !Norms[i].empty() ? Norms[i] : GetOriginalWordStrings(FDOChainWords, i, pGroup, *pWs);
        pWs->AddLemma(SWordSequenceLemma(s));

        SWordHomonymNum W = FDOChainWords[i];
        W.m_HomNum = GetFirstHomonymInTomitaGroup(FDOChainWords, pGroup, i);
        const CHomonym& H = m_Words[W];

        // if there is dictionary lemma then we should append distinct words contained in Lemma property
        if (NImpl::HasDictionaryLemma(H, KW_DICT)) {
            yvector<Wtroka> res;
            NImpl::BuildCommaSeparatedLemmas(H, bMainWords[i], res, KW_DICT);
            if (res.size() > 0) {
                for (size_t uu = 0; uu < res.size(); uu++)
                    pWs->AddWordInfo(res[uu]);
                continue;
            }
        }

        if (Norms[i].empty() && Pair.Size() == 1) {
            // retrieve lemma for words which has not been made agree
            s = H.GetLemma();
        } else {
            CWordSequence* pSourceWS = H.GetSourceWordSequence();
            if (pSourceWS) {
                CFioWordSequence* pFioWs  = dynamic_cast<CFioWordSequence*>(pSourceWS);
                if (pFioWs) {
                    s = pFioWs->GetFio().m_strSurname;
                    if (!s.empty()) {
                        if (bMainWords[i])
                            s.prepend('*');
                        pWs->AddWordInfo(s);
                    }
                    s = pFioWs->GetFio().m_strName;
                    if (!s.empty()) {
                        if (bMainWords[i])
                            s.prepend('*');
                        pWs->AddWordInfo(s);
                    }

                    s = pFioWs->GetFio().m_strPatronomyc;
                    if (!s.empty())
                        pWs->AddWordInfo(s);
                } else {
                    CTextWS* pSourceTextWS = dynamic_cast<CTextWS*>(pSourceWS);
                    if (pSourceTextWS)
                        pWs->AddWordInfo(*pSourceTextWS, bMainWords[i]);
                    else {
                        for (int k = pSourceWS->FirstWord(); k <= pSourceWS->LastWord(); ++k) {
                            const CWord& w = m_Words[k];
                            yset<Wtroka> words;
                            for (CWord::SHomIt h_it = w.IterHomonyms(); h_it.Ok(); ++h_it) {
                                Wtroka lemma = h_it->GetLemma();
                                if (bMainWords[i])
                                    lemma.prepend('*');
                                words.insert(lemma);
                            }
                            yset<Wtroka>::iterator it = words.begin();
                            for (; it != words.end(); it++)
                                pWs->AddWordInfo(*it);
                        }
                    }
                }
                continue;
            }

        }

        if (bMainWords[i])
            s.prepend('*');
        pWs->AddWordInfo(s);
    };
};

void CNormalization::DateNormalize(CWordSequence* ws) const
{
    if (!ws) return;

    SWordHomonymNum ws_date;
    ws_date = ws->GetMainWord();
    const CWord& MultDateWrd = m_Words.GetWord(ws_date);
    if (MultDateWrd.IsOriginalWord()) return;
    const CWordSequence* pWordSeq = m_Words[ws_date].GetSourceWordSequence();
    if (DateTimeWS != pWordSeq->GetWSType()) return;
    const CDateGroup* pDateGroup = CheckedCast<const CDateGroup*>(pWordSeq);

    Stroka strYear, strMonth, strDay;
    if (pDateGroup->m_iDay.size() && 0 < pDateGroup->m_iDay[0])
        strDay = Sprintf("%02d.", pDateGroup->m_iDay[0]);
    else
        strDay = "00.";

    if (pDateGroup->m_iMonth.size() && 0 < pDateGroup->m_iMonth[0])
        strMonth = Sprintf("%02d.", pDateGroup->m_iMonth[0]);
    else
        strMonth = "00.";

    if (pDateGroup->m_iYear.size() && 0 < pDateGroup->m_iYear[0])
        strYear = Sprintf("%d", pDateGroup->m_iYear[0]);
    else
        strYear = "0000";

    ws->AddLemma(SWordSequenceLemma(CharToWide(strDay + strMonth + strYear)));
}

//returns the first homonym number for word @WordNo which is made agree with parsing
int  CNormalization::GetFirstHomonymInTomitaGroup(const yvector<SWordHomonymNum>& FDOChainWords,
                                                  const CFactSynGroup* pGroup, int WordNo) const
{
    const SWordHomonymNum& wh = FDOChainWords[WordNo];
    const CWord& w1 = m_Words.GetWord(wh);
    ui32 homIDs = pGroup->GetChildHomonyms()[WordNo - pGroup->FirstWord()];
    int iH = w1.HasFirstHomonymByHomIDs_i(homIDs);
    YASSERT(iH >= 0);
    return iH;
};

CWordsPair CNormalization::ConvertToTomitaCoordinates(const yvector<SWordHomonymNum>& FDOChainWords, const CWordsPair& group) const
{
    int first = -1, last = -1;
    for (size_t i = 0; i < FDOChainWords.size(); ++i) {
        const CWordsPair& p =  m_Words.GetWord(FDOChainWords[i]).GetSourcePair();
        if (p.FirstWord() == group.FirstWord())
            first = i;

        if (p.LastWord() == group.LastWord()) {
            last = i;
            break;
        }
    }

    return (first != -1 && last != -1) ? CWordsPair(first, last) : CWordsPair();
};

bool CNormalization::ShiftMainWordIfOdinIz(const yvector<SWordHomonymNum>& FDOChainWords, const CFactSynGroup* pGroup,
                                           int& mainWordNo, int& mainWordHomNo, size_t& ShiftSize) const
{
    ShiftSize = 0;

    DECLARE_STATIC_RUS_WORD(kOdin, "один");
    DECLARE_STATIC_RUS_WORD(kIz, "из");

    const CHomonym* pHom =  &m_Words.GetWord(FDOChainWords[mainWordNo]).GetRusHomonym(mainWordHomNo);
    if (pHom->GetLemma() != kOdin || mainWordNo + 1 == (int)FDOChainWords.size())
        return false;

    const CWord* pIzWord = &m_Words.GetWord(FDOChainWords[mainWordNo + 1]);
    if (!NStr::EqualCi(pIzWord->GetOriginalText(), kIz))
        return false;

    ShiftSize = 2;
    if (mainWordNo + (int)ShiftSize != pGroup->LastWord()) {
        const CWord* pPossibleNumeral = &m_Words.GetWord(FDOChainWords[mainWordNo + ShiftSize]);
        if (pPossibleNumeral->m_typ == Digit || pPossibleNumeral->HasPOS(gNumeral))
            ShiftSize++;
    }
    pGroup = pGroup->GetMaxChildWithThisFirstWord(mainWordNo + ShiftSize);
    if (!pGroup) {
        ShiftSize = 0;
        return false;
    }

    const CWord* pMainWord = pGroup->GetMainWord();
    mainWordNo = pGroup->GetMainPrimitiveGroup()->FirstWord();
    mainWordHomNo = pMainWord->HasFirstHomonymByHomIDs_i(pGroup->GetChildHomonyms()[mainWordNo - pGroup->FirstWord()]);
    return true;
};

TGramBitSet CNormalization::GetLemmaGrammems(const CHomonym& hom, ETypeOfPrim eGrafemWrdType) const
{
    TGramBitSet grammems = TMorph::GetLemmaGrammems(hom.Lemma, hom.Grammems.All());
    if (grammems.any())
        return grammems;

    if (hom.GetSourceWordSequence() != 0) {
        const SWordHomonymNum& MainWord = hom.GetSourceWordSequence()->GetMainWord();
        if (MainWord.IsValid() && m_Words[MainWord].Lemma != hom.Lemma)
            return GetLemmaGrammems(m_Words[MainWord], eGrafemWrdType);
        else
            return TGramBitSet();
    }

    //last try - if the word has hyphen in it - get grammems of last part
    if (Hyphen == eGrafemWrdType)
        return TMorph::GetLemmaGrammems(hom.Lemma.substr(hom.Lemma.rfind('-') + 1), hom.Grammems.All());
    else
        return TGramBitSet();
}

Wtroka CNormalization::GetFormOfTheFirstPartOfHyphenedWord(Wtroka FirstPart, const CHomonym& hom, const TGramBitSet& common_grammems) const
{
    if (FirstPart.empty()) return FirstPart;
    if (FirstPart[FirstPart.size() - 1] != '-')
        return FirstPart;
    FirstPart.erase(FirstPart.size() - 1);

    Wtroka res = FirstPart + '-';
    // look in morphology
    THomonymVector homonyms, agreed_homonyms;
    TMorph::GetDictHomonyms(FirstPart, homonyms);

    //leave only homonyms with same case as original two-part homonym
    for (size_t i = 0; i < homonyms.size(); ++i)
        if ((homonyms[i]->Grammems.All() & hom.Grammems.All() & NSpike::AllCases).any())
            agreed_homonyms.push_back(homonyms[i]);

    // more checks only if single homonym
    if (agreed_homonyms.size() == 1) {
        Wtroka Form = GetFormWithThisGrammems(agreed_homonyms[0]->GetLemma(), FirstPart, common_grammems);
        if (!Form.empty())
            res = Form + '-';
    }

    return res;
}

Wtroka CNormalization::GetFormWithGrammemsAndCheck(const CWord& W, const CHomonym& Hom, const TGramBitSet& Grammems) const
{
    Wtroka WordStr = GetFormWithGrammems(W, Hom, Grammems);
    if (WordStr.empty()) {
        WordStr = Hom.GetLemma();
        YASSERT(!WordStr.empty());
        return GetCapitalizedLemma(W, WordStr);
    };
    return GetCapitalizedLemma(W, WordStr);
}

Wtroka CNormalization::GetOriginalWordStrings(const yvector<SWordHomonymNum>& FDOChainWords, int WordNo,
                                              const CFactSynGroup* pGroup, const CWordSequence& wsForNormalization,
                                              bool bOnlyWhenAlwaysReplace) const
{
    Wtroka res;

    SWordHomonymNum wh = FDOChainWords[WordNo];
    wh.m_HomNum = GetFirstHomonymInTomitaGroup(FDOChainWords, pGroup, WordNo);
    const CHomonym& h = m_Words[wh];

    if (NImpl::HasDictionaryLemma(h, KW_DICT)) {
        if (IsAlwaysReplace(h) || (!bOnlyWhenAlwaysReplace && HomonymCheckedByKWType(pGroup, wh, CWordsPair(WordNo))))

        res = GetDictionaryLemma(wh, TGramBitSet(), wsForNormalization);

    } else if (h.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Number, KW_DICT) && HomonymCheckedByKWType(pGroup, wh, CWordsPair(WordNo))) {
        CNumber* pNumber = CheckedCast<CNumber*>(h.GetSourceWordSequence());
        if (!pNumber->m_Numbers.empty())
            res = CharToWide(ToString<int>(static_cast<int>(pNumber->m_Numbers[0])));
    }

    if (res.empty()) {
        CWordsPair p = m_Words.GetWord(wh).GetSourcePair();
        for (int i = p.FirstWord(); i <= p.LastWord(); ++i) {
            res += m_Words[i].GetOriginalText();
            if (i < p.LastWord())
                res +=  ' ';
        }
    }
    return res;
};

// Generates form of given homonym with given grammems
Wtroka CNormalization::GetFormWithGrammems(const CWord& W, const CHomonym& Hom, TGramBitSet grammems) const
{
    if (Hom.HasKWType(GlobalDictsHolder->BuiltinKWTypes().Number, KW_DICT) && W.GetSourcePair().Size() > 1) {
        CNumber* pNumber = (CNumber*)(Hom.GetSourceWordSequence());
        YASSERT(!pNumber->m_Numbers.empty());
        return CharToWide(ToString(static_cast<int>(pNumber->m_Numbers[0])));
    }

    // if the word contains non-russian alphabetical chars - return without normalization
    if (!Hom.HasPOS(gAdjNumeral) || W.m_typ != Hyphen) {
        Wtroka lemma = Hom.GetLemma();
        for (size_t i = 0; i < lemma.size(); ++i)
            if (::IsAlpha(lemma[i]) && !NStr::IsLangAlpha(lemma[i], TMorph::GetMainLanguage()))
                return Hom.GetLemma();
    }

    // remove gender for immutable adjectives
    if (Hom.IsMorphAdj() && Hom.Grammems.IsIndeclinable())
        grammems &= ~NSpike::AllGenders;

    // отключаем вид, чтобы по слову 'американизировать' получить 'американизированный'
    //temporarily disabled
    //grammems &= ~TGramBitSet(gPerfect, gImperfect);

    // remove short and comparative form for adjectives
    grammems &= ~TGramBitSet(gShort, gComparative);

    // preserve superlative form
    if (Hom.HasGrammem(gSuperlative))
        grammems |= TGramBitSet(gSuperlative);

    Wtroka WordStr;

    //if @Hom is ordinal numeral, like "1-iy", "2-oy" [translit]
    if (Hom.HasPOS(gAdjNumeral) && W.m_typ == Hyphen) {
        Wtroka s = Hom.GetLemma();
        size_t ii = s.rfind('-');
        if (ii != Wtroka::npos) {
            const Wtroka* sFlex = TMorph::GetNumeralFlexByGrammems(TWtringBuf(s).SubStr(0, ii), grammems);
            if (sFlex != NULL)
                return s.substr(0, ii+1) + *sFlex;
        }
    }

    if (!Hom.IsDictionary())
        //search with bastards
        WordStr = TMorph::GetClosestForm(Hom.Lemma, W.GetLowerText(), NImpl::TGramFilter(grammems), grammems, true);
    else
        WordStr = GetFormWithThisGrammems(Hom, W.GetLowerText(), grammems, W.m_typ);

    return WordStr;
};

Wtroka CNormalization::GetFormWithThisGrammems(const CHomonym& hom, const Wtroka& original_form,
                                                const TGramBitSet& common_grammems, ETypeOfPrim eGrafemWrdType) const
{
    if (TMorph::FindWord(hom.Lemma))
        return GetFormWithThisGrammems(hom.Lemma, original_form, common_grammems);

    if (Hyphen == eGrafemWrdType) {
        size_t ii = hom.Lemma.rfind('-');
        if (ii == Wtroka::npos)
            return Wtroka();
        size_t jj = original_form.rfind('-');

        Wtroka prefix = GetFormOfTheFirstPartOfHyphenedWord(hom.Lemma.substr(0, ii+1), hom, common_grammems);
        Wtroka lemma = GetFormWithThisGrammems(hom.Lemma.substr(ii+1),
            (jj == Wtroka::npos) ? original_form : original_form.substr(jj + 1), common_grammems);
        if (!prefix.empty() && !lemma.empty())
            return prefix + lemma;
    }
    return Wtroka();
}

Wtroka CNormalization::GetFormWithThisGrammems(const Wtroka& lemma, const Wtroka& original_form,
                                                const TGramBitSet& common_grammems) const
{
    //without bastards
    Wtroka res = TMorph::GetClosestForm(lemma, original_form, NImpl::TGramFilter(common_grammems), common_grammems, false);
    TMorph::ToLower(res);
    return res;
}

bool CNormalization::CheckCompanyShortName(const CWordSequence& wsCompName, const CWordSequence& wsCompanyShortName) const
{
    Wtroka strCompanyShortName;

    for (int i = wsCompanyShortName.FirstWord(); i <= wsCompanyShortName.LastWord(); ++i)
        strCompanyShortName += m_Words[i].GetText();

    if (1 >= strCompanyShortName.size())
        return false;

    TMorph::ToLower(strCompanyShortName);

    Wtroka strCompanyName;
    for (int i = wsCompName.FirstWord(); i <= wsCompName.LastWord(); ++i)
        strCompanyName += m_Words[i].GetText();

    TMorph::ToLower(strCompanyName);

    if (strCompanyShortName[0] != strCompanyName[0])
        return false;

    int iPrev = 0;
    size_t i = 0;
    for (; i < strCompanyShortName.size(); ++i) {
        wchar16 c = strCompanyShortName[i];
        size_t ii = strCompanyName.find(c, iPrev);
        if (ii == Wtroka::npos)
            break;
        iPrev = ii;
    }
    if (i >= strCompanyShortName.size())
        return true;
    return false;
}

int CNormalization::HasQuotedCompanyNameForNormalization(CWordSequence& quoted_compname) const
{
    int iStart, iEnd;
    iStart = m_Words.GetWord(quoted_compname.GetMainWord()).GetSourcePair().FirstWord();
    iEnd = m_Words.GetWord(quoted_compname.GetMainWord()).GetSourcePair().LastWord();
    UNUSED(iStart);
    UNUSED(iEnd);
    return -1;
}

void CNormalization::QuotedWordNormalize(CWordSequence& ws)
{
    int iWrd = HasQuotedCompanyNameForNormalization(ws);
    if (-1 == iWrd) return;
}

