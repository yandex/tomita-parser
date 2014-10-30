#include "topclauses.h"
#include "clauserules.h"

FClauseOperation    CTopClauses::s_ClauseOp[ClauseOperationCount] =
                    {    &CTopClauses::UniteClause,
                        &CTopClauses::IncludeClause,
                        &CTopClauses::NewClause,
                        &CTopClauses::DelTypes
                    };

SSetOfRules CTopClauses::m_SetsOfRules[SetOfRulesCount] =
{
    { 1,
        {
            UniteClauseWithBrackets
        }
    },

    { 3,
        {
            IncludeBrackets,
            IncludeDash,
            IncludeParenthesis
        }
    },

    { 12,
        {
            UniteSubConjEmptyNeedEnd,
            UnitePredicWithSubj,
            UniteSubjWithPredic,
            UniteInfWithPredic,
            UnitePredicWithInf,
            UniteSubConjWithPredic,
            UnitePredicInfWithInf,
            UniteInfWithPredicInf,
            UniteDashClauseWithPredicEllipsis,
            UniteEmptyWithDashEmpty,
            UniteEmptyToSochConj,
            UniteSochConjToEmpty
        }
    },

    { 5,
        {
            ConjSubInfs,
            ConjSubPredics,
            ConjSimParticiples,
            ConjSimVerbAdverb,
            ConjSubSimNodes
        }
    },

    { 11,
        {
            IncludeParenthesis,
            IncludeKotoryj,
            IncludeParticiple,
            IncludeSubConjEmpty,
            IncludeVerbAdverbPredic,
            IncludePredicVerbAdverb,
            IncludeSubConjCorr,
            IncludeAntSubConj,
            IncludePredicSubConj,
            IncludeSubConjPredic,
            IncludeColon
        }
    },

    { 1,
        {
            ConjSimNodes
        }
    }
};

CTopClauses::CTopClauses(CWordVector& words)
    : m_Words(words)
{
    m_FirstNode = new CClauseNode();
    m_LastNode = new CClauseNode();
    m_FirstNode->m_outNodes.push_back(SClauseRelation(m_LastNode));
    m_LastNode->m_inNodes.push_back(SClauseRelation(m_FirstNode));
}

void CTopClauses::Free()
{
    if (m_FirstNode != NULL) {
        FreeNodes(m_FirstNode);
        m_FirstNode = NULL;
    }
}

void CTopClauses::FreeNodes(CClauseNode* node)
{

    for (size_t i = 0; i < node->m_outNodes.size(); i++) {
        if (!node->m_outNodes[i].m_bNodeDeleted)
            FreeNodes(node->m_outNodes[i].m_pNode);
    }
    for (size_t i = 0; i < node->m_inNodes.size(); i++) {
        for (size_t j = 0; j < node->m_inNodes[i].m_pNode->m_outNodes.size(); j++) {
            if (node->m_inNodes[i].m_pNode->m_outNodes[j].m_pNode == node)
                node->m_inNodes[i].m_pNode->m_outNodes[j].m_bNodeDeleted = true;
        }
    }
    node->Free();
    delete node;
}

CTopClauses::~CTopClauses(void)
{
}

void CTopClauses::AddClausesToClauseVar(CClauseVariant& var, CClauseSubTree* pTree)
{
    CPeriodSubTree::period_iter it = pTree->GetPeriodIterBegin();
    for (; it != pTree->GetPeriodIterEnd(); it++)
        var.AddClause((CClause*)*it);

}

void CTopClauses::BuildClauseVariantsRec(yvector<CClauseVariant>& ClauseVars, CClauseNode* node)
{
    yvector<CClauseVariant> saveVars;
    if (node->m_outNodes.size() > 1)
        saveVars = ClauseVars;
    for (size_t i = 0; i < node->m_outNodes.size(); i++) {
        CClauseNode* node1 = node->m_outNodes[i].m_pNode;
        if (node1->IsEndNode())
            return;
        if (i == 0) {
            for (size_t j = 0; j < ClauseVars.size(); j++)
                AddClausesToClauseVar(ClauseVars[j], node1->m_pClauseTree);
                //ClauseVars[j].AddClauses(node1->m_pClauseTree->m_Clauses);
            BuildClauseVariantsRec(ClauseVars, node1);
        } else {
            yvector<CClauseVariant> saveVarsCopy = saveVars;
            for (size_t j = 0; j < saveVarsCopy.size(); j++)
                AddClausesToClauseVar(saveVarsCopy[j], node1->m_pClauseTree);
                //saveVarsCopy[j].AddClauses(node1->m_pClauseTree->m_Clauses);
            BuildClauseVariantsRec(saveVarsCopy, node1);
            ClauseVars.insert(ClauseVars.end(), saveVarsCopy.begin(), saveVarsCopy.end());
        }
    }
}

void CTopClauses::BuildClauseVariants(yvector<CClauseVariant>& ClauseVars)
{
    CClauseVariant ClauseVar(m_Words);
    ClauseVars.push_back(ClauseVar);
    BuildClauseVariantsRec(ClauseVars, m_FirstNode);
    for (size_t i = 0; i < ClauseVars.size(); i++)
        ClauseVars[i].CalculateBadWeight();
}

void CTopClauses::AddClauseVariants(yvector<CClauseVariant>& ClauseVars)
{
    m_FirstNode->m_outNodes.clear();
    m_FirstNode->m_inNodes.clear();

    m_LastNode->m_outNodes.clear();
    m_LastNode->m_inNodes.clear();

    yvector<CClause*> deleted_clause;
    for (size_t i = 0; i < ClauseVars.size(); i++) {
        const_clause_it it;
        CClauseNode* pLastNode = m_FirstNode;
        for (it = ClauseVars[i].GetFirstClause(LeftToRight); it != ClauseVars[i].GetEndClause(); it = ClauseVars[i].GetNextClause(LeftToRight, it))
            pLastNode = AddInitialClause(*it, pLastNode, deleted_clause);
        bool bAdd = true;
        for (size_t j = 0; j < pLastNode->m_outNodes.size(); j++)
            if (pLastNode->m_outNodes[j].m_pNode == m_LastNode) {
                bAdd = false;
                break;
            }
        if (bAdd) {
            pLastNode->m_outNodes.push_back(SClauseRelation(m_LastNode));
            m_LastNode->m_inNodes.push_back(SClauseRelation(pLastNode));
        }
    }
    for (size_t j = 0; j < deleted_clause.size(); j++)
        delete deleted_clause[j];/**/
}

CClauseNode* CTopClauses::FindEqualNode(CClauseNode* pNode, CClause* pClause)
{
    for (size_t i = 0; i < pNode->m_outNodes.size(); i++) {
        if (pNode->m_outNodes[i].m_pNode->IsEndNode())
            return NULL;
        if (pNode->m_outNodes[i].m_pNode->GetMainClause()->EqualByConj(*pClause))
            return pNode->m_outNodes[i].m_pNode;

        CClauseNode* pRet = FindEqualNode(pNode->m_outNodes[i].m_pNode, pClause);
        if (pRet)
            return pRet;
    }

    return NULL;
}

CClauseNode* CTopClauses::AddInitialClause(CClause* pClause, CClauseNode* pLastNode, yvector<CClause*>& deleted_clause)
{

    CClauseNode* pRetNode = NULL;
    pRetNode = FindEqualNode(m_FirstNode, pClause);
    bool bAdd = true;
    if (pRetNode == NULL) {
        CClauseNode* pNewNode = new CClauseNode(pClause);
        pRetNode = pNewNode;
    } else {
        deleted_clause.push_back(pClause);
        for (size_t i = 0; i < pLastNode->m_outNodes.size(); i++)
            if (pLastNode->m_outNodes[i].m_pNode == pRetNode) {
                bAdd = false;
                break;
            }
    }
    if (bAdd) {
        pLastNode->m_outNodes.push_back(SClauseRelation(pRetNode));
        pRetNode->m_inNodes.push_back(SClauseRelation(pLastNode));
    }

    return pRetNode;
}

void CTopClauses::RunSynAn(CClauseNode* pClauseNode)
{
    int count = pClauseNode->GetNeighbourNodesCount(LeftToRight);
    if (!pClauseNode->IsEndNode())
        pClauseNode->GetMainClause()->RunSynAn();
    for (int j = 0; j < count; j++) {
        SClauseRelation& rel = pClauseNode->GetNeighbourNode(LeftToRight, j);
        if (rel.m_pNode->IsEndNode())
            return;
        RunSynAn(rel.m_pNode);
    }
}

const int g_iMaxVars =5;

void CTopClauses::RunRules(bool bCanClone)
{
    CClauseRules ClauseRules(*this);

    RunRules(UniteBracketsRules, false, LeftToRight, false, ClauseRules);
    RunRules(IncludeBracketsRules, false, LeftToRight, false, ClauseRules);

    while (true) {
        bool bUniteRules = RunRules(UniteRules, false, LeftToRight, bCanClone, ClauseRules);

        bool bSubConjRules = RunRules(ConjSubRules, true, LeftToRight, bCanClone, ClauseRules);
        if (bSubConjRules) {
            bCanClone = (ResetAllVisited(m_FirstNode) <= g_iMaxVars);
            continue;
        }

        bool bIncludeRules = RunRules(IncludeRules, true, RightToLeft, bCanClone, ClauseRules);
        if (bIncludeRules) {
            bCanClone = (ResetAllVisited(m_FirstNode) <= g_iMaxVars);
            continue;
        }

        bool bConjRules = RunRules(ConjRules, true, RightToLeft, bCanClone, ClauseRules);
        if (bConjRules) {
            bCanClone = (ResetAllVisited(m_FirstNode) <= g_iMaxVars);
            continue;
        }

        if (!bIncludeRules && !bUniteRules && !bConjRules && !bSubConjRules) {
            break;
        }
    }
    //int vars = ResetAllVisited(m_FirstNode);
}

int CTopClauses::ResetAllVisited(CClauseNode* pNode)
{
    int count = pNode->GetNeighbourNodesCount(LeftToRight);
    int varCount = 0;
    for (int j = 0; j < count; j++) {
        pNode->m_VisitedCount = 0;
        varCount += ResetAllVisited(pNode->GetNeighbourNode(LeftToRight, j).m_pNode);
    }
    if (count == 0)
        varCount = 1;
    return varCount;
}

bool CTopClauses::RunRules(ESetOfRulesType setOfRulesType, bool bBreakAfterFirstRule, EDirection direction,
                           bool bCanClone, CClauseRules& ClauseRules)
{
    CClauseNode* pFirstNode = (direction == LeftToRight) ? m_FirstNode : m_LastNode;

    bool bAppliedAtLeastOnce = false;
    while (true) {
        if (bAppliedAtLeastOnce)
            bCanClone = (ResetAllVisited(m_FirstNode) <= 100);
        int count = pFirstNode->GetNeighbourNodesCount(direction);
        int j;
        for (j = 0; j < count; j++) {
            SClauseRelation& rel = pFirstNode->GetNeighbourNode(direction, j);
            if (rel.m_pNode->IsEndNode())
                return false;
            if (RunRules(rel.m_pNode, setOfRulesType, direction, bCanClone, ClauseRules)) {
                bAppliedAtLeastOnce = true;
                if (bBreakAfterFirstRule)
                    return true;
                else
                    break;
            }
        }

        if (j >= count)
            break;
    }

    return bAppliedAtLeastOnce;
}

bool CTopClauses::RunRules(CClauseNode* pCurNode, ESetOfRulesType setOfRulesType, EDirection direction,
                           bool bCanClone, CClauseRules& ClauseRules)
{
    int count = pCurNode->GetNeighbourNodesCount(direction);
    EDirection rev_direction = (direction == LeftToRight) ? RightToLeft : LeftToRight;
    for (int i = 0; i < count; i++) {
        SClauseRelation& rel = pCurNode->GetNeighbourNode(direction, i);
        if (rel.m_pNode->IsEndNode())
            return false;

        CClauseNode* pNode1 = (direction == LeftToRight) ? pCurNode : rel.m_pNode;
        CClauseNode* pNode2 = (direction == LeftToRight) ? rel.m_pNode : pCurNode;

        for (int i = 0; i <  m_SetsOfRules[(int)setOfRulesType].m_Count; i++) {
            ERuleType rule = m_SetsOfRules[setOfRulesType].m_Rules[i];
            if (rel.ShouldApplyRule(rule)) {
                if (ApplyRule(rule, setOfRulesType, pNode1, pNode2,  bCanClone, ClauseRules))
                    return true;
                else
                    rel.SetChouldNotdApplyRule(rule);
            }
        }

        int rev_count = rel.m_pNode->GetNeighbourNodesCount(rev_direction);
        rel.m_pNode->m_VisitedCount++;
        if (rev_count == rel.m_pNode->m_VisitedCount) {
            rel.m_pNode->m_VisitedCount = 0;
            if (RunRules(rel.m_pNode, setOfRulesType, direction, bCanClone, ClauseRules))
                return true;
        }
    }

    return false;
}

bool CTopClauses::ApplyRule(ERuleType rule, ESetOfRulesType setOfRulesType, CClauseNode* pNode1, CClauseNode* pNode2,
                            bool bCanClone, CClauseRules& ClauseRules)
{
    //try
    {
        SClauseRuleResult res;
        if (!ClauseRules.ApplyRule(rule, pNode1, pNode2, res))
            return false;

        if (res.m_bClone && bCanClone)
            CloneVarByRuleResult(pNode1, pNode2,  res, rule, setOfRulesType, ClauseRules);

        ApplyClauseOp(pNode1, pNode2, res, bCanClone);
        return true;
    }
    /*catch(yexception& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CTopClauses::ApplyRule - (\"%s\")", e.what());
        Stroka words;
        m_Words.SentToString(words);
        strMsg += Stroka("\nWords:\n") + words;
        ythrow yexception() << (const char*)strMsg;
    }
    catch(...)
    {
        ythrow yexception() << "Error in \"CTopClauses::ApplyRule\"";
    }*/
}

void CTopClauses::CloneVarByRuleResult(CClauseNode* pNode1, CClauseNode* pNode2, SClauseRuleResult& res,
                                       ERuleType eRule, ESetOfRulesType setOfRulesType, CClauseRules& ClauseRules)
{

    CClauseNode* pClonedNode1 = pNode1->Clone();
    CClauseNode* pClonedNode2 = pNode2->Clone();

    pClonedNode1->DeleteOutRelationsExcept(pClonedNode2);
    pClonedNode2->DeleteInRelationsExcept(pClonedNode1);

    SClauseRelation *inRel = NULL, *outRel = NULL;

    for (size_t i = 0; i < pClonedNode1->m_outNodes.size(); i++) {
        if (pClonedNode1->m_outNodes[i].m_pNode == pClonedNode2)
            outRel = &pClonedNode1->m_outNodes[i];
    }

    for (size_t i = 0; i < pClonedNode2->m_inNodes.size(); i++) {
        if (pClonedNode2->m_inNodes[i].m_pNode == pClonedNode1)
            inRel = &pClonedNode2->m_inNodes[i];

    }

    if (!outRel || !inRel)
        ythrow yexception() << "Bad clause variant!";

    if (ClauseRules.GetRuleStopOptions(eRule) == CheckJunction) {
        for (int i = 0; i < m_SetsOfRules[setOfRulesType].m_Count; i++) {
            outRel->SetProhibitedRuleAlways(m_SetsOfRules[setOfRulesType].m_Rules[i]);
            inRel->SetProhibitedRuleAlways(m_SetsOfRules[setOfRulesType].m_Rules[i]);
        }
        for (size_t j = 0; j < res.m_StopRules.size(); j++) {
            outRel->SetProhibitedRuleAlways(res.m_StopRules[j]);
            inRel->SetProhibitedRuleAlways(res.m_StopRules[j]);
        }
    } else {
        outRel->SetProhibitedRule(eRule);
        inRel->SetProhibitedRule(eRule);

    }

    CClause* pCl1 = pClonedNode1->GetMainClause();
    if ((res.m_ClauseFuncArg.m_iClause1TypeNum != -1) && (pCl1->m_Types.size() > 1)) {
        pCl1->DelTypesExcept(res.m_ClauseFuncArg.m_iClause1TypeNum);
    }

    CClause* pCl2 = pClonedNode2->GetMainClause();
    if ((res.m_ClauseFuncArg.m_iClause2TypeNum != -1) && (pCl2->m_Types.size() > 1)) {
        pCl2->DelTypesExcept(res.m_ClauseFuncArg.m_iClause2TypeNum);
    }
}

void CTopClauses::ApplyClauseOp(CClauseNode* pNode1, CClauseNode* pNode2, SClauseRuleResult& res, bool bCanClone)
{
    //try
    {
        CClause* pCl1 = pNode1->GetMainClause();
        CClause* pCl2 = pNode2->GetMainClause();

        bool bClause1HasOtherTypes = ((pCl1->m_Types.size() > 1) && (res.m_ClauseFuncArg.m_iClause1TypeNum != -1));
        bool bClause2HasOtherTypes = ((pCl2->m_Types.size() > 1) && (res.m_ClauseFuncArg.m_iClause2TypeNum != -1));

        if ((bClause1HasOtherTypes    ||     bClause2HasOtherTypes)
            &&    bCanClone) {
            if (bClause1HasOtherTypes) {
                if (res.m_ClauseFuncArg.m_bCloneByType1) {
                    CClauseNode* clauseNode = pNode1->Clone();
                    clauseNode->GetMainClause()->DeleteType(res.m_ClauseFuncArg.m_iClause1TypeNum);
                }
                pCl1->DelTypesExcept(res.m_ClauseFuncArg.m_iClause1TypeNum);
                res.m_ClauseFuncArg.m_iClause1TypeNum = 0; //оставили только нужный тип

            }

            if (bClause2HasOtherTypes) {
                if (res.m_ClauseFuncArg.m_bCloneByType2) {
                    CClauseNode* clauseNode = pNode2->Clone();
                    clauseNode->GetMainClause()->DeleteType(res.m_ClauseFuncArg.m_iClause2TypeNum);
                }
                pCl2->DelTypesExcept(res.m_ClauseFuncArg.m_iClause2TypeNum);
                res.m_ClauseFuncArg.m_iClause2TypeNum = 0; //оставили только нужный тип
            }
        }

        ((*this).*(s_ClauseOp[res.m_Operation]))(pNode1, pNode2, res.m_ClauseFuncArg);
    }
    /*catch(yexception& e)
    {
        throw e;
    }
    catch(...)
    {
        ythrow yexception() << "Error in \"CTopClauses::ApplyClauseOp\"";
    }*/
}

void CTopClauses::UpdateNodes(CClauseNode* pNode1, CClauseNode* pNode2,
                              CClauseNode* pClonedNode1, CClauseNode* pClonedNode2, bool bMainFirst)
{
    if (pClonedNode1 && pClonedNode2)
        pClonedNode1->DeleteRelationWith(pClonedNode2);

    if (pClonedNode1) {
        pClonedNode1->DeleteRelationWith(pNode2);
        pNode1->DeleteOutRelationsExcept(pNode2);
    }

    if (pClonedNode2) {
        pNode1->DeleteRelationWith(pClonedNode2, (pClonedNode1 == NULL));
        pNode2->DeleteInRelationsExcept(pNode1);
    }

    if (bMainFirst) {
        pNode1->m_outNodes = pNode2->m_outNodes;
        bool bResetAllRules = pNode1->GetShouldResetAllInRelationsRules();//rel.m_pNode->GetShouldResetAllOutRelationsRules();
        for (size_t i = 0; i < pNode1->m_outNodes.size(); i++) {
            SClauseRelation& rel = pNode1->m_outNodes[i];

            if (bResetAllRules)
                rel.ResetAllRules();
            else
                rel.ResetRules();
            //m_appliedRules = 0;
            //rel.m_prohibitedRules = 0;
            for (size_t j = 0; j < rel.m_pNode->m_inNodes.size(); j++)
                if (rel.m_pNode->m_inNodes[j].m_pNode == pNode2) {
                    rel.m_pNode->m_inNodes[j].m_pNode = pNode1;
                    if (bResetAllRules)
                        rel.m_pNode->m_inNodes[j].ResetAllRules();
                    else
                        rel.m_pNode->m_inNodes[j].ResetRules();
                    //rel.m_pNode->m_inNodes[j].m_appliedRules = 0;
                    //rel.m_pNode->m_inNodes[j].m_prohibitedRules = 0;
                }
        }
        pNode1->ResetInRelations();
        delete pNode2;/**/
    } else {
        pNode2->m_inNodes = pNode1->m_inNodes;
        bool bResetAllRules = pNode2->GetShouldResetAllOutRelationsRules(); //rel.m_pNode->GetShouldResetAllInRelationsRules();
        for (size_t i = 0; i < pNode2->m_inNodes.size(); i++) {
            SClauseRelation& rel = pNode2->m_inNodes[i];
            //bool bResetAllRules = rel.m_pNode->GetShouldResetAllInRelationsRules();
            if (bResetAllRules)
                rel.ResetAllRules();
            else
                rel.ResetRules();
            //rel.m_appliedRules = 0;
            //rel.m_prohibitedRules = 0;
            for (size_t j = 0; j < rel.m_pNode->m_outNodes.size(); j++)
                if (rel.m_pNode->m_outNodes[j].m_pNode == pNode1) {
                    rel.m_pNode->m_outNodes[j].m_pNode = pNode2;
                    if (bResetAllRules)
                        rel.m_pNode->m_outNodes[j].ResetAllRules();
                    else
                        rel.m_pNode->m_outNodes[j].ResetRules();
                    //rel.m_pNode->m_outNodes[j].m_appliedRules = 0;
                    //rel.m_pNode->m_outNodes[j].m_prohibitedRules = 0;
                }
        }
        pNode2->ResetOutRelations();
        delete pNode1;/**/
    }
}

void CTopClauses::UniteClause(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg)
{
    CClause* pCl1 = pNode1->GetMainClause();
    CClause* pCl2 = pNode2->GetMainClause();

    Stroka strStage = "Stage0 ";

    //try
    {
        CClauseNode* pClonedNode1 = NULL;
        CClauseNode* pClonedNode2 = NULL;

        int size1 = pNode1->m_outNodes.size();
        int size2 = pNode2->m_inNodes.size();

        if (size1 > 1)
            pClonedNode1 = pNode1->Clone();

        if (size2 > 1)
            pClonedNode2 = pNode2->Clone();

        if (!arg.m_bMainFirst) {
            strStage = "Stage1 ";
            pNode2->m_pClauseTree->EraseMainClause();

            pCl2->AddClauseWords(*pCl1, false);

            strStage = "Stage2 ";
            pCl1->ClearConjs();

            if (arg.m_iClause2TypeNum != -1)
                pCl2->AddValsToType(arg.m_NewValencies, arg.m_iClause2TypeNum);

            strStage = "Stage3 ";
            if (arg.m_NewType.m_Type != UnknownClause)
                pCl2->DelAllTypesAndAdd(arg.m_NewType);

            if (!arg.m_bRefreshClauseInfo) {
                strStage = "Stage3.1 ";
                pCl2->RunSynAn();
                strStage = "Stage3.2 ";
                pCl2->RefreshConjInfo();
                strStage = "Stage3.3 ";
                pCl2->RefreshValencies();
                strStage = "Stage3.4 ";
            } else {
                strStage = "Stage3.5 ";
                pCl2->RefreshClauseInfo();
                strStage = "Stage3.6 ";
            }

            strStage = "Stage4 ";
            pCl2->AddBadWeight(arg.m_BadWeight);
            pCl2->AddBadWeight(pCl1->m_BadWeight);
            pCl2->AddBadComma(pCl1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef,
                              arg.m_CoefficientToIncreaseBadWeightForCommas);
            strStage = "Stage5 ";
            //if( arg.m_BadWeightForCommas > 0 )
            //    pCl2->m_BadCommas.push_back(SBadComma(pCl1->LastWord(), arg.m_BadWeightForCommas) );//m_BadWeightForCommas += arg.m_BadWeightForCommas;
            //MultiplyBadCommaWeight(*pCl2, arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
            //pCl2->m_BadWeightForCommas += pCl1->m_BadWeightForCommas;
            //pCl2->m_BadWeightForCommas *= arg.m_CoefficientToIncreaseBadWeightForCommas;

            pNode1->m_pClauseTree->EraseMainClause();
            strStage = "Stage6 ";
            delete pCl1;/**/
            strStage = "Stage7 ";
            pNode2->m_pClauseTree->AddSubClauses(pNode1->m_pClauseTree, pCl2);
            strStage = "Stage8 ";
        } else {

            pNode1->m_pClauseTree->EraseMainClause();
            pCl1->AddClauseWords(*pCl2);

            if (arg.m_iClause1TypeNum != -1)
                pCl1->AddValsToType(arg.m_NewValencies, arg.m_iClause1TypeNum);

            if (arg.m_NewType.m_Type != UnknownClause)
                pCl1->DelAllTypesAndAdd(arg.m_NewType);

            if (!arg.m_bRefreshClauseInfo) {
                pCl1->RunSynAn();
                pCl1->RefreshConjInfo();
            } else
                pCl1->RefreshClauseInfo();

            pCl1->AddBadWeight(arg.m_BadWeight);
            pCl1->AddBadWeight(pCl2->m_BadWeight);

            //if( arg.m_BadWeightForCommas > 0 )
            //    pCl1->m_BadCommas.push_back(SBadComma(pCl1->LastWord(), arg.m_BadWeightForCommas) );
            pCl1->m_BadCommas.insert(pCl1->m_BadCommas.begin(),pCl2->m_BadCommas.begin(), pCl2->m_BadCommas.end());
            pCl1->AddBadComma(pCl1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
            //MultiplyBadCommaWeight(*pCl1, arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);

            //pCl1->m_BadWeightForCommas += arg.m_BadWeightForCommas;
            //pCl1->m_BadWeightForCommas *= arg.m_CoefficientToIncreaseBadWeightForCommas;
            pNode2->m_pClauseTree->EraseMainClause();

            delete pCl2;/**/

            pNode1->m_pClauseTree->AddSubClauses(pNode2->m_pClauseTree, pCl1);

        }

        UpdateNodes(pNode1, pNode2, pClonedNode1, pClonedNode2, arg.m_bMainFirst);
    }
    /*catch(yexception& e)
    {
        throw e;
    }
    catch(...)
    {
        ythrow yexception() << "Error in \"CTopClauses::UniteClause\"";
    }*/
}

void CTopClauses::AddValsToSubClauses(CClauseNode* pNode, SClauseFuncArg& arg)
{
    CClause* clause = pNode->GetMainClause();
    clause->AddValsToType(arg.m_NewValencies2, arg.m_iClause2TypeNum);

    if (clause->m_ClauseWords.size() != 0)
        return;
    yvector<CClause*> clauses;
    pNode->m_pClauseTree->GetIncludedClauses(clauses);
    for (size_t i = 0; i < clauses.size(); i++) {
        CClause& cl = *clauses[i];
        //такая заплатка: когда сочиняется клауза уже сочиненная и еще какая-то,
        //то новую валентность (обычно подлежащее) надо прописать и во все непосредственные подклаузы сочиняемой кл.
        if ((cl.GetTypesCount() == 1) &&  (arg.m_NewValencies2.size() == 1)) {
            if (!cl.GetType(0).HasSubjVal()) {
                SValency val = arg.m_NewValencies2[0];
                val.m_ValNum = cl.FindSubjVal(0);
                if (val.m_ValNum > -1)
                    cl.AddValToType(val, 0);
            }
        }
    }

}

void CTopClauses::IncludeClause(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg)
{
    //try
    {
        CClause* clause1 = pNode1->GetMainClause();
        CClause* clause2 = pNode2->GetMainClause();

        CClauseNode* pClonedNode1 = NULL;
        CClauseNode* pClonedNode2 = NULL;

        int size1 = pNode1->m_outNodes.size();
        int size2 = pNode2->m_inNodes.size();

        if (size1 > 1)
            pClonedNode1 = pNode1->Clone();

        if (size2 > 1)
            pClonedNode2 = pNode2->Clone();

        if (arg.m_iClause2TypeNum != -1) //если вызывается из NewClause
            AddValsToSubClauses(pNode2, arg);
            //clause2->AddValsToType(arg.m_NewValencies2, arg.m_iClause2TypeNum);

        if (arg.m_bMainFirst) {
            pNode1->m_pClauseTree->EraseMainClause();

            clause1->SetLastWord(clause2->LastWord());
            if (arg.m_ConjSupValency.IsValid() && (arg.m_iConj != -1))
                clause2->m_Conjs[arg.m_iConj].m_SupVal = arg.m_ConjSupValency;
            if (arg.m_NodeSupValency.IsValid() && (arg.m_iClause2TypeNum != -1))
                clause2->GetType(arg.m_iClause2TypeNum).m_SupVal = arg.m_NodeSupValency;

            clause1->AddBadWeight(arg.m_BadWeight);
            //if( arg.m_BadWeightForCommas > 0 )
            //    clause1->m_BadCommas.push_back(SBadComma(clause1->LastWord(), arg.m_BadWeightForCommas) );
            //MultiplyBadCommaWeight(*clause1, arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
            clause1->AddBadComma(clause1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
            //clause1->m_BadWeightForCommas += arg.m_BadWeightForCommas;
            //clause1->m_BadWeightForCommas *= arg.m_CoefficientToIncreaseBadWeightForCommas;
            pNode1->m_pClauseTree->AddSubClauses(pNode2->m_pClauseTree, clause1);
        } else {
            clause2->AddBadWeight(arg.m_BadWeight);
            pNode2->m_pClauseTree->EraseMainClause();
            clause2->SetFirstWord(clause1->FirstWord());
            /*if( arg.m_ConjSupValency.IsValid() && (arg.m_iConj != -1) )
                clause2->m_Conjs[arg.m_iConj].m_Sup
                Val = arg.m_ConjSupValency;*/

            //if( arg.m_BadWeightForCommas > 0 )
            //    clause2->m_BadCommas.push_back(SBadComma(clause1->LastWord(), arg.m_BadWeightForCommas) );
            //MultiplyBadCommaWeight(*clause2, arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
            clause2->AddBadComma(clause1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
            //clause2->m_BadWeightForCommas += arg.m_BadWeightForCommas;
            //clause2->m_BadWeightForCommas *= arg.m_CoefficientToIncreaseBadWeightForCommas;
            pNode2->m_pClauseTree->AddSubClauses(pNode1->m_pClauseTree, clause2);
        }

        UpdateNodes(pNode1, pNode2, pClonedNode1, pClonedNode2, arg.m_bMainFirst);
    }
    /*catch(...)
    {
        ythrow yexception() << "Error in \"CTopClauses::IncludeClause\"";
    }*/
}

void CTopClauses::DelTypes(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg)
{
    //try
    {
        CClause* clause1 = pNode1->GetMainClause();
        CClause* clause2 = pNode2->GetMainClause();

        size_t i;
        for (i = 0; i < arg.m_DelTypes1.size(); i++)
            clause1->DeleteType(arg.m_DelTypes1[i]);

        if (arg.m_DelTypes1.size() > 0) {
            pNode1->ResetInRelations();
            pNode1->ResetOutRelations();
        }

        for (i = 0; i < arg.m_DelTypes2.size(); i++)
            clause2->DeleteType(arg.m_DelTypes2[i]);

        if (arg.m_DelTypes2.size() > 0) {
            pNode2->ResetInRelations();
            pNode2->ResetOutRelations();
        }
    }
    /*catch(...)
    {
        ythrow yexception() << "Error in \"CTopClauses::DelTypes\"";
    }*/
}

void CTopClauses::NewClause(CClauseNode* pNode1, CClauseNode* pNode2, SClauseFuncArg& arg)
{
    //try
    {
        CClause* clause1 = pNode1->GetMainClause();
        CClause* clause2 = pNode2->GetMainClause();

        bool clause1WordsCountZero = (clause1->m_ClauseWords.size() == 0);
        bool clause2WordsCountZero = (clause2->m_ClauseWords.size() == 0);

        bool bIncludeOp = clause1WordsCountZero || clause2WordsCountZero;

        CClauseNode* pClonedNode1 = NULL;
        CClauseNode* pClonedNode2 = NULL;

        if (!bIncludeOp) {
            int size1 = pNode1->m_outNodes.size();
            int size2 = pNode2->m_inNodes.size();

            if (size1 > 1)
                pClonedNode1 = pNode1->Clone();

            if (size2 > 1)
                pClonedNode2 = pNode2->Clone();

            if (arg.m_iClause2TypeNum != -1)
                clause2->AddValsToType(arg.m_NewValencies2, arg.m_iClause2TypeNum);
        }

        if (clause1WordsCountZero) {
            arg.m_bMainFirst = true;
            IncludeClause(pNode1, pNode2, arg);
            pNode1->GetMainClause()->RunSynAn();
            return;
        }

        if (clause2WordsCountZero) {
            arg.m_bMainFirst = false;
            IncludeClause(pNode1, pNode2, arg);
            pNode2->GetMainClause()->RunSynAn();
            return;
        }

        CClause* pNewClause = CClause::Create(m_Words);
        pNewClause->SetPair(clause1->FirstWord(), clause2->LastWord());

        if (arg.m_bMainFirst)
            pNewClause->m_Types = clause1->m_Types;
        else
            pNewClause->m_Types = clause2->m_Types;

        pNewClause->AddBadWeight(arg.m_BadWeight);
        pNewClause->m_Conjs = clause1->m_Conjs;

        //if( arg.m_BadWeightForCommas > 0 )
        //    pNewClause->m_BadCommas.push_back(SBadComma(clause1->LastWord(), arg.m_BadWeightForCommas) );
        //MultiplyBadCommaWeight(*pNewClause, arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
        pNewClause->AddBadComma(clause1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
        //pNewClause->m_BadWeightForCommas += arg.m_BadWeightForCommas;
        //pNewClause->m_BadWeightForCommas *= arg.m_CoefficientToIncreaseBadWeightForCommas;
        pNode1->m_pClauseTree->AddSubClauses(pNode2->m_pClauseTree, pNewClause);
        pNode1->GetMainClause()->RunSynAn();

        UpdateNodes(pNode1, pNode2, pClonedNode1, pClonedNode2, true);
    }
    /*catch(...)
    {
        ythrow yexception() << "Error in \"CTopClauses::NewClause\"";
    }*/
}

bool CTopClauses::GetNeighbourClauseL(CClauseNode* node, CClauseNode*& next_node)
{
    if (node->m_inNodes[0].m_pNode->IsEndNode())
        return false;
    next_node = node->m_inNodes[0].m_pNode;
    return true; //что сделаешь..., все не проверишь
}

bool CTopClauses::GetNeighbourClauseR(CClauseNode* node, CClauseNode*& next_node)
{
    if (node->m_outNodes[0].m_pNode->IsEndNode())
        return false;
    next_node = node->m_outNodes[0].m_pNode;
    return true; //что сделаешь..., все не проверишь
}

CClause* CTopClauses::GetIncludedClauseR(CClauseNode* node)
{
    return node->GetIncludedClauseR();
}

bool CTopClauses::HasIncludedClauseR(CClauseNode* node)
{
    return (GetIncludedClauseR(node) != NULL);
}

CClause* CTopClauses::GetIncludedClauseL(CClauseNode* node)
{
    return node->GetIncludedClauseL();
}

bool CTopClauses::HasIncludedClauseL(CClauseNode* node)
{
    return (GetIncludedClauseL(node) != NULL);
}
