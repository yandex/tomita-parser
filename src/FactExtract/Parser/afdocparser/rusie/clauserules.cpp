#include "clauserules.h"
#include "topclauses.h"
#include <FactExtract/Parser/afdocparser/rus/gleiche.h>

CClauseRules::SAllRules CClauseRules::m_Rules = CClauseRules::SAllRules();

CClauseRules::SAllRules::SAllRules()
{
    m_Rules[UniteEmptyToSochConj] = SClauseRule(&CClauseRules::UniteEmptyToSochConjRule,    1, CheckJunction, "UniteEmptyToSochConjRule");
    m_Rules[UniteSochConjToEmpty] = SClauseRule(&CClauseRules::UniteSochConjToEmptyRule,    1, CheckJunction, "UniteSochConjToEmptyRule");/**/
    m_Rules[UniteInfWithPredic] = SClauseRule(&CClauseRules::UniteInfWithPredicRule,      1, CheckJunction, "UniteInfWithPredicRule");
    m_Rules[UnitePredicWithInf] = SClauseRule(&CClauseRules::UnitePredicWithInfRule,      1, CheckJunction, "UnitePredicWithInfRule");
    m_Rules[UnitePredicWithSubj] = SClauseRule(&CClauseRules::UnitePredicWithSubjRule, 1, CheckJunction, "UnitePredicWithSubjRule");
    m_Rules[UniteSubjWithPredic] = SClauseRule(&CClauseRules::UniteSubjWithPredicRule, 1, CheckJunction, "UniteSubjWithPredicRule");
    m_Rules[UniteDashClauseWithPredicEllipsis] = SClauseRule(&CClauseRules::UniteDashClauseWithPredicEllipsisRule, 1, CheckJunction, "UniteDashClauseWithPredicEllipsisRule");
    m_Rules[UniteSubConjWithPredic] = SClauseRule(&CClauseRules::UniteSubConjWithPredicRule, 1, CheckJunction, "UniteSubConjWithPredicRule");
    m_Rules[UniteClauseWithBrackets] = SClauseRule(&CClauseRules::UniteClauseWithBracketsRule,1, CheckJunction, "UniteClauseWithBracketsRule");
    m_Rules[UniteEmptyWithDashEmpty] = SClauseRule(&CClauseRules::UniteEmptyWithDashEmptyRule,1, CheckJunction, "UniteEmptyWithDashEmptyRule");
    m_Rules[IncludeKotoryj] = SClauseRule(&CClauseRules::IncludeKotoryjRule,          1, CheckWholeClauses, "IncludeKotoryjRule");
    m_Rules[IncludePredicSubConj] = SClauseRule(&CClauseRules::IncludePredicSubConjRule,    1, CheckWholeClauses, "IncludePredicSubConjRule");
    m_Rules[IncludeSubConjPredic] = SClauseRule(&CClauseRules::IncludeSubConjPredicRule,    1, CheckWholeClauses, "IncludeSubConjPredicRule");
    m_Rules[IncludeParticiple] = SClauseRule(&CClauseRules::IncludeParticipleRule,       1, CheckWholeClauses, "IncludeParticipleRule");
    m_Rules[IncludeSubConjCorr] = SClauseRule(&CClauseRules::IncludeSubConjCorrRule,      1, CheckWholeClauses, "IncludeSubConjCorrRule");
    m_Rules[IncludeAntSubConj] = SClauseRule(&CClauseRules::IncludeAntSubConjRule,       1, CheckWholeClauses, "IncludeAntSubConjRule");
    m_Rules[ConjSubInfs] = SClauseRule(&CClauseRules::ConjSubInfsRule,         1, CheckWholeClauses, "ConjSubInfsRule");
    m_Rules[ConjSimParticiples] = SClauseRule(&CClauseRules::ConjSimParticiplesRule,      1, CheckWholeClauses, "ConjSimParticiplesRule");
    m_Rules[ConjSubPredics] = SClauseRule(&CClauseRules::ConjSubPredicsRule,          1, CheckWholeClauses, "ConjSubPredicsRule");
    m_Rules[ConjSubSimNodes] = SClauseRule(&CClauseRules::ConjSubSimNodesRule,     1, CheckWholeClauses, "ConjSubSimNodesRule");
    m_Rules[ConjSimNodes] = SClauseRule(&CClauseRules::ConjSimNodesRule,            1, CheckWholeClauses, "ConjSimNodesRule");
    m_Rules[ConjSimVerbAdverb] = SClauseRule(&CClauseRules::ConjSimVerbAdverbRule,       1, CheckWholeClauses, "ConjSimVerbAdverbRule");
    m_Rules[IncludeVerbAdverbPredic] = SClauseRule(&CClauseRules::IncludeVerbAdverbPredicRule,1, CheckWholeClauses, "IncludeVerbAdverbPredicRule");
    m_Rules[IncludePredicVerbAdverb] = SClauseRule(&CClauseRules::IncludePredicVerbAdverbRule,1, CheckWholeClauses, "IncludePredicVerbAdverbRule");
    m_Rules[UnitePredicInfWithInf] = SClauseRule(&CClauseRules::UnitePredicInfWithInfRule,   1, CheckJunction, "UnitePredicInfWithInfRule");
    m_Rules[UniteInfWithPredicInf] = SClauseRule(&CClauseRules::UniteInfWithPredicInfRule,   1, CheckJunction, "UniteInfWithPredicInfRule");
    m_Rules[UniteSubConjEmptyNeedEnd] = SClauseRule(&CClauseRules::UniteSubConjEmptyNeedEndRule,    1, CheckJunction, "UniteSubConjEmptyNeedEndRule");
    m_Rules[IncludeParenthesis] = SClauseRule(&CClauseRules::IncludeParenthesisRule,      1, CheckWholeClauses, "IncludeParenthesisRule");
    m_Rules[IncludeBrackets] =  SClauseRule(&CClauseRules::IncludeBracketsRule,     1, CheckWholeClauses, "IncludeBracketsRule");
    m_Rules[IncludeDash] =  SClauseRule(&CClauseRules::IncludeDashRule,         1, CheckWholeClauses, "IncludeDashRule");
    m_Rules[IncludeColon] = SClauseRule(&CClauseRules::IncludeColonRule,            1, CheckWholeClauses, "IncludeColonRule");
    m_Rules[IncludeSubConjEmpty] =  SClauseRule(&CClauseRules::IncludeSubConjEmptyRule, 1, CheckWholeClauses, "IncludeSubConjEmptyRule");
}

SClauseRule& CClauseRules::SAllRules::operator[](ERuleType rule)
{
    return m_Rules[rule];
}

SClauseRuleResult::SClauseRuleResult()
{
    m_bClone = false;
    iTry = 0;
}

CClauseRules::CClauseRules(CClauseStructure& ClauseVariant)
    : m_Clauses(ClauseVariant)
    , m_Words(ClauseVariant.m_Words)
{
}

bool CClauseRules::ApplyRule(ERuleType RuleType, clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    YASSERT(RuleType >= 0 && RuleType < RuleTypeCount);
    SClauseRule& ClauseRule = m_Rules[RuleType];
    return  ((*this).*(ClauseRule.m_Rule))(it1, it2, res);
}

EStopRulesOption CClauseRules::GetRuleStopOptions(ERuleType RuleType) const
{
    YASSERT(RuleType >= 0 && RuleType < RuleTypeCount);
    return m_Rules[RuleType].m_StopOption;
}

EStopRulesOption CClauseRules::GetRuleAlreadyAppliedOptions(ERuleType RuleType) const
{
    YASSERT(RuleType >= 0 && RuleType < RuleTypeCount);
    return m_Rules[RuleType].m_OptionForAlreadyApplied;
}

bool CClauseRules::UniteSubConjEmptyNeedEndRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        if ((clause1.GetWordsCount() == 0) || (clause2.GetWordsCount() == 0))
            return false;

        int iSubConjEmptyNeedEndType = clause1.HasType_i(SubConjEmptyNeedEnd);
        if (iSubConjEmptyNeedEndType == -1)
            return false;
        int iEmpty = clause2.HasType_i(Empty);
        if (iEmpty != -1) {
            if (clause2.HasPunctAtTheVeryEnd()) {
                res.m_ClauseFuncArg.m_NewType = clause1.GetType(iSubConjEmptyNeedEndType);
                res.m_ClauseFuncArg.m_NewType.m_Type = SubConjEmpty;
            }

            res.m_ClauseFuncArg.m_bMainFirst = true;
            res.m_Operation = UniteClausesOp;
            return true;
        } else {
            res.m_ClauseFuncArg.m_bMainFirst = false;
            res.m_ClauseFuncArg.m_bRefreshClauseInfo = true;
            res.m_Operation = UniteClausesOp;
            return true;

        }
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteSubConjEmptyNeedEndRule");
    }*/
}

bool CClauseRules::UniteClauseWithBracketsRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        if ((clause1.GetWordsCount() == 0) || (clause2.GetWordsCount() == 0))
            return false;

        CClause* pInCl = m_Clauses.GetIncludedClauseR(it1);

        if (pInCl == NULL)
            return false;

        CClause& brackets_clause = *pInCl;

        if (!brackets_clause.HasType(Brackets))
            return false;

        if (clause1.HasPunctAtTheVeryEnd())
            return false;

        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_bRefreshClauseInfo = true;

        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteClauseWithBracketsRule");
    }*/
}

bool CClauseRules::UniteEmptyWithDashEmptyRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        if (!clause1.HasDash())
            return false;
        if (clause2.HasDash())
            return false;
        int iEmptyType1 = clause1.HasEmptyType();
        if (iEmptyType1 == -1)
            iEmptyType1 = clause1.HasType_i(NeedNode);
        int iEmptyType2 = clause2.HasEmptyType();
        if ((iEmptyType1 == -1) || (iEmptyType2 == -1))
            return false;
        SWordHomonymNum noun1 = clause1.FindNounNominative();
        if (!noun1.IsValid())
            return false;
        SWordHomonymNum noun2 = clause2.FindNounNominative();
        if (!noun2.IsValid())
            return false;
        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_NewType.m_Type = DashClause;
        res.m_ClauseFuncArg.m_NewType.m_NodeNum = noun2;

        res.m_ClauseFuncArg.m_NewType.m_NodePOS = TGramBitSet(gSubstantive);

        SValency Val;
        Val.m_ActantType = WordActant;
        Val.m_ValNum = 0;
        Val.m_Rel = Subj;
        Val.m_Actant = noun1;

        res.m_ClauseFuncArg.m_NewType.m_Vals.push_back(Val);

        res.m_ClauseFuncArg.m_DelTypes1.push_back(iEmptyType1);
        res.m_ClauseFuncArg.m_DelTypes2.push_back(iEmptyType2);
        res.m_ClauseFuncArg.m_bMainFirst = true;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteEmptyWithDashEmptyRule");
    }*/
}

bool CClauseRules::UniteEmptyToSochConjRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{

    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        //cout << m_Words.GetOriginalWord(clause1.FirstWord()).GetOriginalWordStr() << " -- " << m_Words.GetOriginalWord(clause1.LastWord()).GetOriginalWordStr() << endl;

        if ((clause1.GetWordsCount() == 0) || (clause2.GetWordsCount() == 0))
            return false;

        if ((clause2.HasType_i(Parenth) != -1) || (clause2.HasType_i(Brackets) != -1))
            return false;

        const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
        if (corr_word.HasPOS(gCorrelateConj))
            return false;

        bool bPunctAndSimConj = false;
        bool bSubConjInTheMiddle = false;
        bool bSimilar = false;

        //cout <<    "UniteEmptyToSochConjRule-1 ";
        if (!CanUniteClauses(it1, it2, bPunctAndSimConj, bSubConjInTheMiddle)) {

            if (bPunctAndSimConj) {
                //cout <<    "UniteEmptyToSochConjRule-2 ";
                bSimilar = CheckClausesForSimilar(it1, it2, false, true);
                if (!bSimilar)
                    return false;
            } else if (bSubConjInTheMiddle) {
                    //cout <<    "UniteEmptyToSochConjRule-3 ";
                    bSimilar = CheckClausesForSimilar(it1, it2, false);
                } else
                    return false;
        } else {
            //cout <<    "UniteEmptyToSochConjRule-4 " << endl;
            bSimilar = CheckClausesForSimilar(it1, it2, false);
        }

        if (bSubConjInTheMiddle && !bSimilar)
            return false;

        //cout <<    "UniteEmptyToSochConjRule-5 ";
        int iEmptyType = clause1.HasEmptyType();
        /*if( iEmptyType == -1 )
            iEmptyType = clause1.HasTypeWithPOS_i(gInfinitive);*/

        if (iEmptyType == -1)
            return false;

        int iEmptyType2 = clause2.HasType_i(Empty);

        //cout <<    "UniteEmptyToSochConjRule-6 ";
        if (iEmptyType2 != -1 && clause1.HasTypeWithPOS(gInfinitive))
            return false;
            //тут плохо это правило применять, так как может затеряться
            //тип Infinitive у первой клаузы (здесь по умолчанию главной становится вторая клауза)

        ////cout <<    "UniteEmptyToSochConjRule-3 ";
        int iSimConj = clause2.HasSimConj_i();

        if (((clause2.m_ClauseWords.size() == 1) ||
            ((clause2.m_ClauseWords.size() == 2) && clause2.HasPunctAtTheEnd()))
            && (iSimConj != -1))
            return false;

        //cout <<    "UniteEmptyToSochConjRule-7 ";
        //bool bPartType1  = clause1.HasTypeWithPos(gParticiple) || clause1.HasTypeWithPos(gGerund);            // unused!

        bool bPartType2  =  clause2.HasTypeWithPOS(gParticiple) || clause2.HasTypeWithPOS(gGerund);

        /*if( !bSimilar && bPartType2 )
            return false;*/

        //cout <<    "UniteEmptyToSochConjRule-8 ";
        bool bHAsIncluddeParenthClause = false;
        CClause* pInCl = m_Clauses.GetIncludedClauseL(it2);
        if (pInCl != NULL) {
            CClause& cl = *pInCl;
            bHAsIncluddeParenthClause = cl.HasType(Parenth) || cl.HasType(Brackets);
        }

        if (bPartType2  && (iSimConj == -1) && !bHAsIncluddeParenthClause)
            return false;

        //cout <<    "UniteEmptyToSochConjRule-9 ";
        if (!bSimilar && (res.iTry == 0)) {
            res.m_bClone = true;
            res.m_StopRules.push_back(UniteSochConjToEmpty);
        }

        ////cout <<    "UniteEmptyToSochConjRule-5 ";
        bool bHasIncl = m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2);

        //cout <<    "UniteEmptyToSochConjRule-10 ";
        if (!bSimilar) {
            if (iSimConj == -1) {
                if (!bHasIncl)
                    res.m_ClauseFuncArg.m_BadWeightForCommas++;
            } else {
                const CHomonym& conj = clause2.GetConj(iSimConj);
                if (SConjType::IsSimConjAnd(conj))
                    res.m_ClauseFuncArg.m_BadWeight++;
                else
                    res.m_ClauseFuncArg.m_BadWeightForCommas += 3;
            }
        }

        //cout <<    "UniteEmptyToSochConjRule-11 ";
        if (bHasIncl)/****/
        {
            res.m_bClone = true;
            res.m_StopRules.push_back(UniteSochConjToEmpty);
        }

        if (clause2.HasSubConj() && clause1.HasPunctAtTheEnd() && clause2.HasType(Empty)) {
            res.m_bClone = false;
            res.m_ClauseFuncArg.m_BadWeight = 0;
            res.m_ClauseFuncArg.m_BadWeightForCommas = 0;
            res.m_ClauseFuncArg.m_bCloneByType2 = false;
        }

        bool bBeforePredic;

        //cout <<    "UniteEmptyToSochConjRule-12 ";
        if (CheckForDisruptedConj(it1, it2, res, bBeforePredic)) {
            if (!bSimilar)
                res.m_ClauseFuncArg.m_BadWeightForCommas = 1;
            else
                res.m_ClauseFuncArg.m_BadWeightForCommas = 0;
        }

        //cout <<    "UniteEmptyToSochConjRule-13 ";

        int iPartType = clause2.HasTypeWithPOS_i(gParticiple);
        if ((iPartType != -1) && (clause1.FirstWord() == 0)) {
            res.m_ClauseFuncArg.m_bRefreshClauseInfo = true;
        }
        ////cout <<    "UniteEmptyToSochConjRule-7 ";

        res.m_Operation = UniteClausesOp;
        //int iInfType = clause1.HasTypeWithPOS_i(gInfinitive);
        //if( (iEmptyType2 != -1) && (iInfType != -1) )
        //{
        //  res.m_ClauseFuncArg.m_bMainFirst = true;
        //  res.m_ClauseFuncArg.m_iClause2TypeNum = iEmptyType2;
        //}
        //else
        //{
        //cout <<    "UniteEmptyToSochConjRule-14 ";
        res.m_ClauseFuncArg.m_bMainFirst = false;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iEmptyType;
        //}
        ////cout <<    "UniteEmptyToSochConjRule-8 ";

        return true;

    }
    /*catch(std::out_of_range& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CClauseRules::EmptyToSochConjRule (%s)", e.what());
        ythrow yexception() << (const char*)strMsg;
    }
    catch(std::logic_error& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CClauseRules::EmptyToSochConjRule (%s)", e.what());
        ythrow yexception() << (const char*)strMsg;
    }

    catch(...)
    {
        //Stroka strMsg;
        //strMsg.Format("Error in CClauseRules::EmptyToSochConjRule (stage  {%s})", (const char*)strStage);
        //throw yexception((const char*)strMsg);
        ythrow yexception() << "Error in CClauseRules::EmptyToSochConjRule";
    }*/
}

bool CClauseRules::UniteDashClauseWithPredicEllipsisRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        clause_node it0;
        if (!m_Clauses.GetNeighbourClauseL(it1, it0))
            return false;
        CClause& clause0 = m_Clauses.GetClauseRef(it0);
        if (clause1.HasSubConj())
            return false;
        if (!clause1.HasSimConj())
            return false;
        if (!clause0.HasPunctAtTheVeryEnd())
            return false;
        if (clause0.HasDash())
            return false;
        if (!clause1.HasDash())
            return false;
        if (clause2.HasDash())
            return false;
        int iEmptyType1 = clause1.HasEmptyType();
        int iEmptyType2 = clause2.HasEmptyType();
        if ((iEmptyType1 == -1) || (iEmptyType2 == -1))
            return false;
        int i;
        for (i = 0; i < clause0.GetTypesCount(); i++) {
            if (clause0.GetType(i).HasSubjVal())
                if (CoordDashClausesWithSubj(clause0, clause1, clause2, i))
                    break;
            if (CoordDashClauses(clause0, clause1, clause2, i))
                break;
        }

        if (i >= clause0.GetTypesCount())
            return false;

        res.m_Operation = UniteClausesOp;
        if (clause0.GetType(i).HasPOS(gGerund)) {
            res.m_ClauseFuncArg.m_NewType.m_Type = VerbAdverbEllipsis;
        } else if (clause0.GetType(i).HasPOS(gParticiple)) {
            res.m_ClauseFuncArg.m_NewType.m_Type = ParticipleEllipsis;
        } else
            res.m_ClauseFuncArg.m_NewType.m_Type = Ellipsis;
        /*res.m_ClauseFuncArg.m_NewType.m_NodeNum = noun1;
        res.m_ClauseFuncArg.m_NewType.m_NodePos = Noun;*/
        res.m_ClauseFuncArg.m_DelTypes1.push_back(iEmptyType1);
        res.m_ClauseFuncArg.m_DelTypes2.push_back(iEmptyType2);
        res.m_ClauseFuncArg.m_bMainFirst = true;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteDashClauseWithPredicEllipsisRule");
    }*/

}

bool CClauseRules::CoordDashClauses(CClause& clause0, CClause& clause1, CClause& clause2, int iType)
{
    //try
    {
        SClauseType& type = clause0.GetType(iType);

        if (!type.HasPOS(gGerund) && !type.HasPOS(gParticiple)) {
            int iSubjVal =  type.HasSubjVal_i();
            if (iSubjVal == -1)
                return false;
        }

        bool bFoundFirst = false;
        bool bFoundSecond = false;
        size_t i;
        for (i = 0; i < clause0.m_ClauseWords.size(); i++) {
            CWord& word0 = m_Words.GetWord(clause0.m_ClauseWords[i]);
            int iHom = word0.HasMorphNoun_i();
            if (iHom == - 1)
                continue;
            CHomonym& hom0 = word0.GetRusHomonym(iHom);
            if (!bFoundFirst) {
                size_t j;
                for (j = 0; j < clause1.m_ClauseWords.size(); j++) {
                    CWord& word1 = m_Words.GetWord(clause1.m_ClauseWords[j]);
                    int iHom1 = word1.HasMorphNoun_i();
                    if (iHom1 == - 1)
                        continue;
                    CHomonym& hom1 = word1.GetRusHomonym(iHom1);
                    if (NGleiche::Gleiche(hom1, hom0, NGleiche::CaseCheck))
                        break;
                }
                if (j < clause1.m_ClauseWords.size())
                    bFoundFirst = true;
            } else {
                size_t j;
                for (j = 0; j < clause2.m_ClauseWords.size(); j++) {
                    CWord& word2 = m_Words.GetWord(clause2.m_ClauseWords[j]);
                    int iHom2 = word2.HasMorphNoun_i();
                    if (iHom2 == - 1)
                        continue;
                    CHomonym& hom2 = word2.GetRusHomonym(iHom2);
                    if (NGleiche::Gleiche(hom2, hom0, NGleiche::CaseCheck))
                        break;
                }

                if (j < clause2.m_ClauseWords.size()) {
                    bFoundSecond = true;
                    break;
                }
            }

            if (bFoundFirst && bFoundSecond)
                break;
        }
        if (i < clause0.m_ClauseWords.size())
            return true;

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CoordDashClauses");
    }*/
}

bool CClauseRules::CoordDashClausesWithSubj(CClause& clause0, CClause& clause1, CClause& clause2, int iType)
{
    //try
    {
        SClauseType& type = clause0.GetType(iType);

        if ((type.m_Modality != Possibly) ||
            (type.m_Type != WantsVal))
            return false;
        int iSubjVal =  type.HasSubjVal_i();
        if (iSubjVal == -1)
            return false;
        SValency& val = type.m_Vals[iSubjVal];
        if (!val.m_Actant.IsValid())
            return false;
        if (val.m_ActantType != WordActant)
            return false;
        CWord& word_subj  = m_Words.GetWord(val.m_Actant);
        const CHomonym& hom  = m_Words[val.m_Actant];
        SWordHomonymNum hom_num;
        if (hom.HasGrammem(gNominative)) {
            hom_num = clause1.FindNounNominative();
            if (!hom_num.IsValid())
                return false;
        } else {
            size_t i;
            for (i = 0; i < clause1.m_ClauseWords.size(); i++) {
                CWord& word = m_Words.GetWord(clause1.m_ClauseWords[i]);
                int iHom = word.HasMorphNoun_i();
                if (iHom == - 1)
                    continue;
                if (NGleiche::Gleiche(hom, word.GetRusHomonym(iHom), NGleiche::CaseCheck)) {
                    hom_num = clause1.m_ClauseWords[i];
                    hom_num.m_HomNum = iHom;
                    break;
                }
            }
            if (i >=  clause1.m_ClauseWords.size())
                return false;
        }

        size_t i;
        for (i = 0; i < clause2.m_ClauseWords.size(); i++) {
            const CWord& word = m_Words.GetWord(clause2.m_ClauseWords[i]);
            int iHom2 = word.HasMorphNoun_i();
            if (iHom2 == -1)
                continue;
            const CHomonym& hom2 = word.GetRusHomonym(iHom2);
            size_t j;
            for (j = 0; j < clause0.m_ClauseWords.size(); j++) {
                const CWord& word = m_Words.GetWord(clause0.m_ClauseWords[j]);
                if (word_subj.GetSourcePair().LastWord() >= word.GetSourcePair().LastWord())
                    continue;
                int iHom0 = word.HasMorphNoun_i();
                if (iHom0 == -1)
                    continue;
                if (NGleiche::Gleiche(word.GetRusHomonym(iHom0), hom2, NGleiche::CaseCheck))
                    break;
            }
            if (j < clause0.m_ClauseWords.size())
                break;
        }

        if (i < clause2.m_ClauseWords.size())
            return true;

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CoordDashClausesWithSubj");
    }*/

}

void CClauseRules::CheckBadWeightForUnitedClauses(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {

        bool bHasIncl = m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2);
        int iSimConj = (m_Clauses.GetClauseRef(it2)).HasSimConj_i();

        if (!CheckClausesForSimilar(it1, it2, false)) {
            if (iSimConj == -1) {
                if (!bHasIncl)
                    res.m_ClauseFuncArg.m_BadWeightForCommas++;
            } else {
                const CHomonym&  conj = (m_Clauses.GetClauseRef(it2)).GetConj(iSimConj);
                if (SConjType::IsSimConjAnd(conj)) {
                    if (!(m_Clauses.GetClauseRef(it1)).HasPunctAtTheEnd())
                        res.m_ClauseFuncArg.m_BadWeight++;
                    else
                        res.m_ClauseFuncArg.m_BadWeightForCommas += 2;
                } else
                    res.m_ClauseFuncArg.m_BadWeightForCommas += 3;
            }
        }

        bool bBeforePredic;
        CheckForDisruptedConj(it1, it2, res, bBeforePredic);
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CheckBadWeightForUnitedClauses");
    }*/

}

bool CClauseRules::CheckForDisruptedConj(clause_node it1, clause_node it2, SClauseRuleResult& res, bool& bBeforePredic)
{
    //try
    {
        bBeforePredic = false;

        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        int iSimConj = clause2.HasSimConjWithCorr_i();
        if (iSimConj == -1)
            return false;

        const CHomonym& sim_conj = clause2.GetConj(iSimConj);
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(sim_conj, CONJ_DICT);
        int iCorrsCount = piArt->get_corrs_count();

        if (iCorrsCount == 0)
            return false;

        int i;
        for (i = clause1.m_ClauseWords.size() - 1; i >= 0; i--) {
            const CWord& word = m_Words.GetWord(clause1.m_ClauseWords[i]);
            int iConjHom = word.HasPOSi(gCorrelateConj);
            if (iConjHom != -1) {
                const CHomonym& conj = word.GetRusHomonym(iConjHom);
                const article_t* piConjArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);
                int j = 0;
                for (; j < iCorrsCount; j++) {
                    if (piConjArt->get_title() == piArt->get_corr(j))
                        break;
                }
                if (j < iCorrsCount)
                    break;
            }
        }

        bool bRes = false;

        if ((i >= 0) && ((size_t)i < clause1.m_ClauseWords.size())) {
            for (size_t j = 0; j < clause1.m_Types.size(); j++) {
                if ((clause1.GetType(j).m_Type == HasSubject) ||
                    (clause1.GetType(j).m_Type == NeedSubject) ||
                    (clause1.GetType(j).m_Type == Impersonal) ||
                    (clause1.GetType(j).m_Type == WantsVal) ||
                    (clause1.GetType(j).m_Type == WantsNothing))
                    if (clause1.GetType(j).m_NodeNum.IsValid())
                        if (m_Words.GetWord(clause1.GetType(j).m_NodeNum).GetSourcePair().FirstWord() >  m_Words.GetWord(clause1.m_ClauseWords[i]).GetSourcePair().FirstWord())
                            bBeforePredic = true;
            }

            res.m_bClone = false;
            bRes = true;
        } else
            res.m_ClauseFuncArg.m_BadWeightForCommas++;

        return bRes;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CheckForDisruptedConj");
    }*/

}

bool CClauseRules::UniteSochConjToEmptyRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause1.HasType_i(Brackets) != -1)
            return false;

        if ((clause2.GetWordsCount() == 0) || (clause1.GetWordsCount() == 0))
            return false;

        bool bSimilar = false;
        bool bPunctAndSimConj = false;
        bool bb;
        if (!CanUniteClauses(it1, it2, bPunctAndSimConj, bb)) {
            if (bPunctAndSimConj) {
                bSimilar = CheckClausesForSimilar(it1, it2, true, true);
                if (!bSimilar)
                    return false;
            } else
                return false;
        } else
            bSimilar = CheckClausesForSimilar(it1, it2, true);

        int iEmptyType = clause2.HasEmptyType();

        if (iEmptyType == -1)
            return false;

        int iSimConj = clause2.HasSimConj_i();
        if (((clause2.m_ClauseWords.size() == 1) ||
            ((clause2.m_ClauseWords.size() == 2) && (clause2.HasPunctAtTheEnd()))) &&
            (iSimConj != -1))
            return false;

        bool bPartType  = clause2.HasTypeWithPOS(gParticiple) || clause2.HasTypeWithPOS(gGerund);

        SClauseRuleResult r;

        if (clause2.HasTypeWithPOS(gParticiple))
            bPartType = IncludeParticipleRule(it1, it2, r);

        if (!bSimilar && bPartType)
            return false;

        int iParenthType = clause1.HasType_i(Parenth);

        if (iParenthType != -1)
            if (!(bSimilar &&  (iSimConj != -1)))
                return false;

        if (!bSimilar && (res.iTry == 0)) {
            res.m_bClone = true;
            res.m_StopRules.push_back(UniteEmptyToSochConj);
        }

        bool bHasIncl = m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2);

        if (!bSimilar) {
            if (iSimConj == -1) {
                if (!bHasIncl)
                    res.m_ClauseFuncArg.m_BadWeightForCommas++;
            } else {
                const CHomonym& conj = clause2.GetConj(iSimConj);
                if (SConjType::IsSimConjAnd(conj))
                    res.m_ClauseFuncArg.m_BadWeight++;
                else
                    res.m_ClauseFuncArg.m_BadWeightForCommas += 3;
            }
        }

        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iEmptyType;

        const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
        if (corr_word.HasPOS(gCorrelateConj)) {
            if (HasWordAsConjCorr(clause1, corr_word))
                return false;
            res.m_bClone = true;
            res.m_ClauseFuncArg.m_BadWeight++;
        }

        if (clause2.HasSubConj() && clause1.HasPunctAtTheEnd()) {
            res.m_bClone = false;
            res.m_ClauseFuncArg.m_BadWeight = 0;
        }

        bool bBeforePredic;

        if (CheckForDisruptedConj(it1, it2, res, bBeforePredic)) {
            if (!bSimilar)
                res.m_ClauseFuncArg.m_BadWeightForCommas = 1;
            else
                res.m_ClauseFuncArg.m_BadWeightForCommas = 0;
        }

        if (!bSimilar)
            res.m_bClone = true;

        if (bBeforePredic)
            res.m_ClauseFuncArg.m_BadWeight++;
        /*if( bSimilar )
            res.m_ClauseFuncArg.m_bCloneByType2 = false;*/

        return true;
    }
    /*catch(std::out_of_range& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CClauseRules::SochConjToEmptyRule (%s)", e.what());
        ythrow yexception() << (const char*)strMsg;
    }
    catch(std::logic_error& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CClauseRules::SochConjToEmptyRule (%s)", e.what());
        ythrow yexception() << (const char*)strMsg;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::SochConjToEmptyRule");
    }*/
}

bool CClauseRules::FindNodeValWhichWantsSubConjClause(CClause& clause, SClauseRuleResult& res, bool bCl1, const CHomonym& conj)
{
    //try
    {
        for (size_t i = 0; i < clause.m_Types.size(); i++) {
            if (!clause.GetType(i).m_NodeNum.IsValid())
                continue;

            if (!bCl1)
                if (clause.GetType(i).HasPOS(gParticiple) || clause.GetType(i).HasPOS(gGerund))
                    continue;

            const article_t* piArt = GlobalDictsHolder->GetAuxArticle(m_Words[clause.GetType(i).m_NodeNum], PREDIC_DICT);

            for (size_t j = 0; j < clause.GetType(i).m_Vals.size(); j++) {
                if (clause.GetType(i).m_Vals[j].IsValid())
                    continue;

                const or_valencie_list_t& piValVars = piArt->get_valencies().at(j);
                size_t k;
                for (k = 0; k < piValVars.size(); k++) {
                    const valencie_t& piVal = piValVars[k];
                    if (piVal.m_actant_type != Fragment)
                        continue;
                    if (CompValWithSubConj(piValVars, conj))
                        break;
                }

                if (k < piValVars.size()) {
                    SValency Val;
                    Val.m_ActantType = WordActant;
                    Val.m_ValNum = j;
                    Val.m_Rel = piValVars[k].m_syn_rel;
                    if (bCl1)
                        res.m_ClauseFuncArg.m_iClause1TypeNum = i;
                    else
                        res.m_ClauseFuncArg.m_iClause2TypeNum = i;
                    res.m_ClauseFuncArg.m_NewValencies.push_back(Val);
                    return true;
                }
            }
        }
        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::FindNodeValWhichWantsSubConjClause");
    }*/
}

bool CClauseRules::FindNodeValWhichWantsSubj(CClause& clause, SClauseRuleResult& res, bool bCl1, or_valencie_list_t& piValVarsRes)
{
    //try
    {
        for (size_t i = 0; i < clause.m_Types.size(); i++) {
            if (!clause.GetType(i).m_NodeNum.IsValid())
                continue;

            const article_t* piArt = GlobalDictsHolder->GetAuxArticle(m_Words[clause.GetType(i).m_NodeNum], PREDIC_DICT);

            for (size_t j = 0; j < clause.GetType(i).m_Vals.size(); j++) {
                if (clause.GetType(i).m_Vals[j].IsValid())
                    continue;

                const or_valencie_list_t& piValVars = piArt->get_valencies().at(j);
                size_t k;
                for (k = 0; k < piValVars.size(); k++) {
                    const valencie_t& piVal = piValVars[k];
                    if (piVal.m_actant_type != WordActant)
                        continue;

                    if ((piVal.m_syn_rel == Subj) ||
                        (piVal.m_syn_rel == SubjGen) ||
                        (piVal.m_syn_rel == SubjDat))
                        break;
                }

                if (k < piValVars.size()) {
                    SValency Val;
                    Val.m_ActantType = WordActant;
                    Val.m_Rel = piValVars[k].m_syn_rel;
                    Val.m_ValNum = j;
                    if (bCl1)
                        res.m_ClauseFuncArg.m_iClause1TypeNum = i;
                    else
                        res.m_ClauseFuncArg.m_iClause2TypeNum = i;
                    piValVarsRes = piValVars;
                    res.m_ClauseFuncArg.m_NewValencies.push_back(Val);
                    return true;
                }
            }
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::FindNodeValWhichWantsSubj");
    }*/
}

bool CClauseRules::FindNodeValWhichWantsInf(CClause& clause, SClauseRuleResult& res, bool bCl1)
{
    //try
    {
        for (size_t i = 0; i < clause.m_Types.size(); i++) {
            if (!clause.GetType(i).m_NodeNum.IsValid())
                continue;

            const article_t* piArt = GlobalDictsHolder->GetAuxArticle(m_Words[clause.GetType(i).m_NodeNum], PREDIC_DICT);

            for (size_t j = 0; j < clause.GetType(i).m_Vals.size(); j++) {
                if (clause.GetType(i).m_Vals[j].IsValid())
                    continue;

                const or_valencie_list_t& piValVars = piArt->get_valencies().at(j);
                size_t k;
                for (k = 0; k < piValVars.size(); k++) {
                    const valencie_t& piVal = piValVars[k];
                    if (piVal.m_actant_type != WordActant)
                        continue;
                    if (piVal.m_POSes.find(TGramBitSet(gInfinitive)) != piVal.m_POSes.end())
                        break;
                }

                if (k < piValVars.size()) {
                    SValency Val;
                    Val.m_ActantType = WordActant;
                    Val.m_Rel = piValVars[k].m_syn_rel;
                    Val.m_ValNum = j;
                    if (bCl1)
                        res.m_ClauseFuncArg.m_iClause1TypeNum = i;
                    else
                        res.m_ClauseFuncArg.m_iClause2TypeNum = i;
                    res.m_ClauseFuncArg.m_NewValencies.push_back(Val);
                    return true;
                }
            }
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::FindNodeValWhichWantsInf");
    }*/

}

bool CClauseRules::UniteInfWithPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.HasSubConj())
            return false;

        int iInfType = clause1.HasTypeWithPOS_i(gInfinitive);

        if (iInfType == -1)
            return false;

        if (!FindNodeValWhichWantsInf(clause2, res, false))
            return false;

        res.m_ClauseFuncArg.m_bMainFirst = false;
        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iInfType;
        res.m_ClauseFuncArg.m_NewValencies[0].m_Actant = clause1.GetType(iInfType).m_NodeNum;

        int iSubConj = clause1.HasSubConj_i();

        if (iSubConj != -1)
            if (clause1.m_Conjs[iSubConj].m_Val.m_Actant ==  clause1.GetType(iInfType).m_NodeNum)
                res.m_bClone = true;

        CheckBadWeightForUnitedClauses(it1, it2, res);

        if (clause1.HasSubConj() && (clause1.GetType(iInfType).m_Type != NeedNode))
            res.m_bClone = true;

        return true;
    }
    /*catch(std::out_of_range& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CClauseRules::UniteInfWithPredicRule (%s)", e.what());
        ythrow yexception() << (const char*)strMsg;
    }
    catch(std::logic_error& e)
    {
        Stroka strMsg;
        strMsg.Format("Error in CClauseRules::UniteInfWithPredicRule (%s)", e.what());
        ythrow yexception() << (const char*)strMsg;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteInfWithPredicRule");
    }*/
}

bool CClauseRules::UnitePredicWithInfRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.HasSubConj())
            return false;

        int iWantsType = clause1.HasType_i(WantsVal);
        if (iWantsType == -1)
            return false;

        int iInfType = clause2.HasTypeWithPOS_i(gInfinitive);

        if (iInfType == -1)
            return false;

        if (!FindNodeValWhichWantsInf(clause1, res, true))
            return false;

        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iInfType;
        res.m_ClauseFuncArg.m_NewValencies[0].m_Actant = clause2.GetType(iInfType).m_NodeNum;

        CheckBadWeightForUnitedClauses(it1, it2, res);

        if (clause1.HasSubConj() && (clause2.GetType(iInfType).m_Type != NeedNode))
            res.m_bClone = true;

        return true;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::UnitePredicWithInfRule");
    }*/
}

bool CClauseRules::UnitePredicWithSubjRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (!CanUniteClauses(it1, it2))
            return false;

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        if (clause2.HasSubConj())
            return false;

        int iWantsType = clause1.HasType_i(WantsVal);
        if (iWantsType == -1)
            return false;

        int iEmptyType = clause2.HasType_i(Empty);
        if (iEmptyType == -1) {
            iEmptyType = clause2.HasType_i(NeedNode);
            if (iEmptyType == -1)
                return false;
        }

        or_valencie_list_t piValVars;
        if (!FindNodeValWhichWantsSubj(clause1, res, true, piValVars))
            return false;

        SWordHomonymNum node = clause1.GetType(res.m_ClauseFuncArg.m_iClause1TypeNum).m_NodeNum;
        SWordHomonymNum actant;
        size_t i;
        for (i = 0; i < piValVars.size(); i++) {
            valencie_t& piVal = piValVars[i];
            if (piVal.m_actant_type != WordActant)
                continue;

            if (piVal.m_syn_rel != Subj)
                continue;

            if (clause2.FillValency(node, piVal, actant))
                break;
        }

        if (i >= piValVars.size())
            return false;

        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iEmptyType;
        res.m_ClauseFuncArg.m_CoefficientToIncreaseBadWeightForCommas = 2;
        res.m_ClauseFuncArg.m_PairForBadCoef.SetPair(m_Words.GetWord(node).GetSourcePair().FirstWord(),
                                                     m_Words.GetWord(actant).GetSourcePair().LastWord());
        res.m_ClauseFuncArg.m_NewValencies[0].m_Actant = actant;

        CheckBadWeightForUnitedClauses(it1, it2, res);

        return true;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::UnitePredicWithSubjRule");
    }*/
}

bool CClauseRules::UniteSubjWithPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (!CanUniteClauses(it1, it2))
            return false;

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        if (clause2.HasSubConj())
            return false;

        int iEmptyType = clause1.HasType_i(Empty);
        if (iEmptyType == -1) {
            iEmptyType = clause1.HasType_i(NeedNode);
            if (iEmptyType == -1)
                return false;
        }

        int iWantsType = clause2.HasType_i(WantsVal);
        if (iWantsType == -1)
            return false;

        or_valencie_list_t piValVars;
        if (!FindNodeValWhichWantsSubj(clause2, res, false, piValVars))
            return false;

        SWordHomonymNum node = clause2.GetType(res.m_ClauseFuncArg.m_iClause2TypeNum).m_NodeNum;
        SWordHomonymNum actant;
        size_t i;
        for (i = 0; i < piValVars.size(); i++) {
            valencie_t& piVal = piValVars[i];
            if (piVal.m_actant_type != WordActant)
                continue;

            if ((piVal.m_syn_rel != Subj)    &&
                (piVal.m_syn_rel != SubjDat) &&
                (piVal.m_syn_rel != SubjGen))
                continue;

            if (clause1.FillValency(node, piVal, actant))
                break;
        }

        if (i >= piValVars.size())
            return false;

        res.m_ClauseFuncArg.m_bMainFirst = false;
        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iEmptyType;
        res.m_ClauseFuncArg.m_CoefficientToIncreaseBadWeightForCommas = 2;
        res.m_ClauseFuncArg.m_PairForBadCoef.SetPair(m_Words.GetWord(actant).GetSourcePair().FirstWord(),
                                                     m_Words.GetWord(node).GetSourcePair().LastWord());
        res.m_ClauseFuncArg.m_NewValencies[0].m_Actant = actant;

        CheckBadWeightForUnitedClauses(it1, it2, res);

        return true;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteSubjWithPredicRule");
    }*/
}

bool CClauseRules::UniteSubConjWithPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if ((clause1.GetWordsCount() == 0) || (clause2.GetWordsCount() == 0))
            return false;

        if (clause2.HasSubConj())
            return false;

        int iSubConj = clause1.HasSubConj_i();

        int iNeedNodeType = clause1.HasType_i(NeedNode);
        if (iNeedNodeType == -1)
            return false;

        if (iSubConj == -1)
            return false;

        const CHomonym& conj = m_Words[clause1.m_Conjs[iSubConj].m_WordNum];

        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);
        if (piArt->get_conj_valencies().size() != 1)
            return false;

        const or_valencie_list_t& piValVars = piArt->get_conj_valencies().at(0);

        for (size_t i = 0; i < clause2.m_Types.size(); i++) {
            if (clause2.GetType(i).m_NodePOS.any() && !clause2.GetType(i).HasPOS(gSubstantive))
                if (clause2.CompTypeWithConjVals(clause2.GetType(i), piValVars)) {
                    res.m_Operation = UniteClausesOp;
                    res.m_ClauseFuncArg.m_bMainFirst = false;
                    res.m_ClauseFuncArg.m_iClause1TypeNum = iNeedNodeType;
                    res.m_ClauseFuncArg.m_iClause2TypeNum = i;
                    res.m_ClauseFuncArg.m_iConj = iSubConj;
                    res.m_ClauseFuncArg.m_ConjValency.m_Actant = clause2.GetType(i).m_NodeNum;
                    res.m_ClauseFuncArg.m_ConjValency.m_ValNum = 0;
                    if (!m_Clauses.HasIncludedClauseL(it2) && !m_Clauses.HasIncludedClauseR(it1)) {
                        res.m_ClauseFuncArg.m_CoefficientToIncreaseBadWeightForCommas = 2;
                        res.m_ClauseFuncArg.m_PairForBadCoef.SetPair(
                            m_Words.GetWord(clause1.m_Conjs[iSubConj].m_WordNum).GetSourcePair().FirstWord(),
                                            m_Words.GetWord(clause2.GetType(i).m_NodeNum).GetSourcePair().LastWord());
                    }
                    CheckBadWeightForUnitedClauses(it1, it2, res);

                    return true;
                }
        }

        return false;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteSubConjWithPredicRule");
    }*/
}

bool CClauseRules::HasWordAsConjCorr(CClause& clause, const CWord& corr_word)
{
    //try
    {
        int iCorrHom = corr_word.HasPOSi(gCorrelateConj);
        if (iCorrHom == -1)
            return false;

        const CHomonym& corr = corr_word.GetRusHomonym(iCorrHom);
        const article_t* piCorrArt = GlobalDictsHolder->GetAuxArticle(corr, CONJ_DICT);

        int iSubConj = clause.HasSubConj_i();
        if (iSubConj == -1)
            return false;
        const CHomonym& conj  = clause.GetConj(iSubConj);

        const article_t* piConjArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);
        int iCorrs = piConjArt->get_corrs_count();
        if (iCorrs == 0)
            return false;

        Wtroka strTitle = piCorrArt->get_title();
        for (int i = 0; i < iCorrs; ++i)
            if (strTitle == piConjArt->get_corr(i))
                return true;

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::HasWordAsConjCorr");
    }*/

}

bool CClauseRules::IncludeSubConjCorrRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if ((clause2.GetWordsCount() == 0) || (clause1.GetWordsCount() == 0))
            return false;

        if (m_Clauses.HasIncludedClauseL(it2))
            return false;

        const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);

        if (!HasWordAsConjCorr(clause1, corr_word))
            return false;

        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = false;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeSubConjCorrRule");
    }*/
}

bool CClauseRules::IncludeAntSubConjRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause1.m_ClauseWords.size() == 0)
            return false;

        if (clause2.HasType_i(NeedNode) != -1) {
            int iEmptyType = clause2.HasType_i(Empty);
            if (iEmptyType != -1)
                res.m_ClauseFuncArg.m_iClause2TypeNum = iEmptyType;
            else
                return false;
        }

        int iSubConj = clause2.HasSubConj_i();

        if (iSubConj == -1)
            return false;

        const CHomonym& conj = clause2.GetConj(iSubConj);
        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);
        int iAntsCount = piArt->get_ant_count();
        if (iAntsCount == 0)
            return false;

        int i;
        for (i = 0; i < iAntsCount; i++) {
            if (clause1.FindAntecedent(piArt->get_ant(i)))
                break;
        }

        if (i>= iAntsCount)
            return false;

        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;

        return true;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeSubConjCorrRule");
    }*/
}

bool CClauseRules::IncludeKotoryjRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        int iSubConj = clause2.HasSubConj_i();

        if (iSubConj == -1)
            return false;

        if (clause2.HasSimConj())
            return false;

        if (!clause2.HasOneTypeNot(NeedNode))
            return false;

        const CHomonym& conj = m_Words[clause2.m_Conjs[iSubConj].m_WordNum];

        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);
        if (piArt->get_sup_valencies().size() != 1)
            return false;

        const or_valencie_list_t& piValVars = piArt->get_sup_valencies().at(0);

        for (size_t i = 0; i < piValVars.size(); i++) {
            const valencie_t& piVal = piValVars[i];
            if (piVal.m_actant_type != WordActant)
                continue;

            SWordHomonymNum actant;
            if (clause1.FindActant(clause2.m_Conjs[iSubConj].m_WordNum, piVal , actant, RightToLeft)) {
                //int iActant = clause1.FindClauseWord(actant);
                CWord& wActant = m_Words.GetWord(actant);
                //ASSERT(iActant != -1);
                for (size_t j = 0; j < clause1.m_Types.size(); j++) {
                    //int iNode = clause1.FindClauseWord(clause1.GetType(j).m_NodeNum);
                    //if( iNode == -1)
                    //  continue;

                    if (!clause1.GetType(j).m_NodeNum.IsValid())
                        continue;
                    const CWord& node = m_Words.GetWord(clause1.GetType(j).m_NodeNum);

                    if ((node.GetSourcePair().FromRight(wActant.GetSourcePair())) && (node.GetRusHomonymsCount() == 1))
                        return false;
                }

                res.m_Operation = IncludeClauseOp;
                res.m_ClauseFuncArg.m_bMainFirst = true;
                res.m_ClauseFuncArg.m_ConjSupValency.m_Actant = actant;
                res.m_ClauseFuncArg.m_ConjSupValency.m_ValNum = 0;
                res.m_ClauseFuncArg.m_iConj = iSubConj;
                return true;
            }
        }
        return false;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeKotoryjRule");
    }*/

}

bool CClauseRules::IncludeParticipleRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause1.m_ClauseWords.size() == 0)
            return false;

        int iParticipleType = clause2.HasTypeWithPOS_i(gParticiple);
        int iEmptyType = -1;

        if (iParticipleType == -1)
            iEmptyType = clause2.HasType_i(Empty);

        if ((iParticipleType == -1) && (iEmptyType == -1))
            return false;

        if (clause2.HasSubConj() || clause2.HasSimConj()) {
            if (iParticipleType == -1)
                return false;
            res.m_Operation = DelTypesOp;
            res.m_ClauseFuncArg.m_DelTypes2.push_back(iParticipleType);
            return true;
        }

        if (clause2.HasDash()) {
            if (iParticipleType == -1)
                return false;
            res.m_Operation = DelTypesOp;
            res.m_ClauseFuncArg.m_DelTypes2.push_back(iParticipleType);
            return true;
        }

        clause_node next_it;
        if (m_Clauses.GetNeighbourClauseR(it2, next_it) && !clause2.HasPunctAtTheVeryEnd())
            return false;

        SWordHomonymNum adj_num;

        int iType2 = iParticipleType;

        if (iEmptyType != -1) {
            if (clause2.m_ClauseWords.size() == 0)
                return false;

            adj_num = clause2.m_ClauseWords[0];
            adj_num.m_HomNum = m_Words.GetWord(clause2.m_ClauseWords[0]).HasPOSi(gAdjective);
            if (adj_num.m_HomNum == -1)
                return false;
            iType2 = iEmptyType;
        } else
            adj_num = clause2.GetType(iParticipleType).m_NodeNum;

        const CHomonym& node = m_Words[adj_num];

        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(node, PREDIC_DICT);
        if (piArt->get_sup_valencies().size() != 1)
            return false;

        const or_valencie_list_t& piValVars = piArt->get_sup_valencies().at(0);

        bool bFoundFromRight = false;

        for (size_t i = 0; i < piValVars.size(); i++) {
            const valencie_t& piVal = piValVars[i];
            if (piVal.m_actant_type != WordActant)
                continue;

            SWordHomonymNum actant;
            if (clause1.FindActant(adj_num, piVal , actant, RightToLeft)) {
                //int iActant = clause1.FindClauseWord(actant);
                CWord& wActant = m_Words.GetWord(actant);
                //ASSERT(iActant != -1);
                for (size_t j = 0; j < clause1.m_Types.size(); j++) {
                    //int iNode = clause1.FindClauseWord(clause1.GetType(j).m_NodeNum);
                    //if( iNode == -1)
                    //  continue;

                    if (!clause1.GetType(j).m_NodeNum.IsValid())
                        continue;

                    const CWord& node = m_Words.GetWord(clause1.GetType(j).m_NodeNum);

                    if (node.GetSourcePair().FromRight(wActant.GetSourcePair()) && (node.GetRusHomonymsCount() == 1))
                        return false;
                }

                res.m_Operation = IncludeClauseOp;
                res.m_ClauseFuncArg.m_bMainFirst = true;
                res.m_ClauseFuncArg.m_iClause2TypeNum = iType2;
                res.m_ClauseFuncArg.m_bCloneByType2 = false;
                if (iParticipleType != -1) {
                    CClause* pInCl  = m_Clauses.GetIncludedClauseR(it1);

                    if (pInCl != NULL)
                        if (pInCl->IsSubClause())
                            res.m_ClauseFuncArg.m_BadWeight++;

                    res.m_ClauseFuncArg.m_NodeSupValency.m_Actant = actant;
                    res.m_ClauseFuncArg.m_NodeSupValency.m_ValNum = 0;
                    res.m_ClauseFuncArg.m_NodeSupValency.m_Rel = piVal.m_syn_rel;
                }
                return true;
            } else if ((iParticipleType != -1) && clause2.FindActant(adj_num, piVal , actant, LeftToRight) && !bFoundFromRight)
                    bFoundFromRight = true;
        }

        if ((iParticipleType != -1) && (clause2.m_Types.size() > 1)) {
            CClause* pInCl = m_Clauses.GetIncludedClauseR(it1);

            if (pInCl != NULL)
                if (pInCl->IsSubClause())
                    res.m_ClauseFuncArg.m_BadWeight++;

            res.m_Operation = DelTypesOp;
            res.m_ClauseFuncArg.m_DelTypes2.push_back(iParticipleType);
            return true;
        }
        return false;

    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeParticipleRule");
    }*/
}

bool CClauseRules::ConjSubInfsRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        int iSubConj = clause1.HasSubConj_i();

        if (iSubConj == -1)
            return false;

        int iInfType1 = clause1.HasTypeWithPOS_i(gInfinitive);
        int iInfType2 = clause2.HasTypeWithPOS_i(gInfinitive);

        if ((iInfType1 == -1) || (iInfType2 == -1))
            return false;

        if (!clause1.m_Conjs[iSubConj].m_Val.IsValid())
            return false;

        int iSubConj2 = clause2.HasSubConj_i();

        if (iSubConj2 != -1) {
            const CHomonym& conj1 = m_Words[clause1.m_Conjs[iSubConj].m_WordNum];
            const CHomonym& conj2 = m_Words[clause2.m_Conjs[iSubConj2].m_WordNum];
            if (conj1.GetLemma() != conj2.GetLemma())
                return false;
        }

        /*if( clause2.HasSubConj() )
            return false;*/

        bool bHasSimConj = clause2.HasSimConj();
        bool bHasInclCl =  m_Clauses.HasIncludedClauseR(it1);
        bool bComma  = clause1.HasPunctAtTheVeryEnd();

        if (bComma && bHasSimConj && !bHasInclCl)
            return false;

        if (!bComma && !bHasSimConj)
            return false;

        res.m_bClone = MustCloneSimSubClause(it1, it2);
        res.m_Operation = NewClauseOp;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iInfType1;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iInfType2;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjSubInfsRule");
    }*/
}

int CClauseRules::FindPredicTypeForVerbAdverb(CClause& clause2)
{
    //try
    {
        int iPredicType = clause2.HasTypeWithPOS_i(gVerb);

        if (iPredicType == -1)
            iPredicType = clause2.HasTypeWithPOS_i(gInfinitive);

        if (iPredicType == -1)
            iPredicType = clause2.HasTypeWithPOS_i(gParticiple);

        if (iPredicType == -1)
            iPredicType = clause2.HasTypeWithPOS_i(TGramBitSet(gAdjective, gShort));

        if (iPredicType == -1)
            iPredicType = clause2.HasTypeWithPOS_i(TGramBitSet(gParticiple, gShort));

        if (iPredicType == -1) {
            iPredicType = clause2.HasTypeWithPOS_i(gPraedic);
            if (iPredicType != -1) {
                SClauseType& type = clause2.GetType(iPredicType);
                size_t i;
                for (i = 0; i < type.m_Vals.size(); i++) {
                    if (type.m_Vals[i].IsValid()) {
                        const CHomonym& actant = m_Words[type.m_Vals[i].m_Actant];
                        if (actant.HasPOS(gInfinitive))
                            break;
                    }
                }
                if (i >= type.m_Vals.size())
                    iPredicType = -1;
            }
        }

        return iPredicType;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::FindPredicTypeForVerbAdverb");
    }*/
}

bool CClauseRules::IncludeVerbAdverbPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.m_ClauseWords.size() == 0)
            return false;

        int iVerbAdverbType = clause1.HasTypeWithPOS_i(gGerund);

        if (iVerbAdverbType == -1)
            return false;

        if (clause2.HasSubConj() || clause2.HasSimConj())
            return false;

        if (clause2.HasDash())
            return false;

        int iPredicType = FindPredicTypeForVerbAdverb(clause2);

        if (iPredicType == -1)
            return false;

        res.m_bClone = true;
        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = false;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iVerbAdverbType;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iPredicType;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeVerbAdverbPredicRule");
    }*/
}

bool CClauseRules::IncludePredicVerbAdverbRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause1.m_ClauseWords.size() == 0)
            return false;

        int iVerbAdverbType = clause2.HasTypeWithPOS_i(gGerund);

        if (iVerbAdverbType == -1)
            return false;

        if (clause2.HasSubConj() || clause2.HasSimConj())
            return false;

        if (clause2.HasDash())
            return false;

        int iPredicType = FindPredicTypeForVerbAdverb(clause1);
        if (iPredicType == -1)
            return false;

        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iVerbAdverbType;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iPredicType;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeVerbAdverbPredicRule");
    }*/
}

bool CClauseRules::UnitePredicInfWithInfRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

    /*  if( clause2.HasSubConj() )
            return false;

        bool bHasSimConj = clause2.HasSimConj();

        if( clause1.HasPunctAtTheEnd() && (!(m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2))) && bHasSimConj)
            return false;*/

    /*  if( clause1.HasPunctAtTheEnd() )
            return false;

        if( !clause2.HasSimConj() )
            return false;*/

        if (!CanUniteClauses(it1, it2))
            return false;

        int iPredicType  = clause1.HasTypeWithPOS_i(gVerb);
        if (iPredicType == -1)
            iPredicType  = clause1.HasTypeWithPOS_i(gPraedic);

        if (iPredicType == -1)
            return false;

        int iInfType = clause2.HasTypeWithPOS_i(gInfinitive);

        if (iInfType != -1) {
            if (clause2.HasSubConj() && (clause2.GetType(iInfType).m_Type != NeedNode))
                return false;
        }

        int iCompType = clause2.HasTypeWithPOS_i(gComparative);

        if ((iInfType == -1) && (iCompType == -1))
            return false;

        CWord& wPredic = m_Words.GetWord(clause1.GetType(iPredicType).m_NodeNum);
        //int iW = clause1.FindClauseWord(clause1.m_Types[iPredicType].m_NodeNum);
        //if( iW == -1 )
        //  throw logic_error("Bad node in \"CClauseRules::UnitePredicInfWithInfRule\"");

        for (size_t i = 0; i < clause1.m_ClauseWords.size(); i++) {
            const CWord& word = m_Words.GetWord(clause1.m_ClauseWords[i]);
            if (word.GetSourcePair().FromRight(wPredic.GetSourcePair())) {
                if (iInfType != -1)
                    if (word.HasPOS(gInfinitive))
                        break;

                if (iCompType != -1)
                    if (word.HasPOS(gComparative))
                        break;
            }
        }

        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_ClauseFuncArg.m_iClause1TypeNum = iPredicType;

        if (iInfType != -1) {
            res.m_ClauseFuncArg.m_iClause2TypeNum = iInfType;
            CheckBadWeightForUnitedClauses(it1, it2, res);
            return true;
        }

        if (iCompType != -1) {
            res.m_ClauseFuncArg.m_iClause2TypeNum = iCompType;
            CheckBadWeightForUnitedClauses(it1, it2, res);
            return true;
        }
        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::UnitePredicInfWithInfRule");
    }*/
}

bool CClauseRules::UniteInfWithPredicInfRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.HasSubConj())
            return false;

        /*if( clause1.HasPunctAtTheEnd() )
            return false;

        if( !clause2.HasSimConj() )
            return false;*/

        /*bool bHasSimConj = clause2.HasSimConj();

        if( clause1.HasPunctAtTheEnd() && !m_Clauses.HasIncludedClauseR(it1) && bHasSimConj)
            return false;*/

        if (!CanUniteClauses(it1, it2))
            return false;

        int iPredicType  = clause2.HasTypeWithPOS_i(gVerb);

        if (iPredicType == -1)
            iPredicType  = clause2.HasTypeWithPOS_i(gPraedic);

        if (iPredicType == -1)
            return false;

        int iInfType = clause1.HasTypeWithPOS_i(gInfinitive);

        if (iInfType != -1) {
            if (clause1.HasSubConj() && (clause1.GetType(iInfType).m_Type != NeedNode))
                return false;
        }

        int iCompType = clause1.HasTypeWithPOS_i(gComparative);

        if ((iInfType == -1) && (iCompType == -1))
            return false;

        CWord& wPredic = m_Words.GetWord(clause2.GetType(iPredicType).m_NodeNum);

        for (size_t i = 0; i < clause2.m_ClauseWords.size(); i++) {
            const CWord& word = m_Words.GetWord(clause2.m_ClauseWords[i]);
            if (word.GetSourcePair().FromLeft(wPredic.GetSourcePair())) {
                if (iInfType != -1)
                    if (word.HasPOS(gInfinitive))
                        break;

                if (iCompType != -1)
                    if (word.HasPOS(gComparative))
                        break;
            } else
                break;
        }

        res.m_Operation = UniteClausesOp;
        res.m_ClauseFuncArg.m_bMainFirst = false;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iPredicType;

        if (iInfType != -1) {
            res.m_ClauseFuncArg.m_iClause1TypeNum = iInfType;
            CheckBadWeightForUnitedClauses(it1, it2, res);
            return true;
        }

        if (iCompType != -1) {
            res.m_ClauseFuncArg.m_iClause1TypeNum = iCompType;
            CheckBadWeightForUnitedClauses(it1, it2, res);
            return true;
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::UniteInfWithPredicInfRule");
    }*/
}

bool CClauseRules::CompValWithSubConj(const or_valencie_list_t& piValVars, const CHomonym& conj)
{
    //try
    {
        for (size_t i = 0; i < piValVars.size(); i++) {
            const valencie_t& piVal = piValVars[i];
            if (piVal.m_actant_type != Fragment)
                continue;

            for (size_t j = 0; j < piVal.m_conjs.size(); j++) {
                Wtroka strConj = piVal.m_conjs[j];
                if (strConj == conj.GetLemma())
                    return true;
            }

            if (conj.HasAnyOfPOS(piVal.m_conj_POSes))
                return true;
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CompValWithSubConj");
    }*/
}

bool CClauseRules::IncludePredicSubConjRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        int iSubConj = clause2.HasSubConj_i();
        if (iSubConj == -1)
            return false;

        if (!clause1.HasOneTypeNot(Parenth))
            return false;

        if (!clause1.HasPunctAtTheVeryEnd() || clause2.HasSimConj())
            return false;

        if (clause1.m_ClauseWords.size() == 0)
            return false;

        if (!clause1.HasOneTypeNot(Empty))
            return false;

        int iEmptyType = clause2.HasType_i(Empty);

        res.m_ClauseFuncArg.m_iClause2TypeNum = clause2.HasOneTypeNot_i(NeedNode);

        if (res.m_ClauseFuncArg.m_iClause2TypeNum == -1)
            return false;

        //специально для случая "не_столько ..., сколько ...", где  "сколько" и подч и соч
        if (clause2.HasSimConjWithCorr() && clause1.HasSubConj()) {
            if (CheckSubClauseForDisruptedSimConj(it1, it2, res))
                return false;
        }

        const CHomonym& conj = m_Words[clause2.m_Conjs[iSubConj].m_WordNum];

        bool bHasVal = FindNodeValWhichWantsSubConjClause(clause1, res, true, conj);

        if (bHasVal) {
            res.m_ClauseFuncArg.m_NewValencies[0].m_ActantType = Fragment;
            res.m_ClauseFuncArg.m_NewValencies[0].m_ClauseID = clause2.GetID();
        }

        if (res.m_ClauseFuncArg.m_iClause1TypeNum != -1) {
            if (clause1.GetType(res.m_ClauseFuncArg.m_iClause1TypeNum).HasPOS(gParticiple) ||
                clause1.GetType(res.m_ClauseFuncArg.m_iClause1TypeNum).HasPOS(gGerund) || iEmptyType != -1)
                res.m_bClone = true;
        }

        CClause* pInCl = m_Clauses.GetIncludedClauseR(it1);
        if (pInCl != NULL) {
            CClause& subClause = *pInCl;
            if (subClause.IsSubClause())
                res.m_ClauseFuncArg.m_BadWeight++;
        }

        if (!bHasVal)
            res.m_ClauseFuncArg.m_BadWeight++;

        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludePredicSubConjRule");
    }*/
}

bool CClauseRules::IncludeSubConjPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {

        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.m_ClauseWords.size() == 0)
            return false;

        clause_node prev_it;
        if (clause1.HasSimConj() && m_Clauses.GetNeighbourClauseL(it1, prev_it))
            return false;

        if (clause2.HasSubConj() || clause2.HasSimConj())
            return false;

        if (!clause1.HasPunctAtTheVeryEnd())
            return false;

        int iSubConj = clause1.HasSubConj_i();

        if (iSubConj == -1)
            return false;

        if (clause2.HasType_i(NeedNode) != -1)
            return false;

        if (clause2.HasType(Empty) && clause2.HasTypeWithPOS(gInfinitive) && clause2.GetTypesCount() == 2)
            return false;

        if (!clause2.HasOneTypeNot(Empty) || !clause2.HasOneTypeWithPOSNot(TGramBitSet(gInfinitive)))
            return false;

        const CHomonym& conj = clause1.GetConj(iSubConj);

        const article_t* piArt = GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT);

        if (piArt->get_sup_valencies().size() == 1) {
            const or_valencie_list_t& piValVars = piArt->get_sup_valencies().at(0);

            for (size_t i = 0; i < piValVars.size(); i++) {
                const valencie_t& piVal = piValVars[i];

                if (piVal.m_position == OnTheLeft)
                    return false;

                //это слова типа "который" - они в другом правиле рассматриваются
                if (piVal.m_actant_type == WordActant)
                    return false;

            }
        }

        if (!clause2.HasOneTypeWithPOSNot(TGramBitSet(gGerund)) || !clause2.HasOneTypeWithPOSNot(TGramBitSet(gParticiple)))
            return false;

        bool bHasVal = FindNodeValWhichWantsSubConjClause(clause2, res, false, conj);
        if (bHasVal) {
            res.m_ClauseFuncArg.m_NewValencies[0].m_ActantType = Fragment;
            res.m_ClauseFuncArg.m_NewValencies[0].m_ClauseID = clause1.GetID();
        }

        CClause* pInCl = m_Clauses.GetIncludedClauseL(it2);
        if (pInCl != NULL) {
            CClause& subClause = *pInCl;
            if (subClause.HasSubConj())
                res.m_ClauseFuncArg.m_BadWeight++;
        }

        if (!bHasVal)
            res.m_ClauseFuncArg.m_BadWeight++;

        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = false;
        clause_node next_it;
        res.m_ClauseFuncArg.m_BadWeight++;
        if (m_Clauses.GetNeighbourClauseL(it1, next_it))
            res.m_bClone = true;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeSubConjPredicRule");
    }*/
}

bool CClauseRules::ConjClausesWithSubjs(CClause& clause1, CClause& clause2, SClauseRuleResult& res, bool& bSamePOSes)
{
    //try
    {
        bool bConjIsSubj = false;
        for (size_t i = 0; i < clause1.m_Types.size(); i++) {
            if (!clause1.GetType(i).m_NodeNum.IsValid())
                continue;

            if (clause1.GetType(i).m_Type == WantsVal)
                continue;

            int iSubj1Val = clause1.GetType(i).HasSubjVal_i();
            if (iSubj1Val == -1)
                continue;

            SWordHomonymNum subj1_num = clause1.GetType(i).m_Vals[iSubj1Val].m_Actant;

            bConjIsSubj = m_Words.GetWord(subj1_num).HasConjHomonym();

            for (size_t j = 0; j < clause2.m_Types.size(); j++) {
                if (!clause2.GetType(j).m_NodeNum.IsValid())
                    continue;

                int iSubj2Val = clause2.GetType(j).HasSubjVal_i();
                if ((iSubj2Val == -1) && !(((clause2.GetType(j).m_Type == WantsVal) &&
                                            (clause2.GetType(j).m_Modality == Possibly)) ||
                                            (clause2.GetType(j).m_Type == WantsNothing)
                                            )
                )
                    continue;

                if (clause2.GetType(j).HasPOS(gImperative))
                    continue;

                if (iSubj2Val != -1) {
                    SWordHomonymNum subj2_num = clause2.GetType(j).m_Vals[iSubj2Val].m_Actant;
                    bool bConj2IsSubj = m_Words.GetWord(subj2_num).HasConjHomonym();

                    if (bConjIsSubj != bConj2IsSubj)
                            continue;
                }

                const CHomonym& hom1 = m_Words[clause1.GetType(i).m_NodeNum];
                const CHomonym& hom2 = m_Words[clause2.GetType(j).m_NodeNum];

                bSamePOSes = hom1.HasSamePOS(hom2);

                res.m_ClauseFuncArg.m_iClause1TypeNum = i;
                res.m_ClauseFuncArg.m_iClause2TypeNum = j;
                return true;
            }
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }

    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjClausesWithSubjs");
    }*/
}

bool CClauseRules::ConjSubPredicsRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        int iSubConj1 = clause1.HasSubConj_i();
        if (iSubConj1 == -1)
            return false;

        int iSimConj = clause2.HasSimConj_i();
        bool bHasSimConj = (iSimConj != -1);

        bool bDisruptedConj = CheckSubClauseForDisruptedSimConj(it1, it2, res);

        //bool bDisruptedConj = CheckForDisruptedConj(it1, it2, res);

        bool bIncluded = m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2);

        bool bPunctNotNecessary = true;

        if (iSimConj != -1) {
            const CHomonym& conj = clause2.GetConj(iSimConj);
            bPunctNotNecessary = !(GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT)->get_has_comma_before());
        }

        if (bHasSimConj && clause1.HasPunctAtTheVeryEnd() && !bDisruptedConj && !bIncluded && bPunctNotNecessary)
            return false;

        if (!(bHasSimConj || clause1.HasPunctAtTheVeryEnd()))
            return false;

        int iSubConj2 = clause2.HasSubConj_i();

        if (!bHasSimConj  && clause1.HasPunctAtTheVeryEnd() && (iSubConj2 == -1) && !bDisruptedConj && !bIncluded)
            return false;

        bool bSameConjs = false;
        if (iSubConj2 != -1) {
            const CHomonym& conj1 = m_Words[clause1.m_Conjs[iSubConj1].m_WordNum];
            const CHomonym& conj2 = m_Words[clause2.m_Conjs[iSubConj2].m_WordNum];
            bSameConjs = (conj1.GetLemma() == conj2.GetLemma());

        }

        if (!bSameConjs && !bHasSimConj  && !bDisruptedConj)
            return false;

        if (!bSameConjs && (iSubConj2 != -1) && !bDisruptedConj)
            res.m_ClauseFuncArg.m_BadWeight++;

        bool bSamePOSes = true;
        if (!ConjClausesWithSubjs(clause1, clause2, res, bSamePOSes))
            return false;

        if (!bSamePOSes && !bHasSimConj && !bSameConjs)
            return false;

        if (!bSamePOSes)
            res.m_ClauseFuncArg.m_BadWeight++;

        res.m_Operation = NewClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        if (!res.m_bClone) {
            if (clause1.HasPunctAtTheVeryEnd() && !clause2.HasSimConj())
                res.m_bClone = true;
            else
                res.m_bClone = MustCloneSimSubClause(it1, it2);
        }
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjSubPredicsRule");
    }*/
}

bool CClauseRules::CheckSubClauseForDisruptedSimConj(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if ((clause1.GetWordsCount() == 0) || (clause2.GetWordsCount() == 0))
            return false;

        if (!clause2.HasSimConjWithCorr())
            return false;

        bool bBeforePredic;
        bool bRes = CheckForDisruptedConj(it1, it2, res, bBeforePredic);
        if (!bRes)
            res.m_bClone = true;

        return bRes;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CheckSubClauseForDisruptedSimConj");
    }*/
}

bool CClauseRules::ConjSubSimNodesRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        if (!clause1.HasOneTypeNot(Empty) || !clause2.HasOneTypeNot(Empty))
            return false;

        int iSubConj1 = clause1.HasSubConj_i();
        if (iSubConj1 == -1)
            return false;

        int iSimConj = clause2.HasSimConj_i();

        bool bHasSimConj = (iSimConj != -1);

        bool bPunctNotNecessary = true;

        if (iSimConj != -1) {
            const CHomonym& conj = clause2.GetConj(iSimConj);
            bPunctNotNecessary = !(GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT)->get_has_comma_before());
        }

        bool bDisruptedConj = CheckSubClauseForDisruptedSimConj(it1, it2, res);

        bool bIncluded = m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2);

        if (bHasSimConj && clause1.HasPunctAtTheVeryEnd() && bPunctNotNecessary && !bDisruptedConj && !bIncluded)
            return false;

        bool bHasPunct = clause1.HasPunctAtTheVeryEnd();

        if (!(bHasSimConj ||  bHasPunct))
            return false;

        int iSubConj2 = clause2.HasSubConj_i();

        if (iSubConj2 != -1) {
            const CHomonym& conj1 = m_Words[clause1.m_Conjs[iSubConj1].m_WordNum];
            const CHomonym& conj2 = m_Words[clause2.m_Conjs[iSubConj2].m_WordNum];
            if ((conj1.GetLemma() != conj2.GetLemma()) && !bDisruptedConj)
                return false;
        }

        if (clause2.HasTypeWithSubj() && (iSubConj2 != -1))
            return false;

        bool bSamePOSes = false;
        bool bRet = HasSimNodes(it1, it2, res, bSamePOSes);

        if (!bHasSimConj && (iSubConj2 == -1) && bHasPunct)
            res.m_bClone = true;

        if (!res.m_bClone)
            res.m_bClone = MustCloneSimSubClause(it1, it2);

        if (!bSamePOSes && !bDisruptedConj)
            res.m_ClauseFuncArg.m_BadWeight++;

        if (!bRet)
            if ((!m_Clauses.HasIncludedClauseR(it1) &&
                        ((iSubConj2 != -1) || (!bHasPunct && bHasSimConj))  &&
                        (!bPunctNotNecessary || !bHasPunct || !bHasSimConj)
                    ) ||
                    (!m_Clauses.HasIncludedClauseR(it1) &&  bDisruptedConj)
            ) {
                if (!(clause2.HasTypeWithPOS(gVerb) || clause2.HasTypeWithPOS(gPraedic) ||
                      clause2.HasTypeWithPOS(TGramBitSet(gAdjective, gShort)))
                )
                return false;
                res.m_Operation = NewClauseOp;
                if (!bHasSimConj)
                    res.m_bClone = true;
                res.m_ClauseFuncArg.m_bMainFirst = false;
            } else
                return false;

        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjSubSimNodesRule");
    }*/
}

bool CClauseRules::HasSimNodes(clause_node it1, clause_node it2, SClauseRuleResult& res, bool& bSamePOSes, bool bForce)
{
    const TGramBitSet AdjectiveShort(gAdjective, gShort);
    const TGramBitSet ParticipleShort(gParticiple, gShort);

    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        for (size_t i = 0; i < clause1.m_Types.size(); i++) {
            if (!clause1.GetType(i).m_NodeNum.IsValid())
                continue;

            if (!(clause1.GetType(i).HasPOS(gVerb) || clause1.GetType(i).HasPOS(gPraedic) ||
                  clause1.GetType(i).HasPOS(AdjectiveShort) ||
                  clause1.GetType(i).HasPOS(ParticipleShort)))
                continue;

            for (size_t j = 0; j < clause2.m_Types.size(); j++) {
                if (!clause2.GetType(j).m_NodeNum.IsValid())
                    continue;

                if (!(clause2.GetType(j).HasPOS(gVerb) || clause2.GetType(j).HasPOS(gPraedic) ||
                      clause2.GetType(j).HasPOS(AdjectiveShort) ||
                      clause2.GetType(j).HasPOS(ParticipleShort)))
                    continue;

                SWordHomonymNum noun_num;

                int iSubjVal = clause1.GetType(i).HasSubjVal_i();

                SValency val;

                if (iSubjVal != -1) {
                    val = clause1.GetType(i).m_Vals[iSubjVal];
                    noun_num = clause1.GetType(i).m_Vals[iSubjVal].m_Actant;
                    if (!noun_num.IsValid())
                        continue;
                    if (m_Words.GetWord(noun_num).HasSubConjHomonym())
                        bForce = true;
                    if ((clause2.GetType(j).HasSubjVal_i() != -1) && !bForce)
                        continue;
                }
                /*else
                    continue;*/
                /*{
                    iSubjVal = clause2.GetType(j).HasSubjVal_i();
                    if( iSubjVal != -1 )
                        noun_num = clause1.GetType(i).m_Vals[iSubjVal].m_Actant;
                    else
                        continue;
                }*/

                const CHomonym& hom1 = m_Words[clause1.GetType(i).m_NodeNum];
                const CHomonym& hom2 = m_Words[clause2.GetType(j).m_NodeNum];

                bSamePOSes = hom1.HasSamePOS(hom2);

                if (bSamePOSes && hom1.HasPOS(gVerb))
                    if (!NGleiche::Gleiche(hom1, hom2, NGleiche::TenseCheck) &&
                        (hom1.IsPastTense() || hom1.IsPresentTense()) &&
                        (hom2.IsPastTense() || hom2.IsPresentTense()))

                        res.m_ClauseFuncArg.m_BadWeight++;

                if (noun_num.IsValid()) {
                    or_valencie_list_t piValVars = GlobalDictsHolder->GetSubjValVars(hom2);
                    if (piValVars.size() == 0)
                        return false;
                    size_t k;
                    for (k = 0; k < piValVars.size(); k++) {
                        valencie_t& piVal = piValVars[k];
                        if (piVal.m_actant_type != WordActant)
                            continue;
                        ECoordFunc CoordFunc = piVal.m_coord_func;

                        YASSERT(CoordFunc >= 0 && CoordFunc < CoordFuncCount);

                        if (!clause2.CompValencyWithHomonym(piVal, m_Words[noun_num], AnyPosition))
                            continue;

                        if (NGleiche::Gleiche(m_Words[noun_num], hom2, CoordFunc))
                            break;
                    }

                    if (k >= piValVars.size())
                        continue;

                    res.m_ClauseFuncArg.m_iClause1TypeNum = i;
                    res.m_ClauseFuncArg.m_iClause2TypeNum = j;
                    res.m_ClauseFuncArg.m_bMainFirst = true;
                    res.m_Operation = NewClauseOp;
                    SValency new_val(val);
                    new_val.m_ValNum = clause2.FindSubjVal(j);
                    if (new_val.m_ValNum == -1)
                        continue;
                    res.m_ClauseFuncArg.m_NewValencies2.push_back(new_val);
                    return true;

                } else if (!bForce && (clause2.GetType(j).HasSubjVal_i() == -1) && bSamePOSes &&
                        NGleiche::GleicheSimPredics(m_Words[clause2.GetType(j).m_NodeNum], m_Words[clause1.GetType(i).m_NodeNum])) {
                        res.m_ClauseFuncArg.m_iClause1TypeNum = i;
                        res.m_ClauseFuncArg.m_iClause2TypeNum = j;
                        res.m_ClauseFuncArg.m_bMainFirst = true;
                        res.m_Operation = NewClauseOp;
                        return true;
                    }
            }
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::HasSimNodes");
    }*/
}

bool CClauseRules::ConjSimNodesRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause1.HasSubConj() || clause2.HasSubConj())
            return false;

        bool bPunct = clause1.HasPunctAtTheVeryEnd();

        int iSimConj = clause2.HasSimConj_i();

        bool bHasSimConj = (iSimConj != -1);

        bool bPunctNotNecessary = true;

        if (iSimConj != -1) {
            const CHomonym& conj = clause2.GetConj(iSimConj);
            bPunctNotNecessary = !(GlobalDictsHolder->GetAuxArticle(conj, CONJ_DICT)->get_has_comma_before());
        }

        bool bDisruptedConj = CheckSubClauseForDisruptedSimConj(it1, it2, res);

        bool bIncluded = m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2);

        if (bHasSimConj && bPunct && bPunctNotNecessary && !bDisruptedConj && !bIncluded)
            return false;

        if (!(bHasSimConj || bPunct))
            return false;

        bool bSamePOSes = true;

        bool bRet = HasSimNodes(it1, it2, res, bSamePOSes, (bHasSimConj && bPunctNotNecessary && !bPunct));

        if (!bSamePOSes)
            res.m_ClauseFuncArg.m_BadWeight++;

        return bRet;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjSimNodesRule");
    }*/
}

bool CClauseRules::ConjSimParticiplesRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        int iParticipleType1 = clause1.HasTypeWithPOS_i(gParticiple);
        int iParticipleType2 = clause2.HasTypeWithPOS_i(gParticiple);

        bool bEllipsis = false;
        if (iParticipleType2 == -1) {
            bEllipsis = true;
            iParticipleType2 = clause2.HasType_i(ParticipleEllipsis);
        }

        if ((iParticipleType1 == -1) || (iParticipleType2 == -1))
            return false;

        if ((clause1.m_ClauseWords.size() == 1) || (clause2.m_ClauseWords.size() == 1))
            return false;
        if (!bEllipsis)
            if (!NGleiche::Gleiche(m_Words[clause1.GetType(iParticipleType1).m_NodeNum],
                                   m_Words[clause2.GetType(iParticipleType2).m_NodeNum],
                                   NGleiche::GenderNumberCaseCheck))
                return false;

        res.m_bClone = true;//MustCloneSimSubClause(it2);
        res.m_ClauseFuncArg.m_iClause1TypeNum = iParticipleType1;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iParticipleType2;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_Operation = NewClauseOp;

        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjSimParticiplesRule");
    }*/
}

bool CClauseRules::ConjSimVerbAdverbRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        int iVerbAdverbType1 = clause1.HasTypeWithPOS_i(gGerund);
        int iVerbAdverbType2 = clause2.HasTypeWithPOS_i(gGerund);

        if (iVerbAdverbType2 == -1)
            iVerbAdverbType2 = clause2.HasType_i(VerbAdverbEllipsis);

        if ((iVerbAdverbType1 == -1) || (iVerbAdverbType2 == -1))
            return false;

        if ((clause1.m_ClauseWords.size() <= 1) && (clause2.m_ClauseWords.size() <= 1))
            return false;

        res.m_ClauseFuncArg.m_iClause1TypeNum = iVerbAdverbType1;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iVerbAdverbType2;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_Operation = NewClauseOp;
        res.m_bClone = MustCloneSimSubClause(it1, it2);
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::ConjSimVerbAdverbRule");
    }*/
}

bool CClauseRules::CheckClausesForSimilarNew(clause_node it1, clause_node it2/*, bool bSecondEmpty = true, bool bCheckSimConj = false*/)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        yvector<SWordHomonymNum> clauseWords = clause1.m_ClauseWords;
        clauseWords.insert(clauseWords.end(), clause2.m_ClauseWords.begin(), clause2.m_ClauseWords.end());
        CWordsPair wpBorder;
        wpBorder.SetPair(clause1.m_ClauseWords.size() - 1, clause1.m_ClauseWords.size());
        CSynChains SynChains(m_Words, GlobalDictsHolder->m_SynAnGrammar, clauseWords);
        //SynChains.InitFromWords(clauseWords);
        SynChains.Run();
        bool bFound = false;
        for (size_t i = 0; i < SynChains.m_SynVariants.size(); i++) {
            CSynChainVariant& synVar = SynChains.m_SynVariants[i];
            for (size_t j = 0; j < synVar.m_SynItems.size(); j++) {
                if (synVar.m_SynItems[j]->Includes(wpBorder)) {
                    bFound = true;
                    break;
                }
            }
        }
        SynChains.Free();
        return bFound;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CheckClausesForSimilarNew");
    }*/
}

bool CClauseRules::CheckClausesForSimilar(clause_node it1, clause_node it2, bool bSecondEmpty /* = true*/, bool bCheckSimConj /* = false*/)
{
    //Stroka strStage =  "0 ";
    //try
    {
        //cout <<  "CheckClausesForSimilar-1 " << endl;
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        ////cout <<  "2 ";
        int iStart2 = 0;

        if (clause2.HasSimConj())
            iStart2 = 1;
        //cout <<  "CheckClausesForSimilar-2 " << endl;
        /***********временно!******/
    /*  int iiii = clause2.HasSimConj_i();
        if( (iiii != -1 ) && !clause2.m_Conjs[iiii].IsSimConjAnd() )
            return false;*/
        /****************************/
        yvector<EGroupType> possible_types;

        //cout <<  "CheckClausesForSimilar-3 " << endl;
        possible_types.push_back(SimAdjs);
        ////cout <<  "4.1 ";
        possible_types.push_back(SimAdvs);
        ////cout <<  "4.2 ";
        possible_types.push_back(SimNoun);
        ////cout <<  "4.3 ";
        possible_types.push_back(SimPreps);
        ////cout <<  "4.4 ";
        possible_types.push_back(SimPrepAdv);
        ////cout <<  "4.5 ";
        possible_types.push_back(SimAdvPrep);
        ////cout <<  "4.6 ";

        //гипотезы о возможной правой части возможных однородных групп
        yvector<SSimGroupType2SecondMain> hyps;
        //cout <<  "CheckClausesForSimilar-4 " << endl;

        int iEnd2 = -1;
        if (!bSecondEmpty)
            iEnd2 = clause2.GetFirstClauseWordWithType();
        if (iEnd2 == -1)
            iEnd2 = clause2.m_ClauseWords.size();

        //cout <<  "CheckClausesForSimilar-5 " << endl;
        for (int i = iStart2; i < iEnd2; i++) {
            const CWord& word = m_Words.GetWord(clause2.m_ClauseWords[i]);

            if (word.HasOnlyUnknownPOS())
                continue;

            //cout <<  "CheckClausesForSimilar-6 " << endl;
            CWord::SHomIt hom_it = word.IterHomonyms();

            bool bFound = false;
            for (; hom_it.Ok(); ++hom_it) {
                const CHomonym& hom = *(hom_it);
                ////cout << "8 ";
                if (hom.IsMorphAdj()) {
                    yvector<EGroupType>::iterator it = std::find(possible_types.begin(), possible_types.end(), SimAdjs);

                    if (it != possible_types.end()) {
                        SSimGroupType2SecondMain hyp;
                        hyp.m_Type = SimAdjs;
                        hyp.m_MainWord = clause2.m_ClauseWords[i];
                        hyp.m_MainWord.m_HomNum = hom_it.GetID();
                        hyps.push_back(hyp);
                        possible_types.erase(it);

                        ////cout << "9 ";
                        it = std::find(possible_types.begin(), possible_types.end(), SimAdvs);
                        if (it != possible_types.end())
                            possible_types.erase(it);

                        it = std::find(possible_types.begin(), possible_types.end(), SimPreps);
                        if (it != possible_types.end())
                            possible_types.erase(it);

                        it = std::find(possible_types.begin(), possible_types.end(), SimPrepAdv);
                        if (it != possible_types.end())
                            possible_types.erase(it);

                        it = std::find(possible_types.begin(), possible_types.end(), SimAdvPrep);
                        if (it != possible_types.end())
                            possible_types.erase(it);
                        ////cout << "10 ";
                        bFound = true;
                    }
                }

                ////cout << "11 ";
                if (hom.HasPOS(gAdverb)) {
                    yvector<EGroupType>::iterator it = std::find(possible_types.begin(), possible_types.end(), SimAdvs);
                    if (it != possible_types.end()) {
                        SSimGroupType2SecondMain hyp;
                        hyp.m_Type = SimAdvs;
                        hyp.m_MainWord = clause2.m_ClauseWords[i];
                        hyp.m_MainWord.m_HomNum = hom_it.GetID();
                        hyps.push_back(hyp);
                        possible_types.erase(it);

                        ////cout << "12 ";
                        it = std::find(possible_types.begin(), possible_types.end(), SimPreps);
                        if (it != possible_types.end())
                            possible_types.erase(it);

                        it = std::find(possible_types.begin(), possible_types.end(), SimPrepAdv);
                        if (it != possible_types.end())
                            possible_types.erase(it);

                        it = std::find(possible_types.begin(), possible_types.end(), SimAdvPrep);
                        if (it != possible_types.end())
                            possible_types.erase(it);
                        bFound = true;
                        ////cout << "13 ";
                    }

                }

                if (hom.IsMorphNoun()) {
                    ////cout << "14 ";
                    yvector<EGroupType>::iterator it = std::find(possible_types.begin(), possible_types.end(), SimNoun);
                    bool bAdd = true;
                    if (it != possible_types.end()) {
                        if (i > iStart2) {
                            const CWord& word1 = m_Words.GetWord(clause2.m_ClauseWords[i-1]);
                            if (!word1.HasOnlyUnknownPOS()) {

                                CWord::SHomIt hom_it = word1.IterHomonyms();
                                bAdd = false;
                                for (; hom_it.Ok(); ++hom_it) {
                                    const CHomonym& adj = *(hom_it);
                                    if (!adj.IsMorphAdj())
                                        continue;

                                    ////cout << "15 ";
                                    if (NGleiche::Gleiche(adj, hom, NGleiche::GenderNumberCaseCheck)) {
                                        bAdd = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (bAdd) {
                            SSimGroupType2SecondMain hyp;
                            hyp.m_Type = SimNoun;
                            hyp.m_MainWord = clause2.m_ClauseWords[i];
                            hyp.m_MainWord.m_HomNum = hom_it.GetID();
                            hyps.push_back(hyp);
                            possible_types.erase(it);
                            //possible_types.clear
                            bFound = true;
                            ////cout << "16 ";
                        }
                    }
                }
            }

            if (word.HasSimConjHomonym())
                continue;

            if (!bFound)
                break;
            if (possible_types.size() == 0)
                break;
        }
        //cout <<  "CheckClausesForSimilar-7 " << endl;

        return CheckSimHypsInLeftClause(hyps, clause1, bCheckSimConj);
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        //Stroka strMsg;
        //strMsg.Format("CClauseRules::CheckClausesForSimilar (strStage - {%s})", strStage);
        //throw std::logic_error((const char*)strMsg);
        throw std::logic_error("CClauseRules::CheckClausesForSimilar");
    }*/
}

bool CClauseRules::CheckSimHypsInLeftClause(yvector<SSimGroupType2SecondMain>& hyps, CClause& clause, bool bCheckSimConj /*= false*/)
{
    //try
    {
        if (clause.m_ClauseWords.size() == 0)
            return false;
        int last_word = (int)clause.m_ClauseWords.size() - 1;
        if (clause.HasPunctAtTheEnd())
            if (clause.m_ClauseWords.size() <= 1)
                return false;
            else
                last_word = clause.m_ClauseWords.size() - 2;

        ////cout << "CheckSimHypsInLeftClause-1 ";

        for (size_t j = 0; j < hyps.size(); j++) {
            SSimGroupType2SecondMain& hyp = hyps[j];
            switch (hyp.m_Type) {
                case SimAdjs:
                {
                    const CWord& word = m_Words.GetWord(clause.m_ClauseWords[last_word]);
                    int iH = word.HasMorphAdj_i();
                    if (iH == -1)
                        continue;
                    const CHomonym& hom1 = word.GetRusHomonym(iH);
                    const CHomonym& hom2 = m_Words[hyp.m_MainWord];

                    if (NGleiche::Gleiche(hom1, hom2, NGleiche::GenderNumberCaseCheck)) {
                        if (bCheckSimConj) {
                            for (int k =  last_word - 1; k >= 0; k--) {
                                const CWord& word = m_Words.GetWord(clause.m_ClauseWords[k]);
                                if (word.HasSimConjHomonym())
                                    return true;
                                if (!word.HasPOS(gAdverb))
                                    break;
                            }
                        } else
                            return true;
                    }
                    break;
                }

                case SimAdvs:
                {
                    const CWord& word = m_Words.GetWord(clause.m_ClauseWords[last_word]);
                    if (word.HasPOS(gAdverb)) {
                        if (bCheckSimConj) {
                            if ((last_word >= 1) && (clause.m_ClauseWords.size() > (size_t)last_word)) {
                                const CWord& word = m_Words.GetWord(clause.m_ClauseWords[last_word - 1]);
                                if (word.HasSimConjHomonym())
                                    return true;
                            }
                        } else
                            return true;
                    }

                    break;
                }

                case SimNoun:
                {
                    const CHomonym& hom2 = m_Words[hyp.m_MainWord];
                    for (int i = (int)clause.m_ClauseWords.size() - 1; i >= 0; i--) {
                        const CWord& word = m_Words.GetWord(clause.m_ClauseWords[i]);

                        if (word.HasOnlyUnknownPOS())
                            continue;

                        CWord::SHomIt hom_it = word.IterHomonyms();
                        for (; hom_it.Ok(); ++hom_it) {
                            const CHomonym& hom1 = *hom_it;
                            if (!hom1.IsMorphNoun())
                                continue;

                            if (NGleiche::Gleiche(hom1, hom2, NGleiche::CaseCheck)) {
                                if (bCheckSimConj) {
                                    for (int k =  i - 1; k >= 0; k--) {
                                        const CWord& word = m_Words.GetWord(clause.m_ClauseWords[k]);
                                        if (word.HasSimConjHomonym())
                                            return true;
                                        if (!(word.HasMorphAdj() || word.HasPOS(gAdverb) || word.HasUnknownPOS()))
                                            break;
                                    }
                                } else
                                    return true;
                            }

                        }

                        if (!word.HasMorphAdj() && !word.HasPOS(gAdverb) && !word.HasMorphNounWithGrammems(TGramBitSet(gGenitive)))
                            break;

                    }

                    break;
                }

                default:
                    break;
            }
        }
        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CheckSimHypsInLeftClause");
    }*/
}

bool CClauseRules::CanUniteClauses(clause_node it1, clause_node it2, bool& bPunctAndSimConj, bool& bSubConjInTheMiddle)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        bPunctAndSimConj = false;
        bSubConjInTheMiddle = false;

        if ((clause1.m_ClauseWords.size() == 0) || (clause2.m_ClauseWords.size() == 0))
            return false;

        int iSubConj = clause2.HasSubConj_i();
        if (iSubConj != -1) {
            int iWord = clause2.FindClauseWord(clause2.m_Conjs[iSubConj].m_WordNum);

            if ((iWord > 0) && clause2.HasSimConj() && !clause1.HasPunctAtTheVeryEnd()) {
                bSubConjInTheMiddle = true;
                return false;
            } else if (!(clause2.HasType_i(Empty) != -1))
                    return false;
        }

        int iSimConj = clause2.HasSimConj_i();

        bool bBeforePredic;
        if (iSimConj != -1) {
            SClauseRuleResult res;
            if (!clause2.HasWordsGroupSimConj()) {
                if (GlobalDictsHolder->IsSimConjWithCorr(m_Words[clause2.m_Conjs[iSimConj].m_WordNum]))
                    return CheckForDisruptedConj(it1, it2, res, bBeforePredic);
                else
                    return false;
            }
        }

        if (clause1.HasPunctAtTheVeryEnd() && !(m_Clauses.HasIncludedClauseR(it1) || m_Clauses.HasIncludedClauseL(it2)) && (iSimConj != -1)) {
            if (clause1.HasDash())
                return true;
            const CHomonym& hom = clause2.GetConj(iSimConj);
            const article_t* piArt = GlobalDictsHolder->GetAuxArticle(hom, CONJ_DICT);
            if (!piArt->get_has_comma_before()) {
                bPunctAndSimConj = true;
                return false;
            }
        }

        CClause* pClause = m_Clauses.GetIncludedClauseR(it1);

        if (pClause) {
            if (!clause1.HasPunctAtTheVeryEnd() &&
                !pClause->HasType(Brackets) &&
                (iSimConj != -1))
                return false;
        }

        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::CanUniteClauses");
    }*/
}

bool CClauseRules::CanUniteClauses(clause_node it1, clause_node it2)
{
    bool bb, cc;
    return CanUniteClauses(it1, it2, bb, cc);
}

bool CClauseRules::IncludeColonRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        if (!clause1.HasColon())
            return false;
        int iType = clause2.HasOneTypeNot_i(Empty);
        if (iType == -1)
            return false;
        clause_node next_it;
        if (m_Clauses.GetNeighbourClauseR(it2, next_it))
            return false;
        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iType;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeColonRule");
    }*/
}

bool CClauseRules::IncludeDashRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        if (clause1.m_ClauseWords.size() == 0)
            return false;
        if (!(clause1.HasDash() && clause2.HasDash()))
            return false;
        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeDashRule");
    }*/
}

bool CClauseRules::IncludeBracketsRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);

        if (clause1.m_ClauseWords.size() == 0)
            return false;

        int iBracketsType = clause2.HasType_i(Brackets);

        if (iBracketsType == -1)
            return false;

        res.m_Operation = IncludeClauseOp;
        res.m_ClauseFuncArg.m_bMainFirst = true;
        res.m_ClauseFuncArg.m_iClause2TypeNum = iBracketsType;
        return true;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeBracketsRule");
    }*/
}

bool CClauseRules::IncludeSubConjEmptyRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        int iSubConjEmptyType;

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        if ((iSubConjEmptyType = clause1.HasType_i(SubConjEmpty)) != -1) {
            if (clause2.HasSubConj())
                return false;

            res.m_Operation = IncludeClauseOp;
            res.m_ClauseFuncArg.m_bMainFirst = false;
            res.m_ClauseFuncArg.m_iClause1TypeNum = iSubConjEmptyType;
            return true;

        }

        if ((iSubConjEmptyType = clause2.HasType_i(SubConjEmpty)) != -1) {
            if (clause1.HasSubConj())
                return false;

            res.m_Operation = IncludeClauseOp;
            res.m_ClauseFuncArg.m_bMainFirst = true;
            res.m_ClauseFuncArg.m_iClause2TypeNum = iSubConjEmptyType;
            return true;
        }
        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeSubConjEmptyRule");
    }*/
}

bool CClauseRules::IncludeParenthesisRule(clause_node it1, clause_node it2, SClauseRuleResult& res)
{
    //try
    {
        CClause& clause1 = m_Clauses.GetClauseRef(it1);
        CClause& clause2 = m_Clauses.GetClauseRef(it2);
        int iParenthType;

        if (clause2.GetWordsCount() != 0) {
            const CWord& corr_word = m_Words.GetWord(clause2.m_ClauseWords[0]);
            if (corr_word.HasPOS(gCorrelateConj))
                return false;
        }

        if ((iParenthType = clause1.HasType_i(Parenth)) != -1) {
            if (clause2.HasSubConj())
                return false;

            res.m_Operation = IncludeClauseOp;
            res.m_ClauseFuncArg.m_bMainFirst = false;
            res.m_ClauseFuncArg.m_iClause1TypeNum = iParenthType;
            return true;

        }

        if ((iParenthType = clause2.HasType_i(Parenth)) != -1) {
            if (clause1.HasSubConj())
                return false;

            res.m_Operation = IncludeClauseOp;
            res.m_ClauseFuncArg.m_bMainFirst = true;
            res.m_ClauseFuncArg.m_iClause2TypeNum = iParenthType;
            return true;
        }

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::IncludeParenthesisRule");
    }*/

}

bool CClauseRules::MustCloneSimSubClause(clause_node /*it1*/, clause_node it2)
{
    //try
    {
        clause_node it;
        int iType;
        if (m_Clauses.GetNeighbourClauseR(it2, it)) {
            iType = -1;
            if (IsSubClause(m_Clauses.GetClauseRef(it), iType))
                return true;
        }

        return false;

        /*if( !m_Clauses.GetNeighbourClauseL(it1, it) )
            return false;

        return IsSubClause(**it, iType); */
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::MustCloneSimSubClause");
    }*/
}

bool CClauseRules::IsSubClause(CClause& clause, int iType)
{
    //try
    {
        if (clause.HasSubConj())
            return true;

        if ((iType = clause.HasTypeWithPOS_i(gParticiple)) != -1)
            return true;

        if ((iType = clause.HasTypeWithPOS_i(gGerund)) != -1)
            return true;

        return false;
    }
    /*catch(std::out_of_range& e)
    {
        throw e;
    }
    catch(std::logic_error& e)
    {
        throw e;
    }
    catch(...)
    {
        throw std::logic_error("CClauseRules::IsSubClause");
    }*/
}
