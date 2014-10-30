#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "synchain.h"
#include "factfields.h"
#include "factgroup.h"
#include "tomitaitemsholder.h"
#include "normalization.h"


typedef ymap< Stroka, yvector<CFactFields> > CFactSubsets;

class CCommonGrammarInterpretation: public CTomitaItemsHolder
{
public:
    CCommonGrammarInterpretation(const CWordVector& Words, const CWorkGrammar& gram,
                                 yvector<CWordSequence*>& foundFacts, const yvector<SWordHomonymNum>& clauseWords,
                                 CReferenceSearcher* RefSearcher, size_t factCountConstraint = 128);
    ~CCommonGrammarInterpretation();

    virtual void Run(bool allowAmbiguity=false);
    bool AddFactField(SReduceConstraints* rReduceConstraints) const;
    void AddTextFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const;
    void AddTextFactFieldValue(CTextWS& ws, const fact_field_descr_t fieldDescr) const;
    void AddFioFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const;
    void AddDateFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const;
    void AddBoolFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const;
    void GetFioWSForFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef, CFioWS& newFioWS) const;
    void AddCommonFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef, CWordsPair& rValueWS) const;

    //ф-ция берет заданное поле из уже построенного другой грамматикой факта
    bool TryCutOutFactField(CFactSynGroup* pItem, const fact_field_reference_t& rFieldRef) const;

    //ф-ция разбивает все можество фактов на подмножества по имени факта
    void DivideFactsSetIntoSubsets(CFactSubsets& rFactSubsets, const yvector<CFactFields>& vec_facts) const;

    void AddWordsInfo(const CFactSynGroup* rItem, CWordSequence& fact_group, bool bArtificial = false) const;
    void AddFioFromFactToSequence(CFactSynGroup* pNewGroup);
    void AddWordsInfoToCompanyPostSequence(EWordSequenceType e_WSType, int iWStart, int iWEnd, CWordSequence& fdo_froup);
    virtual bool MergeFactGroups(yvector<CFactFields>& vec_facts1, const yvector<CFactFields>& vec_facts2,
                                 const TGrammarBunch& child_grammems, CWordSequenceWithAntecedent::EEllipses eEllipseType);
    bool MergeEqualNameFacts(yvector<CFactFields>& vec_facts1,
                             yvector<CFactFields>& vec_facts2) const;

    using CTomitaItemsHolder::CreateNewItem;        // to avoid hiding of CTomitaItemsHolder::CreateNewItem(size_t, size_t)
    virtual CInputItem* CreateNewItem(size_t SymbolNo, const CRuleAgreement& agreements,
                                      size_t iFirstItemNo, size_t iLastItemNo,
                                      size_t iSynMainItemNo, yvector<CInputItem*>& children);

    void CheckTextFieldAlgorithms(CFactsWS* pFacts);
    void MergeTwoFacts(CFactFields& rFact1, CFactFields& rFact2) const;

    void MiddleInterpretation(yvector<CFactFields>& facts,  const yvector<CInputItem*>& children) const;
    void MiddleInterpretationExchangeMerge(yvector<CFactFields>& facts,  const Stroka& rKeyFactName) const;
    bool InterpretationExchangeMerge(CFactFields& fact1, CFactFields& fact2) const;

    CWordSequenceWithAntecedent::EEllipses HasGroupEllipse(CFactSynGroup* pNewGroup) const;

    //ограничение на кол-во фактов в предложении
    void CheckFactCountConstraintInSent();
    //(если несклоняемая или ненайденная в морфологии фамилия - одиночная или содержит только инициалы) и (должность стоит в косвенном падеже),
    //то не строить факт
    bool CheckConstraintForLonelyFIO(yvector<CFactFields>& facts);
    void SetGenFlag(CFactFields& occur);

protected:
    void AdjustChildHomonyms(CFactSynGroup* pFactGroup);
    void RefillFactFields(CFactSynGroup* pFactGroup);
    bool RefillFioFactField(CFactSynGroup* pFactGroup, Stroka factName, Stroka factField, CFioWS& value);
    bool RefillTextFactField(CFactSynGroup* pFactGroup, Stroka factName, Stroka factField, CTextWS& value);
    bool IsFactIntersection(yvector<CFactFields>& vec_facts1, yvector<CFactFields>& vec_facts2) const;

    void SimpleFactsMerge(yvector<CFactFields>& vec_facts1, const yvector<CFactFields>& vec_facts2);
    bool IsEqualCoordinationMembers(const yvector<CFactFields>& vec_facts1, const yvector<CFactFields>& vec_facts2) const;
    bool IsOneToMany(const yvector<CFactFields>& vec_facts1, const yvector<CFactFields>& vec_facts2) const;
    void BuildFactWS(const COccurrence& occur, CWordSequence* pFactSequence);
    void LogRule(size_t SymbolNo, size_t iFirstItemNo, size_t iLastItemNo, size_t iSynMainItemNo,
                 const yvector<CInputItem*>& children, const Stroka& msg = Stroka())  const;
    Stroka GetAgreementStr(const SRuleExternalInformationAndAgreements& agr, ECharset encoding) const;
    bool FillFactFieldFromFactWS(const fact_field_reference_t& fact_field, const CFactsWS* factWS, yvector<CFactFields>& newFacts) const;
    bool HasAllNecessaryFields(CFactFields& fact, const CWordsPair& artificialPair) const;
    bool CheckNecessaryFields(yvector<CFactFields>& newFacts, const CWordsPair& artificialPair) const;

    bool TrimTree(CFactsWS* pFactWordSequence, CFactSynGroup* pGroup, yset<int>& excludedWords, CWordsPair& newPair) const;
    bool IsTrimChild(const yvector<CInputItem*>& rChildren) const;
    void ComparePair(int& iL, int& iR, const CWordsPair* rSPair) const;

    void FieldsConcatenation(CFactFields& rFact1, CFactFields& rFact2, const Stroka& sFieldName, EFactFieldType eFieldType) const;
    bool CheckUnwantedStatusAlg(const CFactFields& rStatusFact) const;
    bool CheckShortNameAlg(const CFactFields& rShortNameFact) const;
    void CheckDelPronounAlg(CFactFields& rPronounFact);
    void CheckQuotedOrgNormAlg(CFactFields& rOrgFact);
    void CheckNotNorm(CFactFields& rFact);
    void CheckCapitalized(CFactFields& rFact);

    void CheckCommonAlg(CFactFields& rFact, EFactAlgorithm eAlg, yvector<CTextWS*>& rTWS);

    void NormalizeSequence(CTextWS* sequence, const CFactSynGroup* parent, const TGramBitSet& grammems) const;
    void ReplaceLemmaAlways(CTextWS* sequence, const CFactSynGroup* parent) const;

    void NormalizeFacts(CFactsWS* pFactWordSequence, const CFactSynGroup* parent);
    void NormalizeFact(CFactFields& fact, const CFactSynGroup* parent) const;
    void NormalizeText(CTextWS* pWS, const CFactSynGroup* parent, const fact_field_descr_t& field) const;
    void NormalizeDate(CWordSequence* ws) const;
    void NormalizeFio(CFioWS* fioWS, const CFactSynGroup* parent) const;


protected:
    CNormalization m_InterpNorm;

    //игнорируя покрытие, выбирает из двух построенных графов A и B, пересечение слов которых не пусто,
    //граф, левая граница которого находится ближе к началу предложения
    //(т.е. нужен для эмпирического правила выбора левостороннего ветвления ФДО фактов);
    //для этого в языке вводится помета при правилах:
    //A -> X {left_dominant};
    //B -> Y {right_recessive};
    //из примера: "...президента Юкос М. Ходорковского, корреспондента аналитического журнала "Эксперт"..."
    //будет выбран ФДО факт "президента Юкос М. Ходорковского"
    yset<CWordsPair> m_LeftPriorityOrder;
    //сейчас, если существуют в грамматике два правила с одинаковыми левыми частями
    //X -> A B {trim};
    //X -> C D;
    //процедура trim будет применена ко всем выбранным лучшими деревьям, в которые вошел нетерминал X,
    //несмотря на то, что trim приписан только первому правилу,
    //в будущем, если будет нужно, можно сделать тоньше и отслеживать правило, участвовавшее в построении дерева, а не X
    yset<size_t> m_TrimTree;

private:
    size_t m_FactCountConstraint;
    bool m_bLog;
    TOutputStream* m_pLogStream;

    const CFactSynGroup* m_CurrentFactGroup;

public:
    yvector<CWordSequence*>& m_FoundFacts;
    CReferenceSearcher* m_pReferenceSearcher;
};
