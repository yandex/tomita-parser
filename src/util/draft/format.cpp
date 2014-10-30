#include "format.h"

#include <util/generic/ymath.h>
#include <util/stream/printf.h>
#include <util/stream/format.h>

namespace NFormat {

static inline const char* FloatMask(double num) {
    num = fabs(num);

    if (num < 1)
        return "%.2f";
    else if (num < 10)
        return "%.2f";
    else if (num < 100)
        return "%.1f";
    else
        return "%.0f";
}

TOutputStream& operator << (TOutputStream& o, const TSimpleCount<double>& f) {
    Printf(o, FloatMask(f.Count), f.Count);
    return o;
}

static inline i64 Round(double value) {
    double res1 = floor(value);
    double res2 = ceil(value);
    return (value - res1 < res2 - value)? (i64)res1 : (i64)res2;
}

static inline double ToSeconds(TDuration time) {
    return (double)(time.MilliSeconds()) / 1000.0;
}


TOutputStream& operator << (TOutputStream& o, const TShortCount<TDuration>& f) {
    ui64 microSeconds = f.Count.MicroSeconds();
    if (microSeconds < 1000)
        return o << Short(microSeconds) << "us";
    if (microSeconds < 1000000)
        return o << Short((double)microSeconds/1000.0) << "ms";

    double seconds = ToSeconds(f.Count);
    if (seconds < 60)
        return o << Short(seconds) << 's';

    ui64 s = Round(seconds*1000 + 0.5) / 1000;

    ui64 m = s / 60;
    s = s % 60;

    ui64 h = m / 60;
    m = m % 60;

    ui64 d = h / 24;
    h = h % 24;

    ui64 times[] = {d, h, m, s};
    char names[] = {'d', 'h', 'm', 's'};
    bool first = true;
    for (size_t i = 0; i < ARRAY_SIZE(times); ++i)
        if (times[i] > 0) {
            if (!first)
                o << ' ';
            o << times[i] << names[i];
            first = false;
        }
    return o;
}



static inline bool IsZero(double v) {
    return fabs(v) <= FLT_EPSILON;
}

TOutputStream& operator << (TOutputStream& o, const TPercents& f) {
    if (IsZero(f.Value))
        o << "0%";
    else if (IsZero(f.Total))
        o << "Inf%";
    else
        o << Short(f.Value*100.0/f.Total) << "%";

    return o;
}

} // namespace NFormat
