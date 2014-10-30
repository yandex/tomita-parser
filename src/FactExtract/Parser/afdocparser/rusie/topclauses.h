#pragma once

#include <util/generic/vector.h>

#include "clausenode.h"
#include "clausevariant.h"

enum ESetOfRulesType
{
    UniteBracketsRules,
    IncludeBracketsRules,
    UniteRules,
    ConjSubRules,
    IncludeRules,
    ConjRules,
    SetOfRulesCount
};

struct SSetOfRules
{
    int       m_Count;
    ERuleType m_Rules[20];
};

class CTopClauses;

typedef void (CTopClauses::*FClauseOperation)(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg);

class CTopClauses
{

    friend class CClauseRules;
public:
    CTopClauses(CWordVector& words);
    virtual ~CTopClauses(void);
    void RunRules(bool bCanClone);
    void AddClauseVariants(yvector<CClauseVariant>& ClauseVars);
    void Free();
    bool GetNeighbourClauseL(CClauseNode* node, CClauseNode*& next_node);
    bool GetNeighbourClauseR(CClauseNode* node, CClauseNode*& next_node);
    CClause* GetIncludedClauseR(CClauseNode* node);
    bool HasIncludedClauseR(CClauseNode* node);
    CClause* GetIncludedClauseL(CClauseNode* node);
    bool HasIncludedClauseL(CClauseNode* node);
    void BuildClauseVariants(yvector<CClauseVariant>& ClauseVars);
    CClause& GetClauseRef(CClauseNode* node)
    {
        return *node->GetMainClause();
    }

protected:
    CClauseNode* AddInitialClause(CClause* pClause, CClauseNode* pLastNode, yvector<CClause*>& deleted_clause);
    CClauseNode* FindEqualNode(CClauseNode* pNode, CClause* pClause);

    void BuildClauseVariantsRec(yvector<CClauseVariant>& ClauseVars, CClauseNode* node);

    void RunSynAn(CClauseNode* pClauseNode);
    bool RunRules(CClauseNode* pCurNode, ESetOfRulesType setOfRulesType, EDirection direction, bool bCanClone, CClauseRules& ClauseRules);
    bool ApplyRule(ERuleType rule, ESetOfRulesType setOfRulesType, CClauseNode* pNode1, CClauseNode* pNode2, bool bCanClone, CClauseRules& ClauseRules);
    bool RunRules(ESetOfRulesType setOfRulesType, bool bBreakAfterFirstRule, EDirection direction, bool bCanClone, CClauseRules& ClauseRules);

    void UniteClause(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg);
    void NewClause(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg);
    void DelTypes(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg);
    void IncludeClause(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg);

    void AddValsToSubClauses(CClauseNode* pNode, SClauseFuncArg& arg);

    void ApplyClauseOp(CClauseNode* pNode1, CClauseNode* pNode2, SClauseRuleResult& res, bool bCanClone);

    void UpdateNodes(CClauseNode* pNode1, CClauseNode* pNode2, CClauseNode* pClonedNode1, CClauseNode* pClonedNode2,  bool bMainFirst);
    void CloneVarByRuleResult(CClauseNode* pNode1, CClauseNode* pNode2, SClauseRuleResult& res, ERuleType eRule, ESetOfRulesType setOfRulesType, CClauseRules& ClauseRules);
    bool UpdateInitialClauses(CClauseNode* pNode);
    void FreeNodes(CClauseNode* m_LastNode);

    int ResetAllVisited(CClauseNode* pNode);

    void AddClausesToClauseVar(CClauseVariant& var,  CClauseSubTree* pTree);

protected:
    CClauseNode* m_FirstNode;
    CClauseNode* m_LastNode;
    CWordVector& m_Words;

    static FClauseOperation s_ClauseOp[ClauseOperationCount];
    static SSetOfRules      m_SetsOfRules[SetOfRulesCount];
};
