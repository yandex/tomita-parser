#pragma once

#include <util/system/defaults.h>

#include <cmath>
#include <cfloat>

static const double PI = 3.14159265358979323846;
static const double M_LOG2_10 = 3.32192809488736234787; // log2(10)

/** Returns base 2 logarithm */
double Log2(double);
float Log2f(float);

/** Returns 2^x */
double Exp2(double);
float Exp2f(float);

template <class T>
static inline T Sqr(T t) throw () {
    return t * t;
}

static inline bool IsFinite(double f) {
#if defined(isfinite) || defined(_musl_)
    return isfinite(f);
#elif defined(_win_)
    return _finite(f) != 0;
#elif defined(_darwin_)
    return finite(f);
#else
    return finitef(f);
#endif
}

static inline bool IsNan(double f) {
#if defined(_win_)
    return _isnan(f) != 0;
#else
    return isnan(f);
#endif
}

inline bool IsValidFloat(double f) {
    return IsFinite(f) && !IsNan(f);
}

#ifdef _MSC_VER
double Erf(double x);
#   if _MSC_VER < 1800
    inline double round(double r) {
        return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
    }
#   endif
#else
inline double Erf(double x) {
    return erf(x);
}
#endif

#if defined(__GNUC__)
inline double LogGamma(double x) {
    return __builtin_lgamma(x);
}
#elif defined(_unix_)
inline double LogGamma(double x) {
    return lgamma(x);
}
#else
inline double LogGamma(double x) {
    extern double LogGammaImpl(double);

    return LogGammaImpl(x);
}
#endif
