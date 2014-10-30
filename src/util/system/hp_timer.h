#pragma once

#include <util/system/defaults.h>

namespace NHPTimer
{
    typedef i64 STime;
    // may delay for ~50ms to compute frequency
    double GetSeconds(const STime &a);
    // получить текущее время
    void GetTime(STime *pTime);
    // получить время, прошедшее с момента, записанного в *pTime, при этом в *pTime будет записано текущее время
    double GetTimePassed(STime *pTime);
    // get TSC frequency, may delay for ~50ms to compute frequency
    double GetClockRate();
};
