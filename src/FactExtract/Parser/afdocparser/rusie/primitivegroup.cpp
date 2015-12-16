#include "primitivegroup.h"

#include <FactExtract/Parser/common/utilit.h>


void CPrimitiveGroup::SetActiveSymbol(int iS)
{
    CGroup::SetActiveSymbol(iS);
    for (CWord::SHomIt it = m_pWord->IterHomonyms(); it.Ok32(); ++it)
        if (it->HasTerminalSymbol(iS))
            m_ActiveHomonyms |= (1 << it.GetID());

    ResetGrammems();
}

CGroup* CPrimitiveGroup::Clone() {
    CPrimitiveGroup* pres = new CPrimitiveGroup(m_pWord, First);
    pres->SetActiveSymbol(GetActiveSymbol());
    return pres;
}

void CPrimitiveGroup::GetMainClauseWord(int& iClauseWord, int& iH) const {
    iClauseWord = FirstWord();
    if (m_ActiveHomonyms == 0)
        ythrow yexception() << " m_ActiveHomonyms in \"CPrimitiveGroup::GetMainClauseWord\"";
    for (int i = 0; (size_t)i < sizeof(ui32); i++) {
        if (((1 << i) & m_ActiveHomonyms) != 0) {
            iH = i;//что сделаешь...
            break;
        }
    }

}

void CPrimitiveGroup::IntersectChildHomonyms(const TChildHomonyms& homs) {
    YASSERT(homs.Size() == 1);
    ui32 t = m_ActiveHomonyms & homs[0];
    if (t != 0)
        m_ActiveHomonyms = t;
}

void CPrimitiveGroup::ResetGrammems()
{
    Grammems.Reset();
    for (CWord::SHomIt it = m_pWord->IterHomonyms(); it.Ok(); ++it)
        if (it->HasTerminalSymbol(GetActiveSymbol()))
            Grammems |= it->Grammems;
}

Stroka CPrimitiveGroup::DebugString() const {
    TStringStream str;
    str << "CPrimitiveGroup \"" << NStr::DebugEncode(UTF8ToWide(GetRuleName())) << "\": homonyms [" << m_ActiveHomonyms << "]";
    return str.Str();
}


