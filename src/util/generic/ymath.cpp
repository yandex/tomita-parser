#include "ymath.h"
#include "singleton.h"

#include <cmath>

template <class T>
struct TLog2Holder {
    inline TLog2Holder()
        : Log2Inv((T)1.0 / log((T)2))
    {
    }

    T Log2Inv;
};

double Log2(double x) {
    return log(x) * Singleton<TLog2Holder<double> >()->Log2Inv;
}

float Log2f(float x) {
    return logf(x) * Singleton<TLog2Holder<float> >()->Log2Inv;
}

double Exp2(double x) {
    return pow(2.0, x);
}

float Exp2f(float x) {
    return powf(2.0f, x);
}

#ifdef _MSC_VER

double Erf(double x) {
    static const double _M_2_SQRTPI = 1.12837916709551257390;
    static const double eps = 1.0e-7;
    if (fabs(x) >= 3.75)
        return x > 0 ? 1.0 : -1.0;
    double r = _M_2_SQRTPI * x;
    double f = r;
    for (int i = 1;; ++i) {
        r *= -x * x / i;
        f += r / (2 * i + 1);
        if (fabs(r) < eps * (2 * i + 1))
            break;
    }
    return f;
}

#endif // _win_

namespace {
struct TGammaFunctionCalculator {
    const double LnSqrt2PI;
    const double Coeff9;
    const double Coeff7;
    const double Coeff5;
    const double Coeff3;
    const double Coeff1;
    TGammaFunctionCalculator()
        : LnSqrt2PI(log(sqrt(2.0 * PI)))
        , Coeff9(1.0 / 1188.0)
        , Coeff7(-1.0 / 1680.0)
        , Coeff5(1.0 / 1260.0)
        , Coeff3(-1.0 / 360.0)
        , Coeff1(1.0 / 12.0)
    {}
    double LogGamma(double x) {
        if ((x == 1.0) || (x == 2.0))
            return 0.0; // 0! = 1
        double bonus = 0.0;
        while (x < 3.0) {
            bonus -= log(x);
            x += 1.0;
        }
        double ln_x = log(x);
        double sqr_xinv = 1.0 / (x * x);
        double res = Coeff9 * sqr_xinv + Coeff7;
        res = res * sqr_xinv + Coeff5;
        res = res * sqr_xinv + Coeff3;
        res = res * sqr_xinv + Coeff1;
        res /= x;
        res += x * ln_x - x + LnSqrt2PI - 0.5 * ln_x;
        return res + bonus;
    }
};
}

double LogGammaImpl(double x) {
    return Singleton<TGammaFunctionCalculator>()->LogGamma(x);
}
