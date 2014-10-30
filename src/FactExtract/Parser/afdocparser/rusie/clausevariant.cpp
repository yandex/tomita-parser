#include "clausevariant.h"
#include "clauserules.h"


FClauseOp CClauseVariant::s_ClauseOp[ClauseOperationCount] =
                {   &CClauseVariant::UniteClause,
                    &CClauseVariant::IncludeClause,
                    &CClauseVariant::NewClause,
                    &CClauseVariant::DelTypes
                };

ERuleType CClauseVariant::s_UniteBracketsRules[CClauseVariant::s_UniteBracketsRulesCount] =
{
    UniteClauseWithBrackets
};

ERuleType CClauseVariant::s_IncludeBracketsRules[CClauseVariant::s_IncludeBracketsRulesCount] =
{
    IncludeBrackets,
    IncludeDash,
    IncludeParenthesis
};

ERuleType CClauseVariant::s_UniteRules[CClauseVariant::s_UniteRulesCount] =
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
};

ERuleType CClauseVariant::s_ConjSubRules[CClauseVariant::s_ConjSubRulesCount] =
{
    ConjSubInfs,
    ConjSubPredics,
    ConjSimParticiples,
    ConjSimVerbAdverb,
    ConjSubSimNodes
};

ERuleType CClauseVariant::s_IncludeRules[CClauseVariant::s_IncludeRulesCount] =
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
};

ERuleType CClauseVariant::s_ConjRules[CClauseVariant::s_ConjRulesCount] =
{
    ConjSimNodes
};

bool ClauseOrder(const CClause*& cl1,const CClause*& cl2)
{
    return *(cl1) < *(cl2);
}

/*------------------------------------class CClauseVariant ------------------------------------*/

CClauseVariant::CClauseVariant(CWordVector& words)
    : m_Words(words)
{
    m_BiggestID = 0;
    m_BadWeight = 0;
    m_bCloned = false;
}

CClauseVariant::~CClauseVariant()
{
}

CClauseVariant::CClauseVariant(const CClauseVariant& ClVar)
    : m_Words(ClVar.m_Words)
{
    m_Clauses = ClVar.m_Clauses;
    m_newClauseVariants = ClVar.m_newClauseVariants;
    m_BiggestID = ClVar.m_BiggestID;
    m_ProhibitedRules = ClVar.m_ProhibitedRules;
    m_BadWeight = ClVar.GetBadWeight();
    m_bCloned = ClVar.m_bCloned;
}

CClauseVariant& CClauseVariant::operator=(const CClauseVariant& ClVar)
{
    m_Clauses = ClVar.m_Clauses;
    m_newClauseVariants = ClVar.m_newClauseVariants;
    m_BiggestID = ClVar.m_BiggestID;
    m_ProhibitedRules = ClVar.m_ProhibitedRules;
    m_Words = ClVar.m_Words;
    m_BadWeight = ClVar.GetBadWeight();
    return *this;
}

void CClauseVariant::CalculateBadWeight()
{

    const_clause_it it;

    //1.Количество фрагментов верхнего уровня
    for (it = GetFirstClause(LeftToRight); it != m_Clauses.end(); it = GetNextClause(LeftToRight, it)) {
        CClause& clause = **it;

        if (!clause.HasTypeWithSubj())
            m_BadWeight++;

        if (clause.IsSubClause())
            m_BadWeight += 3;
        else if ((clause.HasType_i(Empty) != -1) && (clause.GetTypesCount() == 1))
                m_BadWeight += 2;

    }

    for (it = m_Clauses.begin(); it != m_Clauses.end(); it++)
        m_BadWeight += (**it).CalculateBadWeight();

}

void CClauseVariant::Clone(CClauseVariant& clonedVar)
{

    clause_it it;
    for (it = m_Clauses.begin(); it !=  m_Clauses.end(); it++) {
        CClause* newClause = CClause::Create((*it)->m_Words);
        newClause->Copy(*(*it));
        clonedVar.AddClause(newClause);
    }
    clonedVar.m_ProhibitedRules = m_ProhibitedRules;
    clonedVar.m_BadWeight = m_BadWeight;
    clonedVar.m_bCloned = true;
}

void CClauseVariant::ChangeLastClauseLastWord(long iW)
{
    clause_rev_it it = m_Clauses.rbegin();
    (*it)->SetLastWord(iW);
}

void  CClauseVariant::AddClauses(yset<CClause*, SClauseOrder>& clauses)
{
    m_Clauses.insert(clauses.begin(), clauses.end());
}

clause_it  CClauseVariant::AddClause(CClause* pClause)
{
    pClause->PutID(m_BiggestID);
    return m_Clauses.insert(pClause).first;
}

void CClauseVariant::DeleteClause(const_clause_it it)
{
    delete *it;
    m_Clauses.erase(*it);
}

void CClauseVariant::DeleteClause(CWordsPair& wordsPair)
{

    clause_it it = m_Clauses.begin(); //find(map_it->second);
    for (; it != m_Clauses.end(); it++)
        if (wordsPair == **it)
            break;

    YASSERT(it != m_Clauses.end());

    delete *it;
    m_Clauses.erase(it);
}

bool CClauseVariant::IsEmpty() const
{
    return m_Clauses.size() == 0;
}

void CClauseVariant::AddInitialClause(CClause* pClause)
{
    clause_it it;
    //yvector<long> deleted_clause;
    yvector<CWordsPair> deleted_clause;

    bool bLastPunct = false;
    if (m_Clauses.size() > 0) {
        for (it = --(m_Clauses.end());; it--) {
            if (((*it)->LastWord() + 1) == pClause->FirstWord())
                break;

            if ((*it)->HasPunctAtTheVeryEnd())
                bLastPunct = true;

            if ((*it)->FirstWord() >= pClause->FirstWord()) {
                deleted_clause.push_back(**it);
                if (it == m_Clauses.begin())
                    break;
                continue;
            }
            if ((*it)->LastWord() >= pClause->FirstWord()) {
                (*it)->SetLastWord(pClause->FirstWord() - 1);
                if ((*it)->Size() == 0) {
                    deleted_clause.push_back(**it);
                    break;
                } else if (bLastPunct)
                        (*it)->PutLastWordPunct();

            }

            if (it == m_Clauses.begin())
                break;
        }
    }

    for (size_t i = 0; i < deleted_clause.size(); i++)
        DeleteClause(deleted_clause[i]);

    CClause* clonedClause = NULL;

    if (m_Clauses.size() == 0)
        clonedClause = pClause->CloneBySubConj();
    else {
        const_clause_it it = GetFirstClause(RightToLeft);
        if (!(*it)->HasDash())
            clonedClause = pClause->CloneBySubConj();
    }

    if (clonedClause) {
        clonedClause->InitClauseTypesByConj();
        clonedClause->PutID(m_BiggestID);
        m_Clauses.insert(clonedClause);
    }

    pClause->InitClauseTypesByConj();
    pClause->PutID(m_BiggestID);
    m_Clauses.insert(pClause);
}

void CClauseVariant::Print(TOutputStream& stream, ECharset encoding) const
{
    yset<CClause*, SClauseOrder>::const_iterator it = m_Clauses.begin();
    int i = 0;
    for (; it !=  m_Clauses.end(); it++) {
        DECLARE_STATIC_RUS_WORD(kFragment, "Фрагмент № ");
        stream << " " << NStr::Encode(kFragment, encoding) << i++ << Endl;
        stream << " " << (*it)->ToString(true) << Endl;
        (*it)->Print(stream, encoding);
    }
}

void CClauseVariant::Reset()
{
    clause_it it;

    for (it = m_Clauses.begin(); it != m_Clauses.end(); it++)
        delete (*it);

    m_Clauses.clear();
}

bool CClauseVariant::CanApplyRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule, CClauseRules& ClauseRules)
{
    if (!CheckStopRules(pCl1, pCl2, rule, ClauseRules.GetRuleStopOptions(rule), m_ProhibitedRules))
        return false;

    return true;
}

bool CClauseVariant::CheckStopRules(const CClause& pCl1, const CClause& pCl2, ERuleType rule,
                                    EStopRulesOption StopOption, prohibited_rules& prh_rules)
{
    if (prh_rules.size() == 0)
        return true;
    prohibited_rules::iterator it;
    it = prh_rules.find(rule);
    if (it == prh_rules.end())
        return true;

    for (size_t i = 0; i < it->second.size(); i++) {
        SStopeRuleInfo& StopeRuleInfo = it->second[i];
        if (StopOption == CheckWholeClauses) {
            if ((StopeRuleInfo.m_Cl1 == pCl1) && (StopeRuleInfo.m_Cl2 == pCl2))
                return false;
        } else {
            SWordHomonymNum Word1;
            if (pCl1.m_ClauseWords.size() > 0)
                Word1 = pCl1.m_ClauseWords[pCl1.m_ClauseWords.size() - 1];

            SWordHomonymNum Word2;
            if (pCl2.m_ClauseWords.size() > 0)
                Word2 = pCl2.m_ClauseWords[0];

            if ((Word1 == StopeRuleInfo.m_Word1) && (Word2 == StopeRuleInfo.m_Word2))
                return false;
        }
    }

    return true;
}


void  CClauseVariant::AddProhibitedRules(const CClause& pCl1, const CClause& pCl2,
                                         yvector<ERuleType> rules, const CClauseRules& ClauseRules)
{
    for (size_t i = 0; i < rules.size(); i++)
        AddProhibitedRule(pCl1, pCl2, rules[i], ClauseRules.GetRuleStopOptions(rules[i]));
}

void  CClauseVariant::AddProhibitedRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule, EStopRulesOption StopOption)
{
    AddStopRule(pCl1, pCl2, rule, StopOption, m_ProhibitedRules);
}

void  CClauseVariant::AddStopRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule,
                                  EStopRulesOption StopOption, prohibited_rules& prh_rules)
{
    prohibited_rules::iterator it;
    it = prh_rules.find(rule);

    SStopeRuleInfo StopeRuleInfo;

    if (StopOption == CheckWholeClauses) {
        StopeRuleInfo.m_Cl1 = pCl1;
        StopeRuleInfo.m_Cl2 = pCl2;
    } else {
        if (pCl1.m_ClauseWords.size() > 0)
            StopeRuleInfo.m_Word1 = pCl1.m_ClauseWords[pCl1.m_ClauseWords.size() - 1];

        if (pCl2.m_ClauseWords.size() > 0)
            StopeRuleInfo.m_Word2 = pCl2.m_ClauseWords[0];
    }

    if (it == prh_rules.end()) {
        yvector< SStopeRuleInfo >stop_rules;
        stop_rules.push_back(StopeRuleInfo);
        prh_rules[rule] = stop_rules;
    } else {
        it->second.push_back(StopeRuleInfo);
    }

}

bool CClauseVariant::GetNeighbourClauseL(const_clause_it it, const_clause_it& next_it) const
{
    const_clause_it save_it = it;

    for (;; it--) {
        if (((*it)->LastWord() + 1) == (*save_it)->FirstWord()) {
            next_it = it;
            return true;
        }

        if (it == m_Clauses.begin())
            break;
    }

    next_it = m_Clauses.end();
    return false;
}

bool CClauseVariant::GetNeighbourClauseR(const_clause_it it, const_clause_it& next_it) const
{
    const_clause_it save_it = it++;
    next_it = m_Clauses.end();

    for (;  it != m_Clauses.end(); it++) {
        if (((*save_it)->LastWord() + 1) == (*it)->FirstWord())
            next_it = it;
    }

    return (next_it != m_Clauses.end());
}

bool CClauseVariant::GetNeighbourClauseR(clause_it it, const_clause_it& next_it, const_clause_it  main_it)
{
    const_clause_it prev_it = main_it;
    next_it = main_it;
    next_it--;

    for (; (**main_it).Includes(**next_it); next_it--) {
        if (it != m_Clauses.end()) {
            if ((**next_it) == (**it)) {
                next_it = prev_it;
                if ((**next_it) == (** main_it))
                    return false;
                else
                    return true;
            }
        }

        if (next_it == m_Clauses.begin())
            break;
        prev_it = next_it;
    }

    if (it == m_Clauses.end()) {
        if ((**main_it).Includes(**prev_it) && !((**prev_it) == (**main_it))) {
            next_it = prev_it;
            return true;
        }
    }

    return false;
}

bool CClauseVariant::GetNeighbourClauseL(const_clause_it it, const_clause_it& next_it, const_clause_it main_it)
{
    if (it == m_Clauses.end()) {
        if (main_it == m_Clauses.begin())
            return false;

        next_it = main_it;
        next_it--;
    } else {
        const_clause_it save_it = it;

        for (;; it--) {
            if ((*it)->FirstWord()  < (*save_it)->FirstWord()) {
                next_it = it;
                break;
            }

            if (it == m_Clauses.begin())
                return false;
        }

    }

    if ((*main_it)->Includes(**next_it))
        return true;
    else
        return false;
}

bool CClauseVariant::Equal(CClauseVariant& var)
{
    clause_it it = m_Clauses.begin();
    for (; it != m_Clauses.end(); it++) {
        clause_it it1 = var.m_Clauses.find(*it);
        if (it1 == var.m_Clauses.end())
            return false;

/*      CClause* cl1 = *it;
        CClause* cl2 = *it1;
        if(  (cl1->m_Conjs != cl2->m_Conjs) || (cl1->m_Types != cl2->m_Types))
            return false;*/
    }
    return true;
}

void CClauseVariant::RunRules(bool bCanClone)
{
            CClauseRules* pClauseRules = NULL;
            CClauseRules& ClauseRules = *pClauseRules;
/*
        if (CGramInfo::s_bFramentationDebug) {
            if (!m_bCloned) {
                SVarDebug debug_var;
                debug_var.m_pVariant = new CClauseVariant(m_Words);
                Clone(*debug_var.m_pVariant);
                m_DebugVars.push_back(debug_var);
            } else {
                Stroka strDescr;
                if (m_DebugVars.size() > 0) {
                    strDescr = m_DebugVars[m_DebugVars.size() - 1].m_strDescr;
                    m_DebugVars[m_DebugVars.size() - 1].m_pVariant->Reset();
                    delete m_DebugVars[m_DebugVars.size() - 1].m_pVariant;
                    m_DebugVars.erase(m_DebugVars.begin() + m_DebugVars.size() - 1);
                }
                SVarDebug debug_var;
                debug_var.m_strDescr = strDescr;
                debug_var.m_pVariant = new CClauseVariant(m_Words);
                Clone(*debug_var.m_pVariant);
                m_DebugVars.push_back(debug_var);

            }
        }
*/
    EStateOfTries StateOfTries = NormalTry;

    RunRules(s_IncludeBracketsRules, s_IncludeBracketsRulesCount, false, StateOfTries, LeftToRight, false, ClauseRules);
    RunRules(s_UniteBracketsRules, s_UniteBracketsRulesCount,  false, StateOfTries, LeftToRight, false, ClauseRules);

    while (true) {
        bool bUniteRules = RunRules(s_UniteRules, s_UniteRulesCount, false, StateOfTries, LeftToRight, bCanClone, ClauseRules);

        bool bSubConjRules = RunRules(s_ConjSubRules, s_ConjSubRulesCount, true, StateOfTries, LeftToRight, bCanClone, ClauseRules);
        if (bSubConjRules)
            continue;

        bool bIncludeRules = RunRules(s_IncludeRules, s_IncludeRulesCount, true, StateOfTries, RightToLeft, bCanClone, ClauseRules);
        if (bIncludeRules)
            continue;

        bool bConjRules = RunRules(s_ConjRules, s_ConjRulesCount, true, StateOfTries, RightToLeft, bCanClone, ClauseRules);
        if (bConjRules)
            continue;

        if (!bIncludeRules && !bUniteRules && !bConjRules && !bSubConjRules) {
            break;
        }
    }
}

const_clause_it CClauseVariant::GetFirstClauseConst(EDirection direction) const
{
    if (direction == RightToLeft) {
        return --(m_Clauses.end());
    } else {
        const_clause_it it;
        for (it = --(m_Clauses.end());; it--) {
            if ((*it)->FirstWord() == 0)
                return it;
            assert(it != m_Clauses.begin());
        }

    }
}

clause_it CClauseVariant::GetFirstClause(EDirection direction)
{
    if (direction == RightToLeft) {
        return --(m_Clauses.end());
    } else {
        clause_it it;
        for (it = --(m_Clauses.end());; it--) {
            if ((*it)->FirstWord() == 0)
                return it;
            assert(it != m_Clauses.begin());
        }

    }
}

const_clause_it CClauseVariant::GetNextClause(EDirection direction, const_clause_it cl_it) const
{
    const_clause_it next_it;

    if (direction == RightToLeft) {
        if (GetNeighbourClauseL(cl_it, next_it))
            return next_it;
        else
            return m_Clauses.end();
    } else {
        if (GetNeighbourClauseR(cl_it, next_it))
            return next_it;
        else
            return m_Clauses.end();
    }
}

bool CClauseVariant::RunRules(ERuleType rules[], int rules_count, bool bBreakAfterFirstRule, EStateOfTries /*eStateOfTry*/, EDirection direction, bool /*bCanClone*/, CClauseRules& ClauseRules)
{
    if (m_Clauses.size() == 0)
        return false;

    const_clause_it it, next_it;

    bool bAppliedAtLeastOnce = false;

    it = GetFirstClause(direction);
    bool bApplied = false;
    while ((next_it = GetNextClause(direction, it)) != m_Clauses.end()) {
        const_clause_it it1, it2;

        if (direction == RightToLeft) {
            it1 = next_it;
            it2 = it;
        } else {
            it1 = it;
            it2 = next_it;
        }

        it = next_it;

        for (int i = 0; i < rules_count; i++) {
            ERuleType rule = rules[i];
            if (!CanApplyRule(**it1, **it2, rule, ClauseRules))
                continue;
        }

        if (bBreakAfterFirstRule && bApplied)
            break;
    }

    return bAppliedAtLeastOnce;
}

void CClauseVariant::CloneVarByRuleResult(const_clause_it it1, const_clause_it it2, SClauseRuleResult& res,
                                          ERuleType eRule, ERuleType rules[], int rules_count,CClauseRules& ClauseRules)
{
    CClause* pCl1 = *it1,* pCl2 = *it2;

    CClauseVariant newClauseVar(m_Words);
    Clone(newClauseVar);

    if (ClauseRules.GetRuleStopOptions(eRule) == CheckJunction) {
        for (int i = 0; i < rules_count; i++)
            res.m_StopRules.push_back(rules[i]);
    } else
        res.m_StopRules.push_back(eRule);

    newClauseVar.AddProhibitedRules(**it1, **it2, res.m_StopRules, ClauseRules);

    if ((res.m_ClauseFuncArg.m_iClause1TypeNum != -1) && (pCl1->m_Types.size() > 1)) {
        clause_it it = newClauseVar.m_Clauses.find(pCl1);
        YASSERT(it != newClauseVar.m_Clauses.end());
        (*it)->DelTypesExcept(res.m_ClauseFuncArg.m_iClause1TypeNum);
    }

    if ((res.m_ClauseFuncArg.m_iClause2TypeNum != -1) && (pCl2->m_Types.size() > 1)) {
        clause_it it = newClauseVar.m_Clauses.find(pCl2);
        YASSERT(it != newClauseVar.m_Clauses.end());
        (*it)->DelTypesExcept(res.m_ClauseFuncArg.m_iClause2TypeNum);
    }

    m_newClauseVariants.push_back(newClauseVar);
}

clause_it CClauseVariant::ApplyClauseOp(const_clause_it it1, const_clause_it it2, SClauseRuleResult& res, bool bCanClone)
{
    CClause* pCl1 = *it1,* pCl2 = *it2;

    bool bClause1HasOtherTypes = ((pCl1->m_Types.size() > 1) && (res.m_ClauseFuncArg.m_iClause1TypeNum != -1));
    bool bClause2HasOtherTypes = ((pCl2->m_Types.size() > 1) && (res.m_ClauseFuncArg.m_iClause2TypeNum != -1));
    if ((bClause1HasOtherTypes ||   bClause2HasOtherTypes)
        &&  bCanClone) {

        CClauseVariant newClauseVar1(m_Words);
        CClauseVariant newClauseVar2(m_Words);
        CClauseVariant newClauseVar3(m_Words);

        int iType1 = res.m_ClauseFuncArg.m_iClause1TypeNum;
        int iType2 = res.m_ClauseFuncArg.m_iClause2TypeNum;

        bool bAdditionalVars = res.m_ClauseFuncArg.m_bCloneByType1 && res.m_ClauseFuncArg.m_bCloneByType2 &&
                                bClause1HasOtherTypes && bClause2HasOtherTypes;

        if (bAdditionalVars) {
            Clone(newClauseVar2);
            Clone(newClauseVar3);
        }

        if (bClause1HasOtherTypes)
            if (res.m_ClauseFuncArg.m_bCloneByType1) {
                if (newClauseVar1.IsEmpty())
                    Clone(newClauseVar1);

                pCl1->DelTypesExcept(res.m_ClauseFuncArg.m_iClause1TypeNum);

                clause_it it = newClauseVar1.m_Clauses.find(pCl1);
                YASSERT(it != newClauseVar1.m_Clauses.end());
                (*it)->DeleteType(res.m_ClauseFuncArg.m_iClause1TypeNum);
                res.m_ClauseFuncArg.m_iClause1TypeNum = 0; //оставили только нужный тип
                if (bAdditionalVars) {
                    it = newClauseVar2.m_Clauses.find(pCl1);
                    (*it)->DeleteType(iType1);

                    it = newClauseVar2.m_Clauses.find(pCl2);
                    (*it)->DelTypesExcept(iType2);
                    m_newClauseVariants.push_back(newClauseVar2);
                }
            } else {
                pCl1->DelTypesExcept(res.m_ClauseFuncArg.m_iClause1TypeNum);
                res.m_ClauseFuncArg.m_iClause1TypeNum = 0; //оставили только нужный тип
            }

        if ((res.m_ClauseFuncArg.m_iClause2TypeNum != -1) && (pCl2->m_Types.size() > 1))
            if (res.m_ClauseFuncArg.m_bCloneByType2) {
                if (newClauseVar1.IsEmpty())
                    Clone(newClauseVar1);

                pCl2->DelTypesExcept(res.m_ClauseFuncArg.m_iClause2TypeNum);

                clause_it it = newClauseVar1.m_Clauses.find(pCl2);
                YASSERT(it != newClauseVar1.m_Clauses.end());
                (*it)->DeleteType(res.m_ClauseFuncArg.m_iClause2TypeNum);

                if (bAdditionalVars) {
                    it = newClauseVar3.m_Clauses.find(pCl1);
                    (*it)->DelTypesExcept(iType1);

                    it = newClauseVar3.m_Clauses.find(pCl2);
                    (*it)->DeleteType(iType2);

                    m_newClauseVariants.push_back(newClauseVar3);
                }

                res.m_ClauseFuncArg.m_iClause2TypeNum = 0; //оставили только нужный тип
            } else {
                pCl2->DelTypesExcept(res.m_ClauseFuncArg.m_iClause2TypeNum);
                res.m_ClauseFuncArg.m_iClause2TypeNum = 0; //оставили только нужный тип
            }

        if (!newClauseVar1.IsEmpty())
            m_newClauseVariants.push_back(newClauseVar1);
    }

    return ((*this).*(s_ClauseOp[res.m_Operation]))(it1, it2, res.m_ClauseFuncArg);
}

clause_it CClauseVariant::UniteClause(const_clause_it it1, const_clause_it it2,  SClauseFuncArg& arg)
{

    if (!arg.m_bMainFirst) {
        CClause* pCl1 = *it1;
        CClause* pCl2 = *it2;

        pCl2->AddClauseWords(*pCl1, false);

        pCl1->ClearConjs();

        if (arg.m_iClause2TypeNum != -1)
            pCl2->AddValsToType(arg.m_NewValencies, arg.m_iClause2TypeNum);

        if (arg.m_NewType.m_Type != UnknownClause)
            pCl2->DelAllTypesAndAdd(arg.m_NewType);

        if (!arg.m_bRefreshClauseInfo) {
            pCl2->RefreshConjInfo();
            pCl2->RefreshValencies();
        } else
            pCl2->RefreshClauseInfo();

        DeleteClause(it1);
        m_Clauses.erase(*it2);
        pCl2->AddBadWeight(arg.m_BadWeight);
        pCl2->AddBadComma(pCl1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
        return m_Clauses.insert(pCl2).first;
    } else {
        (*it1)->AddClauseWords(**it2);
        CClause* pCl1 = *it1;

        if (arg.m_iClause1TypeNum != -1)
            pCl1->AddValsToType(arg.m_NewValencies, arg.m_iClause1TypeNum);

        if (arg.m_NewType.m_Type != UnknownClause)
            pCl1->DelAllTypesAndAdd(arg.m_NewType);

        if (!arg.m_bRefreshClauseInfo)
            pCl1->RefreshConjInfo();
        else
            pCl1->RefreshClauseInfo();

        DeleteClause(it2);
        m_Clauses.erase(*it1);
        pCl1->AddBadWeight(arg.m_BadWeight);
        pCl1->AddBadComma(pCl1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
        return m_Clauses.insert(pCl1).first;
    }
}

clause_it CClauseVariant::IncludeClause(const_clause_it it1, const_clause_it it2, SClauseFuncArg& arg)
{
    CClause* clause1 = *it1;
    CClause* clause2 = *it2;

    if (arg.m_bMainFirst) {
        clause1->SetLastWord(clause2->LastWord());
        if (arg.m_ConjSupValency.IsValid() && (arg.m_iConj != -1))
            clause2->m_Conjs[arg.m_iConj].m_SupVal = arg.m_ConjSupValency;

        m_Clauses.erase(*it1);
        clause1->AddBadWeight(arg.m_BadWeight);
        clause1->AddBadComma(clause1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
        return m_Clauses.insert(clause1).first;
    } else {
        clause2->SetFirstWord(clause1->FirstWord());

        m_Clauses.erase(*it2);
        clause2->AddBadWeight(arg.m_BadWeight);
        clause2->AddBadComma(clause1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
        return m_Clauses.insert(clause2).first;
    }

}

clause_it CClauseVariant::DelTypes(const_clause_it it1, const_clause_it it2,  SClauseFuncArg& arg)
{
    CClause* clause1 = *it1;
    CClause* clause2 = *it2;

    size_t i;
    for (i = 0; i < arg.m_DelTypes1.size(); i++)
        clause1->DeleteType(arg.m_DelTypes1[i]);

    for (i = 0; i < arg.m_DelTypes2.size(); i++)
        clause2->DeleteType(arg.m_DelTypes2[i]);

    // WARNING: performance issues!
    clause_it it_return_value = m_Clauses.find(*it1);
    return it_return_value;
    // END OF WARNING

    // было:
    //return it1;
}

clause_it CClauseVariant::NewClause(const_clause_it it1, const_clause_it it2, SClauseFuncArg& arg)
{
    CClause* clause1 = *it1;
    CClause* clause2 = *it2;

    if (arg.m_iClause2TypeNum != -1)
        clause2->AddValsToType(arg.m_NewValencies2, arg.m_iClause2TypeNum);

    if (clause1->m_ClauseWords.size() == 0) {
        arg.m_bMainFirst = true;
        return IncludeClause(it1, it2, arg);
    }

    if (clause2->m_ClauseWords.size() == 0) {
        arg.m_bMainFirst = false;
        return IncludeClause(it1, it2, arg);
    }

    CClause* pNewClause = CClause::Create(m_Words);
    pNewClause->SetPair(clause1->FirstWord(), clause2->LastWord());

    if (arg.m_bMainFirst)
        pNewClause->m_Types = clause1->m_Types;
    else
        pNewClause->m_Types = clause2->m_Types;

    pNewClause->m_Conjs = clause1->m_Conjs;
    pNewClause->AddBadWeight(arg.m_BadWeight);
    pNewClause->AddBadComma(clause1->LastWord(),arg.m_BadWeightForCommas,  arg.m_PairForBadCoef, arg.m_CoefficientToIncreaseBadWeightForCommas);
    return AddClause(pNewClause);
}

const CClause* CClauseVariant::GetIncludedClauseR(const_clause_it it) const
{
    const_clause_it save_it = it;

    if (it == m_Clauses.begin())
        return NULL;

    it--;

    if ((**it).LastWord() == (**save_it).LastWord())
        return *it;

    return NULL;
}

bool CClauseVariant::HasIncludedClauseR(const_clause_it it) const
{
    return (GetIncludedClauseR(it) != NULL);
}

const_clause_it CClauseVariant::GetFirstClause() const
{
    return m_Clauses.begin();
};

const_clause_it CClauseVariant::GetEndClause() const
{
    return m_Clauses.end();
};

//все клаузы, которые непосредственно включаются в главную
void CClauseVariant::GetIncludedClauses(const_clause_it it, yvector<const CClause*>& subClauses) const
{
    if (it == m_Clauses.begin())
        return;
    const_clause_it main = it;
    int rBorder = (**main).LastWord() + 1;
    it = main;
    do
    {
        it--;
        CWordsPair& wp = **it;
        if (!(**main).Includes(wp))
            break;
        if ((wp.LastWord() < rBorder)) {
            subClauses.push_back(*it);
            rBorder = wp.FirstWord();
        }

    } while (it != m_Clauses.begin());

}

const_clause_it CClauseVariant::GetMainClauseConst(const_clause_it it) const
{
    if (it == GetEndClause()) return GetEndClause();
    const CClause& clause = GetClauseRef(it);
    it++;
    for (; it != GetEndClause(); it++) {
        const CClause& next_clause = GetClauseRef(it);
        if (next_clause.Includes(clause))
            return it;
    }
    return GetEndClause();
}

const CClause* CClauseVariant::GetMainClause(const_clause_it it) const
{
    const_clause_it res = GetMainClauseConst(it);
    if (res != GetEndClause())
        return *res;
    else
        return NULL;
}

CClause* CClauseVariant::GetMainClause(clause_it it)
{
    const_clause_it res = GetMainClauseConst(it);
    if (res != GetEndClause()) {
        const CClause* cl = *res;
        return const_cast<CClause*>(cl);
    } else
        return NULL;
}

CClause& CClauseVariant::GetClauseRef(clause_it cl_it)
{
    return **cl_it;
}

const CClause& CClauseVariant::GetClauseRef(const_clause_it cl_it) const
{
    return **cl_it;
}

const CClause* CClauseVariant::GetIncludedClauseL(const_clause_it it) const
{
    const_clause_it save_it = it;

    if (it == m_Clauses.begin())
        return NULL;

    do
    {
        it--;

        if (!(**save_it).Includes(**it))
            return NULL;

        if ((**it).FirstWord() == (**save_it).FirstWord())
            return *it;

    } while (it!= m_Clauses.begin());

    return NULL;

}

bool CClauseVariant::HasIncludedClauseL(const_clause_it it) const
{
    return (GetIncludedClauseL(it) != NULL);
}

void CClauseVariant::PrintClauseTags(yvector<Stroka>& words)
{
    clause_it it;
    for (it = m_Clauses.begin(); it !=  m_Clauses.end(); it++) {
        CClause* clause = *it;
        int iw1 = clause->FirstWord();
        int iw2 = clause->LastWord();
        words[iw1] = Stroka("<f> ") + words[iw1];
        words[iw2] += Stroka(" </f>");
    }
}
/*
void CClauseVariant::InsertClauseNodeInfo(xmlNodePtr piClEl, const_clause_it it)
{
    CClause* pClause = *it;
    for (size_t i = 0; i < pClause->m_Types.size(); i++) {
        xmlNodePtr piInfo = xmlNewNode(NULL, (const xmlChar*)"info");
        TStringStream ss;
        pClause->PrintType(ss, i, CODES_UTF8);
        AddUtf8Attr(piInfo, "str", ss);
        xmlAddChild(piClEl, piInfo);
    }

}

xmlNodePtr CClauseVariant::PrintClauseSubTreeToXML(const_clause_it cl_it)
{
    xmlNodePtr piClEl = xmlNewNode(NULL, (const xmlChar*)"g");

    if ((*cl_it)->IsDisrupted()) {
        AddAsciiAttr(piClEl, "type", "unite");
        yvector< TPair<Wtroka, TPair<int, int> > > words2pairs;
        (*cl_it)->PrintDisruptedWords(words2pairs);
        int i = words2pairs.size() - 1;

        xmlNodePtr piCurClEl = xmlNewNode(NULL, (const xmlChar*)"g");
        AddAsciiAttr(piCurClEl, "pos", "right");
        AddAttr(piCurClEl, "words", words2pairs[i].first);
        xmlNodePtr piPrevSubCurClEl = NULL;
        const_clause_it it, prev_it = m_Clauses.end();
        xmlNodePtr piPrevSubClEl = NULL;
        while (GetNeighbourClauseL(prev_it, it, cl_it)) {
            xmlNodePtr piSubClEl = PrintClauseSubTreeToXML(it);
            Stroka strPos;
            if ((*it)->LastWord() <  words2pairs[i].second.first) {
                if (i == 0)
                    strPos = "left";
                else {
                    piPrevSubCurClEl = xmlAddPrevSibling(piPrevSubCurClEl, piCurClEl);
                    i--;

                    strPos = "right";
                    xmlNodePtr piCurClEl = xmlNewNode(NULL, (const xmlChar*)"g");
                    piPrevSubClEl = NULL;
                    AddAsciiAttr(piCurClEl, "pos", "right");
                    AddAttr(piCurClEl, "words", words2pairs[i].first);
                }
            } else
                strPos = "right";

            AddAsciiAttr(piSubClEl, "pos", strPos);
            piPrevSubClEl = xmlAddPrevSibling(piPrevSubClEl, piSubClEl);
            prev_it = it;
        }

        AddAsciiAttr(piCurClEl, "type", "first");

        TStringStream str_conjs;
        (*cl_it)->PrintConjsInfo(str_conjs, CODES_UTF8);
        AddUtf8Attr(piCurClEl, "conjs", str_conjs);

        InsertClauseNodeInfo(piCurClEl, cl_it);
        piPrevSubCurClEl = xmlAddPrevSibling(piPrevSubCurClEl, piCurClEl);

    } else {
        const_clause_it it, prev_it = m_Clauses.end();

        Wtroka strClauseWords;
        (*cl_it)->PrintClauseWord(strClauseWords);
        AddAttr(piClEl, "words", strClauseWords);

        xmlNodePtr piPrevSubClEl = NULL;
        bool bRight = false;

        while (GetNeighbourClauseL(prev_it, it, cl_it)) {
            xmlNodePtr piSubClEl = PrintClauseSubTreeToXML(it);
            if (bRight && (prev_it != m_Clauses.end()))
                if ((*it)->LastWord() != ((*prev_it)->FirstWord() - 1))
                    bRight = false;

            if ((*it)->FirstWord() == (*cl_it)->FirstWord() && (*cl_it)->m_ClauseWords.size() != 0)
                AddAsciiAttr(piSubClEl, "pos", "left");
            else if ((*it)->LastWord() == (*cl_it)->LastWord()) {
                    AddAsciiAttr(piSubClEl, "pos", "right");
                    bRight = true;
                } else if (bRight)
                        AddAsciiAttr(piSubClEl, "pos", "right");
                    else
                        AddAsciiAttr(piSubClEl, "pos", "left");

            piPrevSubClEl = xmlAddPrevSibling(piPrevSubClEl, piSubClEl);
            prev_it = it;
        }
    }

    AddAsciiAttr(piClEl, "pos", "right");
    InsertClauseNodeInfo(piClEl, cl_it);

    TStringStream str_conjs;
    (*cl_it)->PrintConjsInfo(str_conjs, CODES_UTF8);
    AddUtf8Attr(piClEl, "conjs", str_conjs);

    return piClEl;
}
*/
int FindClauseWordNum(yvector<CWord*>& sentWords, int iOriginalWord)
{
    for (int i = 0; (size_t)i < sentWords.size(); i++)
        if (sentWords[i]->GetSourcePair().Contains(iOriginalWord))
            return i;
    return -1;
}

int FindClauseWordNum(yvector<CWord*>& sentWords, CWordsPair& wp)
{
    for (int i = 0; (size_t)i < sentWords.size(); i++)
        if (sentWords[i]->GetSourcePair() == wp)
            return i;
    return -1;
}

// функция возвращает минимальную клаузу, которая содеджит PairToSearch
const_clause_it CClauseVariant::GetMinimalClauseThatContains(const CWordsPair& PairToSearch) const
{
    YASSERT(PairToSearch.LastWord() < (int)m_Words.GetAllWordsCount());
    const_clause_it result_it = GetEndClause();
    for (const_clause_it it = GetFirstClause(); it != GetEndClause(); it++) {
        const CClause& clause = **it;
        if  (clause.Includes(PairToSearch)
                &&      ((result_it == GetEndClause())
                            ||  ((**result_it).Size() >  clause.Size())
                        )
            )
        result_it = it;
    }
    return result_it;
}

const_clause_it CClauseVariant::GetTopClause(const_clause_it MinClauseIt, size_t& Nestedness) const
{
    Nestedness = 0;
    for (const_clause_it MainIt = GetMainClauseConst(MinClauseIt); MainIt != GetEndClause(); MainIt = GetMainClauseConst(MainIt)) {
        MinClauseIt = MainIt;
        Nestedness ++;
    }
    return MinClauseIt;
}

bool ClauseVariantsPred(const CClauseVariant& cl1, const CClauseVariant& cl2)
{
    long bw1 = cl1.GetBadWeight(), bw2 = cl2.GetBadWeight();
    if (bw1 == bw2)
        return cl1.GetClauseCount() > cl2.GetClauseCount();
    else
        return bw1 < bw2;
}
