#include "tools.h"

#include <util/generic/ymath.h>
#include <util/generic/algorithm.h>

#include <util/draft/format.h>

namespace NTools {

Stroka PrintItemPerSecond(TDuration time, size_t itemscount, int kbase) {
    TStringStream res;
    double seconds = (double)(time.MilliSeconds()) / 1000.0;
    if (itemscount == 0)
        res << "0";
    else if (time == TDuration::Zero() && seconds == 0.0)
        res << "INF";
    else
        res << NFormat::Short<double>(itemscount / seconds, kbase);
    return res.Str();
}

Stroka PrintTimePerItem(TDuration time, size_t count) {
    TStringStream res;
    if (time == TDuration::Zero())
        res << "0";
    else if (count == 0)
        res << "INF";
    else
        res << NFormat::Duration(time / count);

    return res.Str();
}

static Stroka Summary(TInstant start, size_t totalLines, TInstant prev, size_t prevLines, bool shrinked = true) {
    TInstant current = Now();
    TStringStream res;

    if (shrinked)
        res << NFormat::Short(totalLines);
    else
        res << totalLines;

    res << " processed (" << NFormat::Duration(current - start) << ", "
        << PrintTimePerItem(current - prev, prevLines) << " -> "
        << PrintTimePerItem(current - start, totalLines) << " avg)";

    return res.Str();
}

static Stroka SummaryBytes(TInstant start, size_t totalLines, TInstant prev, size_t prevLines, bool shrinked = true) {
    TInstant current = Now();

    const size_t KB = 1024;
    const size_t MB = 1024*KB;

    TStringStream res;

    if (shrinked)
        res << NFormat::Short(totalLines, KB);
    else
        res << totalLines;

    res << " bytes processed (" << NFormat::Duration(current - start) << ", "
        << PrintTimePerItem(current - prev, prevLines/MB) << "/MB -> "
        << PrintTimePerItem(current - start, totalLines/MB) << "/MB, "
        << PrintItemPerSecond(current - start, totalLines, KB) << "B/s avg)";

    return res.Str();
}


TPerformanceCounterBase::TPerformanceCounterBase(const Stroka& name, size_t logFreq, bool suspend)
    : Name(name)
    , LogFreq(logFreq)
    , StartTime(TInstant::Now())
    , PrevTime(StartTime)
    , LastSuspend(TInstant::Zero())
    , Output(&Clog)
{
    if (suspend)
        LastSuspend = StartTime;
}

void TPerformanceCounterBase::PrintCustomInfo() const {
    Stroka custom = CustomInfo();
    if (!!custom)
        (*Output) << ", " << custom;
}

void TPerformanceCounterBase::DoPrint(bool finished, const size_t currentCounter) const {
    (*Output) << Name << ": " << Summary(StartTime, currentCounter, PrevTime, LogFreq, !finished);
    PrintCustomInfo();
    (*Output) << Endl;
}

void TPerformanceCounterBase::Print(const size_t currentCounter) const {
    DoPrint(false, currentCounter);
}

void TPerformanceCounterBase::Finish() const {
    if (const size_t currentCounter = GetCurrentCounter()) {
        (*Output) << "(Finished) ";
        DoPrint(true, currentCounter);
    }
}


void TBytePerformanceCounter::DoPrint(bool finished, const size_t currentCounter) const {
    (*Output) << Name << ": " << SummaryBytes(StartTime, currentCounter, PrevTime, LogFreq, !finished);
    PrintCustomInfo();
    (*Output) << Endl;
}


} //namespace NTools
