#include "group.h"
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/rusie/primitivegroup.h>


const CHomonym* CGroup::GetFirstMainHomonym() const
{
    int t = GetMainTerminalSymbol();
    const CWord* w = GetMainWord();
    const CPrimitiveGroup* pMainPrimitiveGroup = GetMainPrimitiveGroup();
    ui32 homIDs = pMainPrimitiveGroup->GetMainHomIDs();
    CWord::SHomIt it = w->IterHomonyms();
    for (; it.Ok(); ++it)
        if (((1 << it.GetID()) & homIDs) && it->ContainTerminal(t))
            break;

    int iH = -1;
    if (it.Ok())
        iH = it.GetID();
    else
        iH = w->HasFirstHomonymByTerminal_i(t);
    if (iH == -1)
        ythrow yexception() << "MainWord of Group contains no homonym with MainTerminalSymbol.";
    return &w->GetRusHomonym(iH);
}


bool CGroup::HasEqualGrammems(const CGroup* other) const {
    return this->GetGrammems() == other->GetGrammems();
}

