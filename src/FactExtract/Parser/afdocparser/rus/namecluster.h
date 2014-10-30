#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/common/wordsequence.h>
#include "fiowordsequence.h"


class CFIOOccurenceInText : public CFIOOccurence
{
public:
    CFIOOccurenceInText() {m_iSentNum = -1;};

    void PutSentNum(int iS) { m_iSentNum = iS; };
    int  GetSentNum() const { return m_iSentNum; };

    bool operator< (const CFIOOccurenceInText& WordsPairInText) const
    {
        if (m_iSentNum < WordsPairInText.m_iSentNum)
            return true;

        if (m_iSentNum > WordsPairInText.m_iSentNum)
            return false;

        if (CWordsPair::operator <(WordsPairInText))
            return true;

        if (WordsPairInText.CWordsPair::operator <(*this))
            return false;

        if (GetGrammems().All() != WordsPairInText.GetGrammems().All())
            return GetGrammems().All() < WordsPairInText.GetGrammems().All();

        if (GetGrammems().Forms() != WordsPairInText.GetGrammems().Forms())
            return GetGrammems().Forms() < WordsPairInText.GetGrammems().Forms();

        return m_NameMembers[Surname].m_HomNum < WordsPairInText.m_NameMembers[Surname].m_HomNum;
    }

    bool operator== (const CFIOOccurenceInText& WordsPairInText) const
    {
        return (m_iSentNum == WordsPairInText.m_iSentNum) && CWordsPair::operator ==(WordsPairInText);
    }

protected:
    long m_iSentNum;
};

class CNameCluster
{

friend class CNameClusterBuilder;
friend class CText;

public:
    CNameCluster();
    CNameCluster(const CNameCluster&);
    void Merge(CNameCluster& cluster, bool bUniteCoordinate = true);
    void MergeSurnameLemmas(CNameCluster& cluster);
    bool Gleiche(CNameCluster& cluster);

    const SFullFIO& GetFullName() const { return m_FullName; }

    bool operator<(const CNameCluster& Cluster) const;
    bool operator==(const CNameCluster& Cluster) const;
    bool defaul_order(const CNameCluster& cluster2) const;
    bool order_by_f(const CNameCluster& cluster2) const;
    bool order_by_f_pointer(const CNameCluster* cluster2) const;
    bool order_by_i(const CNameCluster& cluster2) const;
    bool order_by_fi(const CNameCluster& cluster2) const;
    bool order_by_fio(const CNameCluster& cluster2) const;
    bool order_by_io(const CNameCluster& cluster2) const;
    bool order_by_o(const CNameCluster& cluster2) const;
    bool order_by_fiInoIn(const CNameCluster& cluster2) const;
    bool order_by_fioIn(const CNameCluster& cluster2) const;

    yset<CFIOOccurenceInText>   m_NameVariants;
    Wtroka ToString() const;

protected:
    SFullFIO    m_FullName;
    bool m_bToDel;
    int m_iSurnameIsLemmaCount;
    ymap<Wtroka, int> m_SurnameLemma2Count;

};

typedef bool (CNameCluster::*FNameClusterOrder)(const CNameCluster&)const;

class CSetOfFioOccurnces : public CWordsPair
{
public:
    CSetOfFioOccurnces() : m_iSentNum(0){}
    CSetOfFioOccurnces(int iSent, const CWordsPair& wp) : CWordsPair(wp), m_iSentNum(iSent) {}
    int m_iSentNum;
    mutable yvector<CFIOOccurenceInText> m_Occurences;
    bool operator< (const CSetOfFioOccurnces& SetOfFioOccurnces) const
    {
        if (m_iSentNum == SetOfFioOccurnces.m_iSentNum)
            return CWordsPair::operator <(SetOfFioOccurnces);

        return m_iSentNum < SetOfFioOccurnces.m_iSentNum;
    }

};

class CNameClusterBuilder
{

    struct SCluster
    {
        EFIOType m_FioTYPE;
        int m_iCluster;
    };

friend class CText;

public:
    CNameClusterBuilder();
    void AddFIO(const SFullFIO& FIO, const CFIOOccurenceInText& group,
                bool bSuspiciousPatronymic /*= false*/, bool bSurnameLemma /*= false*/, int iCount);
    void AddPotentialSurname(const SFullFIO& FIO, const CFIOOccurenceInText& group);
    void AddFIO(const CNameCluster& cluster);
    void Reset();
    bool Run();
    const yvector<CNameCluster>& GetFioCluster(EFIOType type) { return m_FIOClusterVectors[type];};
    void ClearCluster(EFIOType type) { m_FIOClusterVectors[type].clear(); }
    void SetAllFiosWithSurname() { m_bAllFiosWithSurname = true; };

protected:
    void MergeEqual(yvector<CNameCluster>& fio_vector);
    void MergeWithInitials(EFIOType type);
    void ChangeFirstName(EFIOType type);
    void MergeFIOandF();
    void MergeIOandI();
    void MergeFIandF();
    void MergeFIOandFI();
    void MergeFIOandI();
    void MergeFIOandIO();
    void MergeFIandI();
    void MergeFIandIO();
    void MergeFIandFIin();
    void MergeFIOandFIinOin();
    void MergeFIOandPotentialSurnames();
    void MergeFIandPotentialSurnames();

    template <FNameClusterOrder Order>
    bool Merge(yvector<CNameCluster>& to_vector, yvector<CNameCluster>& from_vector, bool bDelUsedFios = false);

    template <FNameClusterOrder Order>
    bool Merge(yvector<CNameCluster>& vector1, yvector<CNameCluster>& vector2,
               yvector<CNameCluster>& res_vector, bool bDelUsedFios = false);

    template <FNameClusterOrder Order>
    bool MergeWithCluster(yvector<CNameCluster>& to_vector, yvector<CNameCluster>& clustersToCompare,
                          CNameCluster& cluster);

    void ChangePatronymicToSurname(CNameCluster& cluster);

    template <FNameClusterOrder Order>
    void TryToMerge(yvector<CNameCluster>& to_vector, yvector<CNameCluster>& from_vector,
                    yset<int>& to_add, bool bCheckFoundPatronymic = false);

    void ResolvePatronymicInFIO();
    void ResolvePatronymicInIO();

    bool EnlargeIObyF(const SFullFIO& FIO, const CFIOOccurenceInText& group);
    void DeleteUsedFIOS();

    void ResolveFirstNameAndPatronymicContradictions();
    void ResolveFirstNameAndPatronymicContradictions(const CSetOfFioOccurnces& SetOfFioOccurnces);
    void RestoreOriginalFio(yvector<SCluster>& nameVariants, const CFIOOccurenceInText& fioEntry);

    void AddOccurenceToAllFios(const CFIOOccurenceInText& group);
    void AddSinglePredictedSurnamesToAllFios(const yvector<CNameCluster>& fioClusters);

    void AssignLocalSurnamePardigmID();

    void ChooseBestSurname(yvector<CNameCluster>& fio_vector);
    void ChooseBestSurname(CNameCluster& nameCluster);

protected:
    yvector<CNameCluster>   m_FIOClusterVectors[FIOTypeCount];
    yvector<CNameCluster>   m_PotentialSurnames;
    yvector<CNameCluster>   m_FIOwithSuspiciousPatronymic;
    yvector<CNameCluster>   m_IOwithSuspiciousPatronymic;

    bool m_bAllFiosWithSurname;  // для случая, когда CNameClusterBuilder вызывается не из
                                 //CText, а, например, из AfnMorph для ФДО
                                 //там все фио с фамилиями и не нужно вызывать слияние потенциальных фамилий

    bool m_bResolveContradictions;
    yset<CSetOfFioOccurnces> m_AllFios;
};

