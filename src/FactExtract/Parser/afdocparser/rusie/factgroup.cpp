#include "factgroup.h"
#include "primitivegroup.h"

#include <util/string/strip.h>

// ===================================================
// ================  CWeightSynGroup =================
// ===================================================


const int MaxRelationWeight = 3;

static int GetAgreementWeight(const CCheckedAgreement& A)
{
    switch (A.m_AgreementType.e_AgrProcedure) {
        case GendreNumberCase: return MaxRelationWeight;
        case GendreNumber: return 2;
        case GendreCase: return 2;
        case NumberCaseAgr:    return 2;
        case CaseAgr: return 1;
        case FeminCaseAgr: return 1;
        default: return 0;
    }
}

static int GetRelevantAgrWeight(const yvector<CCheckedAgreement>& CheckedAgrs)
{
    int ret = 0;
    for (size_t i = 0; i < CheckedAgrs.size(); i++) {
        ret += GetAgreementWeight(CheckedAgrs[i]);
    }
    return ret;

}

CGroup::TWeight CWeightSynGroup::GetWeightByAgreement() const
{
    int weight = GetRelevantAgrWeight(m_CheckedAgrs);
    size_t group_size = Size();

    if (group_size == 1)
        return TWeight();   // default weight
    else {
        size_t norm = (MaxRelationWeight*group_size*(group_size-1))/2;  // C из n по к, умноженное на MaxRelationWeight,
                                                                        // где n - это размер группы,
                                                                        // k - количество элементов в отношении согласования, т.е. 2
                                                                        // C из n по к = n!/(k!*(n-k)!), в нашем случае
                                                                        // group_size!/(2!*(group_size-2)! = group_size*(group_size-1)/2
                                                                        // нельзя использовать функцию факториала, поскольку она очень быстро выходит  за пределы size_t
/*
        const double ONE = 1.0;
        double res = (weight == 0) ? ONE/(double)norm - ONE/(double)(norm*10)       // 0.9/norm
                                   : (double)Weight/(double)(norm);
        YASSERT(res > 0 && res <= 1);
*/
        TWeight res;
        if (weight == 0)
            return TWeight(9, 10 * norm);
        else
            return TWeight(weight, norm);

        return res;
    }
}

void CWeightSynGroup::IncrementKwtypesCount(int delta)
{
    m_KwtypesCount += delta;
    if ((int)Size() < m_KwtypesCount)
        m_KwtypesCount = Size(); // еслти атомарная группа содержит более одного kwtype, тогда будем
                            // учитывать это только один раз. Это происходит, например, если фио
                            // омонимично (мужчина/женщина)
}

CInputItem::TWeight CWeightSynGroup::GetWeightByKwTypes() const
{
    YASSERT(m_KwtypesCount != -1);
    YASSERT(m_KwtypesCount <= (int)Size());
    return TWeight(m_KwtypesCount, Size());
};

CInputItem::TWeight CWeightSynGroup::GetWeight() const
{
    // combine sub-weights into single weight
    return TWeight(GetWeightByAgreement(), GetWeightByKwTypes(), GetUserWeight());
    //return (GetWeightByAgreement() + GetWeightByKwTypes() + GetUserWeight()) / 3.0;
};

void CWeightSynGroup::AddWeightOfChild(const CInputItem * pGrp)
{
    const CWeightSynGroup* pGroup = dynamic_cast<const CWeightSynGroup*>(pGrp);
    if (!pGroup)
        return;
    IncrementKwtypesCount(pGroup->m_KwtypesCount);
    AddAgreementsFrom(pGroup);
    AddUserWeight(pGroup->GetUserWeight());
};

Stroka CWeightSynGroup::GetWeightStr()  const
{
    return Sprintf("[w_agr=%f(agr.number=%i) + w_kwtype=%f(kwcount=%i) +\\n w_user=%f]/3=%f",
        GetWeightByAgreement().AsDouble(), GetRelevantAgrWeight(m_CheckedAgrs),
        GetWeightByKwTypes().AsDouble(), m_KwtypesCount,
        GetUserWeight().AsDouble(), GetWeight().AsDouble());
};

// сохраняет все бинарные проверки ("agreements'), в которых (возможно, транзитивно) участвует главное слово
void CWeightSynGroup::SaveCheckedAgreements(const yvector< SAgr >& agreements,
                                            size_t iSynMainItemNo, const yvector<CInputItem*>& children)
{
    const size_t MaxChildrenCount = 250;
    assert (children.size() < MaxChildrenCount);
    bool DepentOnRoot[MaxChildrenCount];
    // добавляем все согласование детей
    for (size_t i=0; i < children.size(); i++) {
        DepentOnRoot[i] = false;
        AddAgreementsFrom((CGroup*)children[i]);
    };

    DepentOnRoot[iSynMainItemNo] = true;

    bool bNewAgreemsFound = true;
    while (bNewAgreemsFound) {
        bNewAgreemsFound = false;
        for (size_t i = 0; i < agreements.size(); i++) {
            const SAgr& agr = agreements[i];
            if (agr.IsUnary()) {
                CCheckedAgreement A;
                A.m_AgreementType = agr;
                int iChild = agr.m_AgreeItems[0];
                if ((iChild < 0) || ((size_t)iChild >= children.size())) {
                    ythrow yexception() << "Bad agreement.";
                }
                A.m_WordNo1 = ((CFactSynGroup*)children[iChild])->GetMainPrimitiveGroup()->FirstWord();
                A.m_WordNo2 = ((CFactSynGroup*)children[iChild])->GetMainPrimitiveGroup()->FirstWord();
                m_CheckedAgrs.push_back(A);
                continue;
            }

            if (2 != agr.m_AgreeItems.size())
                continue;
            int i1 = agr.m_AgreeItems[0];
            int i2 = agr.m_AgreeItems[1];
            if (((i1 < 0) || ((size_t)i1 >= children.size())) ||
                ((i2 < 0) || ((size_t)i2 >= children.size()))) {
                ythrow yexception() << "Bad agreement.";
            }

            if (DepentOnRoot[i1] == DepentOnRoot[i2])
                continue;

            if (agr.m_bNegativeAgreement)
                continue;

            bNewAgreemsFound = true;

            CCheckedAgreement A;
            A.m_AgreementType = agr;
            A.m_WordNo1 = ((CFactSynGroup*)children[i1])->GetMainPrimitiveGroup()->FirstWord();
            A.m_WordNo2 = ((CFactSynGroup*)children[i2])->GetMainPrimitiveGroup()->FirstWord();
            m_CheckedAgrs.push_back(A);
            if (!DepentOnRoot[i1])
                DepentOnRoot[i1] = true;
            else
                DepentOnRoot[i2] = true;
        }
    }

    #ifdef _LOG
        if (!m_CheckedAgrs.empty())
            printf ("\tnumber of stored agreements %i\n", m_CheckedAgrs.size());
    #endif

}
// ===================================================
//================  CFactSynGroup ====================
// ===================================================

CFactSynGroup::CFactSynGroup(CGroup* pMainGroup, const THomonymGrammems& grammems, int iSymbolNo)
    : CWeightSynGroup(pMainGroup, grammems, iSymbolNo)
{
}

size_t CFactSynGroup::GetFactsCount() const
{
    return m_Facts.size();
}

size_t CFactSynGroup::GetCoverage() const
{
    int res = 0;
    for (size_t i = 0; i <m_Children.size(); ++i) {
        CPrimitiveGroup* pPrimGroup = dynamic_cast<CPrimitiveGroup*>(m_Children[i]);
        if (pPrimGroup) {
            const CWord* MainWord =  pPrimGroup->GetMainWord();
            if (!MainWord->IsComma() && !MainWord->IsAndConj())
                res += MainWord->GetSourcePair().Size();
                //res++;
        } else {
            CFactSynGroup* pGroup = dynamic_cast<CFactSynGroup*>(m_Children[i]);
            if (pGroup)
                res += pGroup->GetCoverage();
        }
    }
    return res;
}

void CWeightSynGroup::AddAgreementsFrom (const CGroup* F)
{
    const CFactSynGroup*  pGroup = dynamic_cast < const CFactSynGroup* > (F);
    if (pGroup) {
        m_CheckedAgrs.insert(m_CheckedAgrs.end(), pGroup->m_CheckedAgrs.begin(), pGroup->m_CheckedAgrs.end());
    }
};

const CFactSynGroup* CFactSynGroup::GetMaxChildWithThisFirstWord(int FirstWordNo) const
{
    if (FirstWord() == FirstWordNo) return this;
    CWordsPair Group(FirstWordNo);
    for (size_t i=0; i < m_Children.size(); i++) {
        const CGroup* pG = (const CGroup*)m_Children[i];
        if (pG->Includes(Group)) {
            const CFactSynGroup* pG1 = dynamic_cast<const CFactSynGroup*>(pG);
            if (!pG1) return 0;
            return pG1->GetMaxChildWithThisFirstWord(FirstWordNo);
        };
    };
    return 0;
};

const CFactSynGroup* CFactSynGroup::GetChildByWordPair(const CWordsPair& Group) const
{
    if (*this == Group) return this;
    for (size_t i=0; i < m_Children.size(); i++) {
        const CGroup* pG = (const CGroup*)m_Children[i];
        if (pG->Includes(Group)) {
            const CFactSynGroup* pG1 = dynamic_cast<const CFactSynGroup*>(pG);
            if (!pG1) return 0;
            return pG1->GetChildByWordPair(Group);
        };
    };
    return 0;
};

const CWord* CFactSynGroup::GetFirstWord() const
{
    if (Size() == 1)
        return GetMainWord();

    if (m_Children.empty())
        ythrow yexception() << "No children";

    const CFactSynGroup* pG = dynamic_cast<const CFactSynGroup*>(m_Children[0]);
    if (pG)
        return pG->GetFirstWord();
    else
        return ((const CGroup*)m_Children[0])->GetMainWord();

}

const CWord* CFactSynGroup::GetLastWord() const
{
    if (Size() == 1)
        return GetMainWord();

    if (m_Children.empty())
        ythrow yexception() << "No children";

    const CFactSynGroup* pG = dynamic_cast<const CFactSynGroup*>(m_Children.back());
    if (pG)
        return pG->GetLastWord();
    else
        return ((const CGroup*)m_Children.back())->GetMainWord();

}

Wtroka    CFactSynGroup::ToString() const
{
    Wtroka Result;
    for (size_t i=0; i <m_Children.size(); i++) {
        CPrimitiveGroup* pPrimGroup = dynamic_cast<CPrimitiveGroup*>(m_Children[i]);
        if (pPrimGroup)
            Result += pPrimGroup->GetMainWord()->GetText();
        else {
            CFactSynGroup* pGroup = dynamic_cast<CFactSynGroup*>(m_Children[i]);
            if (pGroup)
                Result += pGroup->ToString();
        }
        Result += ' ';
    };
    return StripString(Result);
};
