#pragma once

#include <util/generic/vector.h>

#include "clauses.h"
#include "clauseheader.h"
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "periodtree.h"

class CClauseSubTree : public CPeriodSubTree
{
public:
    CClauseSubTree(CClause* pClause);
    CClauseSubTree();
    ~CClauseSubTree(void);
    CClauseSubTree* Clone();

    void EraseMainClause();
    void AddSubClauses(CClauseSubTree* pSubTree, CClause* pClause);
    CClause* GetMainClause()
    {
        CWordsPair* p = GetMainPeriodBase();
        if (!p)
            return NULL;
        return (CClause*)p;
    }

    CClause* GetIncludedClauseR()
    {
        CWordsPair* p = GetIncludedClauseRBase();
        if (!p)
            return NULL;
        return (CClause*)p;
    }

    //все клаузы, которые непосредственно включаются в главную
    void GetIncludedClauses(yvector<CClause*>& clauses);

    CClause* GetIncludedClauseL()
    {
        CWordsPair* p = GetIncludedClauseLBase();
        if (!p)
            return NULL;
        return (CClause*)p;
    }
protected:
    virtual CPeriodSubTree* CreateNew() { return new CClauseSubTree(); }

};
