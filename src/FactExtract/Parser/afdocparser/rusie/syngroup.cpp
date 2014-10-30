#include "syngroup.h"
#include "primitivegroup.h"

#include <util/generic/cast.h>

CGroup* CSynGroup::Clone() {
    CSynGroup* pRes = new CSynGroup();
    pRes->SetPair(First, Last);
    pRes->Grammems = Grammems;
    pRes->m_pMainGroup = m_pMainGroup;

    pRes->m_ChildHomonyms = m_ChildHomonyms;
    pRes->SetActiveSymbol(GetActiveSymbol());
    return pRes;
}

void CSynGroup::IntersectChildHomonyms(const TChildHomonyms& homs)
{
    YASSERT(homs.Size() == m_ChildHomonyms.size());
    for (size_t i = 0; i < m_ChildHomonyms.size(); i++) {
        ui32 t = m_ChildHomonyms[i] & homs[i];
        if (t != 0)
            m_ChildHomonyms[i]= t;
    }
}

ui32 CSynGroup::GetMainHomIDs() const {
    return m_ChildHomonyms[GetMainPrimitiveGroup()->FirstWord() - FirstWord()];
}

Stroka CSynGroup::DebugString() const {
    TStringStream str;
    str << "CSynGroup \"" << NStr::DebugEncode(UTF8ToWide(GetRuleName())) << "\": homonyms [";
    for (size_t i = 0; i < m_ChildHomonyms.size(); ++i)
        str << m_ChildHomonyms[i] << ",";
    str << "]";
    return str.Str();
}

void CSynGroup::AppendChildHomonymsFrom(const yvector<CInputItem*>& children) {
    for (size_t i = 0; i < children.size(); ++i)
        AppendChildHomonymsFrom(*CheckedCast<CGroup*>(children[i]));
}
