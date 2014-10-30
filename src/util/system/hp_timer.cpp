#include "hp_timer.h"
#include <util/generic/algorithm.h>
#include <util/datetime/cputimer.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NHPTimer;
const double NON_FREQUENCY = 1;
static double fProcFreq1 = NON_FREQUENCY;

//////////////////////////////////////////////////////////////////////////
static double EstimateCPUClock()
{
    ui64 startCycle = 0;
    ui64 startMS = 0;
    for(;;) {
        startMS = MicroSeconds();
        startCycle = GetCycleCount();
        ui64 n = MicroSeconds();
        if (n - startMS < 100) {
            break;
        }
    }
    Sleep(TDuration::MicroSeconds(5000));
    ui64 finishCycle = 0;
    ui64 finishMS = 0;
    for(;;) {
        finishMS = MicroSeconds();
        if (finishMS - startMS < 100)
            continue;
        finishCycle = GetCycleCount();
        ui64 n = MicroSeconds();
        if (n - finishMS < 100) {
            break;
        }
    }
    return (finishCycle - startCycle) * 1000000.0 / (finishMS - startMS);
}

static void InitHPTimer()
{
    const size_t N_VEC = 9;
    double vec[N_VEC];
    for (unsigned i = 0; i < N_VEC; ++i)
        vec[i] = EstimateCPUClock();
    std::sort(vec, vec + N_VEC);

    fProcFreq1 = 1 / vec[N_VEC / 2];
    //printf("freq = %g\n", 1 / fProcFreq1 / 1000000);
}

double NHPTimer::GetSeconds(const NHPTimer::STime &a)
{
    if (fProcFreq1 == NON_FREQUENCY) {
        InitHPTimer();
    }
    return (static_cast<double>(a)) * fProcFreq1;
}

double NHPTimer::GetClockRate()
{
    if (fProcFreq1 == NON_FREQUENCY) {
        InitHPTimer();
    }
    return 1. / fProcFreq1;
}

void NHPTimer::GetTime(STime *pTime)
{
    *pTime = GetCycleCount();
}

double NHPTimer::GetTimePassed( STime *pTime )
{
    STime old(*pTime);
    GetTime(pTime);
    return GetSeconds(*pTime - old);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
