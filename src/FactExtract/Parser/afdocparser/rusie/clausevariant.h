#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "clauses.h"
#include "clauseheader.h"
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>

bool ClauseOrder(const CClause*& cl1,const CClause*& cl2);

class CClauseVariant;
typedef clause_it (CClauseVariant::*FClauseOp)(const_clause_it it1, const_clause_it it2, SClauseFuncArg& arg);

struct SClausePair2TryNum
{
    SClausePair2TryNum()
    {
        m_iTry = 0;
    }

    void Reset() { m_iTry = 0; }
    int  NextTry() { return ++m_iTry; }

    TPair<CWordsPair,CWordsPair> m_ClausePair;
    int m_iTry;
};

struct SStopeRuleInfo
{
    //CWordsPair m_ClL; //клауза слева
    CWordsPair m_Cl1;
    CWordsPair m_Cl2;
    //CWordsPair m_ClR; //клауза справа
    SWordHomonymNum m_Word1;
    SWordHomonymNum m_Word2;

};

const int g_MaxCashedClauseRules = 10;

struct SCashedClauses
{
    SCashedClauses()
    {
        m_iCount = 0;
        m_iLastAdded = 0;
    }

    int m_iCount;
    int m_iLastAdded;
    SStopeRuleInfo m_CashedClausep[g_MaxCashedClauseRules];
};

struct SVarDebug
{
    SVarDebug()
    {
        m_strDescr = "Initial";
    };

    TSharedPtr<CClauseVariant> m_pVariant;
    Stroka m_strDescr;
};

class CClauseVariant
{

    typedef ymap<ERuleType, yvector< SStopeRuleInfo > >  prohibited_rules;
    typedef ymap<ERuleType, yvector< SClausePair2TryNum > >  rules_tries;
    typedef yvector< TPair<CWordsPair,CWordsPair> > words_pair_vector;
    typedef rules_tries::iterator rules_tries_it;

friend class CClauseRules;

public:

    CClauseVariant(CWordVector& words);
    virtual ~CClauseVariant();
    void RunRules(bool bCanClone);

    //запускает группу правил rules
    bool RunRules(ERuleType rules[], int count, bool bBreakAfterFirstRule, EStateOfTries eStateOfTry, EDirection direction, bool bCanClone, CClauseRules& ClauseRules);

    const_clause_it GetFirstClauseConst(EDirection direction) const;
    clause_it GetFirstClause(EDirection direction);
    const_clause_it GetNextClause(EDirection direction, const_clause_it cl_it) const;
    clause_it GetEndClause() { return m_Clauses.end(); };

    const_clause_it GetFirstClause() const;
    const_clause_it GetEndClause() const;

    CClause& GetClauseRef(clause_it cl_it);
    const CClause& GetClauseRef(const_clause_it cl_it) const;

    CClause* GetClauseByID(long ID);

    //основные операции над клаузами
    clause_it UniteClause(const_clause_it it1, const_clause_it it2,  SClauseFuncArg& arg);
    clause_it IncludeClause(const_clause_it it1, const_clause_it it2,  SClauseFuncArg& arg);
    clause_it NewClause(const_clause_it it1, const_clause_it it2,  SClauseFuncArg& arg);
    clause_it DelTypes(const_clause_it it1, const_clause_it it2,  SClauseFuncArg& arg);

    CClauseVariant& operator=(const CClauseVariant& ClVar);
    CClauseVariant(const CClauseVariant& ClVar);
    void Clone(CClauseVariant& cloned);
    void ChangeLastClauseLastWord(long iW);
    void Print(TOutputStream& stream, ECharset encoding) const;
    clause_it AddClause(CClause* pClause);
    void DeleteClause(const_clause_it it);
    void DeleteClause(CWordsPair& wordsPair);
    bool Equal(CClauseVariant& var);
    void CloneVarByRuleResult(const_clause_it it1, const_clause_it it2, SClauseRuleResult& res,
                              ERuleType eRule, ERuleType rules[], int rules_count,CClauseRules& ClauseRules);
    void AddClauses(yset<CClause*, SClauseOrder>& clauses);

    //имеет ли клауза it вложенный фрагмент, правая граница которого совпадает с правой границей it
    bool HasIncludedClauseR(const_clause_it it) const;
    const CClause* GetIncludedClauseR(const_clause_it it) const;
    //имеет ли клауза it вложенный фрагмент, левая граница которого совпадает с правой границей it
    bool HasIncludedClauseL(const_clause_it it) const;
    const CClause* GetIncludedClauseL(const_clause_it it) const;
    //все клаузы, которые непосредственно включаются в главную
    void GetIncludedClauses(const_clause_it it, yvector<const CClause*>& subClauses) const;
    //пытается найти ближайшую клаузу, в которую вкладывается данная
    CClause* GetMainClause(clause_it it);
    //пытается найти ближайшую клаузу, в которую вкладывается данная
    const CClause* GetMainClause(const_clause_it it) const;

    //применяет операцию res.m_Operation, а также клонирует вариант, если было несколько типов у it1 и/или it2
    clause_it ApplyClauseOp(const_clause_it it1, const_clause_it it2,  SClauseRuleResult& res, bool bCanClone);

    void AddInitialClause(CClause* pClause); //эта функция не только добавляет первичную Clause,
                                             //но и следит за тем, чтобы первичные Clause шли непосредственно друг за другом
                                             //то есть может изменять последнее слово предыдущей Clause

    //запрещает в данном ClauseVariant  применять правило rule на таких двух клаузах (или на стыке этих клауз, если bCheckWholeClauses == false)
    void  AddProhibitedRules(const CClause& pCl1, const CClause& pCl2, yvector<ERuleType> rules, const CClauseRules& ClauseRules);
    void  AddProhibitedRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule, EStopRulesOption StopOption);

    //добавляет правило, которое уже применялось для данных двух клауз и ничего не смогло сделать
    void  AddAlreadyAppliedRuleRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule, const CClauseRules& ClauseRules);

    void  AddStopRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule, EStopRulesOption StopOption, prohibited_rules& prh_rules);

    void Reset();

    //берет по отношению к it соседнюю слева клаузу, причем найденная клауза не может включаться ни в какую другую
    bool GetNeighbourClauseL(const_clause_it it, const_clause_it& next_it) const;

    //берет по отношению к it соседнюю слева клаузу, причем найденная клауза является непосредственным потомком main_it
    bool GetNeighbourClauseL(const_clause_it it, const_clause_it& next_it, const_clause_it  main_it);

    //берет по отношению к it соседнюю справа клаузу, причем найденная клауза не может включаться ни в какую другую
    bool GetNeighbourClauseR(const_clause_it it, const_clause_it& next_it) const;
    bool GetNeighbourClauseR(clause_it it, const_clause_it& next_it, const_clause_it  main_it);
    // функция возвращает минимальную клаузу, которая содеджит PairToSearch
    const_clause_it GetMinimalClauseThatContains(const CWordsPair& PairToSearch) const;
    // возвращает максимальную клаузу, которая включает MinClauseIt, и глубину  вложенности
    const_clause_it GetTopClause(const_clause_it MinClauseIt, size_t& Nestedness) const;

    //клонированные варианты, полученные в результате применения правил
    yvector<CClauseVariant> m_newClauseVariants;

    //смотрит, нет ли запрета на применение rule на на таких двух клаузах
    bool CanApplyRule(const CClause& pCl1, const CClause& pCl2, ERuleType rule, CClauseRules& ClauseRules);

    bool CheckStopRules(const CClause& pCl1, const CClause& pCl2, ERuleType rule, EStopRulesOption StopOption, prohibited_rules& prh_rules);
    bool CheckAlreadyAppliedRules(const CClause& pCl1, const CClause& pCl2, ERuleType rule, EStopRulesOption StopOption);

    //расставляет XML теги
    void PrintClauseTags(yvector<Stroka>& words);

//    void PrintToXML(TXmlNodePtrBase piVarEl);
//    void PrintToXMLForJava(TXmlNodePtrBase piSentEl);

//    TXmlNodePtr PrintClauseSubTreeToXML(const_clause_it it);
//    void InsertClauseAttribute(TXmlNodePtrBase piClEl, Stroka strAttrName, Stroka strAttrValue);
//    void InsertClauseNodeInfo(TXmlNodePtrBase piClEl, const_clause_it it);
    void  AddTry(CWordsPair& pCl1, CWordsPair& pCl2, ERuleType rule, int iTry = 0);
    bool FindTryRule(CWordsPair& pCl1, CWordsPair& pCl2, ERuleType rule, int& iTry);
    void DeletTryRule(ERuleType rule, int& iClausesTry);
    bool IsEmpty() const;
    void CalculateBadWeight();
    long GetBadWeight() const { return m_BadWeight; }

    size_t GetClauseCount() const { return m_Clauses.size(); }

    //#ifdef _FRAGM_DEBUG
    //yvector<SVarDebug> m_DebugVars;
    //#endif

    //пытается найти ближайшую клаузу, в которую вкладывается данная
    const_clause_it GetMainClauseConst(const_clause_it it) const;

protected:
    yset<CClause*, SClauseOrder> m_Clauses;
    //ymap<long, CClause*>  m_ID2Clauses;
    long m_BiggestID; //наименьший из свободных ID для клауз

    prohibited_rules    m_ProhibitedRules;
    //SCashedClauses        m_AlreadyAppliedRules[RuleTypeCount];

    //rules_tries           m_Tries;    //попытки
    CWordVector& m_Words;
    //CClauseRules  m_ClauseRules;
    long m_BadWeight;
    bool m_bCloned;

    static FClauseOp    s_ClauseOp[ClauseOperationCount];

    static const int    s_UniteBracketsRulesCount = 1;
    static const int    s_IncludeBracketsRulesCount = 3;
    static const int    s_UniteRulesCount = 12;
    static const int    s_ConjSubRulesCount  = 5;
    static const int    s_IncludeRulesCount  = 11;
    static const int    s_ConjRulesCount = 1;

    static ERuleType s_UniteBracketsRules[s_UniteBracketsRulesCount]; //правила объединения
    static ERuleType s_IncludeBracketsRules[s_IncludeBracketsRulesCount]; //правила сочинения придаточныхnt];
    static ERuleType s_UniteRules[s_UniteRulesCount];                 //правила вложения
    static ERuleType s_ConjSubRules[s_ConjSubRulesCount];            //правила сочинения
    static ERuleType s_IncludeRules[s_IncludeRulesCount];             //правила объединения клауз с вложенными скобками
    static ERuleType s_ConjRules[s_ConjRulesCount];                  //правила вложения скобок

};

bool ClauseVariantsPred(const CClauseVariant& cl1, const CClauseVariant& cl2);

class SClauseVariantsOrderByWeight : public std::binary_function<const CClauseVariant*&, const CClauseVariant*&, bool>
{
public:
    bool operator()(const CClauseVariant& cl1, const CClauseVariant& cl2) const
    {
        return cl1.GetBadWeight() < cl2.GetBadWeight();
    }
};
