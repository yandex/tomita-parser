#include "groupsubtree.h"

CGroupSubTree::CGroupSubTree(void)
{
}

CGroupSubTree::~CGroupSubTree(void)
{
}

CGroupSubTree::CGroupSubTree(CGroup* pGroup) : CPeriodSubTree(pGroup)
{

}

void CGroupSubTree::SetMainGroups(CGroupSubTree* pClonedTree)
{
    period_iter_rev it = m_Periods.rbegin();
    period_iter_rev cloned_it = pClonedTree->m_Periods.rbegin();

    for (; it != m_Periods.rend(); it++ , cloned_it++) {
        CGroup* pGroup = (CGroup*)(*it);
        if (pGroup->IsPrimitive())
            continue;
        const CGroup* pMainGroup = pGroup->GetMainGroup();
        period_iter_rev cloned_it2 = cloned_it;
        cloned_it2++;
        for (; cloned_it2 != pClonedTree->m_Periods.rend(); cloned_it2++) {
            if ((**cloned_it2) == *pMainGroup)
                break;
        }
        if (cloned_it2 == pClonedTree->m_Periods.rend())
            ythrow yexception() << "Error in \"CGroupSubTree::SetMainGroups\", can't find main group.";
        ((CGroup*)(*cloned_it))->SetMainGroup((CGroup*)*cloned_it2);
    }
}

CGroupSubTree* CGroupSubTree::Clone()
{
    CGroupSubTree* pConedTree = (CGroupSubTree*)CloneBase();
    SetMainGroups(pConedTree);
    return pConedTree;

}

void CGroupSubTree::AddSubGroups(CGroupSubTree* pSubTree, CGroup* pClause)
{
    AddSubPeriods(pSubTree, pClause);
}
void CGroupSubTree::AddGroup(CGroup* pClause)
{
    AddPeriod(pClause);
}
