#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/namecluster.h>
#include "clauses.h"
#include "clausevariant.h"
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "topclauses.h"
#include "analyticformbuilder.h"
#include "multiwordcreator.h"
#include "dictsholder.h"

class CSentence;
class CSentenceRusProcessor;
struct SFragmOptions;

const int g_iMaxVarsCount = 100;

enum EInitialClauseSearchState
{
    SearchComma,
    SearchCloseBracket,
    SearchCloseDash
};

struct SChangedHomonyms
{
    yset<int> m_iDeleted;
    yset<int> m_iInserted;
};

class CFragmentationRunner
{
public:
    CFragmentationRunner(CSentenceRusProcessor* pSent);

    bool Run(const SFragmOptions* opt);
    void   freeFragmResults();
    CClauseVariant& GetFirstBestVar();

protected:
    void InitWordIndexes(const SFragmOptions* opt);
    bool BuildInitialTopClauses();
    //разбивает передложение по знакам препинания и
    //может порождать варианты в зависимости от статьи союза
    bool BuildInitialClauses();
    void CloneClausesByConjs(yvector<CClause*>& clonedClausesByConjs, CClause& clause);
    int SkipPuncts(int i);
    bool ReadNextInitialClause(CClause& clause, EInitialClauseSearchState& state, int& iStartWordIndex);
    void RunClauseRules();
    bool FindCorr(const Wtroka& strCorr, int iWordPos);
    bool FindConjForCorr(Wtroka strCorr, int iWordPos);
    void CheckCorrForSimConj();
    void FindConjs();//находим все союзы из словаря
    void FindPreps();//находим все предлоги из словаря
    void FindParenthesis();//находим все вводные слова из словаря
    void FindTemplateArticles();//находим все заглушки
    void FindPredics();//находим все предикативы из словаря
    //сравнивает пословоформно слова с yvector<Stroka>& word, начиная от iStartWord
    int CompWithWords(yvector<Wtroka>& word, int iStartWord);
    void InitPrep(long ArtID, int iW1, int iW2, int iH);
    void InitConj(long ArtID, int iW1, int iW2, int iH);
    void InitParenthesis(long ArtID, int iW1, int iW2);
    bool InitPredic(long ArtID, int iW);
    void DelAdjShortAdverbHomonym();
    void BuildWordsInterpretation(bool bAddEndSymbol, const CWorkGrammar& grammar);
//    void AddDebugVar(TXmlNodePtrBase piSentEl, int iVar);
    CWord& GetWordFromWordIndexes(int i);
    void InitClauseWords(CClause& clause, int iStartWodIndex, int iLastWordIndex);
    bool IsLastPunct(SWordHomonymNum& wh);
    void ClearAllTerminalSymbolInterpretation();
    CClause* CreateClause();

    void AddDeletetHom(int iW, int iH);
    void AddInsertedHom(int iW, int iH);
    void RestoreHomonyms();

    // moved from CWord
//    TXmlNodePtr CreateWordTagAndPrint(const CWord& word) const;

protected:
    CSentenceRusProcessor* m_pSent;
    yvector<CClauseVariant> m_ClauseVars;
    CTopClauses m_ClauseGraph;
    CWordVector& m_Words;
    bool m_bFreeClauseVars;
    CWordSequence::TSharedVector& m_wordSequences;
    CMultiWordCreator& m_MultiWordCreator;
    yvector<SWordHomonymNum> m_WordIndexes;
    yvector<TSharedPtr<CWord> >& m_words;
    TOutputStream* m_pErrorStream;
    CClauseVariant m_EmptyClauseVar;
};
