#pragma once

#include <util/generic/vector.h>

#include "clauses.h"
#include "multiwordcreator.h"
#include "valencyvalues.h"
#include "dictsholder.h"
#include "sitfactinterpretation.h"

class CSituationsSearcherInClause
{
public:
    CSituationsSearcherInClause(CMultiWordCreator& multiWordCreator, const CClause& clause, const CClause* pMainClause, yvector<const CClause*>& subClauses);
    virtual ~CSituationsSearcherInClause(void);
    void Run(const SArtPointer& artPointer);
    void Run(const Wtroka& strArt);
    void Run(TKeyWordType type);
    const yvector<SValenciesValues>& GetFoundVals() const;
    void Interpret();

protected:
    bool FindNodeSituation(const SWordHomonymNum& nodeWH, int iClauseType, const SDictIndex& dicIndex);
    bool CheckSubbordination(const SWordHomonymNum& nodeWH,  int iClauseType, int iSubordination, const SDictIndex& dicIndex);
    void CheckValencyContent(const valencie_t& valency,  int iClauseType, yvector<SValencyValue>& valency_variants_res);
    void CheckValencyKWType(const valencie_t& valency,  int iClauseType, yvector<SValencyValue>& valency_variants_res);
    void CheckValencyTitle(const valencie_t& valency,  int iClauseType, yvector<SValencyValue>& valency_variants_res);
    //проверяет, что найденные актанты находятся в текущей клаузе
    void CheckValencyInsideClause(yvector<SWordHomonymNum>* pSWvector, const valencie_t& valency,  int iClauseType, yvector<SValencyValue>& valency_variants_res);
    bool CheckAntecedent(const  reference_to_wordchain_t& ref2wch, SValencyValue& valency_val);
    void FindValencyVariants(const valencie_list_t& subord , int i, int iClauseType, yvector<SValencyValue>& valency_variants);
    void CheckValency(const valencie_t& valency,  int iClauseType, yvector<SValencyValue>& valency_variants_res);
    CWord* GetWordForMultiWord(int iFirst, int iLast, SWordHomonymNum& newWH, const SWordHomonymNum& mainWord);
    bool CheckGramConditions(const valencie_t& valency,  int iClauseType, SValencyValue& valency_val);
    bool GleicheHomonyms(const CHomonym& h1, const CHomonym& h2, ECoordFunc f);
    bool CheckPrefix(const reference_to_wordchain_t& ref2wch, SValencyValue& valency_val);
    bool CompWordsAsPrefix(const Wtroka& s, SValencyValue& valency_val);
    void GenarateContent(const Wtroka& s, yvector<Wtroka>& content);
    bool CheckPrefixByArticle(yvector<SWordHomonymNum>* pSWVector, SValencyValue& valency_val);
    bool CheckValsOrderConditions(const SValenciesValues& valValues, const valencie_list_t& subord);
    bool CheckValsSingleOrderConditions(const SValenciesValues& valValues, const single_actant_contion_t& sac, const valencie_list_t& subord);
    bool CheckValsSingleAgrConditions(const SValenciesValues& valValues, const single_actant_contion_t& sac, const valencie_list_t& subord);
    bool CheckValsSingleConditions(const SValenciesValues& valValues, const single_actant_contion_t& sac, const valencie_list_t& subord);
    void CheckNodeSynRel(const valencie_t& valency,  int iClauseType, yvector<SValencyValue>& valency_variants_res);
    void CheckNodeSynRel(const valencie_t& valency,  const SClauseType& clauseType, yvector<SValencyValue>& valency_variants_res);
    void FindWordsByValencyContent(yvector<SWordHomonymNum>& SWVector, const valencie_t& valency , int iW1, int iW2);
    bool AddValencyValue(SValenciesValues& valenciesValues, SValencyValue& last, const valencie_list_t& subord, bool& bCanBeOmited);
    //пытаемя поискать "валентности" статьи с пустым полем состав
    void SearchForArticleWithEmptySOSTAV(const SArtPointer& artPointer);
    void SearchForArticleWithEmptySOSTAV(const SDictIndex& dicIndex);

    //оставляет только максимальные по вложению  ( SValenciesValues::Includes)
    void ChooseLargestVariants(yvector<SValenciesValues>& valValues);
    //оставляем вариант с максимальным количеством заполненных валентностей
    void ChooseValenciesWithMaximumValues(yvector<SValenciesValues>& valValues);
    void AddValencyVariants(yvector<SValencyValue>& valency_variants, yvector<SValenciesValues>& valValues, const valencie_list_t& subord);
    void InspectPossibleValValues(yvector<SValenciesValues>& valValues);
    void InspectPossibleValValues(yvector<SValenciesValues>& valValues, int iCurSub, yvector<SValenciesValues>& new_valValues);
    void InspectNecessaryValValues(yvector<SValenciesValues>& valValues, int iCurSub, yvector<SValenciesValues>& new_valValues);
    void AddValencyValue(yvector<SValencyValue>& valency_variants_res, SValencyValue& val);

protected:
    CMultiWordCreator& m_multiWordCreator;
    const CClause&        m_Clause;
    const CClause*        m_pMainClause;// главная клауза, для случая, когда союз есть
                                        // только у первой сочиненной клаузы (он тогда переносится в главную
                                        // клаузу, объединяющую объединенные)
    CWordSequence::TSharedVector& m_wordSequences;
    CWordVector& m_Words;
    yvector<SValenciesValues> m_ValValues;
    yvector<const CClause*>& m_subClauses;
    const CWord* GetCurWNode() const {return m_pCurWNode; }
    const CHomonym* GetCurHNode() const {return m_pCurHNode; }
    CWordsPair GetCurNodeSourcePair() const;

    void SetCurNode(const SWordHomonymNum& nodeWH);

    //те слово и омоним, для которых в данный момент собираются валентности
    //могут быть NULL, если поле состав (вершина) у статьи пусто
    const CWord* m_pCurWNode;
    const CHomonym* m_pCurHNode;
};
