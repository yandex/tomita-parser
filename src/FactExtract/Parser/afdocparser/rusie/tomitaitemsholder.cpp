#include "tomitaitemsholder.h"
#include "factgroup.h"
#include "dictsholder.h"

#include <FactExtract/Parser/afdocparser/rus/namecluster.h>
#include <FactExtract/Parser/geo_hierarchy/geo_hierarchy.h>

#include <FactExtract/Parser/inflectorlib/gramfeatures.h>

#include <kernel/gazetteer/articlepool.h>

#include <util/generic/hash.h>
#include <util/generic/intrlist.h>




class TTomitaGeoHierarchy {    // using non thread-safe impl, as it is always local object
public:
    TTomitaGeoHierarchy();
    bool HaveRelatedGeo(bool negate, const CWord& w1, const CWord& w2, ui32& hom1, ui32& hom2);

    inline bool IsRelatedGeo(const NGzt::TArticlePtr& a1, const NGzt::TArticlePtr& a2) {
        if (Impl.Get())
            return Impl->IsRelated(a1, a2);
        else
            return false;
    }

    bool IsGeoKwType(TKeyWordType kwtype) const {
        // more strict rather than Impl->IsGeoArticle()
        return GlobalDictsHolder->BuiltinKWTypes().IsGeo(kwtype);
    }

private:
    template <typename T>
    bool IsRelatedGeo(const CHomonym& h, const T& obj);

private:
    typedef TGeoHierarchy<false> TImpl;
    THolder<TImpl> Impl;
};



TTomitaGeoHierarchy::TTomitaGeoHierarchy() {
    TKeyWordType geo = GlobalDictsHolder->BuiltinKWTypes().Geo;

    if (geo != NULL) {
        const NGzt::TFieldDescriptor* geopart = geo->FindFieldByName("geopart");

        if (geopart == NULL)
            ythrow yexception() << "Cannot find field \"geopart\" in kwtype \"" << geo->name() << "\".";

        Impl.Reset(new TImpl(GlobalDictsHolder->Gazetteer()->ProtoPool(), *geopart));
    }
}

template <typename T>
bool TTomitaGeoHierarchy::IsRelatedGeo(const CHomonym& h, const T& obj) {
    const TGztArticle& art = h.GetGztArticle();
    if (!IsGeoKwType(art.GetType()))
        return false;

    if (IsRelatedGeo(obj, art))
        return true;

    const CHomonym::TExtraGztArticles& extra = h.GetExtraGztArticles();
    for (CHomonym::TExtraGztArticles::const_iterator e = extra.begin(); e != extra.end(); ++e) {
        YASSERT(e->GetType() == art.GetType());
        if (IsRelatedGeo(obj, *e))
            return true;
    }

    return false;
}

bool TTomitaGeoHierarchy::HaveRelatedGeo(bool negate, const CWord& w1, const CWord& w2, ui32& hom1, ui32& hom2) {

    ui32 res1 = 0;
    ui32 res2 = 0;

    for (CWord::THomMaskIterator it1(w1, hom1); it1.Ok(); ++it1)
        for (CWord::THomMaskIterator it2(w2, hom2); it2.Ok(); ++it2)
            if (IsRelatedGeo(*it1, *it2) != negate) {
                res1 |= 1 << it1.GetID();
                res2 |= 1 << it2.GetID();
            }

    if (res1 && res2) {
        hom1 = res1;
        hom2 = res2;
        return true;
    } else
        return false;
}


CTomitaItemsHolder::CTomitaItemsHolder(const CWordVector& Words, const CWorkGrammar& gram,
                                       const yvector<SWordHomonymNum>& clauseWords)
    : m_Words(Words)
    , m_GLRgrammar(gram)
    , m_FDOChainWords(clauseWords)
    , GeoHierarchy(new TTomitaGeoHierarchy)
{
}

CTomitaItemsHolder::~CTomitaItemsHolder() {
}


void CTomitaItemsHolder::RunParser(CInputSequenceGLR& InputSequenceGLR, yvector< COccurrence >& occurrences, bool bAllowAmbiguity)
{
    for (size_t i = 0; i < m_FDOChainWords.size(); i++)
        InputSequenceGLR.AddTerminal(&m_Words.GetWord(m_FDOChainWords[i]));

    InputSequenceGLR.Process(m_GLRgrammar, false, occurrences);

    if (bAllowAmbiguity) {
        sort(occurrences.begin(), occurrences.end());
        yvector<COccurrence>::iterator it = unique(occurrences.begin(), occurrences.end());
        occurrences.erase(it, occurrences.end());
        return;
    }

    //  then we should apply longest match algorithm ("SolveAmbiguity")
    if (NULL != CGramInfo::s_PrintRulesStream) {
        yvector<COccurrence> dropped;
        SolveAmbiguity(occurrences, dropped);
        PrintOccurrences(*CGramInfo::s_PrintRulesStream, occurrences, dropped);
    } else
        SolveAmbiguity(occurrences);
}

void CTomitaItemsHolder::FreeParser(CInputSequenceGLR& InputSequenceGLR)
{
    InputSequenceGLR.FreeParsers();
}

bool CTomitaItemsHolder::EqualNodes(const CInputItem* item1, const CInputItem* item2) const
{
    const CGroup* pGr1 = (const CGroup*)item1;
    const CGroup* pGr2 = (const CGroup*)item2;
    return pGr1->HasEqualGrammems(pGr2);
}

CInputItem* CTomitaItemsHolder::CreateNewItem(size_t SymbolNo, const CRuleAgreement& agreements,
                                              size_t iFirstItemNo, size_t iLastItemNo,
                                              size_t iSynMainItemNo, yvector<CInputItem*>& children)
{
    if (iSynMainItemNo > children.size())
        ythrow yexception() << "Bad iSynMainItemNo in \"CFDOChain::CreateNewItem\"";


    SReduceConstraints constr(agreements, children, SymbolNo, iFirstItemNo, iSynMainItemNo, iLastItemNo - iFirstItemNo + 1);
    if (!ReduceCheckConstraintSet(&constr))
        return NULL;

    CSynGroup* pNewGroup = new CSynGroup((CGroup*)children[iSynMainItemNo], constr.Grammems, (int)SymbolNo);
#ifndef NDEBUG
    pNewGroup->SetRuleName(m_GLRgrammar.m_UniqueGrammarItems[SymbolNo].m_ItemStrId);
#endif
    pNewGroup->SetPair(iFirstItemNo + m_iCurStartWord, iLastItemNo + m_iCurStartWord - 1);
    pNewGroup->AppendChildHomonymsFrom(children);
    pNewGroup->SetCheckedConstraints(constr.Get());

    return pNewGroup;
}

void CTomitaItemsHolder::GetFormsWithGrammems(const TGramBitSet& grammems, const TGrammarBunch& formGrammems, TGrammarBunch& res) const
{
    for (TGrammarBunch::const_iterator it = formGrammems.begin(); it != formGrammems.end(); ++it)
        if (grammems.HasAll(*it) || it->HasAll(grammems))
            res.insert(*it);
}

void CTomitaItemsHolder::AssignOutputGrammems(THomonymGrammems& resGrammems, const TGramBitSet& outGrammems) const
{
    if (outGrammems.any())
        resGrammems.Reset(NInfl::DefaultFeatures().ReplaceFeatures(resGrammems.All(), outGrammems));
}

const CWord* CTomitaItemsHolder::GetFirstAgrWord(const SReduceConstraints* pConstr, const SAgr& agr) const
{
    if (pConstr->Children.empty())
        return 0;

    //first child affected by agr
    const CGroup* child_group = pConstr->GetChildGroup(agr.m_AgreeItems[0]);
    //first word of this child
    int iFirstWord = child_group->FirstWord();
    return &m_Words.GetWord(m_FDOChainWords[iFirstWord]);
};

const CWord*  CTomitaItemsHolder::GetLastAgrWord(const SReduceConstraints* pConstr, const SAgr& agr) const
{
    if (pConstr->Children.empty())
        return 0;
    //last child affected by agr
    const CGroup* child_group = pConstr->GetChildGroup(agr.m_AgreeItems.back());
    //last word of this child
    int iLastWord = child_group->LastWord();
    return &m_Words.GetWord(m_FDOChainWords[iLastWord]);
};

bool CTomitaItemsHolder::ReduceCheckGrammemUnion(const SReduceConstraints* pConstr, yvector<CGramInterp>& ChildGramInterps) const
{
    //согласование по множеству разделенных граммем
    ymap< int, SGrammemUnion >::const_iterator it = pConstr->Get().m_RuleGrammemUnion.begin();
    for (; it != pConstr->Get().m_RuleGrammemUnion.end(); it++) {
        if (it->second.m_GramUnion.empty()) continue;
        CGramInterp& P = ChildGramInterps[it->first];
        CGramInterp ResInterp;
        for (size_t i = 0; i < it->second.m_GramUnion.size(); i++) {
            THomonymGrammems grammems = P.Grammems;    //copy
            if ((it->second.m_CheckFormGrammem[i] && CheckItemPositiveNegativeGrammems(it->second.m_GramUnion[i], it->second.m_NegGramUnion[i], grammems)) ||
                (!it->second.m_CheckFormGrammem[i] && CheckItemPositiveNegativeUnitedGrammems(it->second.m_GramUnion[i], it->second.m_NegGramUnion[i],
                                                                                              it->first, pConstr->Children)))
                ResInterp.Grammems |= grammems;
        }
        if (ResInterp.Grammems.Empty())
            return false;
        P = ResInterp;
    }

    return true;
}

bool CTomitaItemsHolder::CheckFioAgreement(const CGroup* pGr1, const CGroup* pGr2) const
{
    const CWord* W1 = pGr1->GetMainWord();
    const CWord* W2 = pGr2->GetMainWord();
    // если одно из фио голое (только фамилия), а другое нет, тогда выходим
    if (W1->IsMultiWord() != W2->IsMultiWord()) return false;

    // если оба фио голые(только фамилия) - тогда выходим (с true)
    if (!W1->IsMultiWord() && !W2->IsMultiWord()) return true;
    if (W1->GetRusHomonymsCount() == 0 ||  W2->GetRusHomonymsCount() == 0)  return true;
    const CHomonym& H1 = W1->GetRusHomonym(0);
    const CHomonym& H2 = W2->GetRusHomonym(0);
    const CFioWordSequence* WS1 = dynamic_cast<CFioWordSequence*>(H1.GetSourceWordSequence());
    const CFioWordSequence* WS2 = dynamic_cast<CFioWordSequence*>(H2.GetSourceWordSequence());
    if (!WS1 || !WS2) return  true;
    if (!WS2->m_NameMembers[Surname].IsValid() || !WS2->m_NameMembers[Surname].IsValid()) return true;

    if (WS1->m_NameMembers[InitialName].IsValid()
            &&  WS2->m_NameMembers[InitialName].IsValid()
        ) {
        // есть оба инициала, проверяем расположение инициалов относительно фамилии
        return (m_Words.GetWord(WS1->m_NameMembers[InitialName]).GetSourcePair().FromLeft(m_Words.GetWord(WS1->m_NameMembers[Surname]).GetSourcePair())
                    ==
                        m_Words.GetWord(WS2->m_NameMembers[InitialName]).GetSourcePair().FromLeft(m_Words.GetWord(WS2->m_NameMembers[Surname]).GetSourcePair())
                );

    } else {
        if  (WS1->m_NameMembers[FirstName].IsValid()
                &&  WS2->m_NameMembers[FirstName].IsValid()
            )
            // есть оба имени, проверяем расположение имен относительно фамилии
            return (m_Words.GetWord(WS1->m_NameMembers[FirstName]).GetSourcePair().FromLeft(m_Words.GetWord(WS1->m_NameMembers[Surname]).GetSourcePair())
                        ==
                        m_Words.GetWord(WS2->m_NameMembers[FirstName]).GetSourcePair().FromLeft(m_Words.GetWord(WS2->m_NameMembers[Surname]).GetSourcePair())
                    );
        else
            return true;
    }

}

bool CTomitaItemsHolder::HasOnlyOneHomId(ui32 iHomIDs) const
{
    if (iHomIDs == 0)
        return false;
    ui32 ququ = 0;
    int i;
    for (i = 0; i < (int)sizeof(iHomIDs); i++) {
        if ((1 << i) & iHomIDs)
            break;
    }
    ququ = (1<<i);
    return ququ == iHomIDs;
}

//у pConstr->m_ChildHomonyms правим номера омонимов для слов
//которые являются главными у детей строящейся группы (pConstr->p_children)
//ChildGramInterps - подправленные грамматические характеристики детей строящейся группы
//GetNewHomIDsByGrammems по получившимся в результате всяких согласований граммемкам
//отбираем омонимы. Так как граммемы могут являться объединением граммем различных омонимов,
//то в NewHomIDsByGrammems можем проверять только пересечение граммем очередного омонима с ChildGramInterps[i]
void CTomitaItemsHolder::AdjustChildHomonyms(const yvector<CGramInterp>&  ChildGramInterps, SReduceConstraints* pConstr) const
{
    for (size_t i = 0; i < pConstr->Children.size(); i++) {
        if (ChildGramInterps[i].IsEmpty())
            continue;
        const CGroup* pGr1 = pConstr->GetChildGroup(i);
        if (ChildGramInterps[i].HasEqualGrammems(pGr1))
            continue; //правим номера омонимов только у тех групп, у которых что-нибудь изменилось
        int iMainWord = pGr1->GetMainPrimitiveGroup()->FirstWord();
        ui32 iHomIDs = pGr1->GetMainHomIDs();
        if (HasOnlyOneHomId(iHomIDs))
            continue;
        ui32 iNewHomIDs = GetNewHomIDsByGrammems(ChildGramInterps[i], *pGr1->GetMainWord(), iHomIDs);
        int ii = iMainWord - (pConstr->FirstItemIndex + m_iCurStartWord);
        pConstr->ChildHomonyms[ii] = iNewHomIDs & iHomIDs;
    }
}

ui32 CTomitaItemsHolder::GetNewHomIDsByGrammems(const CGramInterp& gramInterp, const CWord& w, ui32 iOldHomIDs) const
{
    ui32 resHomIDs= 0;
    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it) {
        if (((1 << it.GetID()) & iOldHomIDs) == 0)
            continue;
        const CHomonym& h = *it;
        bool good = true;
        if (gramInterp.Grammems.HasForms()) {
            good = false;
            for (THomonymGrammems::TFormIter it = gramInterp.Grammems.IterForms(); it.Ok(); ++it)
                if (h.Grammems.HasFormWith(*it)) {
                    good = true;
                    break;
                }
        } else if (gramInterp.Grammems.All().any())
            good = h.Grammems.HasAny(gramInterp.Grammems.All());

        if (good)
            resHomIDs |= (1 << it.GetID());
    }
    return resHomIDs;
}

bool CTomitaItemsHolder::ReduceCheckGroupAgreement(SReduceConstraints* pConstr) const
{
    pConstr->Grammems.Reset();

    yvector<CGramInterp> ChildGramInterps;
    for (size_t i=0; i < pConstr->Children.size(); i++) {
        const CGroup* pGr1 = pConstr->GetChildGroup(i);
        ChildGramInterps.push_back(CGramInterp(pGr1));
    };

    for (size_t i = 0; i < pConstr->Get().m_RulesAgrs.size(); i++) {
        const SAgr& agr = pConstr->Get().m_RulesAgrs[i];

        if (agr.e_AgrProcedure == GeoAgr)   // checked separately
            continue;

        if (1 == agr.m_AgreeItems.size()) {
            if (CoordFuncCount != agr.e_AgrProcedure) {
                if (!ReduceCheckQuotedPositions(pConstr, agr))
                    return false;
            } else { // проверка граммем
                CGramInterp& p = ChildGramInterps[agr.m_AgreeItems[0]];
                if (!CheckItemPositiveNegativeGrammems(agr.m_Grammems, agr.m_NegativeGrammems, p.Grammems))
                    return false;
            }
            continue;
        }

        size_t i1 = agr.m_AgreeItems[0];
        size_t i2 = agr.m_AgreeItems[1];

        const CGroup* pGr1 = pConstr->GetChildGroup(i1);
        const CGroup* pGr2 = pConstr->GetChildGroup(i2);

        if (FioAgr == agr.e_AgrProcedure) {
            if (!CheckFioAgreement(pGr1, pGr2))
                return false;
            continue;
        }

        if ((SubjVerb == agr.e_AgrProcedure || GendreNumberCase == agr.e_AgrProcedure) && pGr2->GetMainWord()->HasMorphNoun()) {
            ::DoSwap(pGr1, pGr2);
            ::DoSwap(i1, i2);
        }

        CGramInterp& Interp1 = ChildGramInterps[i1];
        CGramInterp& Interp2 = ChildGramInterps[i2];

        //если согласование указано, а у слова нет никаких граммем, то пропускаем его и используем его при нормализации
        if ((pGr1->GetMainWord()->HasUnknownPOS() && Interp1.IsEmpty()) ||
            (pGr2->GetMainWord()->HasUnknownPOS() && Interp2.IsEmpty()))
            continue;

        THomonymGrammems ancode1 = Interp1.Grammems;        // source forms
        THomonymGrammems ancode2 = Interp2.Grammems;

        TGrammarBunch interp_ancodes1, interp_ancodes2;     // agreed forms

        if (agr.m_bNegativeAgreement) {
            // для "отрицательного" согласования проверяем анкоды остальных омонимов, нужно,
            // чтобы согласование в принципе не проходило
            for (CWord::SHomIt it = pGr1->GetMainWord()->IterHomonyms(); it.Ok(); ++it)
                ancode1 |= it->Grammems;
            for (CWord::SHomIt it = pGr2->GetMainWord()->IterHomonyms(); it.Ok(); ++it)
                ancode2 |= it->Grammems;
        }

        if (!ancode1.HasForms() && !ancode2.HasForms()) {
            TGramBitSet gUnion = NGleiche::GleicheGrammems(Interp1.Grammems.All(), Interp2.Grammems.All(), agr.e_AgrProcedure);
            bool agreed = gUnion.any();
            if (agreed == agr.m_bNegativeAgreement)
               return false;
            if (agreed) {
                AssignOutputGrammems(Interp1.Grammems, gUnion);
                AssignOutputGrammems(Interp2.Grammems, gUnion);
            }
        } else {
            TGramBitSet gUnion;   //agreed grammems
            if (!ancode1.HasForms())
                gUnion = NGleiche::GleicheAndRestoreForms(ancode2.Forms(), Interp1.Grammems.All(), interp_ancodes2, interp_ancodes1, agr.e_AgrProcedure);
            else if (!ancode2.HasForms())
                gUnion = NGleiche::GleicheAndRestoreForms(ancode1.Forms(), Interp2.Grammems.All(), interp_ancodes1, interp_ancodes2, agr.e_AgrProcedure);
            else
                gUnion = NGleiche::GleicheForms(ancode1.Forms(), ancode2.Forms(), interp_ancodes1, interp_ancodes2, agr.e_AgrProcedure);

            bool agreed = gUnion.any();
            if (agreed == agr.m_bNegativeAgreement)
               return false;

            if (agreed) {
                //if groups are agreed - remove their non-agreed forms
                Interp1.Grammems.Reset(interp_ancodes1);
                Interp2.Grammems.Reset(interp_ancodes2);
            }
        }

    } // end of  for

    if (!ReduceCheckGrammemUnion(pConstr, ChildGramInterps))
        return false;

    pConstr->Grammems = ChildGramInterps[pConstr->MainItemIndex].Grammems;
    AdjustChildHomonyms(ChildGramInterps, pConstr);
    return true;
}

bool CTomitaItemsHolder::CheckItemPositiveNegativeGrammems(const TGramBitSet& PositGrammems, const TGramBitSet& NegGrammems,
                                                           THomonymGrammems& resGrammems) const
{
    if (NegGrammems.any() && !CheckItemGrammems(NegGrammems, false, resGrammems))
        return false;

    if (PositGrammems.any() && !CheckItemGrammems(PositGrammems, true, resGrammems))
        return false;

    return true;
}

bool CTomitaItemsHolder::CheckItemPositiveNegativeUnitedGrammems(const TGramBitSet& PositGrammems, const TGramBitSet& NegGrammems,
                                                                 int iChildNum, const yvector<CInputItem*>& children) const
{
    const CGroup* pGr = (CGroup*)children[iChildNum];
    const THomonymGrammems& srcGram = pGr->GetGrammems();
    if (NegGrammems.any() && srcGram.HasAll(NegGrammems))
        return false;
    else
        return srcGram.HasAll(PositGrammems);
}

bool CTomitaItemsHolder::CheckItemGrammems(const TGramBitSet& grammemsToCheck, bool allow, THomonymGrammems& resGrammems) const
{
    YASSERT(grammemsToCheck.any());

    // если у группы есть омоформы - сравниваться по ним
    if (resGrammems.HasForms()) {
        for (THomonymGrammems::TFormIter it = resGrammems.IterForms(); it.Ok(); ++it)
            if (it->HasAll(grammemsToCheck) != allow) {
                // <spike to imitate previous behaviour: gram=~XXX will remove homonym as a whole even if there are homforms without XXX>
                if (!allow)
                    return false;
                // </spike>

                TGrammarBunch tmp;
                for (it = resGrammems.IterForms(); it.Ok(); ++it)
                    if (it->HasAll(grammemsToCheck) == allow)
                        tmp.insert(*it);
                if (!tmp.empty()) {
                    resGrammems.Reset(tmp);
                    return true;
                } else
                    return false;
            }
        return true;
    } else if (resGrammems.All().any() && resGrammems.HasAll(grammemsToCheck) == allow)
        // если у группы пустые омоформы - сравниваться по граммемам
    {
        // вызываем  AssignOutputGrammems, чтобы удалить категории из граммем группы.
        // если, например, GrammemsToCheck содержит "рд", то  AssignOutputGrammems удалит все падежи, кроме родительного
        if (allow)
            AssignOutputGrammems(resGrammems, grammemsToCheck);
        return !resGrammems.Empty();
    } else
        return !allow;
}

CInputItem* CTomitaItemsHolder::CreateNewItem(size_t SymbolNo, size_t iFirstItemNo)
{
    const CWord* pW = &m_Words.GetWord(m_FDOChainWords[iFirstItemNo + m_iCurStartWord]);

    CPrimitiveGroup* pNewGroup = new CPrimitiveGroup(pW, iFirstItemNo + m_iCurStartWord);
#ifndef NDEBUG
    pNewGroup->SetRuleName(m_GLRgrammar.m_UniqueGrammarItems[SymbolNo].m_ItemStrId);
#endif
    pNewGroup->SetActiveSymbol(SymbolNo);
    return pNewGroup;

}

void CTomitaItemsHolder::BuildFinalCommonGroup(const COccurrence& occur, CWordSequence* pWordSeq)
{
    CGroup* pGroup = (CGroup*) occur.m_pInputItem;
    if (!pGroup)
        ythrow yexception() << "No root in \"CFDOChain::BuildFPC\"";

    if (pGroup->IsPrimitive())
        ythrow yexception() << "The root is primitive group in \"CFDOChain::BuildFPC\"";

    CPrimitiveGroup* pPrimGr = ((CSynGroup*)pGroup)->GetMainPrimitiveGroup();

    const CWord* pW = pPrimGr->GetMainWord();
    for (CWord::SHomIt it = pW->IterHomonyms(); it.Ok(); ++it)
        if (it->m_AutomatSymbolInterpetationUnion.end() != find(it->m_AutomatSymbolInterpetationUnion.begin(),
                                                                 it->m_AutomatSymbolInterpetationUnion.end(), pPrimGr->GetActiveSymbol())) {
            pWordSeq->SetMainWord(SWordHomonymNum(m_FDOChainWords[pPrimGr->FirstWord()].m_WordNum, it.GetID(), m_FDOChainWords[pPrimGr->FirstWord()].m_bOriginalWord));
            break;
        }

    pWordSeq->SetGrammems(pGroup->GetGrammems());
}

bool CTomitaItemsHolder::ReduceCheckOutputGrammems(SReduceConstraints* pConstr) const
{
    if (pConstr->Get().m_ExtraRuleInfo.m_OutGrammems.any())
        AssignOutputGrammems(pConstr->Grammems, pConstr->Get().m_ExtraRuleInfo.m_OutGrammems);
    return true;
}

bool CTomitaItemsHolder::ReduceCheckOutputGroupSize(const SReduceConstraints* pConstr) const
{
    return pConstr->ChainSize <= pConstr->Get().m_ExtraRuleInfo.m_OutCount;
}

bool    CTomitaItemsHolder::ReduceCheckKWSets(SReduceConstraints* pConstr) const
{
    ymap< int, SKWSet >::const_iterator it = pConstr->Get().m_KWSets.begin();
    for (; it != pConstr->Get().m_KWSets.end(); it++) {
        const CGroup* p_Item = pConstr->GetChildGroup(it->first);
        const CFactSynGroup* pFactGroup = dynamic_cast<const CFactSynGroup*>(p_Item);

        const CWord* pWrdRus;
        int iCheckWord = -1;
        ui32 oldHomIDs = 0;
        if (it->second.m_bApplyToTheFirstWord && pFactGroup) {
                pWrdRus = pFactGroup->GetFirstWord();
                iCheckWord = pFactGroup->FirstWord();
                oldHomIDs = pFactGroup->GetChildHomonyms()[0];
        } else {
                pWrdRus = p_Item->GetMainWord();
                iCheckWord = p_Item->GetMainPrimitiveGroup()->FirstWord();
                oldHomIDs = p_Item->GetMainHomIDs();
        }

        yset<SArtPointer>::const_iterator it_art = it->second.m_KWset.begin();
        ui32 usedHomonyms = 0;
        for (; it_art != it->second.m_KWset.end(); ++it_art) {
            if (it->second.m_bNegative) {
                if (pWrdRus->HasArticle_i(*it_art) != -1)
                    return false;
            } else
                pWrdRus->GetHomIDsByKWType(*it_art, usedHomonyms);
        }

        // apply previously filtered homonyms mask if any
        if (pConstr->ChildHomonyms[it->first] != 0)
            usedHomonyms &= pConstr->ChildHomonyms[it->first];

        if (!it->second.m_bNegative && usedHomonyms == 0)
            return false;

        if (!HasOnlyOneHomId(oldHomIDs)) {
            int ii = iCheckWord - (pConstr->FirstItemIndex + m_iCurStartWord);
            YASSERT(ii >= 0 && (size_t)ii < pConstr->ChildHomonyms.size());
            ui32 resHomonyms = oldHomIDs & usedHomonyms;
            if (resHomonyms == 0)
                return true; // !!!

            pConstr->ChildHomonyms[ii] = resHomonyms;
            if (pConstr->Grammems.HasForms() && resHomonyms != oldHomIDs) {
                // remove gram forms presented in dropped homonyms and not presented in remained homonyms
                // first, collect all of them
                ui32 droppedHomonyms = oldHomIDs & ~usedHomonyms;
                TGrammarBunch droppedForms, remainedForms;
                for (CWord::SHomIt homIt = pWrdRus->IterHomonyms(); homIt.Ok(); ++homIt)
                    if ((1 << homIt.GetID()) & resHomonyms)
                        for (THomonymGrammems::TFormIter formIt = homIt->Grammems.IterForms(); formIt.Ok(); ++formIt)
                            remainedForms.insert(*formIt);
                    else if ((1 << homIt.GetID()) & droppedHomonyms)
                        for (THomonymGrammems::TFormIter formIt = homIt->Grammems.IterForms(); formIt.Ok(); ++formIt)
                            droppedForms.insert(*formIt);

                TGrammarBunch resForms;
                for (THomonymGrammems::TFormIter formIt = pConstr->Grammems.IterForms(); formIt.Ok(); ++formIt)
                    if (droppedForms.find(*formIt) == droppedForms.end() || remainedForms.find(*formIt) != remainedForms.end())
                        resForms.insert(*formIt);

                pConstr->Grammems.Reset(resForms);
            }
        }
    }
    return true;
}

bool    CTomitaItemsHolder::ReduceCheckFirstWord(const SReduceConstraints* pConstr) const
{
    ymap<int, bool>::const_iterator it = pConstr->Get().m_FirstWordConstraints.begin();
    for (; it != pConstr->Get().m_FirstWordConstraints.end(); it++) {
        int iChildNo = it->first;
        bool isFirstWord = it->second;
        const CGroup* p_Item = pConstr->GetChildGroup(iChildNo);
        const CFactSynGroup* pFactGroup = dynamic_cast<const CFactSynGroup*>(p_Item);
        const CWord* pFW = NULL;
        if (pFactGroup)
            pFW = pFactGroup->GetFirstWord();
        else
            pFW = p_Item->GetMainWord();

        if (pFW->GetSourcePair().FirstWord() == 0) {
            if (!isFirstWord)
                return false;
        } else {
            if (isFirstWord)
                return false;
        }
    }
    return true;
}

bool    CTomitaItemsHolder::ReduceCheckWFSets(const SReduceConstraints* pConstr) const
{
    ymap< int, CWordFormRegExp >::const_iterator it = pConstr->Get().m_WFRegExps.begin();
    for (; it != pConstr->Get().m_WFRegExps.end(); it++) {

        const CGroup* p_Item = pConstr->GetChildGroup(it->first);
        const CFactSynGroup* pFactGroup = dynamic_cast<const CFactSynGroup*>(p_Item);

        const CWord* pWrdRus = p_Item->GetMainWord();
        const CWordFormRegExp& RE = it->second;
        if (pFactGroup)
            switch (RE.Place) {
                case CWordFormRegExp::ApplyToFirstWord: pWrdRus = pFactGroup->GetFirstWord(); break;
                case CWordFormRegExp::ApplyToLastWord: pWrdRus = pFactGroup->GetLastWord(); break;
                default: break;
            }

        YASSERT(!RE.IsEmpty());
        if (!RE.Accepted(pWrdRus->GetText()))
            return false;
    }
    return true;
}

bool CTomitaItemsHolder::ReduceCheckTrim(SReduceConstraints* pConstr) const
{
    if (pConstr->TrimTree != NULL && pConstr->Get().m_ExtraRuleInfo.m_bTrim)
        pConstr->TrimTree->insert(pConstr->SymbolNo);
    return true;
}

bool    CTomitaItemsHolder::ReduceCheckLeftDominantRecessive(SReduceConstraints* pConstr) const
{
    if (pConstr->LeftPriorityOrder == NULL || !pConstr->Get().m_ExtraRuleInfo.m_DomReces.HasValue())
        return true;

    YASSERT(!pConstr->Children.empty());

    const CWordsPair* pFWP = CheckedCast<const CWordsPair*>(pConstr->GetChildGroup(0));
    const CWordsPair* pSWP = CheckedCast<const CWordsPair*>(pConstr->GetChildGroup(pConstr->Children.size() - 1));
    CWordsPair WP(pFWP->FirstWord(), pSWP->LastWord());

    if (pConstr->Get().m_ExtraRuleInfo.m_DomReces.IsDominant())
        pConstr->LeftPriorityOrder->insert(WP);

    if (pConstr->Get().m_ExtraRuleInfo.m_DomReces.IsRecessive()) {
        yset<CWordsPair>::const_iterator it = pConstr->LeftPriorityOrder->begin();
        for (; it != pConstr->LeftPriorityOrder->end(); ++it)
            if (it->FirstWord() <= WP.FirstWord() && it->LastWord() >=WP.FirstWord())
                return false;
    }
    return true;
}

bool    CTomitaItemsHolder::ReduceCheckNotHRegFact(const SReduceConstraints* pConstr) const
{
    if (!pConstr->Get().m_ExtraRuleInfo.m_bNotHRegFact)
            return true;

    bool bNoFact = true;
    for (size_t i = 0; i < pConstr->Children.size(); ++i) {
        const CFactSynGroup* pFactGroup = dynamic_cast<const CFactSynGroup*>(pConstr->GetChildGroup(i));
        if (!pFactGroup)
            continue;

        for (size_t j = 0; j < pFactGroup->m_Facts.size(); ++j) {
            bNoFact = false;

            const CFactFields& rFact = pFactGroup->m_Facts.at(j);
            const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(rFact.GetFactName());

            for (size_t k = 0; k < pFactType->m_Fields.size(); ++k) {
                const fact_field_descr_t& field_descr = pFactType->m_Fields[k];

                const CWordsPair* pWP = rFact.GetValue(field_descr.m_strFieldName);
                if (!pWP) continue;
                if (!IsWSUpperCase(*pWP))
                    return true;
            }
        }
    }

    return bNoFact;
}

bool    CTomitaItemsHolder::ReduceCheckConstraintSet(SReduceConstraints* pConstr) const
{
    for (size_t i = 0; i < pConstr->Agreements.size(); ++i) {
        pConstr->CurrentConstraintIndex = i;
        if (ReduceCheckGroupAgreement(pConstr) &&
            ReduceCheckOutputGrammems(pConstr) &&
            ReduceCheckOutputGroupSize(pConstr) &&
            ReduceCheckKWSets(pConstr) &&
            ReduceCheckWFSets(pConstr) &&
            ReduceCheckTrim(pConstr) &&
            ReduceCheckLeftDominantRecessive(pConstr) &&
            ReduceCheckFirstWord(pConstr) &&
            ReduceCheckNotHRegFact(pConstr) &&
            ReduceCheckGeoAgreement(pConstr))

            return true;
    }
    return false;
}

bool CTomitaItemsHolder::IsWSUpperCase(const CWordsPair& seq) const
{
    if (seq.IsValid())
        for (int i = seq.FirstWord(); i <= seq.LastWord(); ++i)
            if (!m_Words[i].HasAtLeastTwoUpper()  &&  !m_Words[i].IsUpperLetterPointInitial()
                &&  m_Words[i].GetText().size() > 1)
                return false;
    return true;
}

SReduceConstraints::SReduceConstraints(const CRuleAgreement& agreements,  const yvector<CInputItem*>& children,
                                       size_t symbolNo, size_t firstItem, size_t mainItem, size_t chainSize)
    : Agreements(agreements)
    , Children(children)

    , TrimTree(NULL)
    , LeftPriorityOrder(NULL)

    , Grammems()
    , ChildHomonyms(chainSize, 0)

    , SymbolNo(symbolNo)

    , FirstItemIndex(firstItem)
    , MainItemIndex(mainItem)
    , ChainSize(chainSize)

    , CurrentConstraintIndex(0)


{
}

Stroka CTomitaItemsHolder::GetDumpOfWords(const CInputItem* item1, ECharset encoding)  const
{
    const CPrimitiveGroup* pPrimGroup = dynamic_cast<const CPrimitiveGroup*>(item1);
    if (pPrimGroup) {
        TStringStream grammems_str;
        CHomonym::PrintGrammems(pPrimGroup->GetGrammems().All(), grammems_str, encoding);
        return NStr::Encode(pPrimGroup->GetMainWord()->GetText(), encoding) + " " + grammems_str;
    } else {
        const CFactSynGroup* pGroup = dynamic_cast<const CFactSynGroup*>(item1);
        if (pGroup) {
            TStringStream grammems_str;
            CHomonym::PrintGrammems(pGroup->GetGrammems().All(), grammems_str, encoding);
            return Substitute("$0\\n$1\\n$2", NStr::Encode(pGroup->ToString(), encoding), grammems_str, pGroup->GetWeightStr());
        } else
            return  " ";
    }
}

bool    CTomitaItemsHolder::ReduceCheckQuotedPositions(const SReduceConstraints* pConstr, const SAgr& agr) const
{
    if (pConstr->Children.empty())
        return true;

    bool bQuoted = false;
    // проверка кавычек
    switch (agr.e_AgrProcedure) {
        case CheckQuoted:  // достаем первое и последнее слово группы
            bQuoted = GetFirstAgrWord(pConstr, agr)->HasOpenQuote() &&
                      GetLastAgrWord(pConstr, agr)->HasCloseQuote();
            break;
        case CheckLQuoted:
            bQuoted = GetFirstAgrWord(pConstr, agr)->HasOpenQuote();
            break;
        case CheckRQuoted:
            bQuoted = GetLastAgrWord(pConstr, agr)->HasCloseQuote();
            break;
        default:
            return true;
    }
    return bQuoted != agr.m_bNegativeAgreement;
}


template <typename T>
class TCopyOnWritePtr {
public:
    inline TCopyOnWritePtr(T* data)
        : Data(data)
        , Ptr(data)
    {
    }

    inline const T* Get() const {
        return Ptr;
    }

    inline const T* operator->() const {
        return Get();
    }

    inline const T& operator*() const {
        const T* ret = Get();
        YASSERT(ret);
        return *ret;
    }

    inline const T& Const() const {
        YASSERT(Ptr);
        return *Ptr;
    }


    T& Mutable() {
        YASSERT(Data != NULL);
        if (Ptr == Data) {
            if (LocalCopy.Get())
                *LocalCopy = *Data;
            else
                LocalCopy.Reset(new T(*Data));
            Ptr = LocalCopy.Get();
        }
        return *Ptr;
    }

    inline void Commit() {
        if (Data != NULL && Ptr != Data)
            ::DoSwap(*Data, *LocalCopy);
        Ptr = Data;
    }

    inline void Discard() {
        Ptr = Data;
    }

private:
    T* Data;
    T* Ptr;
    THolder<T> LocalCopy;
};

bool CTomitaItemsHolder::ReduceCheckGeoAgreement(SReduceConstraints* pConstr) const {

    TCopyOnWritePtr< yvector<ui32> > childHomonyms(&pConstr->ChildHomonyms);

    for (size_t i = 0; i < pConstr->Get().m_RulesAgrs.size(); i++) {
        const SAgr& agr = pConstr->Get().m_RulesAgrs[i];
        if (agr.e_AgrProcedure == GeoAgr)
            for (size_t i2 = 1; i2 < agr.m_AgreeItems.size(); ++i2) {
                size_t i1 = i2 - 1;
                const CGroup* pGr1 = pConstr->GetChildGroup(i1);
                const CGroup* pGr2 = pConstr->GetChildGroup(i2);

                ui32 mask1 = childHomonyms.Const()[i1];
                ui32 mask2 = childHomonyms.Const()[i2];

                if (!GeoHierarchy->HaveRelatedGeo(agr.m_bNegativeAgreement, *pGr1->GetMainWord(), *pGr2->GetMainWord(), mask1, mask2))
                    return false;

                childHomonyms.Mutable()[i1] = mask1;
                childHomonyms.Mutable()[i2] = mask2;
            }
    }
    childHomonyms.Commit();
    return true;
}

void CTomitaItemsHolder::PrintOccurrences(TOutputStream &stream,
                                          const yvector<COccurrence>& occurrences,
                                          const yvector<COccurrence>& dropped_occurrences) const
{
    if (occurrences.empty() && dropped_occurrences.empty())
        return;
    stream << "===================== " << m_GramFileName << " =====================\n" << Endl;
    for (size_t i = 0; i < occurrences.size(); ++i)
        PrintOccurrence(stream, occurrences[i], false);

    for (size_t i = 0; i < dropped_occurrences.size(); ++i)
        PrintOccurrence(stream, dropped_occurrences[i], true);

    stream << '\n' << Endl;
}

void CTomitaItemsHolder::PrintOccurrence(TOutputStream &stream, const COccurrence& occurrence, bool dropped) const
{
    CWeightSynGroup wgroup;
    wgroup.m_KwtypesCount = 0;

    //calculations copied from solveperiodambiguity.cpp
    size_t coverage = 0;
    CInputItem::TWeight weight = CInputItem::TWeight::Zero();
    if (occurrence.m_pInputItem != NULL) {
        wgroup.SetLastWord(occurrence.second - 1);
        wgroup.AddWeightOfChild(occurrence.m_pInputItem);
        weight = wgroup.GetWeight();
        coverage = occurrence.m_pInputItem->GetCoverage();
    }
    if (coverage == 0)
        coverage = occurrence.second - occurrence.first;

    if (dropped)
        stream << "DROPPED OCCURRENCE, ";
    stream << "coverage: " << coverage << ", weight: " << weight.AsDouble() << '\n';
    PrintRulesTree(stream, dynamic_cast<const CGroup*>(occurrence.m_pInputItem));
    stream << Endl;
}

void CTomitaItemsHolder::PrintRulesTree(TOutputStream &stream, const CGroup* group, const Stroka& indent1, const Stroka& indent2) const
{
    static const Stroka empty_indent = "    ";
    static const Stroka branch_indent = "|__ ";
    static const Stroka branch_child_indent = "|   ";

    stream << indent1;
    PrintFlatRule(stream, group);

    const CWeightSynGroup* fgroup = dynamic_cast<const CWeightSynGroup*>(group);
    if (fgroup != NULL)
        PrintRuleTreeBranches(stream, fgroup->m_Children, indent2);
}

static inline Stroka GetDebugItemName(const CGrammarItem& item) {
    return (item.m_Lemma.empty()) ? item.m_ItemStrId : NStr::DebugEncode(item.m_Lemma);
}

void CTomitaItemsHolder::PrintFlatRule(TOutputStream &stream, const CGroup* group) const
{
    stream << GetDebugItemName(m_GLRgrammar.m_UniqueGrammarItems[group->GetActiveSymbol()]) << "  ->";
    const CWeightSynGroup* fgroup = dynamic_cast<const CWeightSynGroup*>(group);
    if (fgroup != NULL)
        for (size_t i = 0; i < fgroup->ChildrenSize(); ++i)
            PrintFlatRuleChild(stream, fgroup->GetChild(i), fgroup->GetMainGroup(), true);
    else
        PrintFlatRuleChild(stream, group);

    stream << " :: " << group->GetWeight().AsDouble() << '\n';
}

void CTomitaItemsHolder::PrintFlatRule(TOutputStream &stream, size_t SymbolNo, size_t iSynMainItemNo,
                                                 const yvector<CInputItem*>& children) const
{
    stream << GetDebugItemName(m_GLRgrammar.m_UniqueGrammarItems[SymbolNo]) << "  ->";
    const CGroup* maingroup = (const CGroup*)children[iSynMainItemNo];
    for (size_t i = 0; i < children.size(); ++i)
        PrintFlatRuleChild(stream, (const CGroup*)children[i], maingroup, true);

    stream << " ::\n";
}

void CTomitaItemsHolder::PrintFlatRuleChild(TOutputStream &stream, const CGroup* child, const CGroup* maingroup /* = NULL */,
                                                      bool verbose_primitives /* = false */) const
{
    bool is_primitive = dynamic_cast<const CPrimitiveGroup*>(child) != NULL;
    stream << "  ";
    const CGrammarItem& GramItem = m_GLRgrammar.m_UniqueGrammarItems[child->GetActiveSymbol()];
    if (!is_primitive || verbose_primitives) {
        stream << GetDebugItemName(GramItem) << "[";
        if (child == maingroup)
            stream << '*';
    }

    //stable part - word itself
    stream << NStr::Encode(m_Words.GetWord(m_FDOChainWords[child->FirstWord()]).GetText(), CGramInfo::s_DebugEncoding);
    for (int w = child->FirstWord() + 1; w <= child->LastWord(); ++w)
        stream << ' ' << NStr::Encode(m_Words.GetWord(m_FDOChainWords[w]).GetText(), CGramInfo::s_DebugEncoding);

    if (!is_primitive || verbose_primitives) {
        stream << ']';

        SArtPointer art = GramItem.GetPostfixKWType(DICT_KWTYPE);
        Stroka KWYes = art.HasKWType() ? art.GetKWType()->name() : NStr::Encode(art.GetStrType(), CGramInfo::s_DebugEncoding);

        if (GramItem.IsNonePostfixKWType())
            KWYes = "none";

        art = GramItem.GetPostfixKWType(DICT_NOT_KWTYPE);
        Stroka KWNo = art.HasKWType() ? art.GetKWType()->name() : NStr::Encode(art.GetStrType(), CGramInfo::s_DebugEncoding);

        if (!KWYes.empty() || !KWNo.empty()) {
            stream << "<kwt=" << KWYes;

            if (!KWNo.empty()) {
                if (!KWYes.empty())
                    stream << ",";
                stream << "~" << KWNo;
            }
            stream << ">";
        }
    }
}

void CTomitaItemsHolder::PrintRuleTreeBranches(TOutputStream &stream, const yvector<CInputItem*>& children, const Stroka& indent) const
{
    if (children.empty())
        return;

    static const Stroka empty_indent = "    ";
    static const Stroka branch_indent = "|__ ";
    static const Stroka branch_child_indent = "|   ";

    Stroka new_indent1 = indent, new_indent2 = indent;
    if (children.size() >= 2) {
        new_indent1 += branch_indent;
        new_indent2 += branch_child_indent;
    } else {
        new_indent1 += empty_indent;
        new_indent2 += empty_indent;
    }

    for (int i = 0; i < (int)children.size()-1; ++i) {
        stream << new_indent2 << '\n';
        PrintRulesTree(stream, (const CGroup*)(children[i]), new_indent1, new_indent2);
    }
    if (children.size() > 1)
        stream << new_indent2 << '\n';
    PrintRulesTree(stream, (const CGroup*)(children[children.size() - 1]), new_indent1, indent + empty_indent);
}

void CTomitaItemsHolder::InitKwtypesCount(CWeightSynGroup* pNewGroup, const yvector<CInputItem*>& children,
                                          const SRuleExternalInformationAndAgreements& ruleInfo) const
{
    pNewGroup->m_KwtypesCount = 0;
    for (size_t  i = 0; i < children.size(); i++) {
        const CWeightSynGroup* pChild = dynamic_cast< CWeightSynGroup* >(children[i]);
        if (pChild)
            pNewGroup->IncrementKwtypesCount(pChild->m_KwtypesCount);
    }

    if (pNewGroup->Size() == 1) {
        assert (children.size() == 1);
        const CGroup* pChild = (const CGroup*)children[0];
        const CGrammarItem& Item = m_GLRgrammar.m_UniqueGrammarItems[ pChild->GetActiveSymbol()];
        if (!Item.IsNonePostfixKWType() && Item.GetPostfixKWType(DICT_KWTYPE).GetKWType() != NULL)
            pNewGroup->IncrementKwtypesCount(1);
    }

    for (ymap< int, SKWSet >::const_iterator it = ruleInfo.m_KWSets.begin(); it != ruleInfo.m_KWSets.end(); it++)
        if (!it->second.m_bNegative && !it->second.m_bApplyToTheFirstWord)
            pNewGroup->IncrementKwtypesCount(1);
};


void CTomitaItemsHolder::InitUserWeight(CWeightSynGroup* pNewGroup, const yvector<CInputItem*>& children,
                                        const SRuleExternalInformationAndAgreements& ruleInfo) const
{
    pNewGroup->AddUserWeight(CGroup::TWeight(ruleInfo.m_ExtraRuleInfo.m_OutWeight));
    for (size_t i = 0; i < children.size(); i++) {
        pNewGroup->AddUserWeight(children[i]);

        // also add weight from assigned gzt-articles if gzt-weight is specified
        double gztWeight = 1;
        if (GetGztWeight(ruleInfo, CheckedCast<const CGroup*>(children[i]), i, gztWeight))
            pNewGroup->AddUserWeight(CGroup::TWeight(gztWeight));
    }
}


bool CTomitaItemsHolder::GetGztWeight(const SRuleExternalInformationAndAgreements& ruleInfo,
                                      const CGroup* group, size_t childIndex, double& gztWeight) const {

    ymap<int, TGztArticleField>::const_iterator it = ruleInfo.m_GztWeight.find(childIndex);

    if (it == ruleInfo.m_GztWeight.end())
        return false;

    const TGztArticleField& artfield = it->second;
    YASSERT(artfield.Article.IsValid());

    for (CWord::THomMaskIterator hom(*group->GetMainWord(), group->GetMainHomIDs()); hom.Ok(); ++hom) {
        TGztArticle gztart;
        if (GlobalDictsHolder->GetMatchingArticle(*hom, artfield.Article, gztart) &&
            gztart.GetNumericField(artfield.Field, gztWeight))
                return true;
    }

    return false;
}


