#pragma once

#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/morph.h>
#include <FactExtract/Parser/afdocparser/common/wordsequence.h>


class CFIOOccurenceInText;

struct SFullFIO
{
    TGramBitSet m_Genders;
    int   m_iLocalSuranamePardigmID; //чтобы уметь сортировать фамилии не найденные в морфологии,
                                     //припишем всем фамилиям временный PardigmID:
                                     //если фамилия из морфологии - то если они просто посимвольно совпадают
                                     //то приписываем им одинаковый PardigmID
                                     //иначе если на них срабатывает SimilarSurnSing
                                     //то приписываем им одинаковый PardigmID
                                     //фамилии из морфологии и нет между собой не сравниваем

    bool  m_bFoundSurname;
    bool  m_bInitialPatronomyc;
    bool  m_bInitialName;
    bool  m_bSurnameLemma;
    bool  m_bFoundPatronymic;

    Wtroka m_strSurname;
    Wtroka m_strName;
    Wtroka m_strPatronomyc;

    SFullFIO();
    SFullFIO(const SFullFIO& fio);

    EFIOType GetType() const;

    void Reset();
    //Wtroka BuildFIOString() const;
    void BuildFIOStringRepresentations(yset<Wtroka>& fioStrings) const;
    void BuildFIOStringsForSearsching(yset<Wtroka>& fioStrings, bool bFullName, bool bFullPatronymic, bool bAnyPatronymic) const;
    bool Gleiche(SFullFIO& fio);

    //приписывает граммемы (m_commonMuscGrammems и m_commonFemGrammems) CFIOOccurenceInText
    //исходя из имени и bRurnameLemma (только для несловарных фамилий)
    //void InitGrammems(bool bRurnameLemma, CFIOOccurenceInText& wp);

    Wtroka GetFemaleName() const;
    bool HasInitials() const;
    bool HasInitialName() const;
    bool HasInitialPatronomyc() const;
    void MakeLower();
    void MakeUpper();
    bool IsEmpty() const;
    bool operator<(const SFullFIO& FIO) const;
    bool operator==(const SFullFIO& FIO) const;
    bool order_by_f(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_i(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_fi(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_fio(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_fioIn(const SFullFIO& FIO2, bool bUseInitials = true) const;
    bool order_by_fiInoIn(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_io(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_o(const SFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_oIn(const SFullFIO& cluster2) const;
    bool order_by_iIn(const SFullFIO& FIO2) const;
    bool order_by_fiIn(const SFullFIO& FIO2, bool bUseInitials = true) const;

    bool default_order(const SFullFIO& cluster2) const;

    bool equal_by_f(const SFullFIO& FIO2, bool bUseInitials = true) const;
    bool equal_by_i(const SFullFIO& FIO2, bool bUseInitials = true) const;
    bool equal_by_o(const SFullFIO& FIO2, bool bUseInitials = true) const;
    Wtroka ToString() const;
    Wtroka ToStringForChecker() const;

    Wtroka GetNameStub() const
    {
        return g_strNameStubShort;
    }

    Wtroka GetSurnameMark() const
    {
        return Wtroka();
    }

    Wtroka GetPatronomycStub() const
    {
        return g_strPatronomycStubShort;
    }

    Wtroka GetInitialNameStub() const
    {
        return g_strInitialNameStubShort;
    }

    Wtroka GetInitialPatronomycStub() const
    {
        return g_strInitialPatronomycStubShort;
    }

    Wtroka GetFioFormPrefix() const
    {
        return g_strFioFormPrefix;
    }
    Wtroka GetFioLemmaPrefix() const
    {
        return g_strFioLemmaPrefix;
    }

    Wtroka GetFemaleSurname() const;
    Wtroka PredictFemaleSurname() const;
    Wtroka GetFemaleSurnameFromMorph(Wtroka strSurname, bool& bFoundInMorph) const;
    Wtroka PredicSurnameAsAdj(TGrammar gender) const;

};

class CFIOOccurence : public CWordSequenceWithAntecedent
{
public:
    SWordHomonymNum m_NameMembers[NameTypeCount];

    CFIOOccurence()
    {
        PutWSType(FioWS);
        m_iSimilarOccurencesCount = 0;
    }

    CFIOOccurence(const CWordsPair& w_p)
    {
        SetPair(w_p);
        PutWSType(FioWS);
        m_iSimilarOccurencesCount = 0;
    }

    CFIOOccurence(int i)
    {
        SetPair(i,i);
        PutWSType(FioWS);
        m_iSimilarOccurencesCount = 0;
    }

    int m_iSimilarOccurencesCount;  //количество других вхождений ФИО, которые смогли слиться с этим
                                    //(нужно для выбора варианта у одной цепочки ФИО ("Евгения Петрова"))

};

class CFioWordSequence : public CFIOOccurence
{
public:
    CFioWordSequence(const SFullFIO& fio)
        : m_Fio(fio)
    {
    }

    CFioWordSequence()
        : m_Fio()
    {
    }

    SFullFIO& GetFio()
    {
        return m_Fio;
    }

    virtual Wtroka GetLemma() const;
    virtual Wtroka GetCapitalizedLemma() const;

    SFullFIO m_Fio;
};
