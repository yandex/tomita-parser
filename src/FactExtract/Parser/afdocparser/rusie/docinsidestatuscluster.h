#pragma once

#pragma once
#include <FactExtract/Parser/afdocparser/rus/text.h>
#include "textprocessor.h"

struct CFioAndCoord: public SFullFIO
{
    int m_SentNo;
    int m_WordSequenceNo;
    int m_FactNo;

    bool operator < (const CFioAndCoord& _X) const
    {
        return default_order(_X);
    };
};

struct CFioWithAddress: public SFullFIO
{
    SFactAddressAndFieldName m_Address;
    bool operator < (const CFioWithAddress& _X) const
    {
        return default_order(_X);
    };
};
