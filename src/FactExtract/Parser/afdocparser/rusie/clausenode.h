#pragma once

#include <util/generic/vector.h>

#include "clausesubtree.h"

class CClauseNode;

const i64 one_i64 = 1;

struct SClauseRelation
{
    SClauseRelation(CClauseNode* pNode):
        m_pNode(pNode)
    {
        m_appliedRules = 0;
        m_prohibitedRulesAlways = 0;
        m_prohibitedRules = 0;
        m_bNodeDeleted = false;
    }

    SClauseRelation(CClauseNode* pNode, i64 appliedRules, i64  prohibitedRules , i64 prohibitedRulesAlways)
    :    m_appliedRules(appliedRules),
        m_prohibitedRulesAlways(prohibitedRulesAlways),
        m_prohibitedRules(prohibitedRules),
        m_pNode(pNode),
        m_bNodeDeleted(false)
    {

    }

    bool ShouldApplyRule(ERuleType rule)
    {
        return (!(m_appliedRules & (one_i64 << rule)) &&
                 !(m_prohibitedRulesAlways & (one_i64 << rule)) &&
                 !(m_prohibitedRules & (one_i64 << rule)));
    }

    void SetChouldNotdApplyRule(ERuleType rule)
    {
        m_appliedRules |= (one_i64 << rule);
    }

    void SetProhibitedRuleAlways(ERuleType rule)
    {
        m_prohibitedRulesAlways |= (one_i64 << rule);
    }

    void SetProhibitedRule(ERuleType rule)
    {
        m_prohibitedRules |= (one_i64 << rule);
    }

    void ResetRules()
    {
        m_prohibitedRules = 0;
        m_appliedRules = 0;
    }

    void ResetAllRules()
    {
        m_prohibitedRules = 0;
        m_appliedRules = 0;
        m_prohibitedRulesAlways = 0;
    }

    i64 m_appliedRules;
    i64 m_prohibitedRulesAlways;
    i64 m_prohibitedRules;
    CClauseNode* m_pNode;
    bool m_bNodeDeleted;
};

class CClauseNode
{
public:
    CClauseNode(void);
    virtual ~CClauseNode(void);
    CClauseNode* Clone();
    int GetNeighbourNodesCount(EDirection dir)
    {
        if (dir == LeftToRight)
            return m_outNodes.size();
        else
            return m_inNodes.size();
    }

    CClause* GetIncludedClauseR()
    {
        return m_pClauseTree->GetIncludedClauseR();
    }

    CClause* GetIncludedClauseL()
    {
        return m_pClauseTree->GetIncludedClauseL();
    }

    SClauseRelation& GetNeighbourNode(EDirection dir, int i)
    {
        if (dir == LeftToRight) {
            return m_outNodes[i];
        } else {
            return m_inNodes[i];
        }
    }

    CClause* GetMainClause()
    {
        if (m_pClauseTree == NULL)
            return NULL;
        return m_pClauseTree->GetMainClause();
    }

    bool IsEndNode()
    {
        return m_pClauseTree == NULL;
    }

    CClauseNode(CClause* pCl);

    void Free()
    {
        if (m_pClauseTree) {
            m_pClauseTree->Free();
            delete m_pClauseTree;
            m_pClauseTree = NULL;
        }
    }

    void DeleteOutRelationWith(CClauseNode* pNode, bool bCheck = true);
    void DeleteInRelationWith(CClauseNode* pNode,  bool bCheck = true);
    void DeleteThisInOutRelation(bool bCheck = true);
    void DeleteThisInInRelation(bool bCheck = true);
    void DeleteRelationWith(CClauseNode* pNode, bool bCheck = true);
    void DeleteOutRelationsExcept(CClauseNode* pNode);
    void DeleteInRelationsExcept(CClauseNode* pNode);
    bool GetShouldResetAllOutRelationsRules();
    bool GetShouldResetAllInRelationsRules();
    void ResetInRelations();
    void ResetOutRelations();

    int    m_VisitedCount;

//protected:
    CClauseSubTree*    m_pClauseTree;
    yvector<SClauseRelation> m_outNodes;
    yvector<SClauseRelation> m_inNodes;
};
