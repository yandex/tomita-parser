#include "docinsidestatuscluster.h"
#include <FactExtract/Parser/afdocparser/rus/text.h>


// если статус состоит из одного слова, например, "директор", и в фдо-факте в poste есть вхождение этого
// слова, тогда нужно игнорировать этот статус-факт
bool CheckStatusToDelete(const CTextProcessor* pText, const SFactAddress& ad1, const SFactAddress& ad2)
{
    const CFactFields& rFact1 = pText->GetFact(ad1);
    const CTextWS* PostWS = rFact1.GetTextValue(CFactFields::Post);
    if (PostWS==NULL) return false;

    const CFactFields& rFact2 = pText->GetFact(ad2);
    const CTextWS* StatusWS = rFact2.GetTextValue(CFactFields::Status);
    if (StatusWS == NULL || StatusWS->GetLemmas().size() != 1)
        return false;

    for (size_t i = 0; i < PostWS->GetLemmas().size(); ++i)
        if (PostWS->GetLemmas()[i].m_LemmaToShow == StatusWS->GetLemmas()[0].m_LemmaToShow)
            return true;

    return false;
};

//выдает те факты, которые находятся в одном предложении и у них совпадают фио, но они из разых вхождений
//например ("А. Петров, старый козел (не путать с А.Петровым, знатным пианистом).")
void CFactsCollection::GetFactsEqualByFioToIgnore(const ymultiset<CFioWithAddress>& FioClusters, yset<SFactAddressAndFieldName>& factsToIgnore)
{
    ymultiset<CFioWithAddress>::const_iterator start;
    for (start = FioClusters.begin();  start != FioClusters.end();) {
        ymultiset<CFioWithAddress>::const_iterator  end1 = start;
        for (;  end1 != FioClusters.end()&& *end1==*start; end1++) {
            for (ymultiset<CFioWithAddress>::const_iterator end2 = start; end2 != FioClusters.end()&& *end2==*start; end2++) {
                if (end1  == end2)
                    continue;
                EFactEntryEqualityType t = GetFactEntryEqualityType(end1->m_Address, end2->m_Address, true);
                if (t == FromOneSent) {
                    factsToIgnore.insert(end1->m_Address);
                    factsToIgnore.insert(end2->m_Address);
                }
            }
        }
        start = end1;
    }
}

void CFactsCollection::GroupByFios()
{

    ymultiset<CFioWithAddress> FioClusters;
    for (SetOfFactsIt it = m_UniqueFacts.begin(); it != m_UniqueFacts.end(); it++) {
        CFactFields& rFact = m_pText->GetFact(*it);

        for (CFactFields::FioFieldIterConst fio_it=rFact.GetFioFieldsSequenceBegin(); fio_it!=rFact.GetFioFieldsSequenceEnd(); fio_it++) {
            const CFioWS& Fio = fio_it->second;
            CFioWithAddress C;
            C.m_Address.m_FactAddr = *it;
            C.m_Address.m_strFieldName = fio_it->first;
            (SFullFIO&)C = Fio.m_Fio;
            FioClusters.insert(C);
        }
    }

    yset<SFactAddressAndFieldName> factsToIgnore;
    GetFactsEqualByFioToIgnore(FioClusters, factsToIgnore);

    yset<SFactAddress> FactsToDelete;
    for (ymultiset<CFioWithAddress>::iterator start = FioClusters.begin();  start != FioClusters.end();) {
        ymultiset<CFioWithAddress>::iterator end1 = start;
        for (;  end1 != FioClusters.end()&& *end1==*start; end1++) {
            for (ymultiset<CFioWithAddress>::iterator end2 = start;  end2 != FioClusters.end()&& *end2==*start; end2++)
            if (end1  != end2) {
                //const CFioWithAddress& debug1 = *end2;
                //const CFioWithAddress& debug2 = *start;
                EFactEntryEqualityType t = GetFactEntryEqualityType(end1->m_Address, end2->m_Address);
                SEqualFactsPair Pair(t, EqualByFio,end1->m_Address, end2->m_Address);

                if (CheckStatusToDelete(m_pText, end1->m_Address.m_FactAddr, end2->m_Address.m_FactAddr))
                    FactsToDelete.insert(end1->m_Address.m_FactAddr);

                if (CheckStatusToDelete(m_pText, end2->m_Address.m_FactAddr, end1->m_Address.m_FactAddr))
                    FactsToDelete.insert(end2->m_Address.m_FactAddr);

                //если хотя бы один из фактов встречается с кем-то в одном предложении
                //и при этом у них одинаковые, но не совпадающие по положению ФИО,
                //то игнорируем такие
                if ((factsToIgnore.find(end1->m_Address) == factsToIgnore.end()) &&
                    (factsToIgnore.find(end2->m_Address) == factsToIgnore.end())) {
                    m_EqualFactPairs.insert(Pair);
                }

            }
        }
        start = end1;
    }

    for (SetOfFactsIt it = m_UniqueFacts.begin(); it != m_UniqueFacts.end();) {
        //CFactFields& rFact = m_pText->GetFact(*it);       // unused!
        if (FactsToDelete.find(*it) != FactsToDelete.end()) {
            SetOfFactsIt del_it = it;
            it++;
            m_UniqueFacts.erase(del_it);
        } else
            it++;

    }
}
