#pragma once

#include <util/generic/vector.h>

#include "multiwordcreator.h"

class CFilterCheck
{
public:
    CFilterCheck(CMultiWordCreator& rMultiWordCreator):
        m_MultiWordCreator(rMultiWordCreator)
    {
    };
    ~CFilterCheck() {};
    bool CheckFilters(const CFilterStore& filters);
    bool CheckFiltersWithoutOrder(const CFilterStore& filters);
    bool AssignFilterItems(ymap<int, yset<int> >* FilterItemToWords, const CFilterStore& filters, int iID, bool bFull = true);
    bool CompareTermTypeWithWord(const CWord* pW, eTerminals eTerm);
    void ConvertToOriginalWords(ymap<int, yset<int> >& FilterItemToWords, const yvector<SWordHomonymNum>& ItemWrds, int iID);

private:
    CMultiWordCreator& m_MultiWordCreator;
};
