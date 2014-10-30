#pragma once

#include <util/generic/vector.h>

#include "clauses.h"
#include "clauseheader.h"
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "clausenode.h"
#include "dictsholder.h"
#include "topclauses.h"

typedef CTopClauses CClauseStructure;
//typedef CClauseVariant CClauseStructure;

struct SSimGroupType2SecondMain
{
    EGroupType  m_Type;
    SWordHomonymNum m_MainWord;
};

struct SClauseRule
{
    SClauseRule() {};
    SClauseRule(FClauseRule rule, int iTryesCount, EStopRulesOption StopOption, const Stroka& name,
                EStopRulesOption OptionForAlreadyApplied = CheckWholeClauses)
    {
        m_iTryesCount = iTryesCount;
        m_Rule = rule;
        m_StopOption = StopOption;
        m_Name = name;
        m_OptionForAlreadyApplied = OptionForAlreadyApplied;
    };

    FClauseRule m_Rule;
    int m_iTryesCount;
    EStopRulesOption m_StopOption;
    EStopRulesOption m_OptionForAlreadyApplied;
    Stroka m_Name;
};

class CClauseRules
{

    struct SAllRules
    {
        SAllRules();
        SClauseRule& operator[](ERuleType);
        SClauseRule m_Rules[RuleTypeCount];
    };

public:
    CClauseRules(CClauseStructure& ClauseVariant);

    //применяет правило RuleType к it1 и it2 и записывает результат res
    //правила не могут изменят клаузы
    bool ApplyRule(ERuleType RuleType, clause_node it1, clause_node it2, SClauseRuleResult& res);

    EStopRulesOption GetRuleStopOptions(ERuleType RuleType) const;
    EStopRulesOption GetRuleAlreadyAppliedOptions(ERuleType RuleType) const;
    Stroka GetRuleName(ERuleType RuleType)
    {
        return m_Rules[RuleType].m_Name;
    }

protected:
    //правила объединения
    //тупое объединение клауз с вложенными скобками
    bool UniteClauseWithBracketsRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение пустыхи и непустыхи
    bool UniteEmptyToSochConjRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение непустыхи и пустыхи
    bool UniteSochConjToEmptyRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фрагмента "инфинитив" с фрагментом, у вершины  которого есть незаполненная валентность на инфинитив.
    bool UniteInfWithPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фрагмента, у вершины  которого есть незаполненная валентность на инфинитив, с фрагментом "инфинитив"
    bool UnitePredicWithInfRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фрагмента с вершиной, у которой незаполненная валентность на подлежащее, с фрагментом с подлежащим
    bool UnitePredicWithSubjRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фрагмента с подлежащим с фрагментом с вершиной, у которой незаполненная валентность на подлежащее
    bool UniteSubjWithPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фр. с типом "хочу вершину" с фрагментом с подходящей вершиной
    bool UniteSubConjWithPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение пустыхи с пустыхой, разделенных "-"
    bool UniteEmptyWithDashEmptyRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фр. с вершиной "предикатив" или "гл_личн" с фр. с вершиной "инфинитив" или "сравнительная степень"
    bool UnitePredicInfWithInfRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение фр. с вершиной "инфинитив" или "сравнительная степень" с фр. с вершиной "предикатив" или "гл_личн"
    bool UniteInfWithPredicInfRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединяет два фрагмента, разделенные тире, согласуя при этом подледащее из первого и какое-нибудь слово из второго с соответствующими словами из предыдущей клаузы
    bool UniteDashClauseWithPredicEllipsisRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //объединение клаузы типа "как сука" и " и сволочь,"
    bool UniteSubConjEmptyNeedEndRule(clause_node it1, clause_node it2, SClauseRuleResult& res);

    //правила вложения
    //вложение фрагмента с подч. союзом, у которого есть валентность ГЛАВ с "ТИП = слово", во фрагмент у которго есть подходящее слово
    bool IncludeKotoryjRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение причастного оборота
    bool IncludeParticipleRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение деепричастного оборота
    bool IncludeVerbAdverbPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение деепричастного оборота
    bool IncludePredicVerbAdverbRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение фрагмента с подч. союзом во фрагмент с вершиной, у которой есть валентность на такой союз
    bool IncludePredicSubConjRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение фрагмента с подч. союзом во фрагмент с вершиной, у которой есть валентность на такой союз (в другом порядке)
    bool IncludeSubConjPredicRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение вводных оборотов
    bool IncludeParenthesisRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение типа "если ... то"
    bool IncludeSubConjCorrRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение типа "... тот, что ..."
    bool IncludeAntSubConjRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение клаузы, заключенной в скобки
    bool IncludeBracketsRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение клаузы между двумя тире
    bool IncludeDashRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение клаузы правую клаузу с двоеточием
    bool IncludeColonRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вложение клауз типа "как сука"
    bool IncludeSubConjEmptyRule(clause_node it1, clause_node it2, SClauseRuleResult& res);

    //правила сочинения
    //сочинение двух подчинительных фрагментов с вершинами типа инфинитив
    bool ConjSubInfsRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //сочинение двух полных подчинительных фрагментов (с подлежащими)
    bool ConjSubPredicsRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //сочинение двух подчинительных фрагментов с однородными вершинами
    bool ConjSubSimNodesRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //сочинение двух фрагментов с однородными вершинами
    bool ConjSimNodesRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //сочинение двух фрагментов с однороднами причастиями
    bool ConjSimParticiplesRule(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //сочинение двух фрагментов с однороднами деепричастиями
    bool ConjSimVerbAdverbRule(clause_node it1, clause_node it2, SClauseRuleResult& res);

    //вспомогательные функции
    int FindPredicTypeForVerbAdverb(CClause& clause2);
    bool CompValWithSubConj(const or_valencie_list_t& piValVars, const CHomonym& conj);
    bool FindNodeValWhichWantsSubConjClause(CClause& clause, SClauseRuleResult& res, bool bCl1, const CHomonym& conj);
    bool ConjClausesWithSubjs(CClause& clause1, CClause& clause2, SClauseRuleResult& res, bool& bSamePOSes);
    //ищет такой тип, вершина которого имеет незапоненную валентность на инфинитив
    // запихивает найденную валентность в res.m_ClauseFuncArg.m_NewValencies, а также в зависимости от  bCl1
    // res.m_ClauseFuncArg.m_iClause1TypeNum или res.m_ClauseFuncArg.m_iClause2TypeNum
    bool FindNodeValWhichWantsInf(CClause& clause, SClauseRuleResult& res, bool bCl1);
    //примерно тоже самое, только ищет валентность с СИН_О подл
    bool FindNodeValWhichWantsSubj(CClause& clause, SClauseRuleResult& res, bool bCl1, or_valencie_list_t& piValVars);
    //пытается найти на стыке фрагментов однородные сущ. или прилаг. или ...
    bool CheckClausesForSimilar(clause_node it1, clause_node it2, bool bSecondEmpty = true, bool bCheckSimConj = false);
    bool CheckClausesForSimilarNew(clause_node it1, clause_node it2/*, bool bSecondEmpty = true, bool bCheckSimConj = false */);
    //пророверяет на однородность вершин два фрагмента
    bool HasSimNodes(clause_node it1, clause_node it2, SClauseRuleResult& res, bool& bSamePOSes, bool bForce = false);
    //для CheckClausesForSimilar
    bool CheckSimHypsInLeftClause(yvector<SSimGroupType2SecondMain>& hyps, CClause& clause, bool bCheckSimConj = false);
    //пророверяет, можно ли объединить фрагменты
    bool CanUniteClauses(clause_node it1, clause_node it2);
    bool CanUniteClauses(clause_node it1, clause_node it2, bool& bPunctAndSimConj, bool& bSubConjInTheMiddle);
    //пророверяет, может ли эта клауза быть подчинительной (прич, деепр. или с подч. союзом)
    bool IsSubClause(CClause& clause, int iType);
    //при сочинении двух фрагментов с подч союзами, прверяет нужно ли клонировать вариант, в зависимости от того, есть ли справа подинительная клауза
    bool MustCloneSimSubClause(clause_node it1, clause_node it2);
    //проверяет есть ли разрывные сочинительные союзы
    bool CheckForDisruptedConj(clause_node it1, clause_node it2, SClauseRuleResult& res, bool& bBeforePredic);
    //проверяет есть ли разрывные сочинительные союзы в подчинительных фрагментах
    bool CheckSubClauseForDisruptedSimConj(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //вызывается при любом объединении - проверяет есть ли согласование, разрывные союзы, и выставляет плохой вес
    void CheckBadWeightForUnitedClauses(clause_node it1, clause_node it2, SClauseRuleResult& res);
    //проверяет, может ли какой-нибудь омоним слова corr_word быть корелятом, для подч. союза из clause
    bool HasWordAsConjCorr(CClause& clause, const CWord& corr_word);
    bool CoordDashClausesWithSubj(CClause& claise0, CClause& claise1, CClause& claise2, int iType);
    bool CoordDashClauses(CClause& claise0, CClause& claise1, CClause& claise2, int iType);

    //CClauseVariant& m_ClauseVariant;
    CClauseStructure& m_Clauses;
    CWordVector&          m_Words;
    static SAllRules m_Rules;
};
