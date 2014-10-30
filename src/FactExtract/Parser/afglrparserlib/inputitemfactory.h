#pragma once

#include <util/generic/vector.h>
#include "agreement.h"

class CInputItem;

class CInputItemFactory
{
public:
    CInputItemFactory(void);
    virtual ~CInputItemFactory(void);

    virtual CInputItem* CreateNewItem(size_t SymbolNo, const CRuleAgreement& agr,
                                      size_t iFirstItemNo, size_t iLastItemNo,
                                      size_t iSynMainItemNo, yvector<CInputItem*>& children) = 0;

    virtual CInputItem* CreateNewItem(size_t SymbolNo, size_t iFirstItemNo) = 0;

    virtual bool EqualNodes(const CInputItem*, const CInputItem*)  const {
        return true;
    }

    virtual Stroka GetDumpOfWords(const CInputItem*, ECharset) const {
        return "not implemented";
    }

    void SetCurStartWord(int i){
        m_iCurStartWord = i;
    }

    int    m_iCurStartWord;
};
