#pragma once

#include <util/generic/vector.h>

#include "group.h"
#include <FactExtract/Parser/afdocparser/common/homonymbase.h>

class CWord;
class CPrimitiveGroup;

struct SRuleExternalInformationAndAgreements;

class CSynGroup: public CGroup {
public:
    CSynGroup()
        : m_pMainGroup(NULL)
        , CheckedConstraints_(NULL)
    {
    }

    CSynGroup(CGroup* pMainGroup, const THomonymGrammems& grammems, int iSymbolNo)
        : Grammems(grammems)
        , m_pMainGroup(pMainGroup)
        , CheckedConstraints_(NULL)
    {
        SetActiveSymbol(iSymbolNo);
    }

    CGroup* Clone();

    const CGroup* GetMainGroup() const {
        return m_pMainGroup;
    }

    virtual void SetMainGroup(CGroup* pGroup) {
        m_pMainGroup = pGroup;
    }

    const CWord* GetMainWord() const {
        return m_pMainGroup->GetMainWord();
    }

    virtual int GetMainTerminalSymbol() const {
        return m_pMainGroup->GetMainTerminalSymbol();
    }

    CPrimitiveGroup* GetMainPrimitiveGroup() {
        return m_pMainGroup->GetMainPrimitiveGroup();
    }

    const CPrimitiveGroup* GetMainPrimitiveGroup() const {
        return m_pMainGroup->GetMainPrimitiveGroup();
    }

    virtual void GetMainClauseWord(int& iClauseWord, int& iH) const {
        return m_pMainGroup->GetMainClauseWord(iClauseWord, iH);
    }

    virtual const THomonymGrammems& GetGrammems() const {
        return Grammems;
    }

    virtual void SetGrammems(const THomonymGrammems& grammems) {
        Grammems = grammems;
    }

    bool HasEqualGrammems(const CGroup* other) const {
        return Grammems == other->GetGrammems();
    }

    virtual bool IsPrimitive() const {
        return false;
    }

    virtual ui32 GetMainHomIDs() const;

    virtual TChildHomonyms GetChildHomonyms() const {
        return TChildHomonyms(m_ChildHomonyms.begin(), m_ChildHomonyms.end());
    }

    virtual void IntersectChildHomonyms(const TChildHomonyms& homs);

    void AppendChildHomonymsFrom(CGroup& group) {
        TChildHomonyms homs = group.GetChildHomonyms();
        m_ChildHomonyms.insert(m_ChildHomonyms.end(), homs.Start, homs.Stop);
    }

    void AppendChildHomonymsFrom(const yvector<CInputItem*>& children);

    virtual Stroka DebugString() const;

    const SRuleExternalInformationAndAgreements* CheckedConstraints() const {
        return CheckedConstraints_;
    }

    void SetCheckedConstraints(const SRuleExternalInformationAndAgreements& constr) {
        CheckedConstraints_ = &constr;
    }

protected:
    THomonymGrammems Grammems;
    CGroup*    m_pMainGroup;

    //считаем, что у каждого слова не больше 32 омонимов
    //в каждом m_ChildHomonyms[i] выставлены в 1 битики, соответсвующие омонимам,
    //которые участвовали в построении этой группы
    yvector<ui32> m_ChildHomonyms;

    // all constraints used when reducing this group, could be NULL
    // points directly to constraint in corresponding grammar (which should outlive this group)
    const SRuleExternalInformationAndAgreements* CheckedConstraints_;
};
