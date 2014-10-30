#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/set.h>

#include "factaddress.h"
#include "factfields.h"
#include <FactExtract/Parser/common/sdocattributes.h>

enum EFactEqualityType {EqualByFio, EqualByComp};

class CTextProcessor;

struct SFactAddressAndFieldName
{
    Stroka            m_strFieldName;
    SFactAddress    m_FactAddr;
    bool operator<(const SFactAddressAndFieldName& _X) const
    {
        if (!(m_FactAddr == _X.m_FactAddr))
            return m_FactAddr < _X.m_FactAddr;

        return m_strFieldName < _X.m_strFieldName;
    };

    bool operator==(const SFactAddressAndFieldName& _X) const
    {
        return        (m_FactAddr == _X.m_FactAddr)
                &&    (m_strFieldName == _X.m_strFieldName);
    };
};

class SEqualFactsPair
{
    // m_Fact1 должен быть не больше, чем m_Fact2, на этои основывается operator <
    SFactAddressAndFieldName    m_Fact1;
    SFactAddressAndFieldName    m_Fact2;

public:
    EFactEqualityType            m_EqualityType;
    EFactEntryEqualityType        m_EntryType;

    SEqualFactsPair(EFactEntryEqualityType entryType, EFactEqualityType _T , const SFactAddressAndFieldName& Fact1, const SFactAddressAndFieldName& Fact2);
    bool operator<(const SEqualFactsPair& EqualFactsPair) const;
    const SFactAddressAndFieldName& GetFact1()    const { return m_Fact1; };
    const SFactAddressAndFieldName& GetFact2()    const { return m_Fact2; };
};

struct SEqualFacts
{
    SEqualFacts(EFactEqualityType _T , const char* strFieldName)
    {
        m_EqualityType = _T;
        m_strFieldName = strFieldName;
    };

    bool operator<(const SEqualFacts& EqualFacts) const
    {
        return (m_EqualityType < EqualFacts.m_EqualityType
                  || m_EqualFacts < EqualFacts.m_EqualFacts
                 );
    }

    bool operator==(const SEqualFacts& EqualFacts) const
    {
        return (m_EqualityType == EqualFacts.m_EqualityType
                  && m_EqualFacts == EqualFacts.m_EqualFacts
                 );
    }

    yset<SFactAddress> m_EqualFacts;
    EFactEqualityType m_EqualityType;

    struct SPairsIterator
    {
        SPairsIterator(yset<SFactAddress>::const_iterator itBeg, yset<SFactAddress>::const_iterator itEnd,
                       const Stroka& strFieldName, const CTextProcessor& text);

        SPairsIterator& operator++();
        void GetFactsPair(SFactAddress& factAddress1, SFactAddress& factAddress2, EFactEntryEqualityType& factEntryEqualityType) const;
        bool IsEnd() const;

        protected:
            yset<SFactAddress>::const_iterator m_It1;
            yset<SFactAddress>::const_iterator m_It2;
            yset<SFactAddress>::const_iterator m_ItEnd;
            const CTextProcessor& m_Text;
            Stroka m_strFieldName;
    };

    Stroka m_strFieldName; //поле по которому эти факты эквивалентны

    SEqualFacts::SPairsIterator GetPairsIterator(const CTextProcessor& pText) const;
};

struct CFioWithAddress;

class CFactsCollection
{
public:

    typedef yset<SEqualFacts>::const_iterator SetOfEqualFactsConstIt;
    typedef yset<SEqualFactsPair>::const_iterator SetOfEqualFactPairsConstIt;
    typedef yset<SFactAddress>::iterator SetOfFactsIt;
    typedef yset<SFactAddress>::const_iterator SetOfFactsConstIt;
    typedef yset<SEqualFacts>::iterator SetOfEqualFactsIt;

    CFactsCollection(CTextProcessor* pText);
    virtual ~CFactsCollection(void);
    void AddToUniqueFacts(const SFactAddress& factAddress);
    void Init();
    void GroupByCompanies();
    void GroupByFios();
    void GroupByQuotes();
    void Reset();
    SetOfEqualFactsConstIt GetEqualFactsItBegin() const;
    SetOfEqualFactsConstIt GetEqualFactsItEnd() const;

    SetOfEqualFactPairsConstIt GetEqualFactPairsItBegin() const;
    SetOfEqualFactPairsConstIt GetEqualFactPairsItEnd() const;

    SetOfFactsConstIt GetFactsItBegin() const;
    SetOfFactsConstIt GetFactsItEnd() const;
    int GetFactsCount() const;

    EFactEntryEqualityType GetFactEntryEqualityType(const SFactAddressAndFieldName& factAddress1, const SFactAddressAndFieldName& factAddress2, bool bUseSameSent = false) const;

    //география и адреса
    void GetAddressGeoAttrVal(const SDocumentAttribtes* docAttrs, Stroka& sAddr, Stroka& sLeads, ECharset encoding) const;

protected:
    yset<SFactAddress>    m_UniqueFacts;
    yset<SEqualFacts>    m_EqualFacts;
    yset<SEqualFactsPair>    m_EqualFactPairs;

    CTextProcessor* m_pText;

private:
    void GetFactsEqualByFioToIgnore(const ymultiset<CFioWithAddress>& FioClusters, yset<SFactAddressAndFieldName>& factsToIgnore);
    bool IsEqualCompaniesWords(const yvector< SetOfFactsIt >& CompFacts, int iCompF, int iCompS, bool bShortS = false) const;
    bool IsEqualCompaniesLemmas(const CWordSequence* pWSF, const CWordSequence* pWSS) const;
    void GroupByCompanies(const yvector< SetOfFactsIt >& rCompFacts);
    void CheckEqualCompaniesCommonLemma();
    void CheckEqualCompaniesWordsPair();
    //география и адреса
    Stroka GetAddressGeoFieldVal(const CTextWS* pWS, ECharset encoding) const;
};

class SFactAddressOrder : public std::binary_function<const SFactAddress*, const SFactAddress*, bool>
{
public:
    SFactAddressOrder(const CTextProcessor* pText)
    {
        m_pText = pText;
    }

    bool operator()(const SFactAddress* adr1, const SFactAddress* adr2) const;

protected:
    const CTextProcessor* m_pText;
};
