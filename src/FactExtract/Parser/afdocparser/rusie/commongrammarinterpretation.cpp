#include "commongrammarinterpretation.h"
#include "referencesearcher.h"

#include <FactExtract/Parser/afdocparser/normalize/sequence.h>

#include "sstream"
#include <iostream>

CCommonGrammarInterpretation::CCommonGrammarInterpretation(const CWordVector& words, const CWorkGrammar& gram,
                                                           yvector<CWordSequence*>& foundFacts,
                                                           const yvector<SWordHomonymNum>& clauseWords,
                                                           CReferenceSearcher* refSearcher, size_t factCountConstraint)
    : CTomitaItemsHolder(words, gram, clauseWords)
    , m_InterpNorm(words, &m_GLRgrammar)
    , m_FactCountConstraint(factCountConstraint)
    , m_bLog(NULL != CGramInfo::s_PrintGLRLogStream)
    , m_pLogStream(CGramInfo::s_PrintGLRLogStream)
    , m_CurrentFactGroup(NULL)
    , m_FoundFacts(foundFacts)
    , m_pReferenceSearcher(refSearcher)
{
}

CCommonGrammarInterpretation::~CCommonGrammarInterpretation()
{}

bool CCommonGrammarInterpretation::HasAllNecessaryFields(CFactFields& fact, const CWordsPair& artificialPair) const
{
    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(fact.GetFactName());

    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& field_descr = pFactType->m_Fields[i];
        if (!field_descr.m_bNecessary)
            continue;
        if (!fact.HasValue(field_descr.m_strFieldName)) {
            bool bAdded = false;
            switch (field_descr.m_Field_type) {
                case BoolField:
                    {
                        //у поля типа bool всегда есть значение по умолчанию: третьего не дано
                        CBoolWS ws(field_descr.m_bBoolValue);
                        ws.SetPair(artificialPair);
                        fact.AddValue(field_descr.m_strFieldName, ws);
                        bAdded = true;
                        break;
                    }
                case TextField:
                    {
                        if (field_descr.m_StringValue.length() > 0) {
                            CTextWS ws;
                            ws.SetArtificialPair(artificialPair);
                            AddTextFactFieldValue(ws, field_descr);
                            fact.AddValue(field_descr.m_strFieldName, ws, false);
                            bAdded = true;
                            break;
                        }
                    }
                default:
                    break;
            }
            if (!bAdded)
                return false;
        }
    }
    return true;
}

//смотрит, заполнены ли все поля обязательные у факта
//если нет, то пытается заполнить значением по умолчанию
//если и это не удается - удаляет такой факт
bool CCommonGrammarInterpretation::CheckNecessaryFields(yvector<CFactFields>& newFacts, const CWordsPair& artificialPair) const
{
    bool res = true;
    for (int i = newFacts.size() - 1; i >= 0; i--) {
        if (!HasAllNecessaryFields(newFacts[i], artificialPair)) {
            newFacts.erase(newFacts.begin() + i);
            res = false;
        } else
            newFacts[i].UpdateArtificialPairs();
    }

    return res;
};

//если сама валентность заполнена CFactsWS, то
//1. проходим по фактам из CFactsWS и выбираем у них поля с именем fact_field.m_strFieldName
//(или, если не  пусто, из m_strSourceFieldName (оператор from) ),
//   при этом следим за уникальность значения, так как одно и то же заначение может быть ф разных факта
//  (например, фио из факта FdoFact и StatusFact)
//2. если количество уникальных значений из CFactsWS == newFacts.size и их > 1(конструкция типа 3 фио - 3 компании),
//      то сортируем значения из CFactsWS по адресам в предложении и пихаем их в newFacts, закладываясь на то, что там
//      порядок фактов тоже правильный
//  иначе просто пихаем вытащенные значения в newFacts, и множим факты, если нужно
//возвращает false, если нет ни одного факта с таким полем

bool CCommonGrammarInterpretation::FillFactFieldFromFactWS(const fact_field_reference_t& fact_field, const CFactsWS* factWS,
                                                           yvector<CFactFields>& newFacts) const
{
    yvector<int> factsIndexes;

    for (size_t i = 0; i < newFacts.size(); i++) {
        if (newFacts[i].GetFactName() == fact_field.m_strFactTypeName)
            factsIndexes.push_back(i);
    }

    Stroka strSourceFieldName =  fact_field.m_strFieldName;
    if (!fact_field.m_strSourceFieldName.empty())
        strSourceFieldName = fact_field.m_strSourceFieldName;
    yvector<const CWordsPair*> wsValues;
    for (int i = 0; i < factWS->GetFactsCount(); i++) {
        if (!fact_field.m_strSourceFactName.empty()) {
            if (factWS->GetFact(i).GetFactName() != fact_field.m_strSourceFactName)
                continue;
        }
        const CWordsPair* pWS = factWS->GetFact(i).GetValue(strSourceFieldName);
        if (pWS == NULL)
            continue;

        size_t j = 0;
        for (; j < wsValues.size(); j++)
            if ((*(CWordsPair*)pWS) ==  (*(CWordsPair*)wsValues[j]))
                break;
        if (j >= wsValues.size())
            wsValues.push_back(pWS);
    }

    if (wsValues.size() == 0)
        return false;

    sort(wsValues.begin(), wsValues.end(), SPeriodOrder());

    if ((wsValues.size() > 1) && (factsIndexes.size() > 1)) {
        if (wsValues.size() == factsIndexes.size()) {
            for (size_t i = 0; i < factsIndexes.size(); i++) {
                newFacts[factsIndexes[i]].AddValue(fact_field.m_strFieldName, *wsValues[i], fact_field.m_Field_type,
                                                   fact_field.m_bConcatenation);
            }
        } else
            return true;
    } else if (factsIndexes.size() == 0) {
        for (size_t i = 0; i < wsValues.size(); i++) {
            CFactFields fact(fact_field.m_strFactTypeName);
            fact.AddValue(fact_field.m_strFieldName, *wsValues[i], fact_field.m_Field_type, fact_field.m_bConcatenation);
            newFacts.push_back(fact);
        }
    } else {
        if ((factsIndexes.size() >= 1) && (wsValues.size() == 1)) {
            for (size_t i = 0; i < factsIndexes.size(); i++)
                newFacts[factsIndexes[i]].AddValue(fact_field.m_strFieldName, *wsValues[0], fact_field.m_Field_type,
                                                   fact_field.m_bConcatenation);
        } else if ((factsIndexes.size() == 1) && (wsValues.size() >= 1)) {
                if (wsValues.size() == 1)
                    newFacts[factsIndexes[0]].AddValue(fact_field.m_strFieldName, *wsValues[0], fact_field.m_Field_type,
                                                       fact_field.m_bConcatenation);
                else {
                    CFactFields newFactSave = newFacts[factsIndexes[0]];
                    for (size_t j = 0; j < wsValues.size(); j++) {
                        if (j == 0)
                            newFacts[factsIndexes[0]].AddValue(fact_field.m_strFieldName, *wsValues[j], fact_field.m_Field_type,
                                                               fact_field.m_bConcatenation);
                        else {
                            newFacts.push_back(newFactSave);
                            newFacts.back().AddValue(fact_field.m_strFieldName, *wsValues[j], fact_field.m_Field_type,
                                                     fact_field.m_bConcatenation);
                        }
                    }
                }
            }

    }
    return true;
}

bool CCommonGrammarInterpretation::AddFactField(SReduceConstraints* pConstr) const
{
    ymap<int, yvector<fact_field_reference_t> >::const_iterator it = pConstr->Get().m_FactsInterpretation.begin();
    for (; it != pConstr->Get().m_FactsInterpretation.end(); it++) {
        CFactSynGroup* p_Item = CheckedCast<CFactSynGroup*>(pConstr->GetChildGroup(it->first));

        for (size_t i = 0; i < it->second.size(); i++) {
            if (TryCutOutFactField(p_Item, it->second[i]))
                continue;
            switch (it->second[i].m_Field_type) {
                case TextField: AddTextFactField(p_Item, it->second[i]); break;
                case FioField : AddFioFactField(p_Item, it->second[i]); break;
                case DateField: AddDateFactField(p_Item, it->second[i]); break;
                case BoolField: AddBoolFactField(p_Item, it->second[i]); break;
                default: break;
            }
        }
    }

    return (pConstr->Get().m_FactsInterpretation.size() > 0);
}

//ф-ция берет заданное поле из уже построенного другой грамматикой факта
bool CCommonGrammarInterpretation::TryCutOutFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const
{
    CWordSequence* pWsq = pItem->GetFirstMainHomonym()->GetSourceWordSequence();
    CFactsWS* factWS = dynamic_cast<CFactsWS*>(pWsq);
    if (NULL == factWS) return false;

    bool bRes = FillFactFieldFromFactWS(rFieldRef, factWS, pItem->m_Facts);

    //если не можем найти нужное поле в факте, то возвращаем false,
    //чтобы приписать просто текст, соответсвующий обрабатываемому терминалу,
    //но если есть помета eOnlyFromFact, то в любом случае возвращаем true,
    //чтобы уже не приписывать весь текст.
    if (rFieldRef.m_eFactAlgorithm.Test(eOnlyFromFact))
        return true;
    else
        return bRes;
}

void CCommonGrammarInterpretation::AddTextFactFieldValue(CTextWS& ws, const fact_field_descr_t fieldDescr) const
{
    if (!NStr::IsEqual(fieldDescr.m_StringValue, "$SENT"))
        ws.AddLemma(SWordSequenceLemma(fieldDescr.m_StringValue));
    else
        ws.AddLemma(SWordSequenceLemma(m_Words.SentToString()));
}

void CCommonGrammarInterpretation::AddTextFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const
{
    CTextWS GroupChain;
    if (!rFieldRef.m_StringValue.empty()) {
        AddWordsInfo(pItem, GroupChain, true);
        AddTextFactFieldValue(GroupChain, rFieldRef);
    } else
        AddWordsInfo(pItem, GroupChain);

    // preserving normalization info (?)
    GroupChain.m_iNormGrammems = rFieldRef.m_iNormGrammems;

    AddCommonFactField(pItem, rFieldRef, GroupChain);
}

void CCommonGrammarInterpretation::GetFioWSForFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef, CFioWS& newFioWS) const
{
    CWordSequence* pWsq = pItem->GetFirstMainHomonym()->GetSourceWordSequence();
    const CFioWordSequence* pFioWS = dynamic_cast<const CFioWordSequence*>(pWsq);
    if (pFioWS == NULL)
        ythrow yexception() << Substitute("Can't get fio value for $0.$1",
                rFieldRef.m_strFactTypeName, rFieldRef.m_strFieldName);

    newFioWS = CFioWS(*pFioWS);
    AddWordsInfo(pItem, newFioWS);
    if (!newFioWS.HasLemmas()) {
        Wtroka sLemmas;
        m_pReferenceSearcher->GetSentenceProcessor(m_pReferenceSearcher->GetCurSentence())->GetWSLemmaString(sLemmas, newFioWS, false);
        newFioWS.AddLemma(SWordSequenceLemma(sLemmas));
    }
}

void CCommonGrammarInterpretation::AddFioFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const
{
    CFioWS newFioWS;
    GetFioWSForFactField(pItem, rFieldRef, newFioWS);

    // preserving normalization info (?)
    newFioWS.m_iNormGrammems = rFieldRef.m_iNormGrammems;

    AddCommonFactField(pItem, rFieldRef, newFioWS);
}

void CCommonGrammarInterpretation::AddDateFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const
{
    CWordSequence* pWsq = pItem->GetFirstMainHomonym()->GetSourceWordSequence();
    CDateWS* dateWS = dynamic_cast<CDateWS*>(pWsq);
    if (NULL == dateWS) return;

    CDateWS dateCopy = *dateWS;

    AddCommonFactField(pItem, rFieldRef, dateCopy);
}

void CCommonGrammarInterpretation::NormalizeSequence(CTextWS* sequence, const CFactSynGroup* parent, const TGramBitSet& grammems) const {
    TSequenceInflector norm(*sequence, *parent, m_Words, m_FDOChainWords);
    norm.SetGrammar(&m_GLRgrammar);
    norm.Normalize(grammems);
}

void CCommonGrammarInterpretation::ReplaceLemmaAlways(CTextWS* sequence, const CFactSynGroup* parent) const {
    TSequenceInflector norm(*sequence, *parent, m_Words, m_FDOChainWords);
    norm.SetGrammar(&m_GLRgrammar);
    norm.ReplaceLemmaAlways();
}

void CCommonGrammarInterpretation::AddCommonFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef,
                                                      CWordsPair& rValueWS) const
{
    if (rFieldRef.HasAlgorithmInfo()) {
        CTextWS* pWS = dynamic_cast<CTextWS*>(&rValueWS);
        CFioWS* pFS = dynamic_cast<CFioWS*>(&rValueWS);
        if (pWS != NULL)
            static_cast<CFieldAlgorithmInfo&>(*pWS) = rFieldRef;
        else if (pFS != NULL)
            static_cast<CFieldAlgorithmInfo&>(*pFS) = rFieldRef;
        else
            return;

        if (pWS != NULL) {
            if (pWS->m_eFactAlgorithm.Test(eNotNorm)) {
                pWS->m_eFactAlgorithm.Reset(eNotNorm); //чтобы эта информация не влияла на все поле факта
                ReplaceLemmaAlways(pWS, pItem);
            } else if (rFieldRef.m_bConcatenation) {
                NormalizeSequence(pWS, pItem, pWS->m_iNormGrammems);
            }
        }
    }

    bool bFound = false;
    for (size_t j = 0; j < pItem->m_Facts.size(); ++j) {
        CFactFields& fact = pItem->m_Facts[j];
        if (fact.GetFactName() != rFieldRef.m_strFactTypeName)
            continue;

        bFound = true;

        // otherwise just reset fact field value (replacing old value if any!)
        fact.AddValue(rFieldRef.m_strFieldName, rValueWS, rFieldRef.m_Field_type);
    }
    if (!bFound) {
        //добавляем новый факт
        CFactFields newFactFields(rFieldRef.m_strFactTypeName);
        newFactFields.AddValue(rFieldRef.m_strFieldName, rValueWS, rFieldRef.m_Field_type);
        pItem->m_Facts.push_back(newFactFields);
    }
}

void CCommonGrammarInterpretation::AddBoolFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const
{
    CBoolWS boolWS(CWordsPair(m_Words.GetWord(m_FDOChainWords[pItem->FirstWord()]).GetSourcePair().FirstWord(),
                              m_Words.GetWord(m_FDOChainWords[pItem->LastWord()]).GetSourcePair().LastWord()),
                   rFieldRef.m_bBoolValue);

    AddCommonFactField(pItem, rFieldRef, boolWS);
}
//для всех детей pFactGroup правим номера омонимов слов, входящих в них,
//чем выше группа, тем меньше из-за всяких согласований в m_ChildHomonyms хранится
//омонимов. Омонимы из m_ChildHomonyms одной группы всегда являются подмножествами
//омонимов из соответсвующих m_ChildHomonyms ее подгрупп.
//Рекурсивно спускаемся по дереву, оставляя только те омонимы, которые хранятся у главной группы
void CCommonGrammarInterpretation::AdjustChildHomonyms(CFactSynGroup* pFactGroup)
{
    CGroup::TChildHomonyms childHomonymsOfParentGroup = pFactGroup->GetChildHomonyms();

    for (size_t i = 0; i < pFactGroup->m_Children.size(); i++) {
        CGroup* group = CheckedCast<CGroup*>(pFactGroup->m_Children[i]);
        group->IntersectChildHomonyms(childHomonymsOfParentGroup.Split(group->GetChildHomonyms().Size()));
        if (!group->IsPrimitive())
            AdjustChildHomonyms(CheckedCast<CFactSynGroup*>(group));

    }
}

void CCommonGrammarInterpretation::SetGenFlag(CFactFields& occur)
{
    bool bHasGenitive = false;
    for (CFactFields::FioFieldIterConst it = occur.GetFioFieldsSequenceBegin(); it != occur.GetFioFieldsSequenceEnd(); it++) {
        if ((*it).second.GetGrammems().Has(gGenitive)) {
            bHasGenitive = true;
            break;
        }
    }

    if (bHasGenitive) {

        for (CFactFields::TextFieldIterConst it = occur.GetTextFieldsSequenceBegin(); it != occur.GetTextFieldsSequenceEnd(); it++) {
            CTextWS& pTextWS = const_cast<CTextWS&>(it->second);
            pTextWS.SetGen();
        }

    }

}

void CCommonGrammarInterpretation::BuildFactWS(const COccurrence& occur, CWordSequence* pFactSequence)
{
    CFactSynGroup* pFactGroup = (CFactSynGroup*)occur.m_pInputItem;

    BuildFinalCommonGroup(occur, (CWordSequence*)pFactSequence);
    AdjustChildHomonyms(pFactGroup);

    if (m_GLRgrammar.m_ExternalGrammarMacros.m_bNoInterpretation) {
        CTextWS* pTextSequence = dynamic_cast< CTextWS* > (pFactSequence);
        if (!pTextSequence)
            ythrow yexception() << "Bad dynamic casting \"CCommonGrammarInterpretation::BuildFactWS\".";

        NormalizeSequence(pTextSequence, pFactGroup, pTextSequence->m_iNormGrammems);
        return;
    }

    CFactsWS* pFacts = dynamic_cast< CFactsWS* > (pFactSequence);
    if (!pFacts)
        ythrow yexception() << "Bad dynamic casting \"CCommonGrammarInterpretation::BuildFactWS\".";

    for (size_t j = 0; j < pFactGroup->m_Facts.size(); j++) {
        SetGenFlag(pFactGroup->m_Facts[j]);
        pFacts->AddFact(pFactGroup->m_Facts[j]);
    }

    CWordsPair artificialPair(pFacts->FirstWord(), pFacts->LastWord());
    CheckNecessaryFields(pFacts->m_Facts , artificialPair);
    NormalizeFacts(pFacts, pFactGroup);
    yset<int> excludedWords;
    bool bTrimed = false;
    CWordsPair trimedPair;
    bTrimed = TrimTree(pFacts, pFactGroup, excludedWords, trimedPair);

    //нормализация всей выделенной грамматикой цепочки
    TSequenceInflector norm(*pFacts, *pFactGroup, m_Words, m_FDOChainWords);
    norm.SetGrammar(&m_GLRgrammar);
    for (yset<int>::const_iterator excl = excludedWords.begin(); excl != excludedWords.end(); ++excl)
        if (*excl >= 0)
            norm.ExcludeWord(*excl);
    norm.Normalize(pFacts->m_iNormGrammems);

    if (bTrimed && trimedPair.IsValid())
        pFacts->SetPair(trimedPair);
}

void CCommonGrammarInterpretation::Run(bool allowAmbiguity)
{
    CInputSequenceGLR InputSequenceGLR(this);
#ifdef NDEBUG
    try {
#endif
        yvector< COccurrence > occurrences;
        RunParser(InputSequenceGLR, occurrences, allowAmbiguity);

        for (size_t i = 0; i < occurrences.size(); i++) {
            const CWord& wStart = m_Words.GetWord(m_FDOChainWords[occurrences[i].first]);
            if (wStart.IsEndOfStreamWord())
                continue;

            int iLast = occurrences[i].second - 1;
            //несуществующий конец предложения (m_EndWord) может быть концом occurrences[i] - не включать его
            if (m_Words.GetWord(m_FDOChainWords[iLast]).IsEndOfStreamWord())
                iLast -= 1;
            const CWord& wEnd = m_Words.GetWord(m_FDOChainWords[iLast]);

            THolder<CWordSequence> pFactSequence;
            if (!m_GLRgrammar.m_ExternalGrammarMacros.m_bNoInterpretation)
                pFactSequence.Reset(new CFactsWS());
            else
                pFactSequence.Reset(new CTextWS());

            pFactSequence->SetPair(wStart.GetSourcePair().FirstWord() , wEnd.GetSourcePair().LastWord());
            BuildFactWS(occurrences[i], pFactSequence.Get());

            CFactsWS* pFacts = dynamic_cast<CFactsWS*>(pFactSequence.Get());
            if ((pFacts != NULL && pFacts->GetFactsCount() > 0) || m_GLRgrammar.m_ExternalGrammarMacros.m_bNoInterpretation)
                m_FoundFacts.push_back(pFactSequence.Release());
        }
        if (!allowAmbiguity)
            CheckFactCountConstraintInSent();

        FreeParser(InputSequenceGLR);
#ifdef NDEBUG
    } catch (...) {
        FreeParser(InputSequenceGLR);
        throw;
    }
#endif

}

void CCommonGrammarInterpretation::AddWordsInfo(const CFactSynGroup* rItem, CWordSequence& fact_group, bool bArtificial) const
{
    {   // установка границ
        int iWStart = m_Words.GetWord(m_FDOChainWords[rItem->FirstWord()]).GetSourcePair().FirstWord();
        int iWEnd = m_Words.GetWord(m_FDOChainWords[rItem->LastWord()]).GetSourcePair().LastWord();
        if (bArtificial)
            fact_group.SetArtificialPair(CWordsPair(iWStart, iWEnd));
        else
            fact_group.SetPair(iWStart, iWEnd);
    }

    //не забыть заполнить имя нетерминала
    fact_group.PutNonTerminalSem(m_GLRgrammar.m_UniqueGrammarItems[rItem->GetActiveSymbol()].m_ItemStrId);

    {   // установка главного слова
        int MainWordNo = rItem->GetMainPrimitiveGroup()->FirstWord();
        SWordHomonymNum mainWH = m_FDOChainWords[MainWordNo];
        mainWH.m_HomNum = m_InterpNorm.GetFirstHomonymInTomitaGroup(m_FDOChainWords, rItem, MainWordNo);
        fact_group.SetMainWord(mainWH);
    };
}

void CCommonGrammarInterpretation::LogRule(size_t SymbolNo, size_t /*iFirstItemNo*/, size_t /*iLastItemNo*/, size_t iSynMainItemNo,
                                           const yvector<CInputItem*>& children, const Stroka& msg)  const
{
    if (NULL != m_pLogStream) {
        PrintFlatRule(*m_pLogStream, SymbolNo, iSynMainItemNo, children);
        *m_pLogStream << msg << "\n";
        *m_pLogStream << "\n";
    }
};

CInputItem* CCommonGrammarInterpretation::CreateNewItem(size_t SymbolNo, const CRuleAgreement& agreements,
                                                            size_t iFirstItemNo, size_t iLastItemNo,
                                                            size_t iSynMainItemNo, yvector<CInputItem*>& children)
{
    if (iSynMainItemNo > children.size())
        ythrow yexception() << "Bad iSynMainItemNo in \"CCommonGrammarInterpretation::CreateNewItem\"";

    SReduceConstraints constr(agreements, children, SymbolNo, iFirstItemNo, iSynMainItemNo, iLastItemNo - iFirstItemNo);
    constr.TrimTree = &m_TrimTree;
    constr.LeftPriorityOrder = &m_LeftPriorityOrder;

/*
    //uncomment for using as breakpoint while debugging
    const Stroka& rule_name = m_GLRgrammar.m_UniqueGrammarItems[SymbolNo].m_ItemStrId;
    if (rule_name == "")
        LogRule(SymbolNo, iFirstItemNo, iLastItemNo, iSynMainItemNo, children);
*/
    if (!ReduceCheckConstraintSet(&constr)) {
        if (m_bLog) {
            Stroka msg = ">> FAILED";
            if (agreements.size() > 0) {
                msg += ":\n";
                for (size_t i = 0; i < agreements.size(); i++)
                    msg += Stroka("        ") + GetAgreementStr(agreements[i], CGramInfo::s_DebugEncoding) + "\n";
            }
            LogRule(SymbolNo, iFirstItemNo, iLastItemNo, iSynMainItemNo, children, msg);
        }
        return NULL;
    }

    CGroup* mainSubgroup = CheckedCast<CGroup*>(children[iSynMainItemNo]);
    THolder<CFactSynGroup> pNewGroup(new CFactSynGroup(mainSubgroup, constr.Grammems, SymbolNo));
    m_CurrentFactGroup = pNewGroup.Get();

#ifndef NDEBUG
    pNewGroup->SetRuleName(m_GLRgrammar.m_UniqueGrammarItems[SymbolNo].m_ItemStrId);
#endif
    pNewGroup->SetPair(iFirstItemNo + m_iCurStartWord, iLastItemNo + m_iCurStartWord - 1);
    pNewGroup->m_Children = children;
    pNewGroup->AppendChildHomonymsFrom(children);
    pNewGroup->IntersectChildHomonyms(CGroup::TChildHomonyms(constr.ChildHomonyms));
    AdjustChildHomonyms(pNewGroup.Get());

    pNewGroup->SaveCheckedAgreements(constr.Get().m_RulesAgrs, iSynMainItemNo, children);
    pNewGroup->SetCheckedConstraints(constr.Get());

    InitKwtypesCount(pNewGroup.Get(), children, constr.Get());
    InitUserWeight(pNewGroup.Get(), children, constr.Get());

    AddFactField(&constr);

    TGrammarBunch child_grammems;
    for (size_t i = 0; i < children.size(); i++) {
        if (!((CGroup*)children[i])->IsPrimitive()) {
            CFactSynGroup* psyngroup = dynamic_cast<CFactSynGroup*>(children[i]);
            child_grammems.insert(psyngroup->GetGrammems().All());
            if (!MergeFactGroups(pNewGroup->m_Facts, psyngroup->m_Facts, child_grammems,
                                  CWordSequenceWithAntecedent::eEllipseCount)) {
                if (m_bLog)
                    LogRule(SymbolNo, iFirstItemNo, iLastItemNo, iSynMainItemNo, children, ">> not passed merging");
                return NULL;
            }
        }
    }

    MiddleInterpretation(pNewGroup->m_Facts, children);

    if (m_bLog) {
        Stroka msg;
        if (agreements.size() > 0)
            for (size_t i = 0; i < agreements.size(); i++) {
                Stroka agrStr = GetAgreementStr(agreements[i], CGramInfo::s_DebugEncoding);
                if (StripString(TStringBuf(agrStr)).empty())
                    continue;
                if (i == constr.CurrentConstraintIndex)
                    msg += "    Ok: ";
                else
                    msg += "        ";
                msg += agrStr + '\n';
            }
        LogRule(SymbolNo, iFirstItemNo, iLastItemNo, iSynMainItemNo, children, msg);
    }

    return pNewGroup.Release();
}

bool CCommonGrammarInterpretation::MergeFactGroups(yvector<CFactFields>& vec_facts1,
                                                    const yvector<CFactFields>& vec_facts2,
                                                    const TGrammarBunch& /*child_grammems*/,
                                                    CWordSequenceWithAntecedent::EEllipses /*eEllipseType*/)
{
    //try
    {
        if (0 == vec_facts2.size()) return true;

        if (0 == vec_facts1.size()) {
            vec_facts1 = vec_facts2;
            return true;
        }

        CFactSubsets mFactSubsets1;
        CFactSubsets mFactSubsets2;

        DivideFactsSetIntoSubsets(mFactSubsets1, vec_facts1);
        DivideFactsSetIntoSubsets(mFactSubsets2, vec_facts2);

        //сливаем одного типа факты между собой
        CFactSubsets::iterator it = mFactSubsets1.begin();
        for (; it != mFactSubsets1.end(); it++) {
            CFactSubsets::iterator it2 = mFactSubsets2.find(it->first);
            if (mFactSubsets2.end() == it2)
                continue;

            if (IsFactIntersection(it->second, it2->second)) {
                for (size_t i = 0; i < it2->second.size(); i++)
                    it->second.push_back(it2->second[i]);

                continue;
            }

            if (!MergeEqualNameFacts(it->second, it2->second))
                return false;
        }

        //добавляем новые факты
        for (it = mFactSubsets2.begin(); it != mFactSubsets2.end(); it++) {
            CFactSubsets::iterator it2 = mFactSubsets1.find(it->first);
            if (mFactSubsets1.end() == it2)
                mFactSubsets1[it->first] = it->second;
        }

        vec_facts1.clear();
        for (it = mFactSubsets1.begin(); it != mFactSubsets1.end(); it++)
            vec_facts1.insert(vec_facts1.end(), it->second.begin(), it->second.end());

        return true;
    }
    /*catch(yexception& e)
    {
        Stroka msg;
        msg.Format("Error in \"CCommonGrammarInterpretation::MergeFactGroups\": %s", e.what());
        ythrow yexception() << (const char*)msg;
    }
    catch(...)
    {
        ythrow yexception() << "Unknown error in \"CCommonGrammarInterpretation::MergeFactGroups\".";
    }*/
}

bool CCommonGrammarInterpretation::MergeEqualNameFacts(yvector<CFactFields>& vec_facts1, yvector<CFactFields>& vec_facts2) const
{
    if (IsEqualCoordinationMembers(vec_facts1, vec_facts2)) {
        for (size_t i = 0; i < vec_facts1.size(); i++)
            MergeTwoFacts(vec_facts1[i], vec_facts2[i]);

        return true;
    }
    if (IsOneToMany(vec_facts1, vec_facts2)) {
        for (size_t i = 0; i < vec_facts2.size(); i++)
            MergeTwoFacts(vec_facts2[i], vec_facts1[0]);

        vec_facts1 = vec_facts2;
        return true;
    }
    if (IsOneToMany(vec_facts2, vec_facts1)) {
        for (size_t i = 0; i < vec_facts1.size(); i++)
            MergeTwoFacts(vec_facts1[i], vec_facts2[0]);

        return true;
    }

    return false;
}

//ф-ция разбивает все можество фактов на подмножества по имени факта
void CCommonGrammarInterpretation::DivideFactsSetIntoSubsets(CFactSubsets& rFactSubsets, const yvector<CFactFields>& vec_facts) const
{
    for (size_t i = 0; i < vec_facts.size(); i++) {
        CFactSubsets::iterator it = rFactSubsets.find(vec_facts[i].GetFactName());
        if (it != rFactSubsets.end())
            it->second.push_back(vec_facts[i]);
        else
            rFactSubsets[ vec_facts[i].GetFactName() ] = yvector<CFactFields>(1, vec_facts[i]);
    }
}

void CCommonGrammarInterpretation::NormalizeFact(CFactFields& fact, const CFactSynGroup* pGroup) const {
    const fact_type_t* fact_type = &GlobalDictsHolder->RequireFactType(fact.GetFactName());
    for (size_t i = 0; i < fact_type->m_Fields.size(); i++) {
        const fact_field_descr_t& field = fact_type->m_Fields[i];
        const Stroka& fieldName = field.m_strFieldName;
        switch (field.m_Field_type) {
            case DateField:
                NormalizeDate(fact.GetDateValue(fieldName));
                break;
            case FioField:
                NormalizeFio(fact.GetFioValue(fieldName), pGroup);
                break;
            case TextField:
                NormalizeText(fact.GetTextValue(fieldName), pGroup, field);
                break;
            default:
                break;
        }
    }
}

void CCommonGrammarInterpretation::NormalizeFio(CFioWS* dstFio, const CFactSynGroup* parent) const {
    if (dstFio == NULL)
        return;
    TTomitaWords tomitaWords(m_Words, m_FDOChainWords);
    const CFactSynGroup* group = tomitaWords.GetChildGroup(*parent, *dstFio);
    //если не смогли найти нужную группу, то значит поле ФИО пришло из другого факта, а значит и апдейтить нечего
    if (group) {
        const CFioWordSequence* srcFio = dynamic_cast<const CFioWordSequence*>(group->GetFirstMainHomonym()->GetSourceWordSequence());
        if (srcFio) {
            dstFio->m_Fio = srcFio->m_Fio;
            memcpy(dstFio->m_NameMembers, srcFio->m_NameMembers, sizeof(srcFio->m_NameMembers));
            dstFio->SetMainWord(tomitaWords.GetFirstHomonymNum(group, group->GetMainPrimitiveGroup()->FirstWord()));
        }
    }
}

static Stroka PrintDatePart(const yvector<int>& parts, const char* mask, const char* deflt) {
    if (!parts.empty() && parts[0] > 0)
        return Sprintf(mask, parts[0]);
    else
        return deflt;
}

void CCommonGrammarInterpretation::NormalizeDate(CWordSequence* pWS) const {
    if (pWS) {
        SWordHomonymNum main = pWS->GetMainWord();
        const CWord& word = m_Words.GetWord(main);
        if (word.IsOriginalWord())
            return;

        const CWordSequence* dateWS = m_Words[main].GetSourceWordSequence();
        if (dateWS->GetWSType() == DateTimeWS) {
            const CDateGroup* pDateGroup = CheckedCast<const CDateGroup*>(dateWS);

            Stroka strYear  = PrintDatePart(pDateGroup->m_iDay, "%02d.", "00.");
            Stroka strMonth = PrintDatePart(pDateGroup->m_iMonth, "%02d.", "00.");
            Stroka strDay   = PrintDatePart(pDateGroup->m_iYear, "%d", "0000");
            pWS->AddLemma(SWordSequenceLemma(CharToWide(strDay + strMonth + strYear)));
        }
    }
}

void CCommonGrammarInterpretation::NormalizeText(CTextWS* pWS, const CFactSynGroup* parent, const fact_field_descr_t& field) const {
    if (pWS != NULL) {
        TGramBitSet caseForNorm = pWS->m_iNormGrammems;
        static const Stroka GENNORM = "gennorm";
        if (field.m_options.find(GENNORM) != field.m_options.end())
            caseForNorm.ReplaceByMask(TGramBitSet(gGenitive), NSpike::AllCases);
        NormalizeSequence(pWS, parent, caseForNorm);
    }
}

void CCommonGrammarInterpretation::NormalizeFacts(CFactsWS* pFactWordSequence, const CFactSynGroup* pGroup)
{
    for (int i = 0; i < pFactWordSequence->GetFactsCount(); ++i) {
        NormalizeFact(pFactWordSequence->GetFact(i), pGroup);
    }
    CheckTextFieldAlgorithms(pFactWordSequence);
}

void CCommonGrammarInterpretation::CheckTextFieldAlgorithms(CFactsWS* pFacts)
{
    for (int i = 0; i < pFacts->GetFactsCount(); i++) {
        CheckDelPronounAlg(pFacts->GetFact(i));
        CheckQuotedOrgNormAlg(pFacts->GetFact(i));
        CheckNotNorm(pFacts->GetFact(i));
        CheckCapitalized(pFacts->GetFact(i));

        if (!CheckUnwantedStatusAlg(pFacts->GetFact(i)) || !CheckShortNameAlg(pFacts->GetFact(i))) {
            pFacts->m_Facts.erase((pFacts->m_Facts.begin()+i));
            i--;
        }
    }
}

void CCommonGrammarInterpretation::CheckFactCountConstraintInSent()
{
    ymap<Stroka, size_t> mFactTypeCounts;

    for (size_t i = 0; i < m_FoundFacts.size(); i++) {
        CFactsWS* factWS = dynamic_cast<CFactsWS*>(m_FoundFacts[i]);
        if (factWS != NULL)
            for (int j = 0; j < factWS->GetFactsCount(); ++j)
                mFactTypeCounts[factWS->GetFact(j).GetFactName()] += 1;
    }

    ymap<Stroka, size_t>::const_iterator it = mFactTypeCounts.begin();
    for (; it != mFactTypeCounts.end(); ++it)
        if (it->second > m_FactCountConstraint) {
            Cerr << "Too many facts of type \"" << it->first << "\" found (" << it->second << "), will be ignored.\nThe current limit is " << m_FactCountConstraint << "." << Endl;
            for (size_t i = 0; i < m_FoundFacts.size(); i++)
                delete m_FoundFacts[i];
            m_FoundFacts.clear();
        }
}

//(если несклоняемая или ненайденная в морфологии фамилия - одиночная или содержит только инициалы)
//и (должность стоит в косвенном падеже),
//то не строить факт.
//Однако в отношении президента России Муров спокоен, так как, ...
//Было  также  определено  ,  о  чем  сообщал  целый  ряд  СМИ  ,  что  председателем  исполкома  НПСР  должен  стать  А.  Кондауров  ,  главным  редактором  "Правды"  -  А.  Баранов  .
bool CCommonGrammarInterpretation::CheckConstraintForLonelyFIO(yvector<CFactFields>& facts)
{
    for (size_t i = 0; i < facts.size(); i++) {
        if (facts[i].GetFactName() != "Fdo")
            continue;

        const CFioWS* pFioWS = facts[i].GetFioValue(CFactFields::Fio);
        const CTextWS* pPostWS = facts[i].GetTextValue(CFactFields::Post);

        if (!pFioWS || !pPostWS)
            continue;

        const CWord& word = m_Words.GetWord(pFioWS->m_NameMembers[Surname]);
        if ((pFioWS->m_NameMembers[InitialName].IsValid() || 1 == pFioWS->Size()) &&
            (word.HasMorphNounWithGrammems(NSpike::AllMajorCases) || !word.HasMorphNounWithGrammems(TGramBitSet(gSurname))))
            if (!m_Words.GetWord(pPostWS->GetMainWord()).HasMorphNounWithGrammems(TGramBitSet(gNominative)))
                return false;
    }

    return true;
}

bool CCommonGrammarInterpretation::IsFactIntersection(yvector<CFactFields>& vec_facts1, yvector<CFactFields>& vec_facts2) const
{
    if (vec_facts1.empty() || vec_facts2.empty())
        return false;

    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(vec_facts1[0].GetFactName());

    bool bIntersect = false;
    yset<int> set_intersect1;
    yset<int> set_intersect2;
    for (size_t i = 0; i < vec_facts1.size(); i++)
        for (size_t j = 0; j < vec_facts2.size(); j++) {
            size_t k;
            for (k = 0; k < pFactType->m_Fields.size(); k++)
                if (vec_facts1[i].HasValue(pFactType->m_Fields.at(k).m_strFieldName)
                     && vec_facts2[j].HasValue(pFactType->m_Fields.at(k).m_strFieldName)
                    ) {
                    const CWordsPair* pWP = vec_facts1[i].GetValue(pFactType->m_Fields.at(k).m_strFieldName);
                    const CTextWS* pWS = dynamic_cast<const CTextWS* >(pWP);
                    const CWordsPair* pWP2 = vec_facts2[j].GetValue(pFactType->m_Fields.at(k).m_strFieldName);
                    const CTextWS* pWS2 = dynamic_cast<const CTextWS* >(pWP2);
                    //проверка на конкатенацию двух полей
                    if ((pWS != NULL || pWS2 != NULL)
                        && (pWS->m_bConcatenation
                            || pWS2->m_bConcatenation
                            )
                        )
                        continue;

                    bIntersect = true;
                    break;
                    //return true;
                }
            if (k == pFactType->m_Fields.size()) {
                set_intersect1.insert(i);
                set_intersect2.insert(j);
            }
        }

    if (!bIntersect) return false;
    if (bIntersect && 0 == set_intersect1.size()) return true;

    //все, что написано ниже, можно рассматривать в качестве эксперимента:
    //в работающих грамматиках покамест такого случая записи интерпретации не встречалось,
    //когда интерпретация (слияние фактов) идет слева направо,
    //то (при определенной записи интерпретации в правилах грамматики)
    //может возникнуть случай, который в упрощенном варианте выглядит так
    //[1]факт1.поле1 + [2]факт1.поле2 + [3]факт1.поле2 + [4]факт1.поле1 (1)
    //или
    //[1]факт1.поле1 + [2]факт1.поле2 + [3]факт1.поле1 + [4]факт1.поле2 (2)
    //правильно записывать в грамматике такие случаи отдельными правилами (сейчас так и сделано),
    //т.е. отдельными правилами собирать ([1] и [2]), а потом ([3] и [4]).
    //если же факты сливаются в приведенной последовательности,
    //то должен сработать этот механизм.
    //механизм включается только при условии, что пересечение одноименных фактов частичное,
    //т.е. в примере (1) это произойдет, когда будет проверяться пересечение фактов для [4] c ([1, 2] и [3]),
    //тогда [4] будет пересечено с [1, 2] и нет с [3],
    //соответсвенно [4] будет добавлен к [3] и образует полный факт [3,4], иначе (без этого механизма)
    //образует отдельный неполный факт [4], т.е. ( [1, 2] и [3] и [4] )
    yvector<CFactFields> vec_merge1;
    yvector<CFactFields> vec_merge2;

    yset<int>::const_iterator it = set_intersect1.begin();
    for (; it != set_intersect1.end(); it++)
        vec_merge1.push_back(vec_facts1[*it]);

    it = set_intersect2.begin();
    for (; it != set_intersect2.end(); it++)
        vec_merge2.push_back(vec_facts2[*it]);

    if (!MergeEqualNameFacts(vec_merge1, vec_merge2))
        return true;

    int ii;
    for (ii = (int)vec_facts1.size()-1; ii >= 0; ii--)
        if (set_intersect1.find(ii) == set_intersect1.end())
            vec_merge1.insert(vec_merge1.begin(), vec_facts1[ii]);

    for (ii = 0; (size_t)ii < vec_facts2.size(); ii++)
        if (set_intersect2.find(ii) == set_intersect2.end())
            vec_merge1.insert(vec_merge1.end(), vec_facts2[ii]);

    vec_facts1 = vec_merge1;
    vec_facts2.clear();

    return true;
}

void CCommonGrammarInterpretation::MergeTwoFacts(CFactFields& rFact1, CFactFields& rFact2) const
{
    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(rFact1.GetFactName());

    for (size_t k = 0; k < pFactType->m_Fields.size(); k++) {
        Stroka sFieldName = pFactType->m_Fields.at(k).m_strFieldName;
        bool bHasVal1, bHasVal2;
        bHasVal2 = rFact2.HasValue(sFieldName);
        bHasVal1 = rFact1.HasValue(sFieldName);
        if (bHasVal2 && !bHasVal1)
            rFact1.AddValue(sFieldName, *(rFact2.GetValue(sFieldName)), pFactType->m_Fields.at(k).m_Field_type);
        if (bHasVal2 && bHasVal1)
            FieldsConcatenation(rFact1, rFact2, sFieldName, pFactType->m_Fields.at(k).m_Field_type);
    }
}

bool CCommonGrammarInterpretation::IsEqualCoordinationMembers(const yvector<CFactFields>& vec_facts1,
                                                              const yvector<CFactFields>& vec_facts2) const
{
    return (vec_facts1.size() == vec_facts2.size());
}

bool CCommonGrammarInterpretation::IsOneToMany(const yvector<CFactFields>& vec_facts1,
                                               const yvector<CFactFields>& vec_facts2) const
{
    return (1 == vec_facts1.size() && 1 < vec_facts2.size());
}

void CCommonGrammarInterpretation::FieldsConcatenation(CFactFields& rFact1, CFactFields& rFact2, const Stroka& sFieldName,
                                                       EFactFieldType eFieldType) const
{
    CWordsPair* pWP1 = rFact1.GetValue(sFieldName);
    CWordsPair* pWP2 = rFact2.GetValue(sFieldName);
    CTextWS* pWS1 = dynamic_cast< CTextWS* >(pWP1);
    CTextWS* pWS2 = dynamic_cast< CTextWS* >(pWP2);

    if (!pWS1 || !pWS2) return;

    bool bFConcat = pWS1->m_bConcatenation;
    bool bSConcat = pWS2->m_bConcatenation;

    if (bFConcat && bSConcat)
        if (pWS1->FirstWord() < pWS2->FirstWord())
            bFConcat = false;
        else
            bSConcat = false;

    if (bFConcat) {
        yvector<SWordSequenceLemma> LemmasBCP = pWS2->GetLemmas();
        //m_InterpNorm.Normalize(rFact2, m_FDOChainWords,  (const CFactSynGroup*)(m_ReduceConstraints.m_pCurrentFactGroup));
        NormalizeFact(rFact2, m_CurrentFactGroup);
        pWS2->AddLemmas(pWS1->GetLemmas(), pWS1->IsNormalizedLemma());

        //если конкатенируем искусственную пару (напр. "министр") с парой, которой
        //соответствует какая-то реальная цпочка, то получившуюся пару не делаем искусственной
        //и приписываем координаты неискусственной
        if (pWS2->IsArtificialPair() && !pWS1->IsArtificialPair()) {
            pWS2->SetArtificialPair(false);
            pWS2->SetPair(*pWS1);
        } else
            //меняем координаты при конакатенации неискусственных пар
            if (pWS2->IsArtificialPair() && pWS1->IsArtificialPair() &&
                (pWS2->LastWord() + 1 == pWS1->FirstWord()))
                pWS2->SetLastWord(pWS1->LastWord());

        rFact1.AddValue(sFieldName, *pWS2, eFieldType, false);

        //нужно все равно для этой части запустить нормализацию, так как нужно
        //заполнить CTextWS::m_MainWords, которые заполняются из ф-ции нормализации

        //сохраним "старые" леммы, лучше не менять ничего
        yvector<SWordSequenceLemma> saveLemmas = pWS1->GetLemmas();
        //m_InterpNorm.NormalizeWordSequence(m_FDOChainWords, (const CFactSynGroup*)(m_ReduceConstraints.m_pCurrentFactGroup), pWS1);
        NormalizeSequence(pWS1, m_CurrentFactGroup, TGramBitSet());

        CTextWS* pNewVal = rFact1.GetTextValue(sFieldName);
        pNewVal->AddPrefix(*pNewVal);

        pWS1->ResetLemmas(saveLemmas, pWS1->IsNormalizedLemma());
        pWS1->ClearWordsInfos();
        pWS2->ResetLemmas(LemmasBCP, pWS2->IsNormalizedLemma());
    } else if (bSConcat) {
        //m_InterpNorm.Normalize(rFact1, m_FDOChainWords,  (const CFactSynGroup*)(m_ReduceConstraints.m_pCurrentFactGroup));
        NormalizeFact(rFact1, m_CurrentFactGroup);
        pWS1->AddLemmas(pWS2->GetLemmas(), pWS2->IsNormalizedLemma());

        //если конкатенируем искусственную пару (напр. "министр") с парой, которой
        //соответствует какая-то реальная цпочка, то получившуюся пару не делаем искусственной
        //и приписываем координаты неискусственной
        if (pWS1->IsArtificialPair() && !pWS2->IsArtificialPair()) {
            pWS1->SetArtificialPair(false);
            pWS1->SetPair(*pWS2);
        } else
            //меняем координаты при конкатенации неискусственных пар
            if (pWS1->IsArtificialPair() && pWS2->IsArtificialPair() &&
                (pWS1->LastWord() + 1 == pWS2->FirstWord()))
                pWS1->SetLastWord(pWS2->LastWord());

        //нужно все равно ... (см. выше)

        yvector<SWordSequenceLemma> saveLemmas = pWS2->GetLemmas();
        //m_InterpNorm.NormalizeWordSequence(m_FDOChainWords, (const CFactSynGroup*)(m_ReduceConstraints.m_pCurrentFactGroup), pWS2);
        TSequenceInflector norm(*pWS2, *m_CurrentFactGroup, m_Words, m_FDOChainWords);
        NormalizeSequence(pWS2, m_CurrentFactGroup, TGramBitSet());

        pWS1->AddPostfix(*pWS2);
        pWS2->ClearWordsInfos();
        pWS2->ResetLemmas(saveLemmas, pWS2->IsNormalizedLemma());
    }
    pWS1->SetConcatenationLemma();
}

bool CCommonGrammarInterpretation::TrimTree(CFactsWS* pFactWordSequence, CFactSynGroup* pGroup,
                                            yset<int>& excludedWords, CWordsPair& newPair) const
{
    if (m_TrimTree.empty()) return false;

    yset<size_t>::const_iterator it = m_TrimTree.find(pGroup->GetActiveSymbol());
    if (it == m_TrimTree.end())
        if (!IsTrimChild(pGroup->m_Children))
            return false;

    int iL  = 1000;
    int iR  = -1;
    for (int i = 0; i < pFactWordSequence->GetFactsCount(); i++) {
        CFactFields& fact = pFactWordSequence->GetFact(i);
        const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(fact.GetFactName());

        for (size_t j = 0; j < pFactType->m_Fields.size(); j++) {
            const fact_field_descr_t& field_descr = pFactType->m_Fields[j];
            ComparePair(iL, iR, fact.GetValue(field_descr.m_strFieldName));
        }
    }

    if (1000 == iL || -1 == iR)
        return false;

    newPair.SetPair(iL, iR);
    for (size_t i=0; i < m_FDOChainWords.size(); i++) {
        const CWordsPair& P =  m_Words.GetWord(m_FDOChainWords[i]).GetSourcePair();
        if ((P.FirstWord() > iR) || (P.LastWord() < iL))
            excludedWords.insert(i);
    }

    //pFactWordSequence->SetPair(iL, iR);

    const CWord& main_w = m_Words.GetWord(pFactWordSequence->GetMainWord());

    //подправим главное слово, если оно выскочило за пределы pFactWordSequence
    if (!pFactWordSequence->Includes(main_w.GetSourcePair())) {
        SWordHomonymNum wh;
        for (int k = pGroup->FirstWord(); k <= pGroup->LastWord(); k++) {
            wh = m_FDOChainWords[m_iCurStartWord+k];
            const CWord& w = m_Words.GetWord(wh);
            if (pFactWordSequence->Includes(w.GetSourcePair()))
                break;
        }

        ui32 homIDs = pGroup->GetChildHomonyms()[0];
        const CWord& w = m_Words.GetWord(wh);
        wh.m_HomNum =w.HasFirstHomonymByHomIDs_i(homIDs);

        const CHomonym& h = m_Words[wh];
        pGroup->SetGrammems(h.Grammems);
        pFactWordSequence->SetGrammems(pGroup->GetGrammems());
        pFactWordSequence->SetMainWord(wh);
    }
    return true;
    }

bool CCommonGrammarInterpretation::IsTrimChild(const yvector<CInputItem*>& rChildren) const
{
    size_t i;
    for (i = 0; i < rChildren.size(); i++) {
        yset<size_t>::const_iterator it = m_TrimTree.find(((const CGroup*)rChildren[i])->GetActiveSymbol());
        if (it != m_TrimTree.end())
            return true;
    }

    for (i = 0; i < rChildren.size(); i++) {
        const CFactSynGroup* pFamily = dynamic_cast< const CFactSynGroup* >(rChildren[i]);
        if (pFamily != NULL)
            if (IsTrimChild(pFamily->m_Children))
                return true;
    }

    return false;
}

void CCommonGrammarInterpretation::ComparePair(int& iL, int& iR, const CWordsPair* rSPair) const
{
    if (!rSPair)
        return;

    if (rSPair->IsValid()) {
        if (rSPair->FirstWord() < iL)
            iL = rSPair->FirstWord();
        if (rSPair->LastWord() > iR)
            iR = rSPair->LastWord();
    }
}

bool CCommonGrammarInterpretation::CheckUnwantedStatusAlg(const CFactFields& rStatusFact) const
{
    const CDictsHolder* pDictsHolder = GlobalDictsHolder;
    // нет статуса - проходим мимо этого факта
    const fact_type_t* pFactType = &pDictsHolder->RequireFactType(rStatusFact.GetFactName());

    const CTextWS* pStatus = NULL;
    const CFioWS*  pFIO    = NULL;

    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& field_descr = pFactType->m_Fields[i];

        const CWordsPair* pWP = rStatusFact.GetValue(field_descr.m_strFieldName);
        if (!pWP) continue;
        if (!pStatus) {
            pStatus = dynamic_cast< const CTextWS* > (pWP);
            if (pStatus != NULL && !pStatus->m_eFactAlgorithm.Test(eUnStatus)) {
                pStatus = NULL;
                continue;
            }
        }
        if (!pFIO)
            pFIO = dynamic_cast< const CFioWS* > (pWP);
    }

    if (!pStatus)  return true;

    if (pStatus->HasPossPronouns())  return false;

    // если главное слово - "создание" или "голова" - тогда это плохой статус, см. antistatus_words.txt
    if (m_Words.GetWord(pStatus->GetMainWord()).HasKWType(pDictsHolder->BuiltinKWTypes().AntiStatus))
        return false;

    // удаляем факт, если его статус состоит только "легких"  слов
    bool bLightStatus = true;
    int  OpenQuotesCount  = 0;
    int  CloseQuotesCount  = 0;
    for (int k = pStatus->FirstWord(); k <= pStatus->LastWord(); k++) {
        const CWord& w = m_Words.GetOriginalWord(k);
        bLightStatus = bLightStatus && (w.HasKWType(pDictsHolder->BuiltinKWTypes().LightStatus)
                                        ||  w.HasPOS(gAdjNumeral)
                                        ||  w.HasPOS(gNumeral)
                                        ||  (w.m_typ == Digit));
        if (w.HasOpenQuote())
            OpenQuotesCount++;
        if (w.HasCloseQuote())
            CloseQuotesCount++;

    }
    if (bLightStatus)
        return false;

    int  FioOpenQuotesCount  = 0;
    int  FioCloseQuotesCount  = 0;

    if (pFIO != NULL)
        for (int k = pFIO->FirstWord(); k <= pFIO->LastWord(); k++) {
            const CWord& w = m_Words.GetOriginalWord(k);
            if (w.HasOpenQuote())
                FioOpenQuotesCount++;
            if (w.HasCloseQuote())
                FioCloseQuotesCount++;
        }

    if (pFIO != NULL && pFIO->FirstWord()==pStatus->LastWord()+1) {
        if ((OpenQuotesCount == 1)
                &&  (CloseQuotesCount == 0)
                &&  m_Words.GetOriginalWord(pStatus->FirstWord()).HasOpenQuote()
            ) {
            // считаем кавычки в ФИО, если фио стоит контактно с статусом
            // и статус начинается с кавычки
            OpenQuotesCount += FioOpenQuotesCount;
            CloseQuotesCount += FioCloseQuotesCount;
        } else if ((FioOpenQuotesCount != 0) ||  (FioCloseQuotesCount != 0))
                return false;
    }

    if (OpenQuotesCount != CloseQuotesCount)
        return false;

    return true;
}

bool CCommonGrammarInterpretation::CheckShortNameAlg(const CFactFields& rShortNameFact) const
{
    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(rShortNameFact.GetFactName());

    const CTextWS* pShName = NULL;
    const CTextWS* pCompName = NULL;

    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& field_descr = pFactType->m_Fields[i];

        const CWordsPair* pWP = rShortNameFact.GetValue(field_descr.m_strFieldName);
        if (!pWP) continue;
        if (!pShName) {
            pShName = dynamic_cast< const CTextWS* > (pWP);
            if (pShName != NULL) {
                if (!pShName->m_eFactAlgorithm.Test(eShName))
                    pShName = NULL;
                else
                    continue;
            }
        }
        if (!pCompName)
            pCompName = dynamic_cast< const CTextWS* > (pWP);
    }

    if (!pShName || !pCompName)  return true;

    return m_InterpNorm.CheckCompanyShortName(*pCompName, *pShName);
}

void CCommonGrammarInterpretation::CheckDelPronounAlg(CFactFields& rPronounFact)
{
    yvector<CTextWS*> v_TWS;
    CheckCommonAlg(rPronounFact, eDlPronoun, v_TWS);
    for (size_t i = 0; i < v_TWS.size(); i++)
        v_TWS[i]->DeletePronouns();
}

void CCommonGrammarInterpretation::CheckCommonAlg(CFactFields& rFact, EFactAlgorithm eAlg, yvector<CTextWS*>& rTWS)
{
    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(rFact.GetFactName());

    CTextWS* pField = NULL;

    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& field_descr = pFactType->m_Fields[i];

        CWordsPair* pWP = rFact.GetValue(field_descr.m_strFieldName);
        if (!pWP) continue;
        pField = dynamic_cast< CTextWS* > (pWP);
        if (pField != NULL && pField->m_eFactAlgorithm.SafeTest(eAlg))
            rTWS.push_back(pField);
    }
}

void CCommonGrammarInterpretation::CheckQuotedOrgNormAlg(CFactFields& rOrgFact)
{
    yvector<CTextWS*> v_TWS;
    CheckCommonAlg(rOrgFact, eQuotedOrgNorm, v_TWS);
    for (size_t i = 0; i < v_TWS.size(); i++) {
        v_TWS[i]->ClearLemmas();
        if (m_InterpNorm.HasQuotedCompanyNameForNormalization(*(v_TWS[i])) > -1)
            m_InterpNorm.QuotedWordNormalize(*(v_TWS[i]));
        else {
            Wtroka sLemmas;
            m_pReferenceSearcher->GetSentenceProcessor(m_pReferenceSearcher->GetCurSentence())->GetWSLemmaString(sLemmas, *(v_TWS[i]), false);
            v_TWS[i]->AddLemma(SWordSequenceLemma(sLemmas));
        }
    }
}

//посмотрим на помету при описании факта про капитализацию (h-reg1, h-reg2, h-reg3)
//и поднимем, что нужно
void CCommonGrammarInterpretation::CheckCapitalized(CFactFields& rFact)
{
    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(rFact.GetFactName());

    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& field_descr = pFactType->m_Fields[i];
        CTextWS* ws = rFact.GetTextValue(field_descr.m_strFieldName);
        if (ws == NULL)
            continue;
        if (field_descr.m_bCapitalizeAll)
            ws->CapitalizeAll();
        else if (field_descr.m_bCapitalizeFirstLetter1)
            ws->CapitalizeFirstLetter1();
        else if (field_descr.m_bCapitalizeFirstLetter2)
            ws->CapitalizeFirstLetter2();
    }
}

void CCommonGrammarInterpretation::CheckNotNorm(CFactFields& rFact)
{
    yvector<CTextWS*> v_TWS;
    CheckCommonAlg(rFact, eNotNorm, v_TWS);
    for (size_t i = 0; i < v_TWS.size(); i++) {
        v_TWS[i]->ClearLemmas();
        Wtroka sLemmas;
        m_pReferenceSearcher->GetSentenceProcessor(m_pReferenceSearcher->GetCurSentence())->GetWSLemmaString(sLemmas, *(v_TWS[i]), false);
        v_TWS[i]->AddLemma(SWordSequenceLemma(sLemmas));
    }
}

bool CCommonGrammarInterpretation::InterpretationExchangeMerge(CFactFields& fact1, CFactFields& fact2) const
{
    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(fact1.GetFactName());

    bool bIntersect = false;
    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& field_descr = pFactType->m_Fields[i];

        if (!fact1.HasValue(field_descr.m_strFieldName)) {
            CWordsPair* pWP = fact2.GetValue(field_descr.m_strFieldName);
            if (!pWP) continue;
            fact1.AddValue(field_descr.m_strFieldName, *pWP, field_descr.m_Field_type);
            continue;
        }

        if (!fact2.HasValue(field_descr.m_strFieldName)) {
            CWordsPair* pWP = fact1.GetValue(field_descr.m_strFieldName);
            if (!pWP) continue;
            fact1.AddValue(field_descr.m_strFieldName, *pWP, field_descr.m_Field_type);
            continue;
        }

        bIntersect = true;
    }

    return bIntersect;
}

void CCommonGrammarInterpretation::MiddleInterpretation(yvector<CFactFields>& facts,  const yvector<CInputItem*>& children) const
{
    for (size_t i = 0; i < children.size(); i++) {
        if (((CGroup*)children[i])->IsPrimitive()) continue;
        const yvector<CFactFields>& FFacts = dynamic_cast< const CFactSynGroup* >(children[i])->m_Facts;
        if (FFacts.empty()) continue;

        yset<Stroka> KeyFacts;
        CFactSubsets mFactSubsets1;
        DivideFactsSetIntoSubsets(mFactSubsets1, FFacts);

        for (size_t j = i+1; j < children.size(); j++) {
            if (((CGroup*)children[j])->IsPrimitive()) continue;

            const yvector<CFactFields>& SFacts = dynamic_cast< const CFactSynGroup* >(children[j])->m_Facts;
            if (SFacts.empty()) continue;

            CFactSubsets mFactSubsets2;
            DivideFactsSetIntoSubsets(mFactSubsets2, SFacts);

            CFactSubsets::iterator it = mFactSubsets1.begin();
            for (; it != mFactSubsets1.end(); it++) {
                CFactSubsets::iterator it2 = mFactSubsets2.find(it->first);
                if (mFactSubsets2.end() == it2)
                    continue;

                if (IsFactIntersection(it->second, it2->second)) {
                    if (KeyFacts.find(it->first) != KeyFacts.end())
                        MiddleInterpretationExchangeMerge(facts, it->first);
                    continue;
                } else {
                    KeyFacts.insert(it->first);
                }
            }
        }
    }
}

void CCommonGrammarInterpretation::MiddleInterpretationExchangeMerge(yvector<CFactFields>& facts,  const Stroka& rKeyFactName) const
{
    for (int i = facts.size()-1; i >=0; i--) {
        if (facts[i].GetFactName() != rKeyFactName) continue;
        CWordsPair artificialPair(m_CurrentFactGroup->FirstWord(), m_CurrentFactGroup->LastWord());
        if (HasAllNecessaryFields(facts[i], artificialPair)) continue;

        for (int j = i-1; j >= 0; j--) {
            if (facts[j].GetFactName() != rKeyFactName) continue;

            if (!InterpretationExchangeMerge(facts[i], facts[j])) {
                facts.erase(facts.begin()+j);
                i--;
            }
            if (HasAllNecessaryFields(facts[i], artificialPair))
                break;
        }
    }
}

Stroka GetAgrProcedureStr(ECoordFunc  A)
{
    switch (A) {
        case SubjVerb: return "subj";
        case SubjAdjShort: return "subj_Adj";
        case AnyFunc:   return "any";
        case CoordFuncCount:    return "coord";
        case GendreNumberCase   : return "gnc-agr";
        case GendreNumber       : return "gn-agr";
        case NumberCaseAgr      : return "nc-agr";
        case CaseAgr            : return "c-agr";
        case FeminCaseAgr       : return "fem-c-agr";
        case NumberAgr          : return "n-agr";
        case FioAgr             : return "fio-agr";
        case GendreCase         : return "gc-agr";
        case IzafetAgr          : return "izf-agr";
        case AfterNumAgr        : return "after-num-agr";
        case GeoAgr             : return "geo-agr";
        default             : return "unk-agr";
    };
};

Stroka CCommonGrammarInterpretation::GetAgreementStr(const SRuleExternalInformationAndAgreements& agr, ECharset encoding) const
{
    Stroka Result;
    for (size_t i=0; i < agr.m_RulesAgrs.size(); i++) {
        const SAgr& A = agr.m_RulesAgrs[i];

        if (A.m_AgreeItems.size() == 1) {
            if (A.e_AgrProcedure == CheckQuoted) {
                if (A.m_bNegativeAgreement)
                    Result += "~";
                Result += "quoted";
            } else if (CheckLQuoted == A.e_AgrProcedure) {
                if (A.m_bNegativeAgreement)
                    Result += "~";
                Result += "l-quoted";
            } else if (CheckRQuoted == A.e_AgrProcedure) {
                if (A.m_bNegativeAgreement)
                    Result += "~";
                Result += "r-quoted";
            } else
                Result += "grams";
        } else {
            if (A.m_bNegativeAgreement) Result += "~";
            Result += GetAgrProcedureStr(A.e_AgrProcedure);
        };

        if (!A.m_AgreeItems.empty()) {
            Result += "(";
            for (size_t j=0; j < A.m_AgreeItems.size(); j++) {
                Result += ::ToString(A.m_AgreeItems[j]);
                if (j+1 != A.m_AgreeItems.size())
                    Result += ",";
            };
            Result += ")";
        }

        if (A.m_Grammems.any()) {
            TStringStream grm_str;
            CHomonym::PrintGrammems(A.m_Grammems, grm_str, encoding);
            Result += "=";
            Result += grm_str;
        }

        if (A.m_NegativeGrammems.any()) {
            TStringStream grm_str;
            CHomonym::PrintGrammems(A.m_NegativeGrammems, grm_str, encoding);
            Result += "=~";
            Result += grm_str;
        }
        Result += " ";
    }

    for (ymap< int, SKWSet >::const_iterator it = agr.m_KWSets.begin(); it != agr.m_KWSets.end(); it++) {
        int ItemNo = it->first;
        const SKWSet& V = it->second;
        Result += Sprintf("kwset(%i)", ItemNo);
        if (V.m_bNegative)
            Result += "~=[";
        else
            Result += "=[";

        for (yset<SArtPointer>::const_iterator it_art = V.m_KWset.begin(); it_art != V.m_KWset.end(); it_art++) {
            const SArtPointer& kwtype = *it_art;
            if (kwtype.GetKWType() != NULL /*EKWTypeCount*/)
                Result += kwtype.GetKWType()->name();
            else
                Result += WideToChar(kwtype.GetStrType(), encoding);
            Result += " ";
        }
        Result += "] ";
    }

    for (ymap< int, CWordFormRegExp >::const_iterator it = agr.m_WFRegExps.begin(); it != agr.m_WFRegExps.end(); it++) {
        const CWordFormRegExp& re = it->second;
        Result += re.DebugString(it->first);
    }

    for (ymap< int, bool>::const_iterator it = agr.m_FirstWordConstraints.begin(); it != agr.m_FirstWordConstraints.end(); it++) {
        bool isFirstWord = it->second;
        if (isFirstWord)
            Result += ",fw";
        else
            Result += ",~fw";
    }

    for (ymap< int, SGrammemUnion >::const_iterator it = agr.m_RuleGrammemUnion.begin(); it != agr.m_RuleGrammemUnion.end(); it++) {
        for (size_t i = 0; i < it->second.m_GramUnion.size(); i++) {
            if (it->second.m_GramUnion[i].any()) {
                Result += Sprintf("GU(%i)=&[", it->first);
                TStringStream grm_str;
                CHomonym::PrintGrammems(it->second.m_GramUnion[i], grm_str, encoding);
                Result += grm_str;
                Result += "]";
            }
            if (it->second.m_NegGramUnion[i].any()) {
                Result += Sprintf("GU(%i)=~[", it->first);
                TStringStream grm_str;
                CHomonym::PrintGrammems(it->second.m_NegGramUnion[i], grm_str, encoding);
                Result += grm_str;
                Result += "]";
            }
        }
    }
    return Result;
}
