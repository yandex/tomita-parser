#include "periodtree.h"

CPeriodSubTree::CPeriodSubTree(CWordsPair* pPeriod)
{
    m_Periods.insert(pPeriod);
}

CPeriodSubTree::CPeriodSubTree()
{

}

CPeriodSubTree::~CPeriodSubTree(void)
{
}

void CPeriodSubTree::EraseMainPeriod()
{
    period_iter it = --m_Periods.end();
    m_Periods.erase(it);
}

void CPeriodSubTree::Free()
{
    period_iter it = m_Periods.begin();

    for (; it != m_Periods.end(); it++) {
        CWordsPair* pCl = *it;
        delete pCl;
    }
}

CPeriodSubTree* CPeriodSubTree::CloneBase()
{
    CPeriodSubTree* pNewSubTree = CreateNew();
    pNewSubTree->SetPair(FirstWord(), LastWord());
    period_iter it = m_Periods.begin();

    for (; it != m_Periods.end(); it++) {
        CWordsPair* pCl = *it;
        pNewSubTree->m_Periods.insert(pCl->CloneAsWordsPair());
    }

    return pNewSubTree;
}

void CPeriodSubTree::AddPeriod(CWordsPair* pPeriod)
{
    m_Periods.insert(pPeriod);
    if (!Includes(*pPeriod))
        SetPair(*pPeriod);
}

void CPeriodSubTree::AddSubPeriods(CPeriodSubTree* pSubTree, CWordsPair* pPeriod)
{
    if (pSubTree->m_Periods.size() > 0) {
        yset<CWordsPair*, SPeriodOrder>::iterator it = pSubTree->m_Periods.begin();
        for (; it != pSubTree->m_Periods.end(); it++)
            m_Periods.insert(*it);
    }

    m_Periods.insert(pPeriod);
    SetPair(*GetMainPeriodBase());
}

void CPeriodSubTree::GetIncludedPeriodsBase(yvector<CWordsPair*>& periods)
{
    if (m_Periods.size() == 1)
        return;
    period_iter main = m_Periods.end();
    --main;
    int rBorder = (**main).LastWord() + 1;
    period_iter it = main;
    do
    {
        it--;
        CWordsPair& wp = **it;
        if (wp.LastWord() < rBorder) {
            periods.push_back(*it);
            rBorder = wp.FirstWord();
        }

    } while (it != m_Periods.begin());
}
