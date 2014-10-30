#pragma once

#include <util/generic/vector.h>

#include "fragmentationrunner.h"
#include "situationssearcherinclause.h"

class CSentenceRusProcessor;

class CSituationsSearcher
{
public:
    CSituationsSearcher(CSentenceRusProcessor* pSent);
    virtual ~CSituationsSearcher(void);

    virtual void Run(TKeyWordType type);
    virtual void Run(const Wtroka& strArt);
    virtual void Run(const SArtPointer&);
    const yvector<SValenciesValues>& GetFoundVals() const;
    void Reset();

protected:
    CMultiWordCreator& m_MultiWordCreator;
    yvector<SValenciesValues> m_FoundVals;
    const CClauseVariant& m_clausesStructure;
};
