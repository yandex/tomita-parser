#pragma once

#include <util/generic/cast.h>
#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "syngroup.h"
#include <FactExtract/Parser/afglrparserlib/agreement.h>
#include "factfields.h"

struct CCheckedAgreement
{
    SAgr    m_AgreementType;
    int        m_WordNo1;
    int        m_WordNo2;
    bool    IsUnary() const
    {
        return m_AgreementType.IsUnary() && (m_WordNo1 == m_WordNo2);
    }
};

class CWeightSynGroup : public CSynGroup {
private:
    void AddAgreementsFrom (const CGroup* F);
    TWeight GetWeightByAgreement() const;
    TWeight GetWeightByKwTypes() const;

public:
    int m_KwtypesCount;

    // все бинарные согласования, которые были проверены, чтобы построить эту группу
    yvector<CCheckedAgreement> m_CheckedAgrs;

    yvector<CInputItem*> m_Children;

public:
    CWeightSynGroup()
        : m_KwtypesCount(-1)
    {
    }

    CWeightSynGroup(CGroup* pMainGroup, const THomonymGrammems& grammems, int iSymbolNo)
        : CSynGroup(pMainGroup, grammems, iSymbolNo)
        , m_KwtypesCount(-1)
    {
    }

    void SaveCheckedAgreements(const yvector<SAgr>& agreements, size_t iSynMainItemNo, const yvector<CInputItem*>& children);
    virtual TWeight GetWeight() const;
    Stroka GetWeightStr()  const;
    void IncrementKwtypesCount(int delta);
    void AddWeightOfChild(const CInputItem * pGrp);

    size_t ChildrenSize() const {
        return m_Children.size();
    }

    const CGroup* GetChild(size_t i) const {
        return CheckedCast<const CGroup*>(m_Children[i]);
    }

    virtual const CGroup* GetFirstGroup() const {
        return GetChild(0);
    }
};

class CFactSynGroup : public CWeightSynGroup
{
public:
    yvector<CFactFields>            m_Facts;

    CFactSynGroup(CGroup* pMainGroup, const THomonymGrammems& grammems, int iSymbolNo);

    virtual size_t            GetCoverage() const;
    virtual size_t          GetFactsCount() const;
    const CFactSynGroup*        GetMaxChildWithThisFirstWord(int FirstWordNo) const;
    const CFactSynGroup*        GetChildByWordPair(const CWordsPair& Group) const;
    const CWord*                GetFirstWord() const;
    const CWord*                GetLastWord() const;
    Wtroka                        ToString() const;
};
