#pragma once

#include <util/generic/vector.h>

#include "primitivegroup.h"
#include "factgroup.h"

class CFloatingDateGroup
{
public:
    CFloatingDateGroup(yvector<i64> DateInterpretation);
    CFloatingDateGroup();
    ~CFloatingDateGroup() {};
public:
    yvector<i64> m_vDateInterpretation;
};

class CFloatingPrimitiveDateGroup :    public CFloatingDateGroup,
                                    public CPrimitiveGroup
{
public:
    CFloatingPrimitiveDateGroup(const CWord* pWord, int iW, yvector<i64> DateInterpretation);
    ~CFloatingPrimitiveDateGroup() {};
};

class CFloatingSynDateGroup : public CFloatingDateGroup,
                              public CWeightSynGroup//CSynGroup
{
public:
    CFloatingSynDateGroup(CGroup* pMainGroup, const THomonymGrammems& grammems, int iSymbolNo);
    ~CFloatingSynDateGroup(){};
};
