#include "floatingdategroup.h"


CFloatingDateGroup::CFloatingDateGroup(yvector<i64> DateInterpretation)
{
    m_vDateInterpretation = DateInterpretation;
}

CFloatingDateGroup::CFloatingDateGroup()
{
}

CFloatingPrimitiveDateGroup::CFloatingPrimitiveDateGroup(const CWord* pWord, int iW, yvector<i64> DateInterpretation):
CFloatingDateGroup(DateInterpretation), CPrimitiveGroup(pWord, iW)
{
}

CFloatingSynDateGroup::CFloatingSynDateGroup(CGroup* pMainGroup, const THomonymGrammems& grammems, int iSymbolNo)
    : CWeightSynGroup(pMainGroup, grammems, iSymbolNo)
{
}

