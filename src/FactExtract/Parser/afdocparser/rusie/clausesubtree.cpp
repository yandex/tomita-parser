#include "clausesubtree.h"

CClauseSubTree::CClauseSubTree(CClause* pClause) : CPeriodSubTree(pClause)
{
}

CClauseSubTree::CClauseSubTree()
{

}

CClauseSubTree::~CClauseSubTree(void)
{
}

void CClauseSubTree::EraseMainClause()
{
    EraseMainPeriod();
}

//void CClauseSubTree::Free()
//{
//    clause_iter it = m_Clauses.begin();

//    for( ; it != m_Clauses.end() ; it++ )
//    {
//        CClause* pCl = *it;
//        delete pCl;
//    }
//}

CClauseSubTree* CClauseSubTree::Clone()
{
    return (CClauseSubTree*)CloneBase();
}

//все клаузы, которые непосредственно включаются в главную
void CClauseSubTree::GetIncludedClauses(yvector<CClause*>& clauses)
{
    yvector<CWordsPair*> periods;
    GetIncludedPeriodsBase(periods);
    for (size_t i = 0; i < periods.size(); i++)
        clauses.push_back((CClause*)periods[i]);
}

void CClauseSubTree::AddSubClauses(CClauseSubTree* pSubTree, CClause* pClause)
{
    AddSubPeriods(pSubTree, pClause);
}
