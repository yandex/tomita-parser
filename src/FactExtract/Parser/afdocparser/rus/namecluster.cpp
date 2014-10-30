#include "namecluster.h"

#include <util/generic/algorithm.h>

CNameCluster::CNameCluster()
    : m_FullName()
{
    m_bToDel = false;
    m_iSurnameIsLemmaCount = 0;
}

CNameCluster::CNameCluster(const CNameCluster& NameCluster) : m_FullName(NameCluster.m_FullName)
{
    m_NameVariants = NameCluster.m_NameVariants;
    m_bToDel = NameCluster.m_bToDel;
    m_iSurnameIsLemmaCount = NameCluster.m_iSurnameIsLemmaCount;
    m_SurnameLemma2Count = NameCluster.m_SurnameLemma2Count;
}

bool CNameCluster::defaul_order(const CNameCluster& cluster2) const
{
    return m_FullName.default_order(cluster2.m_FullName);
}

bool CNameCluster::order_by_f(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_f(cluster2.m_FullName);
}

bool CNameCluster::order_by_f_pointer(const CNameCluster* cluster2) const
{
    return m_FullName.order_by_f(cluster2->m_FullName);
}

bool CNameCluster::order_by_i(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_i(cluster2.m_FullName);
}

bool CNameCluster::order_by_o(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_o(cluster2.m_FullName);
}

bool CNameCluster::order_by_fio(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_fio(cluster2.m_FullName);
}

bool CNameCluster::order_by_io(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_io(cluster2.m_FullName);
}

bool CNameCluster::order_by_fi(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_fi(cluster2.m_FullName);
}

bool CNameCluster::Gleiche(CNameCluster& cluster)
{
    return m_FullName.Gleiche(cluster.m_FullName);
}

bool CNameCluster::order_by_fiInoIn(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_fiInoIn(cluster2.m_FullName);
}

bool CNameCluster::order_by_fioIn(const CNameCluster& cluster2) const
{
    return m_FullName.order_by_fioIn(cluster2.m_FullName);
}

Wtroka CNameCluster::ToString() const
{
    Wtroka strRes;
    NStr::Append(strRes, "\t{\n");
    if (!m_FullName.m_strSurname.empty()) {
        NStr::Append(strRes, "\t\t");
        strRes += m_FullName.m_strSurname;
        NStr::Append(strRes, "\n");
    }
    if (!m_FullName.m_strName.empty()) {
        NStr::Append(strRes, "\t\t");
        strRes += m_FullName.m_strName;
        NStr::Append(strRes, "\n");
    }
    if (!m_FullName.m_strPatronomyc.empty()) {
        NStr::Append(strRes, "\t\t");
        strRes += m_FullName.m_strPatronomyc;
        NStr::Append(strRes, "\n");
    }

    NStr::Append(strRes, "\t\t");
    yset<CFIOOccurenceInText>::const_iterator it = m_NameVariants.begin();
    for (; it != m_NameVariants.end(); ++it) {
        const CFIOOccurenceInText& fioWP = *it;
        Stroka strAddress = Substitute("$0[$1, $2] ", fioWP.GetSentNum(), fioWP.FirstWord(), fioWP.LastWord());
        NStr::Append(strRes, strAddress.c_str());
    }
    NStr::Append(strRes, "\n\t}");
    return strRes;
}

void CNameCluster::MergeSurnameLemmas(CNameCluster& cluster)
{
    ymap<Wtroka, int>::iterator it = cluster.m_SurnameLemma2Count.begin();
    for (; it != cluster.m_SurnameLemma2Count.end(); it++) {
        const Wtroka& lemma = it->first;
        int lemmaCount = it->second;
        ymap<Wtroka, int>::iterator it2 = m_SurnameLemma2Count.find(lemma);
        if (it2 == m_SurnameLemma2Count.end())
            m_SurnameLemma2Count[lemma] = lemmaCount;
        else
            it2->second += lemmaCount;
    }
}

void CNameCluster::Merge(CNameCluster& cluster, bool bUniteCoordinate /*=true*/)
{

    m_FullName.m_bFoundSurname = (cluster.m_FullName.m_bFoundSurname || m_FullName.m_bFoundSurname);
    MergeSurnameLemmas(cluster);

    if (m_FullName.m_Genders.any() && cluster.m_FullName.m_Genders.any()) {
        m_FullName.m_Genders &= cluster.m_FullName.m_Genders;
        //Stroka ss = m_FullName.m_pMorph->GetGramInfo()->Grammems2str(m_FullName.m_Genders);
    } else
        m_FullName.m_Genders |= cluster.m_FullName.m_Genders;

    if (m_FullName.m_bInitialName) {
        if (cluster.m_FullName.m_strName.size() > 0) {
            m_FullName.m_strName = cluster.m_FullName.m_strName;
            m_FullName.m_bInitialName = cluster.m_FullName.m_bInitialName;
        }
    } else {
        if (m_FullName.m_strName.empty() &&  !cluster.m_FullName.m_strName.empty())
            m_FullName.m_strName = cluster.m_FullName.m_strName;
    }

    if (m_FullName.m_strSurname.size() == 0) {
        if (cluster.m_FullName.m_strSurname.size() > 0)
            m_FullName.m_strSurname = cluster.m_FullName.m_strSurname;
    } else {
        if (cluster.m_FullName.m_strSurname.size() > 0) {
            if (!m_FullName.m_bSurnameLemma && cluster.m_FullName.m_bSurnameLemma) {
                m_FullName.m_strSurname = cluster.m_FullName.m_strSurname;
                m_FullName.m_bSurnameLemma = true;
            } else {
                if (!m_FullName.m_bSurnameLemma ||
                    (m_FullName.m_bSurnameLemma && cluster.m_FullName.m_bSurnameLemma)) {
                    if ((m_FullName.m_bSurnameLemma && cluster.m_FullName.m_bSurnameLemma)) {
                        if (m_iSurnameIsLemmaCount < cluster.m_iSurnameIsLemmaCount)
                            m_FullName.m_strSurname = cluster.m_FullName.m_strSurname;
                    }

                    if (!m_FullName.m_bSurnameLemma ||
                        (m_iSurnameIsLemmaCount == cluster.m_iSurnameIsLemmaCount)) {
                        if (cluster.m_FullName.m_strSurname.size() < m_FullName.m_strSurname.size())
                            m_FullName.m_strSurname = cluster.m_FullName.m_strSurname;
                        else if ((cluster.m_FullName.m_strSurname.size() == m_FullName.m_strSurname.size())  &&
                                (cluster.m_FullName.m_strSurname < m_FullName.m_strSurname))
                                m_FullName.m_strSurname = cluster.m_FullName.m_strSurname;
                    }
                }

            }
        }
    }

    if (m_FullName.m_bInitialPatronomyc) {
        if (cluster.m_FullName.m_strPatronomyc.size() > 1) {
            m_FullName.m_strPatronomyc = cluster.m_FullName.m_strPatronomyc;
            m_FullName.m_bInitialPatronomyc = cluster.m_FullName.m_bInitialPatronomyc;
        }
    } else {
        if (m_FullName.m_strPatronomyc.empty() && !cluster.m_FullName.m_strPatronomyc.empty())
            m_FullName.m_strPatronomyc = cluster.m_FullName.m_strPatronomyc;
    }

    m_iSurnameIsLemmaCount += cluster.m_iSurnameIsLemmaCount;

    if (bUniteCoordinate)
        set_union(m_NameVariants.begin(), m_NameVariants.end(),
                    cluster.m_NameVariants.begin(), cluster.m_NameVariants.end(),
                    inserter(m_NameVariants, m_NameVariants.begin()));
}

bool CNameCluster::operator<(const CNameCluster& Cluster) const
{
    return m_FullName < Cluster.m_FullName;
}

bool CNameCluster::operator==(const CNameCluster& Cluster) const
{
    return m_FullName == Cluster.m_FullName;
}

/*-----------------class CNameClusterBuilder-----------------*/

CNameClusterBuilder::CNameClusterBuilder()
{
    m_bAllFiosWithSurname = false;
    m_bResolveContradictions = false;
}

template< class T >
inline T* bad_cast(const T& _t)
{
    return (T*) &_t;
}

bool CNameClusterBuilder::EnlargeIObyF(const SFullFIO& newFIO, const CFIOOccurenceInText& newgroup)
{
    if (!newgroup.m_NameMembers[Surname].IsValid())
        return false;
    for (size_t i = 0; i < m_FIOClusterVectors[IOname].size(); i++) {
        CNameCluster&  cluster = m_FIOClusterVectors[IOname][i];
        if (cluster.m_NameVariants.size() != 1)
            continue;
        CFIOOccurenceInText& group = *::bad_cast(*cluster.m_NameVariants.begin());
        if (group.GetSentNum()!= newgroup.GetSentNum())
            continue;
        if ((group.FirstWord() !=  group.m_NameMembers[FirstName].m_WordNum) &&
            (group.FirstWord() !=  group.m_NameMembers[InitialName].m_WordNum))
            continue;
        if (newgroup.LastWord() + 1 != group.FirstWord())
            continue;
        group.SetFirstWord(newgroup.LastWord());
        group.m_NameMembers[Surname] = newgroup.m_NameMembers[Surname];
        cluster.m_FullName.m_strSurname = newFIO.m_strSurname;
        cluster.m_FullName.m_bSurnameLemma = newFIO.m_bSurnameLemma;
        m_FIOClusterVectors[FIOname].push_back(cluster);
        m_FIOClusterVectors[IOname].erase(m_FIOClusterVectors[IOname].begin() + i);
        return true;
    }
    return false;
}

void CNameClusterBuilder::AddPotentialSurname(const SFullFIO& FIO, const CFIOOccurenceInText& group)
{
    if (FIO.IsEmpty())
        return;

    CNameCluster NameCluster;
    NameCluster.m_FullName = FIO;
    if (FIO.m_bSurnameLemma && !FIO.m_strSurname.empty())
        NameCluster.m_SurnameLemma2Count[FIO.m_strSurname] = 1;

    NameCluster.m_FullName.MakeLower();
    NameCluster.m_NameVariants.insert(group);
    m_PotentialSurnames.push_back(NameCluster);
}

void CNameClusterBuilder::AddFIO(const SFullFIO& FIO, const CFIOOccurenceInText& group, bool bSuspiciousPatronymic, bool bSurnameLemma, int iSurnameIsLemmaCount)
{
    if (FIO.IsEmpty())
        return;

    CNameCluster NameCluster;
    NameCluster.m_FullName = FIO;

    NameCluster.m_FullName.m_bSurnameLemma = bSurnameLemma;

    if (!FIO.m_bFoundSurname &&
            !NameCluster.m_FullName.m_strSurname.empty() &&
            !NameCluster.m_FullName.m_bInitialName &&
            !NameCluster.m_FullName.m_bSurnameLemma) {
        NameCluster.m_FullName.m_bSurnameLemma = group.GetGrammems().HasAll(TGramBitSet(gNominative, gSingular));
    }

    NameCluster.m_FullName.MakeLower();
    NameCluster.m_NameVariants.insert(group);

    if (bSurnameLemma && !NameCluster.m_FullName.m_strSurname.empty()) {
        if (iSurnameIsLemmaCount == 0)
            iSurnameIsLemmaCount = 1;
        NameCluster.m_iSurnameIsLemmaCount = iSurnameIsLemmaCount;
        NameCluster.m_SurnameLemma2Count[NameCluster.m_FullName.m_strSurname] = iSurnameIsLemmaCount;
    }

    if (!bSuspiciousPatronymic)
        m_FIOClusterVectors[NameCluster.m_FullName.GetType()].push_back(NameCluster);
    else {
        if (FIO.GetType() == FIOname)
            m_FIOwithSuspiciousPatronymic.push_back(NameCluster);
        if (FIO.GetType() == IOname)
            m_IOwithSuspiciousPatronymic.push_back(NameCluster);
    }
    AddOccurenceToAllFios(group);
}

void CNameClusterBuilder::AddOccurenceToAllFios(const CFIOOccurenceInText& group)
{
    CSetOfFioOccurnces setOfFioOccurnces(group.GetSentNum(), group);
    yset<CSetOfFioOccurnces>::iterator it = m_AllFios.find(setOfFioOccurnces);
    if (it == m_AllFios.end()) {
        setOfFioOccurnces.m_Occurences.push_back(group);
        m_AllFios.insert(setOfFioOccurnces);
    } else
        //::bad_cast( *it )->m_Occurences.push_back(group);
        it->m_Occurences.push_back(group);

}

void CNameClusterBuilder::ChangeFirstName(EFIOType type)
{
    for (size_t i = 0; i < m_FIOClusterVectors[type].size(); i++) {
        if (m_FIOClusterVectors[type][i].m_FullName.m_strName.empty())
            return;

        Wtroka strName = GlobalGramInfo->GetFullFirstName(m_FIOClusterVectors[type][i].m_FullName.m_strName);
        if (strName != m_FIOClusterVectors[type][i].m_FullName.m_strName)
            if (!TMorph::HasDictLemma(strName))
                strName.clear();

        if (!strName.empty())
            m_FIOClusterVectors[type][i].m_FullName.m_strName = strName;
    }
}

void CNameClusterBuilder::ChooseBestSurname(CNameCluster& nameCluster)
{
    int max = 0;
    ymap<Wtroka, int>::iterator it = nameCluster.m_SurnameLemma2Count.begin();
    for (; it != nameCluster.m_SurnameLemma2Count.end(); ++it)
        if (it->second > max)
            max = it->second;

    it = nameCluster.m_SurnameLemma2Count.begin();
    for (; it != nameCluster.m_SurnameLemma2Count.end(); ++it)
        if (max == it->second)
            break;

    if (it != nameCluster.m_SurnameLemma2Count.end() && !it->first.empty())
        nameCluster.m_FullName.m_strSurname = it->first;
}

void CNameClusterBuilder::ChooseBestSurname(yvector<CNameCluster>& fio_vector)
{
    for (size_t i = 0; i < fio_vector.size(); i++)
        ChooseBestSurname(fio_vector[i]);
}

void CNameClusterBuilder::MergeEqual(yvector<CNameCluster>& fio_vector)
{
    yvector<CNameCluster> fio_vector_save;
    fio_vector_save.reserve(fio_vector.size());
    fio_vector_save = fio_vector;
    fio_vector.clear();
    bool bFirst = true;
    for (size_t i = 0; i < fio_vector_save.size(); i++) {
        if (bFirst) {
            fio_vector.push_back(fio_vector_save[i]);
            bFirst = false;
            continue;
        }

        int k = (int)fio_vector.size() -1;
        bool bMerged = false;
        while ((k >= 0) && (fio_vector[k].m_FullName == fio_vector_save[i].m_FullName)) {
            if (fio_vector[k].m_FullName.Gleiche(fio_vector_save[i].m_FullName)) {
                fio_vector[k].Merge(fio_vector_save[i]);
                bMerged = true;
            }
            k--;
        }
        if (!bMerged)
            fio_vector.push_back(fio_vector_save[i]);
    }
}

void CNameClusterBuilder::MergeIOandI()
{
    Merge<&CNameCluster::order_by_i>(m_FIOClusterVectors[IOname], m_FIOClusterVectors[Iname]);
    std::sort(m_FIOClusterVectors[IOname].begin(), m_FIOClusterVectors[IOname].end());
}

void CNameClusterBuilder::MergeFIOandF()
{
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[Fname]))
        std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());

    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIinOinName], m_FIOClusterVectors[Fname]))
        std::sort(m_FIOClusterVectors[FIinOinName].begin(), m_FIOClusterVectors[FIinOinName].end());

    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIOinName], m_FIOClusterVectors[Fname]))
        std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());
}

void CNameClusterBuilder::AddSinglePredictedSurnamesToAllFios(const yvector<CNameCluster>& fioClusters)
{
    for (size_t i = 0; i < fioClusters.size(); i++) {
        const CNameCluster& cluster = fioClusters[i];
        if (cluster.GetFullName().m_bFoundSurname)
            continue;
        yset<CFIOOccurenceInText>::const_iterator it = cluster.m_NameVariants.begin();
        for (; it != cluster.m_NameVariants.end(); it++) {
            const CFIOOccurenceInText& group = *it;
            if (group.m_NameMembers[Surname].IsValid() &&
                !group.m_NameMembers[FirstName].IsValid() &&
                !group.m_NameMembers[InitialName].IsValid()) {
                AddOccurenceToAllFios(group);
            }
        }

    }
}

void CNameClusterBuilder::MergeFIandPotentialSurnames()
{
    std::sort(m_PotentialSurnames.begin(), m_PotentialSurnames.end());
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIname], m_PotentialSurnames))
        //добавим к m_AllFios - раньше не добавили, так как m_PotentialSurnames может быть слишком много
        //зачем лишнее добавлять
        AddSinglePredictedSurnamesToAllFios(m_FIOClusterVectors[FIname]);
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());

    std::sort(m_FIOClusterVectors[FIinName].begin(), m_FIOClusterVectors[FIinName].end());
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIinName], m_PotentialSurnames))
        AddSinglePredictedSurnamesToAllFios(m_FIOClusterVectors[FIinName]);
    std::sort(m_FIOClusterVectors[FIinName].begin(), m_FIOClusterVectors[FIinName].end());

}

void CNameClusterBuilder::MergeFIOandPotentialSurnames()
{
    std::sort(m_PotentialSurnames.begin(), m_PotentialSurnames.end());
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIOname], m_PotentialSurnames))
        AddSinglePredictedSurnamesToAllFios(m_FIOClusterVectors[FIOname]);
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());

    std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIOinName], m_PotentialSurnames))
        AddSinglePredictedSurnamesToAllFios(m_FIOClusterVectors[FIOinName]);
    std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());

    std::sort(m_FIOClusterVectors[FIinOinName].begin(), m_FIOClusterVectors[FIinOinName].end());
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIinOinName], m_PotentialSurnames))
        AddSinglePredictedSurnamesToAllFios(m_FIOClusterVectors[FIinOinName]);
    std::sort(m_FIOClusterVectors[FIinOinName].begin(), m_FIOClusterVectors[FIinOinName].end());

}

void CNameClusterBuilder::MergeFIandF()
{
    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIname], m_FIOClusterVectors[Fname]))
        std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());

    if (Merge<&CNameCluster::order_by_f>(m_FIOClusterVectors[FIinName], m_FIOClusterVectors[Fname]))
        std::sort(m_FIOClusterVectors[FIinName].begin(), m_FIOClusterVectors[FIinName].end());
}

void CNameClusterBuilder::MergeFIOandFI()
{
    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[FIname]))
        std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());

    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIinOinName], m_FIOClusterVectors[FIinName]))
        std::sort(m_FIOClusterVectors[FIinOinName].begin(), m_FIOClusterVectors[FIinOinName].end());

    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIinOinName], m_FIOClusterVectors[FIname], m_FIOClusterVectors[FIOinName]))
        std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());

    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIOinName], m_FIOClusterVectors[FIinName]))
        std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());

    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIOinName], m_FIOClusterVectors[FIname]))
        std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());
}

void CNameClusterBuilder::MergeFIandFIin()
{
    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIname], m_FIOClusterVectors[FIinName], false))
        std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());
    if (Merge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[FIinName], true))
        std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());

}

/*
template <typename T, bool (T::*Func)(const T&)>
inline bool Cmp(const T& a1, const T& a2) {
    return (a1.*Func)(a2);
}
*/

template <FNameClusterOrder Func>
inline bool CmpCluster (const CNameCluster& a1, const CNameCluster& a2) {
    return (a1.*Func)(a2);
}

void CNameClusterBuilder::MergeFIOandFIinOin()
{

    if (Merge<&CNameCluster::order_by_fiInoIn>(m_FIOClusterVectors[FIOinName], m_FIOClusterVectors[FIinOinName], false))
        std::sort(m_FIOClusterVectors[FIOinName].begin(), m_FIOClusterVectors[FIOinName].end());

    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end(), CmpCluster<&CNameCluster::order_by_fiInoIn>);
    if (Merge<&CNameCluster::order_by_fiInoIn>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[FIinOinName], true))
        std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());

    Merge<&CNameCluster::order_by_fio>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[FIOinName], true);

    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
}

void CNameClusterBuilder::MergeFIandI()
{
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end(), CmpCluster<&CNameCluster::order_by_i>);
    Merge<&CNameCluster::order_by_i>(m_FIOClusterVectors[FIname], m_FIOClusterVectors[Iname]);
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());
}

void CNameClusterBuilder::MergeFIOandI()
{
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end(), CmpCluster<&CNameCluster::order_by_i>);
    Merge<&CNameCluster::order_by_i>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[Iname]);
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
}

void CNameClusterBuilder::MergeFIOandIO()
{
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end(), CmpCluster<&CNameCluster::order_by_io>);
    Merge<&CNameCluster::order_by_io>(m_FIOClusterVectors[FIOname], m_FIOClusterVectors[IOname]);
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
}

void CNameClusterBuilder::MergeFIandIO()
{
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end(), CmpCluster<&CNameCluster::order_by_i>);

    //Merge(FIname, IOname, &CNameCluster::order_by_i);
    //так как при слиянии FIname IOname мы получаем FIOname, то нужно из вектора с
    //FIname удалить те, которые стали FIOname и перепмсать их в вектор с FIOname
    yvector<CNameCluster>::iterator res_it;
    yvector<CNameCluster>::iterator fio_it = m_FIOClusterVectors[FIname].begin();

    yset<int> FItoDel;

    for (int i = 0; i < (int)m_FIOClusterVectors[IOname].size(); i++) {
        CNameCluster& cluster = m_FIOClusterVectors[IOname][i];

        bool bDel = false;

        fio_it = m_FIOClusterVectors[FIname].begin();

        res_it = ::LowerBound(fio_it, m_FIOClusterVectors[FIname].end(), cluster, CmpCluster<&CNameCluster::order_by_i>);

        if ((res_it == m_FIOClusterVectors[FIname].end()))//|| (res_it->m_FullName.m_strSurname != cluster.m_FullName.m_strSurname) )
            continue;
        fio_it = res_it;

        yvector<CNameCluster> newClusters;
        yvector<int> delFI;
        while ((fio_it != m_FIOClusterVectors[FIname].end()) && (!cluster.order_by_i(*fio_it)  && !(*fio_it).order_by_i(cluster))) {
            if (!cluster.Gleiche(*fio_it)) {
                fio_it++;
                continue;
            }
            CNameCluster new_cluster(*fio_it);
            new_cluster.Merge(cluster, false);

            if (new_cluster.m_FullName.GetType() == FIOname) {
                //может возникнуть ситуация, что то FIOname, которое мы получаем, уже было в тексте, мы уже к
                //нему подливали и FIname и IOname, но так как мы их не удаляем после сливания, то может
                //возникнуть такая ситуевина (например,  Александр Иванович, Александр Иванов, Александр Иванович Иванов)
                if (std::find(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end(), new_cluster) == m_FIOClusterVectors[FIOname].end()) {
                    newClusters.push_back(new_cluster);
                }
            }

            bDel = true;
            delFI.push_back(fio_it - m_FIOClusterVectors[FIname].begin());
            fio_it++;
        }

        //если несколько вариантов (Сергей Петров, СЕРГЕЙ ВЛАДИЛЕНОВИЧ, Сергей Иванов),
        //то не сливаем
        if (newClusters.size() > 1) {
            delFI.clear();
            bDel = false;
        } else {
            m_FIOClusterVectors[FIOname].insert(m_FIOClusterVectors[FIOname].end(), newClusters.begin(), newClusters.end());
        }

        if (bDel) {
            for (size_t k = 0; k < delFI.size(); k++)
                FItoDel.insert(delFI[k]);
            m_FIOClusterVectors[IOname].erase(m_FIOClusterVectors[IOname].begin() + i);
            i--;
        }

        if (res_it == m_FIOClusterVectors[FIname].end())
            break;
    }

    yset<int>::iterator set_it = FItoDel.end();
    if (FItoDel.size()) {
        do
            m_FIOClusterVectors[FIname].erase(m_FIOClusterVectors[FIname].begin() + *(--set_it));
        while (set_it != FItoDel.begin());
    }

    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
}

template <FNameClusterOrder Order>
bool CNameClusterBuilder::Merge(yvector<CNameCluster>& clustersToCompare, yvector<CNameCluster>& from_vector,
                                yvector<CNameCluster>& to_vector, bool bDelUsedFios)
{
    bool bMergedSomething = false;
    for (int i = 0; i < (int)from_vector.size(); i++) {
        CNameCluster& cluster = from_vector[i];
        bMergedSomething = MergeWithCluster<Order>(to_vector, clustersToCompare, cluster) || bMergedSomething;
    }

    if (bDelUsedFios) {
        for (int j = (int)from_vector.size() - 1; j >= 0; j--)
            if (from_vector[j].m_bToDel)
                from_vector.erase(from_vector.begin() + j);
    }

    return bMergedSomething;
}

template <FNameClusterOrder Order>
bool CNameClusterBuilder::Merge(yvector<CNameCluster>& to_vector, yvector<CNameCluster>& from_vector, bool bDelUsedFios)
{
    yvector<CNameCluster> clustersToCompare = to_vector;
    to_vector.clear();

    bool bRes =  Merge<Order>(clustersToCompare, from_vector, to_vector, bDelUsedFios);

    for (size_t i = 0; i < clustersToCompare.size(); i++) {
        if (!clustersToCompare[i].m_bToDel)
            to_vector.push_back(clustersToCompare[i]);
    }
    return bRes;
}

template <FNameClusterOrder Order>
bool CNameClusterBuilder::MergeWithCluster(yvector<CNameCluster>& to_vector, yvector<CNameCluster>& clustersToCompare,
                                           CNameCluster& cluster)
{
    yvector<CNameCluster>::iterator fio_it = clustersToCompare.begin();
    yvector<CNameCluster>::iterator fio_it_end = clustersToCompare.end();

    bool bMergedSomething = false;

    fio_it = clustersToCompare.begin();

    fio_it = ::LowerBound(fio_it, fio_it_end, cluster, CmpCluster<Order>);

    if ((fio_it == fio_it_end))//|| (res_it->m_FullName.m_strSurname != cluster.m_FullName.m_strSurname) )
        return false;

    while ((fio_it != fio_it_end) &&
            (!(cluster.*Order)(*fio_it)  &&  !((*fio_it).*Order)(cluster))) {
        if (!cluster.Gleiche(*fio_it)) {
            fio_it++;
            continue;
        }

        to_vector.push_back(*fio_it);
        to_vector.back().Merge(cluster);
        to_vector.back().m_bToDel = false;
        fio_it->m_bToDel = true;
        fio_it++;
        cluster.m_bToDel = true;
        bMergedSomething = true;
    }

    return bMergedSomething;
}

void CNameClusterBuilder::Reset()
{
        for (int i = 0; i < FIOTypeCount; i++)
            m_FIOClusterVectors[(EFIOType)i].clear();
        m_PotentialSurnames.clear();
        m_FIOwithSuspiciousPatronymic.clear();
        m_IOwithSuspiciousPatronymic.clear();
        m_AllFios.clear();

}

void CNameClusterBuilder::DeleteUsedFIOS()
{
    for (int i = 0; i < FIOTypeCount; i++) {
        for (int j = (int)m_FIOClusterVectors[(EFIOType)i].size() - 1; j >= 0; j--)
            if (m_FIOClusterVectors[(EFIOType)i][j].m_bToDel)
                m_FIOClusterVectors[(EFIOType)i].erase(m_FIOClusterVectors[(EFIOType)i].begin() + j);
    }
}

template <FNameClusterOrder Order>
void CNameClusterBuilder::TryToMerge(yvector<CNameCluster>& to_vector, yvector<CNameCluster>& from_vector,
                                     yset<int>& to_add, bool bCheckFoundPatronymic)
{
    yvector<CNameCluster>::iterator fio_it = to_vector.begin();
    yvector<CNameCluster>::iterator fio_it_end = to_vector.end();
    yvector<CNameCluster>::iterator res_it;
    yvector<CNameCluster> copies_with_initials;

    for (int i = 0; i < (int)from_vector.size(); i++) {
        CNameCluster& cluster = from_vector[i];
        if (!cluster.m_FullName.m_bFoundPatronymic && bCheckFoundPatronymic)
            continue;

        //bool bDel = false;
        fio_it = to_vector.begin();

        res_it = ::LowerBound(fio_it, fio_it_end, cluster, CmpCluster<Order>);

        if ((res_it == fio_it_end))
            continue;

        fio_it = res_it;

        if (!(cluster.*Order)(*fio_it)  && !((*fio_it).*Order)(cluster) &&
            cluster.m_FullName.m_strPatronomyc != fio_it->m_FullName.m_strSurname)
            to_add.insert(i);
    }
}

void CNameClusterBuilder::ChangePatronymicToSurname(CNameCluster& cluster)
{
    cluster.m_FullName.m_strSurname = cluster.m_FullName.m_strPatronomyc;
    cluster.m_FullName.m_strPatronomyc.clear();

    if (cluster.m_FullName.m_bFoundPatronymic)
        cluster.m_FullName.m_bFoundSurname = true;
    else
        cluster.m_FullName.m_bFoundSurname = false;

    cluster.m_FullName.m_bFoundPatronymic = true;//восстановим значение по умолчанию

    yset<CFIOOccurenceInText>::iterator it = cluster.m_NameVariants.begin();
    for (; it != cluster.m_NameVariants.end(); it++) {
        CFIOOccurenceInText& address = *::bad_cast(*it);
        if (!address.m_NameMembers[Patronomyc].IsValid())
            continue;

        if (address.m_NameMembers[Surname].m_WordNum == address.FirstWord())
            address.SetFirstWord(address.FirstWord()+1);
        if (address.m_NameMembers[Surname].m_WordNum == address.LastWord())
            address.SetLastWord(address.LastWord()-1);

        address.m_NameMembers[Surname] = address.m_NameMembers[Patronomyc];
    }
}

void CNameClusterBuilder::ResolvePatronymicInFIO()
{
    if (m_FIOwithSuspiciousPatronymic.size() == 0)
        return;
    yset<int> added;

    std::sort(m_FIOwithSuspiciousPatronymic.begin(), m_FIOwithSuspiciousPatronymic.end());

    TryToMerge<&CNameCluster::order_by_fi>(m_FIOClusterVectors[FIname], m_FIOwithSuspiciousPatronymic, added);

    for (int i = 0; i < (int)m_FIOwithSuspiciousPatronymic.size(); i++) {
        CNameCluster& cluster = m_FIOwithSuspiciousPatronymic[i];
        if (added.find(i) == added.end()) {
            ChangePatronymicToSurname(cluster);
            m_FIOClusterVectors[FIname].push_back(cluster);
        } else
            m_FIOClusterVectors[FIOname].push_back(cluster);

    }
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
}

void CNameClusterBuilder::ResolvePatronymicInIO()
{
    if (m_IOwithSuspiciousPatronymic.size() == 0)
        return;

    yset<int> added;

    std::sort(m_IOwithSuspiciousPatronymic.begin(), m_IOwithSuspiciousPatronymic.end());

    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end(), CmpCluster<&CNameCluster::order_by_io>);
    TryToMerge<&CNameCluster::order_by_io>(m_FIOClusterVectors[FIOname], m_IOwithSuspiciousPatronymic, added);

    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end(), CmpCluster<&CNameCluster::order_by_i>);
    TryToMerge<&CNameCluster::order_by_i>(m_FIOClusterVectors[FIname], m_IOwithSuspiciousPatronymic, added, true);

    for (size_t i = 0; i < m_IOwithSuspiciousPatronymic.size(); i++) {
        CNameCluster& cluster = m_IOwithSuspiciousPatronymic[i];
        if (added.find(i) == added.end()) {
            ChangePatronymicToSurname(cluster);
            m_FIOClusterVectors[FIname].push_back(cluster);
        } else {
            //мы его приписали в ф-ции AssignLocalSurnamePardigmID, в
            //надежде на то, что отчество может стать фамилией
            //не стало...
            cluster.m_FullName.m_iLocalSuranamePardigmID = -1;
            m_FIOClusterVectors[IOname].push_back(cluster);
        }
    }
    std::sort(m_FIOClusterVectors[FIname].begin(), m_FIOClusterVectors[FIname].end());
    std::sort(m_FIOClusterVectors[FIOname].begin(), m_FIOClusterVectors[FIOname].end());
    std::sort(m_FIOClusterVectors[IOname].begin(), m_FIOClusterVectors[IOname].end());

}

void CNameClusterBuilder::RestoreOriginalFio(yvector<SCluster>& nameVariants, const CFIOOccurenceInText& fioEntry)
{
    CNameCluster newCluster = m_FIOClusterVectors[nameVariants[0].m_FioTYPE][nameVariants[0].m_iCluster];
    newCluster.m_NameVariants.clear();
    newCluster.m_NameVariants.insert(fioEntry);

    if (fioEntry.m_NameMembers[InitialPatronomyc].IsValid())
        newCluster.m_FullName.m_strPatronomyc.assign(newCluster.m_FullName.m_strPatronomyc, 0, 1);
    else if (!fioEntry.m_NameMembers[Patronomyc].IsValid())
        newCluster.m_FullName.m_strPatronomyc.clear();

    if (fioEntry.m_NameMembers[InitialName].IsValid())
        newCluster.m_FullName.m_strName.assign(newCluster.m_FullName.m_strName, 0, 1);
    else if (!fioEntry.m_NameMembers[FirstName].IsValid())
        newCluster.m_FullName.m_strName.clear();

    for (size_t j = 0; j < nameVariants.size(); j++) {
        CNameCluster& oldCluster = m_FIOClusterVectors[nameVariants[j].m_FioTYPE][nameVariants[j].m_iCluster];
        yset<CFIOOccurenceInText>::iterator it = oldCluster.m_NameVariants.find(fioEntry);
        if (it != oldCluster.m_NameVariants.end())
            oldCluster.m_NameVariants.erase(it);
    }

    m_FIOClusterVectors[newCluster.m_FullName.GetType()].push_back(newCluster);
}

void CNameClusterBuilder::ResolveFirstNameAndPatronymicContradictions()
{
    yset<CSetOfFioOccurnces>::const_iterator it = m_AllFios.begin();
    for (; it != m_AllFios.end(); it++)
        //рассматриваем одну цепочку в предложении с, возможно, несколькими фио
        //например "Евгения Петрова" - один CSetOfFioOccurnces,
        //в котором CSetOfFioOccurnces.m_Occurences - 2 варианта
        ResolveFirstNameAndPatronymicContradictions(*it);
}

void CNameClusterBuilder::ResolveFirstNameAndPatronymicContradictions(const CSetOfFioOccurnces& SetOfFioOccurnces)
{
    yvector< yvector<SCluster> > setOfFioOccurncesNameVariants;
    bool bHasContradictions = false;
    setOfFioOccurncesNameVariants.resize(SetOfFioOccurnces.m_Occurences.size());
    for (size_t i = 0; i < SetOfFioOccurnces.m_Occurences.size(); i++) {
        const CFIOOccurenceInText& fioEntry = SetOfFioOccurnces.m_Occurences[i];
        yvector<SCluster>& nameVariants = setOfFioOccurncesNameVariants[i];
        for (int j = 0;j < FIOTypeCount; j++) {
            if ((j != FIOname) && (j != FIname) &&
                (j != FIinOinName) && (j != FIinName) && (j != FIOinName))
                continue;
            for (size_t k = 0; k < m_FIOClusterVectors[j].size(); k++) {
                CNameCluster& cluster = m_FIOClusterVectors[j][k];
                //if( cluster.m_NameVariants.size() > 1 )
                {
                    yset<CFIOOccurenceInText>::iterator it = cluster.m_NameVariants.find(fioEntry);
                    if (it != cluster.m_NameVariants.end()) {
                        SCluster clusterAddr;
                        clusterAddr.m_FioTYPE = (EFIOType)j;
                        clusterAddr.m_iCluster = k;
                        nameVariants.push_back(clusterAddr);
                    }
                }
            }
        }
        if (nameVariants.size() >= 2)
            bHasContradictions = true;

    }

    if (bHasContradictions)
        //если хотя бы один омоним фио противоречив, то
        //удаляем все полученные слияния
        //то есть, если, например, у фио "Евгения Петрова" м.р. совпал с двумя противоречивыми фио, а ж.р.
        //совпал с непротиворечивыми фио, то не считаем, что это повод удалить мужской омоним,
        //и отменям все расширения фио для обоих омонимов
        for (size_t i = 0; i < setOfFioOccurncesNameVariants.size(); i++) {
            if (setOfFioOccurncesNameVariants[i].size() > 0)
                RestoreOriginalFio(setOfFioOccurncesNameVariants[i], SetOfFioOccurnces.m_Occurences[i]);
        }
}

void CNameClusterBuilder::AssignLocalSurnamePardigmID()
{
    yvector<CNameCluster*> allClusters;
    size_t i = 0;
    for (; i < (size_t)FIOTypeCount; i++)
        for (size_t j = 0; j < m_FIOClusterVectors[(EFIOType)i].size(); j++) {
            if (!m_FIOClusterVectors[(EFIOType)i][j].m_FullName.m_strSurname.empty())
                allClusters.push_back(&m_FIOClusterVectors[(EFIOType)i][j]);
        }

    for (i = 0; i < m_PotentialSurnames.size(); i++)
        allClusters.push_back(&m_PotentialSurnames[i]);

    for (i = 0; i < m_FIOwithSuspiciousPatronymic.size(); i++)
        allClusters.push_back(&m_FIOwithSuspiciousPatronymic[i]);

    for (i = 0; i < m_IOwithSuspiciousPatronymic.size(); i++) {
        //так как отчества из m_FIOwithSuspiciousPatronymic тоже могут стать фамилиями (CNameClusterBuilder::ResolvePatronymicInIO),
        //то и этим отчествам (как фамилиям припишем PardigmID)
        m_IOwithSuspiciousPatronymic[i].m_FullName.m_strSurname = m_IOwithSuspiciousPatronymic[i].m_FullName.m_strPatronomyc;
        allClusters.push_back(&m_IOwithSuspiciousPatronymic[i]);
    }

    std::sort(allClusters.begin(), allClusters.end(), std::mem_fun(&CNameCluster::order_by_f_pointer));
    int iCurParadigmID = 0;
    for (i = 0; i < allClusters.size(); i++) {
        iCurParadigmID++;
        SFullFIO& fullFio = allClusters[i]->m_FullName;
        //может оказаться, что этому  ФИО уже приписали
        //на предыдущих итерациях
        if (fullFio.m_iLocalSuranamePardigmID != -1)
            continue;
        fullFio.m_iLocalSuranamePardigmID = iCurParadigmID;
        if (fullFio.m_strSurname.size() < 2)
            continue;

        for (size_t j = i+1; j < allClusters.size(); j++) {
            SFullFIO& fullFio2 = allClusters[j]->m_FullName;
            if (fullFio2.m_strSurname.size() < 2)
                break;

            //считаем, что если первые две буквы не совпадают, то
            //сравнивать фамилии уже бессмысленно
            if (TWtringBuf(fullFio2.m_strSurname).SubStr(0, 2) != TWtringBuf(fullFio.m_strSurname).SubStr(0, 2))
                break;

            if (fullFio2.m_iLocalSuranamePardigmID != -1)
                continue;

            if (fullFio.m_bFoundSurname != fullFio2.m_bFoundSurname)
                continue;

            if (fullFio.m_bFoundSurname && (fullFio.m_strSurname == fullFio2.m_strSurname))
                fullFio2.m_iLocalSuranamePardigmID = iCurParadigmID;
            else if (!fullFio.m_bFoundSurname && TMorph::SimilarSurnSing(fullFio.m_strSurname,fullFio2.m_strSurname))
                    fullFio2.m_iLocalSuranamePardigmID = iCurParadigmID;

        }
    }

    for (i = 0; i < m_IOwithSuspiciousPatronymic.size(); ++i)
        //очистим фамилии, ранее восстановленные из отчеств
        m_IOwithSuspiciousPatronymic[i].m_FullName.m_strSurname.clear();
}

bool CNameClusterBuilder::Run()
{
        //присвоем PardigmID для всех фамилий (словарных и несловарных),
        //т.е. разобьем все фамилии на классы эквивалентности (для неслованых фамилийи с помощью ф-ции SimilarSurnSing)
        //и пронумеруем эти классы - номер класса и будет m_iLocalSuranamePardigmID
        AssignLocalSurnamePardigmID();

        //проверим, может и нет ничего
        {
            int i = 0;
            for (; i < FIOTypeCount; i++) {
                if (m_FIOClusterVectors[(EFIOType)i].size() > 0)
                    break;
            }

            if ((i >=  FIOTypeCount) && (m_IOwithSuspiciousPatronymic.size() == 0)
                && (m_FIOwithSuspiciousPatronymic.size() == 0))
                return true;
        }

        if (!m_bAllFiosWithSurname) {
            ResolvePatronymicInFIO();
            ResolvePatronymicInIO();
            std::sort(m_PotentialSurnames.begin(), m_PotentialSurnames.end());
            MergeEqual(m_PotentialSurnames);
        }

        for (int i = 0; i < FIOTypeCount; i++) {
            std::sort(m_FIOClusterVectors[(EFIOType)i].begin(), m_FIOClusterVectors[(EFIOType)i].end());
            //заменяем уменьшительные на полные
            ChangeFirstName((EFIOType)i);
            MergeEqual(m_FIOClusterVectors[i]);
        }

        MergeFIandFIin();
        MergeFIOandFIinOin();

        if (!m_bAllFiosWithSurname) {
            MergeFIandPotentialSurnames();
            MergeFIOandPotentialSurnames();
        }

        MergeFIandF();
        MergeFIOandF();
        if (!m_bAllFiosWithSurname)
            MergeFIandI();
        MergeFIOandFI();
        if (!m_bAllFiosWithSurname) {
            MergeIOandI();
            MergeFIOandI();
            MergeFIOandIO();
            //отрубим пока это стремное слияние для фактов
            //может надо в будущем смотреть на растояние...
            if (!m_bResolveContradictions)
                MergeFIandIO();
        }

        DeleteUsedFIOS();

        for (int i = 0; i < FIOTypeCount; i++) {
            MergeEqual(m_FIOClusterVectors[i]);
        }

        if (m_bResolveContradictions)
            //рассматриваем случаи, когда одно фио
            //слилось с двумя противоречивыми фио (А. Петров, Алексей Петров и Алесандр Петров)
            ResolveFirstNameAndPatronymicContradictions();

        for (int i = 0; i < FIOTypeCount; i++) {
            ChooseBestSurname(m_FIOClusterVectors[i]);
        }

        return true;
}
