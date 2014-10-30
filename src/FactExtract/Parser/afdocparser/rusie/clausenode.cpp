#include "clausenode.h"
#include "clausesubtree.h"

CClauseNode::CClauseNode(void)
{
    m_pClauseTree = NULL;
    m_VisitedCount = 0;
}

CClauseNode::CClauseNode(CClause* pCl)
{
    m_VisitedCount = 0;
    m_pClauseTree = new CClauseSubTree(pCl);
}

CClauseNode::~CClauseNode(void)
{
    if (m_pClauseTree)
        delete m_pClauseTree;
}

void CClauseNode::DeleteThisInOutRelation(bool bCheck)
{
    for (size_t i = 0; i < m_outNodes.size(); i++)
        m_outNodes[i].m_pNode->DeleteInRelationWith(this, bCheck);
}

void CClauseNode::DeleteThisInInRelation(bool bCheck)
{
    for (size_t i = 0; i < m_inNodes.size(); i++)
        m_inNodes[i].m_pNode->DeleteOutRelationWith(this, bCheck);
}

void CClauseNode::DeleteInRelationWith(CClauseNode* pNode, bool bCheck)
{
    int size = m_inNodes.size();
    int i;
    for (i = 0; i < size; i++)
        if (m_inNodes[i].m_pNode == pNode) {
            m_inNodes.erase(m_inNodes.begin() + i);
            break;
        }

    if (bCheck)
        if (i >= size)
            ythrow yexception() << "Bad graph in \" CClauseNode::DeleteInRelationWith\"";

}

void CClauseNode::DeleteOutRelationsExcept(CClauseNode* pNode)
{
    if (m_outNodes.size() == 0)
        ythrow yexception() << "Bad graph in \"CClauseNode::DeleteOutRelationsExcept\"";

    for (int i = m_outNodes.size() - 1; i >= 0; i--) {
        if (m_outNodes[i].m_pNode != pNode) {
            m_outNodes[i].m_pNode->DeleteInRelationWith(this);
            m_outNodes.erase(m_outNodes.begin() + i);
        }
    }
}

bool CClauseNode::GetShouldResetAllInRelationsRules()
{
    //return false; /***/
    if (IsEndNode())
        return false;
    CClause& clause = *GetMainClause();
    if (clause.m_ClauseWords.size() > 0) {
        SWordHomonymNum& wn = clause.m_ClauseWords[clause.m_ClauseWords.size()-1];
        const CWord& w= clause.m_Words.GetWord(wn);
        return (w.GetSourcePair().LastWord() != clause.LastWord());
    } else
        return true;
}

void CClauseNode::ResetInRelations()
{
    for (size_t i = 0; i < m_inNodes.size(); i++) {
        bool bResetAllRules = m_inNodes[i].m_pNode->GetShouldResetAllInRelationsRules();

        if (bResetAllRules)
            m_inNodes[i].ResetAllRules();
        else
            m_inNodes[i].ResetRules();

        for (size_t j = 0; j < m_inNodes[i].m_pNode->m_outNodes.size(); j++)
            if (m_inNodes[i].m_pNode->m_outNodes[j].m_pNode == this) {
                if (bResetAllRules)
                    m_inNodes[i].m_pNode->m_outNodes[j].ResetAllRules();
                else
                    m_inNodes[i].m_pNode->m_outNodes[j].ResetRules();
            }
    }
}

bool CClauseNode::GetShouldResetAllOutRelationsRules()
{
    //return false; /***/
    if (IsEndNode())
        return false;
    CClause& clause = *GetMainClause();
    if (clause.m_ClauseWords.size() > 0) {
        SWordHomonymNum& wn = clause.m_ClauseWords[0];
        const CWord& w = clause.m_Words.GetWord(wn);
        return (w.GetSourcePair().FirstWord() != clause.FirstWord());
    } else
        return true;
}

void CClauseNode::ResetOutRelations()
{
    for (size_t i = 0; i < m_outNodes.size(); i++) {
        bool bResetAllRules = m_outNodes[i].m_pNode->GetShouldResetAllOutRelationsRules();

        if (bResetAllRules)
            m_outNodes[i].ResetAllRules();
        else
            m_outNodes[i].ResetRules();

        for (size_t j = 0; j < m_outNodes[i].m_pNode->m_inNodes.size(); j++)
            if (m_outNodes[i].m_pNode->m_inNodes[j].m_pNode == this) {
                if (bResetAllRules)
                    m_outNodes[i].m_pNode->m_inNodes[j].ResetAllRules();
                else
                    m_outNodes[i].m_pNode->m_inNodes[j].ResetRules();
            }
    }
}

void CClauseNode::DeleteInRelationsExcept(CClauseNode* pNode)
{
    if (m_inNodes.size() == 0)
        ythrow yexception() << "Bad graph in \"CClauseNode::DeleteInRelationsExcept\"";

    for (int i = m_inNodes.size() - 1; i >= 0; i--) {
        if (m_inNodes[i].m_pNode != pNode) {
            m_inNodes[i].m_pNode->DeleteOutRelationWith(this);
            m_inNodes.erase(m_inNodes.begin() + i);
        }
    }

}

void CClauseNode::DeleteOutRelationWith(CClauseNode* pNode, bool bCheck)
{
    int size = m_outNodes.size();
    int i;
    for (i = 0; i < size; i++)
        if (m_outNodes[i].m_pNode == pNode) {
            m_outNodes.erase(m_outNodes.begin() + i);
            break;
        }

    if (bCheck)
        if (i >= size)
            ythrow yexception() << "Bad graph in \" CClauseNode::DeleteOutRelationWith\"";
}

void CClauseNode::DeleteRelationWith(CClauseNode* pNode, bool bCheck)
{
    DeleteOutRelationWith(pNode, bCheck);
    pNode->DeleteInRelationWith(this, bCheck);
}

CClauseNode* CClauseNode::Clone()
{
    CClauseNode* pNewNode = new CClauseNode();
    pNewNode->m_pClauseTree = m_pClauseTree->Clone();

    pNewNode->m_inNodes = m_inNodes;
    for (size_t i = 0; i <  m_inNodes.size(); i++) {
        CClauseNode* pNode;
        size_t j;
        for (j = 0; j < m_inNodes[i].m_pNode->m_outNodes.size(); j++) {
             pNode = m_inNodes[i].m_pNode->m_outNodes[j].m_pNode;
            if (pNode == this)
                break;
        }
        if (j >= m_inNodes[i].m_pNode->m_outNodes.size())
            ythrow yexception() << "Error in clause graph!";

        SClauseRelation& rel = m_inNodes[i].m_pNode->m_outNodes[j];
        m_inNodes[i].m_pNode->m_outNodes.push_back(SClauseRelation(pNewNode, rel.m_appliedRules, rel.m_prohibitedRules, rel.m_prohibitedRulesAlways));
    }

    pNewNode->m_outNodes = m_outNodes;
    for (size_t i = 0; i <  m_outNodes.size(); i++) {
        CClauseNode* pNode;
        size_t j;
        for (j = 0; j < m_outNodes[i].m_pNode->m_inNodes.size(); j++) {
            pNode = m_outNodes[i].m_pNode->m_inNodes[j].m_pNode;
            if (pNode == this)
                break;
        }
        if (j >= m_outNodes[i].m_pNode->m_inNodes.size())
            ythrow yexception() << "Error in clause graph!";
        SClauseRelation& rel = m_outNodes[i].m_pNode->m_inNodes[j];
        m_outNodes[i].m_pNode->m_inNodes.push_back(SClauseRelation(pNewNode, rel.m_appliedRules, rel.m_prohibitedRules, rel.m_prohibitedRulesAlways));
    }

    return pNewNode;
}

