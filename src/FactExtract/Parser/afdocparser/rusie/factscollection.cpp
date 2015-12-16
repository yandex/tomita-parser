#include "factscollection.h"
#include "textprocessor.h"

#include <util/string/cast.h>

//------------------SPairsIterator-------------------------------

SEqualFacts::SPairsIterator::SPairsIterator(yset<SFactAddress>::const_iterator itBeg,
                                            yset<SFactAddress>::const_iterator itEnd,
                                            const Stroka& strFieldName,
                                            const CTextProcessor& text):
    m_Text(text)

{
    m_strFieldName = strFieldName;
    m_ItEnd = itEnd;
    m_It1 = itBeg;
    m_It2 = m_It1;
    m_It2++;
}

bool SEqualFacts::SPairsIterator::IsEnd() const
{
    return m_It1 == m_ItEnd;
}

SEqualFacts::SPairsIterator& SEqualFacts::SPairsIterator::operator++()
{
    m_It2++;
    if (m_It2 == m_ItEnd) {
        m_It1++;
        m_It2 = m_It1;
        m_It2++;
        if (m_It2 == m_ItEnd)
            m_It1 = m_ItEnd;
    }
    return *this;
}

//выдает все возможные пары из множества m_EqualFacts,
//не выдаются пары с одинаковыми элементами, и среди пар, которые отличаются только порядком выдаются только одни из них
void SEqualFacts::SPairsIterator::GetFactsPair(SFactAddress& factAddress1, SFactAddress& factAddress2, EFactEntryEqualityType& factEntryEqualityType) const
{
    if ((m_It1 == m_ItEnd) || (m_It2 == m_ItEnd))
        ythrow yexception() << "Bad iterator in \"CFactsCollection::SPairsIterator::GetFactsPair\"";
    factAddress1 = *m_It1;
    factAddress2 = *m_It2;

    factEntryEqualityType = FromOneDoc;

    if ((factAddress1.m_iSentNum == factAddress2.m_iSentNum) &&
        (factAddress1.m_WordNum == factAddress2.m_WordNum) &&
        (factAddress1.m_HomNum == factAddress2.m_HomNum)) {
        const CFactFields& fact1 =  m_Text.GetFact(factAddress1);
        const CFactFields& fact2 =  m_Text.GetFact(factAddress2);

        if (*fact1.GetValue(m_strFieldName) == *fact2.GetValue(m_strFieldName))
            factEntryEqualityType = SameEntry;
    }
}

//------------------------SEqualFactsPair-----------------------------

SEqualFactsPair::SEqualFactsPair(EFactEntryEqualityType entryType, EFactEqualityType _T , const SFactAddressAndFieldName& Fact1, const SFactAddressAndFieldName& Fact2)
{
    m_EqualityType = _T;
    m_EntryType = entryType;
    m_Fact1 = Fact1;
    m_Fact2 = Fact2;
    if (m_Fact2 < m_Fact1)
        std::swap(m_Fact2, m_Fact1);
};

bool SEqualFactsPair::operator<(const SEqualFactsPair& EqualFactsPair) const
{

    if (m_EqualityType != EqualFactsPair.m_EqualityType)
        return m_EqualityType < EqualFactsPair.m_EqualityType;

    if (!(m_Fact1 == EqualFactsPair.m_Fact1))
        return m_Fact1 < EqualFactsPair.m_Fact1;

    return m_Fact2 < EqualFactsPair.m_Fact2;
};

//------------------------SEqualFacts-----------------------------

SEqualFacts::SPairsIterator SEqualFacts::GetPairsIterator(const CTextProcessor& text) const
{
    return SEqualFacts::SPairsIterator(m_EqualFacts.begin(), m_EqualFacts.end(), m_strFieldName, text);
}

//----------------------SFactAddressOrder--------------------------

bool SFactAddressOrder::operator()(const SFactAddress* adr1, const SFactAddress* adr2) const
{
    if (adr1->m_iSentNum != adr2->m_iSentNum)
        return adr1->m_iSentNum < adr2->m_iSentNum;

    const CWordVector& rRusWords = m_pText->GetSentenceProcessor(adr1->m_iSentNum)->GetRusWords();
    const CWordsPair& rWP1 = rRusWords.GetWord((SWordHomonymNum)(*adr1)).GetSourcePair();
    const CWordsPair& rWP2 = rRusWords.GetWord((SWordHomonymNum)(*adr2)).GetSourcePair();

    if (rWP1.FirstWord() != rWP2.FirstWord())
        return rWP1.FirstWord() < rWP2.FirstWord();

    if (adr1->m_HomNum != adr2->m_HomNum)
        return adr1->m_HomNum < adr2->m_HomNum;
    if (adr1->m_iFactNum != adr2->m_iFactNum)
        return adr1->m_iFactNum < adr2->m_iFactNum;
    return false;
}

//------------------CFactsCollection-------------------------------

CFactsCollection::CFactsCollection(CTextProcessor* pText)
{
    m_pText = pText;
}

CFactsCollection::~CFactsCollection(void)
{
}

void CFactsCollection::Init()
{
    m_pText->FillFactAddresses(m_UniqueFacts);
}

EFactEntryEqualityType CFactsCollection::GetFactEntryEqualityType(const SFactAddressAndFieldName& factAddress1, const SFactAddressAndFieldName& factAddress2, bool bUseSameSent /*=false*/) const
{
    EFactEntryEqualityType factEntryEqualityType = FromOneDoc;
    const SFactAddress& addr1 = factAddress1.m_FactAddr;
    const SFactAddress& addr2 = factAddress2.m_FactAddr;

    if (addr1.m_iSentNum == addr2.m_iSentNum) {
        const CFactFields& fact1 =  m_pText->GetFact(factAddress1.m_FactAddr);
        const CFactFields& fact2 =  m_pText->GetFact(factAddress2.m_FactAddr);

        if (bUseSameSent)
            factEntryEqualityType = FromOneSent;

        if (*fact1.GetValue(factAddress1.m_strFieldName) == *fact2.GetValue(factAddress2.m_strFieldName)) {

                    factEntryEqualityType = SameEntry;
            }
    }

    return factEntryEqualityType;
}

void CFactsCollection::GroupByCompanies()
{
    yvector< SetOfFactsIt > CompFacts;
    for (SetOfFactsIt it = m_UniqueFacts.begin(); it != m_UniqueFacts.end(); it++) {
        CFactFields& rFacts = m_pText->GetFact(*it);
        if (NULL != rFacts.GetTextValue(CFactFields::CompanyName))
            CompFacts.push_back(it);
    }
    GroupByCompanies(CompFacts);
    CheckEqualCompaniesCommonLemma();
    CheckEqualCompaniesWordsPair();
}

void CFactsCollection::CheckEqualCompaniesCommonLemma()
{
    for (SetOfEqualFactsIt it = m_EqualFacts.begin(); it != m_EqualFacts.end(); it++) {
        if (EqualByComp != it->m_EqualityType) continue;
        Wtroka sLemmas;
        Wtroka sLemmasCopy;

        yset< Wtroka > ShortNames;
        for (yset<SFactAddress>::const_iterator fact_it = it->m_EqualFacts.begin(); fact_it != it->m_EqualFacts.end(); fact_it++) {
            CFactFields& rFacts = m_pText->GetFact(*fact_it);
            const CSentenceRusProcessor *ptrSentenceRus = m_pText->GetSentenceProcessor(fact_it->m_iSentNum);
            const CWordSequence* WS = rFacts.GetTextValue(CFactFields::CompanyShortName);
            if (!WS) continue;
            ptrSentenceRus->GetWSLemmaString(sLemmasCopy, *WS, false);
            if (!sLemmasCopy.empty())
                ShortNames.insert(sLemmasCopy);
        }

        for (yset<SFactAddress>::const_iterator fact_it = it->m_EqualFacts.begin(); fact_it != it->m_EqualFacts.end(); fact_it++) {
            CFactFields& rFacts = m_pText->GetFact(*fact_it);
            const CSentenceRusProcessor *ptrSentenceRus = m_pText->GetSentenceProcessor(fact_it->m_iSentNum);
            const CWordSequence* WS = rFacts.GetTextValue(CFactFields::CompanyName);
            assert(WS != NULL);

            if (rFacts.GetBoolValue(CFactFields::CompanyIsLemma))
                if (ptrSentenceRus->GetWSLemmaString(sLemmasCopy, *WS, false)) {
                    if (ShortNames.find(sLemmasCopy) == ShortNames.end()) {
                        sLemmas = sLemmasCopy;
                        break;
                    } else
                        continue;
                }
            ptrSentenceRus->GetWSLemmaString(sLemmasCopy, *WS, false);
            if (sLemmasCopy.size() > sLemmas.size())
                sLemmas = sLemmasCopy;
        }
        if (sLemmas.empty()) continue;

        for (yset<SFactAddress>::const_iterator fact_it = it->m_EqualFacts.begin(); fact_it != it->m_EqualFacts.end(); fact_it++) {
            CFactFields& rFacts = m_pText->GetFact(*fact_it);
            CWordSequence* WS = (CWordSequence*)rFacts.GetTextValue(CFactFields::CompanyName);
            assert(WS != NULL);
            WS->SetCommonLemmaString(sLemmas);
        }
    }
}

void CFactsCollection::CheckEqualCompaniesWordsPair()
{
    for (SetOfEqualFactsIt it = m_EqualFacts.begin(); it != m_EqualFacts.end(); it++) {
        if (EqualByComp != it->m_EqualityType) continue;

        //pair<sentence number, word number>
        ymap< TPair<int, int>, yvector<CWordsPair*> > mpEqualFields;

        for (yset<SFactAddress>::const_iterator fact_it = it->m_EqualFacts.begin(); fact_it != it->m_EqualFacts.end(); fact_it++) {
            CFactFields& rFacts = m_pText->GetFact(*fact_it);
            CWordSequence* WS = (CWordSequence*)rFacts.GetTextValue(CFactFields::CompanyName);
            assert(WS != NULL);
            TPair<int, int> WS_ID(fact_it->m_iSentNum, WS->FirstWord());
            ymap< TPair<int, int>, yvector<CWordsPair*> >::iterator equal_it = mpEqualFields.find(WS_ID);
            if (equal_it != mpEqualFields.end())
                equal_it->second.push_back(WS);
            else
                mpEqualFields[WS_ID] = yvector<CWordsPair*>(1, WS);
        }

        ymap< TPair<int, int>, yvector<CWordsPair*> >::iterator equal_it = mpEqualFields.begin();
        for (; equal_it != mpEqualFields.end(); equal_it++) {
            if (equal_it->second.size() < 2) continue;
            CWordsPair largeWP;
            for (size_t i = 0; i < equal_it->second.size(); i++)
                if (equal_it->second[i]->Size() > largeWP.Size())
                    largeWP.SetPair(*(equal_it->second[i]));

            for (size_t i = 0; i < equal_it->second.size(); i++)
                if (equal_it->second[i]->Size() != largeWP.Size())
                    equal_it->second[i]->SetPair(largeWP);
        }
    }
}

void CFactsCollection::GroupByCompanies(const yvector< SetOfFactsIt >& rCompFacts)
{
    yvector<bool> processed(rCompFacts.size(), false);
    yvector<int> current;

    for (size_t i = 0; i < rCompFacts.size(); ++i) {
        if (processed[i])
            continue;

        current.clear();
        current.push_back(i);
        for (size_t mm = 0; mm < current.size(); ++mm)
            for (size_t j = i + 1; j < rCompFacts.size(); ++j)
                if (!processed[j] && (IsEqualCompaniesWords(rCompFacts, current[mm], j, false) ||
                                      IsEqualCompaniesWords(rCompFacts, current[mm], j, true) ||
                                      IsEqualCompaniesWords(rCompFacts, j, current[mm], true))) {
                    current.push_back(j);
                    processed[j] = true;
                }

        if (current.size() > 1) {
            SEqualFacts EqualFacts(EqualByComp, "CompanyName");
            for (size_t kk = 0; kk < current.size(); ++kk)
                EqualFacts.m_EqualFacts.insert(*(rCompFacts[current[kk]]));
            m_EqualFacts.insert(EqualFacts);
        }
    }
}

CFactsCollection::SetOfEqualFactPairsConstIt CFactsCollection::GetEqualFactPairsItBegin() const
{
    return m_EqualFactPairs.begin();
}

CFactsCollection::SetOfEqualFactPairsConstIt CFactsCollection::GetEqualFactPairsItEnd() const
{
    return m_EqualFactPairs.end();
}

bool CFactsCollection::IsEqualCompaniesWords(const yvector< SetOfFactsIt >& CompFacts, int iCompF, int iCompS, bool bShortS) const
{
    CFactFields& rFactsF = m_pText->GetFact(*(CompFacts[iCompF]));
    CFactFields& rFactsS = m_pText->GetFact(*(CompFacts[iCompS]));

    const CWordSequence* WSF = rFactsF.GetTextValue(CFactFields::CompanyName);
    const CWordSequence* WSS = NULL;
    if (!bShortS)
        WSS = rFactsS.GetTextValue(CFactFields::CompanyName);
    else
        WSS = rFactsS.GetTextValue(CFactFields::CompanyShortName);

    if (NULL == WSF || NULL == WSS) return false;
    //в случае конкатенации сравнивать по леммам, а не по словам
    if (!bShortS && (WSF->IsArtificialPair() || WSF->IsConcatenationLemma() || WSS->IsArtificialPair() || WSS->IsConcatenationLemma()))
        return IsEqualCompaniesLemmas(WSF, WSS);
    if (WSF->Size() != WSS->Size()) return false;

    CSentenceRusProcessor *ptrSentenceRusF = m_pText->GetSentenceProcessor(CompFacts[iCompF]->m_iSentNum);
    CSentenceRusProcessor *ptrSentenceRusS = m_pText->GetSentenceProcessor(CompFacts[iCompS]->m_iSentNum);

    const CWordVector& rRusWordsF = ptrSentenceRusF->GetRusWords();
    const CWordVector& rRusWordsS = ptrSentenceRusS->GetRusWords();

    for (size_t i = 0; i < WSF->Size(); ++i) {
        const CWord& WrdF = rRusWordsF.GetOriginalWord(i + WSF->FirstWord());
        const CWord& WrdS = rRusWordsS.GetOriginalWord(i + WSS->FirstWord());

        if (!WrdF.HasCommonHomonyms(WrdS))
            return false;
    }

    return true;
}

void CFactsCollection::AddToUniqueFacts(const SFactAddress& factAddress)
{
    m_UniqueFacts.insert(factAddress);
}

CFactsCollection::SetOfEqualFactsConstIt CFactsCollection::GetEqualFactsItBegin() const
{
    return m_EqualFacts.begin();
}

CFactsCollection::SetOfEqualFactsConstIt CFactsCollection::GetEqualFactsItEnd() const
{
    return m_EqualFacts.end();
}

CFactsCollection::SetOfFactsConstIt CFactsCollection::GetFactsItBegin() const
{
    return m_UniqueFacts.begin();
}

int CFactsCollection::GetFactsCount() const
{
    return m_UniqueFacts.size();
}

CFactsCollection::SetOfFactsConstIt CFactsCollection::GetFactsItEnd() const
{
    return m_UniqueFacts.end();
}

void CFactsCollection::Reset()
{
    m_UniqueFacts.clear();
    m_EqualFacts.clear();
    m_EqualFactPairs.clear();
}

bool CFactsCollection::IsEqualCompaniesLemmas(const CWordSequence* pWSF, const CWordSequence* pWSS) const
{
    return pWSF->GetLemma() == pWSS->GetLemma();
}

//*********************logs адресов***********************

void CFactsCollection::GetAddressGeoAttrVal(const SDocumentAttribtes* docAttrs, Stroka& sAddr, Stroka& sLeads, ECharset encoding) const
{
    ymap<size_t, yvector<CWordsPair> > vLeads;
    for (SetOfFactsConstIt it = m_UniqueFacts.begin(); it != m_UniqueFacts.end(); it++) {
        const SWordHomonymNum& rWHN = (const SWordHomonymNum&)(*it);
        const CFactFields& rFact = m_pText->GetFact(*it);
        const CTextWS* pWSStreet = rFact.GetTextValue(CFactFields::Street);
        if (NULL != pWSStreet) {
            if (sAddr.empty())
                sAddr = ::ToString(docAttrs->m_iDocID);

            sAddr += "\t";
            sAddr += GetAddressGeoFieldVal(pWSStreet, encoding);

            const CTextWS* pWSStreetDescr = rFact.GetTextValue(CFactFields::StreetDescr);
            sAddr += " ";
            sAddr += GetAddressGeoFieldVal(pWSStreetDescr, encoding);

            const CTextWS* pWSHouse = rFact.GetTextValue(CFactFields::House);
            sAddr += " ";
            sAddr += GetAddressGeoFieldVal(pWSHouse, encoding);

            const CTextWS* pWSKorpus = rFact.GetTextValue(CFactFields::Korpus);
            sAddr += " ";
            sAddr += GetAddressGeoFieldVal(pWSKorpus, encoding);

            const CTextWS* pWSTown = rFact.GetTextValue(CFactFields::Settlement);
            sAddr += " ";
            sAddr += GetAddressGeoFieldVal(pWSTown, encoding);

            const CWordsPair& rWP = m_pText->GetSentence(it->m_iSentNum)->GetRusWords().GetWord(rWHN).GetSourcePair();
            ymap<size_t, yvector<CWordsPair> >::iterator itLeads = vLeads.find(it->m_iSentNum);
            if (itLeads != vLeads.end())
                itLeads->second.push_back(rWP);
            else
                vLeads[it->m_iSentNum] = yvector<CWordsPair>(1, rWP);
        }
    }
    if (!sAddr.empty()) {
        Stroka sUrl;
        Wtroka sTitle;
        if (!docAttrs->m_strUrl.empty())
            sUrl = docAttrs->m_strUrl;
        else
            sUrl = "null";
        if (!docAttrs->m_strTitle.empty())
            sTitle = docAttrs->m_strTitle;
        else
            NStr::Assign(sTitle, "no title");
        sLeads = Substitute("$0\t$1\t$2\t", docAttrs->m_iDocID, sUrl, WideToChar(sTitle, encoding));
        ymap<size_t, yvector<CWordsPair> >::const_iterator it_lead = vLeads.begin();
        for (; it_lead != vLeads.end(); it_lead++)
            sLeads += m_pText->GetSentence(it_lead->first)->ToHTMLColorString(it_lead->second, "red", encoding);
    }
}

Stroka CFactsCollection::GetAddressGeoFieldVal(const CTextWS* pWS, ECharset encoding) const
{
    if (!pWS)
        return "0";

    Stroka s_tmp;
    for (size_t i = 0; i < pWS->GetLemmas().size(); ++i) {
        s_tmp += "_";
        s_tmp += WideToChar(pWS->GetLemmas()[i].m_LemmaToShow, encoding);
    }

    for (Stroka::iterator it = s_tmp.begin(); it != s_tmp.end(); ++it)
        if (*it == ' ')
            *it = '_';

    if (s_tmp.empty())
        return "0";

    return s_tmp;
}
