#pragma once

#include <util/generic/vector.h>


class SClauseOrder : public std::binary_function<const CClause*&, const CClause*&, bool>
{
public:
    bool operator()(const CClause* cl1, const CClause* cl2) const
    {
        return *(cl1) < *(cl2);
    }
};

typedef yset<CClause*, SClauseOrder>::const_iterator const_clause_it;
typedef yset<CClause*, SClauseOrder>::iterator clause_it;
typedef yset<CClause*, SClauseOrder>::reverse_iterator clause_rev_it;

//все правила
enum ERuleType
{
    UniteEmptyToSochConj,
    UniteSochConjToEmpty,
    UniteInfWithPredic,
    UnitePredicWithInf,
    UnitePredicWithSubj,
    UniteSubjWithPredic,
    UniteSubConjWithPredic,
    UnitePredicInfWithInf,
    UniteInfWithPredicInf,
    UniteClauseWithBrackets,
    UniteEmptyWithDashEmpty,
    UniteDashClauseWithPredicEllipsis,
    UniteSubConjEmptyNeedEnd,
    ConjSubInfs,
    ConjSubPredics,
    ConjSubSimNodes,
    ConjSimNodes,
    ConjSimParticiples,
    ConjSimVerbAdverb,
    IncludeKotoryj,
    IncludeParticiple,
    IncludeVerbAdverbPredic,
    IncludePredicVerbAdverb,
    IncludePredicSubConj,
    IncludeSubConjPredic,
    IncludeParenthesis,
    IncludeBrackets,
    IncludeDash,
    IncludeColon,
    IncludeSubConjCorr,
    IncludeAntSubConj,
    IncludeSubConjEmpty,
    RuleTypeCount
};

enum EStopRulesOption
{
    CheckJunction,
    CheckWholeClauses,
    CheckWholeClausesAndLeft,
    CheckWholeClausesAndRight,
    CheckWholeClausesAndLeftRight
};

//операции нал клаузами
enum EClauseOperation
{
    UniteClausesOp = 0,
    IncludeClauseOp,
    NewClauseOp,
    DelTypesOp,
    ClauseOperationCount
};

//результат применения правила (то, что подается на вход функциям-операциям над клаузами )
struct SClauseFuncArg
{
    SClauseFuncArg()
    {
        m_iClause1TypeNum = -1;
        m_iClause2TypeNum = -1;
        m_iConj = -1;
        m_bMainFirst = true;
        m_bCloneByType1 = true;
        m_bCloneByType2 = true;
        m_BadWeight = 0;
        m_bRefreshClauseInfo = false;
        m_BadWeightForCommas = 0;
        m_CoefficientToIncreaseBadWeightForCommas = 1;
    };

    yvector<SValency>    m_NewValencies; // новые валентности для типа m_iClause1TypeNum или m_iClause2TypeNum в зависимости от m_bMainFirst
    yvector<SValency>    m_NewValencies2; // новые валентности для типа m_iClause2TypeNum
    SValency            m_NodeSupValency; // новая валентность для из поля ГЛАВ
    SValency            m_ConjSupValency; // новая валентность для из поля ГЛАВ
    SValency            m_ConjValency; // новая валентность для из поля СОЮЗ_УПР
    yvector<int>            m_DelTypes1;
    yvector<int>            m_DelTypes2;
    yvector<SClauseType>            m_AddTypes1;
    yvector<SClauseType>            m_AddTypes2;
    int                    m_iClause1TypeNum; //тип, который использовался в правиле для первой клаузы
    bool                m_bCloneByType1;//клонировать ли вариант, если кроме использованного типа, есть еще какие-то типы в первой клаузе
    int                    m_iClause2TypeNum; //тип, который использовался в правиле для второй клаузы
    bool                m_bCloneByType2;//клонировать ли вариант, если кроме использованного типа, есть еще какие-то типы в второй клаузе
    bool                m_bMainFirst;
    int                    m_iConj;            //номер союза, для которого m_ConjSupValency или m_ConjValency
    int                    m_BadWeight;        //плохой вес, который это правило может добавить всему варианту
    bool                m_bRefreshClauseInfo;
    int                    m_CoefficientToIncreaseBadWeightForCommas;
    CWordsPair            m_PairForBadCoef; //пара слов, между которыми немотивированные запятые должны помножаться на m_CoefficientToIncreaseBadWeightForCommas
                                          //например, подлеж. и сказ.
    SClauseType            m_NewType;
    int                    m_BadWeightForCommas; //плохой все клаузы, получившийся из-за
                                              // немотивированной запятой (он может
                                              //увиличиться, если эта поганая запятая
                                              //встряля между подлежащим и сказуемым или
                                              //между союзом и вершиной)
};

//результат применения правила
struct SClauseRuleResult
{
    SClauseRuleResult();
    yvector<ERuleType>    m_StopRules;
    bool                m_bClone; //клонировать ли вариант, с запретом применения данного правила
    EClauseOperation    m_Operation; //какую операцию над клаузами применить
    SClauseFuncArg        m_ClauseFuncArg; //что именно делать с клаузами, типами и валентностями
    int iTry;
};

class CClauseRules;
class CClauseNode;
typedef CClauseNode* clause_node;
//typedef clause_it clause_node;

typedef    bool (CClauseRules::*FClauseRule)(clause_node it1, clause_node it2, SClauseRuleResult& res);

enum EGroupType
{
    SimAdjs,
    SimAdvs,
    SimNoun,
    SimPreps,
    SimPrepAdv,
    SimAdvPrep,
    GroupCount
};

enum EStateOfTries //команды для попыток
{
    ResetAllTries,    //изменить все поппытки на начальные
    NextTry,        //применять следующие (более слабые попытки)
    NormalTry        //применять попытки без изменений
};
