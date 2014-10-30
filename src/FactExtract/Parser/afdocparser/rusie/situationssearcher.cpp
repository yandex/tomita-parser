#include "situationssearcher.h"
#include "filtercheck.h"
#include "sentencerusprocessor.h"

CSituationsSearcher::CSituationsSearcher(CSentenceRusProcessor* pSent)
    : m_MultiWordCreator(pSent->GetMultiWordCreator())
    , m_clausesStructure(pSent->GetClauseStructureOrBuilt())
{
}

CSituationsSearcher::~CSituationsSearcher(void)
{
}

void CSituationsSearcher::Run(TKeyWordType type)
{
    Run(SArtPointer(type));
}

void CSituationsSearcher::Run(const Wtroka& strArt)
{
    Run(SArtPointer(strArt));
}

const yvector<SValenciesValues>& CSituationsSearcher::GetFoundVals() const
{
    return m_FoundVals;
}

void CSituationsSearcher::Run(const SArtPointer& artPointer)
{
    const_clause_it it = m_clausesStructure.GetFirstClause();
    yvector<const CClause*> subClauses;
    for (; it != m_clausesStructure.GetEndClause(); it++) {
        subClauses.clear();
        m_clausesStructure.GetIncludedClauses(it, subClauses);
        CSituationsSearcherInClause situationsSearcherInClause(m_MultiWordCreator, m_clausesStructure.GetClauseRef(it),
                                                               m_clausesStructure.GetMainClause(it), subClauses);
        situationsSearcherInClause.Run(artPointer);
        const yvector<SValenciesValues>& foundVals = situationsSearcherInClause.GetFoundVals();
        m_FoundVals.insert(m_FoundVals.begin(), foundVals.begin(), foundVals.end());
    }
}

void CSituationsSearcher::Reset()
{
    m_FoundVals.clear();
}
