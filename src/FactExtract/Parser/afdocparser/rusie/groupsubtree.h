#pragma once
#include "periodtree.h"
#include "group.h"

class CGroupSubTree :
    public CPeriodSubTree
{
public:
    CGroupSubTree(void);
    CGroupSubTree(CGroup* pGroup);
    virtual ~CGroupSubTree(void);
    CGroupSubTree* Clone();

    void EraseMainClause();
    void AddSubGroups(CGroupSubTree* pSubTree, CGroup* pClause);
    void AddGroup(CGroup* pClause);
    CGroup* GetMainGroup()
    {
        CWordsPair* p = GetMainPeriodBase();
        if (!p)
            return NULL;
        return (CGroup*)p;
    }

    CGroup* GetIncludedGroupR()
    {
        CWordsPair* p = GetIncludedClauseRBase();
        if (!p)
            return NULL;
        return (CGroup*)p;
    }

    CGroup* GetIncludedGroupL()
    {
        CWordsPair* p = GetIncludedClauseLBase();
        if (!p)
            return NULL;
        return (CGroup*)p;
    }

protected:
    void SetMainGroups(CGroupSubTree* pClonedTree);
    virtual CPeriodSubTree* CreateNew() { return new CGroupSubTree(); }

};
