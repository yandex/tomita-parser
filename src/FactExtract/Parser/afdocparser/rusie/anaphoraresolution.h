#pragma once

#include "referencesearcher.h"
#include "factaddress.h"


struct CAntecWordInfo
{
    size_t m_ClauseNestedness;
    size_t m_SentenceRole;
    bool m_bMorphAnimative;

    Stroka GetParamStr() const;
};

class CAntecedentInfo
{
public:
    SMWAddress m_Address;
    size_t m_SentenceDistance;
    size_t m_TopClauseDistance;
    Wtroka m_AntecedentWordStr;
    size_t m_AntecedentPosition;
    bool m_bAntecedentIsBeforePredicate;
    bool m_bTheSameClause;
    size_t m_TokenDistance;

    CAntecWordInfo m_Anaphor;
    CAntecWordInfo m_Antecedent;

    Stroka GetParamStr() const;

};

class CAnaphoraResolution
{
public:
    CAnaphoraResolution(const CReferenceSearcher* pReferenceSearcher)
        : m_pReferenceSearcher(pReferenceSearcher)
    {
    }

    void ResolveAnaphora();

    bool GetBestAntecedent(const CWord* pAnapWord, CAntecedentInfo& Info) const;

private:
    void ResolveOneAnaphora(const SMWAddress& AnaAddr);
    bool BuildAntecedentInfo(CAntecedentInfo& Info, const SMWAddress& AnaAddr);

    CSentenceRusProcessor* GetSentPrc(size_t SentNo) const;
    int GetSubjWordNo(const CClause* pClause) const;
    int GetPredicateWordNo(const CClause* pClause) const;
    const CHomonym* GetHomonym (const SMWAddress& a) const;
    bool IsIsolatedParticiple (int SentNo, int WordNo, const CHomonym& PartHom) const;
    size_t GetTextPosition(const SMWAddress& AnaAddr) const;

    const CClauseVariant& GetClauseStructureOrBuilt(int SentNo) const;

private:
    const CReferenceSearcher* m_pReferenceSearcher;

    // for each processed homonym store its found antecedents (moved from CHomonym)
    // careful, does not own CHomonym* (thus, some of pointers could be invalid)
    yhash<const CHomonym*, yvector<CAntecedentInfo> > m_PossibleAntecedents;
};
