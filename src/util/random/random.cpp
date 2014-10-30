#include "random.h"
#include "entropy.h"
#include "mersenne.h"

#include <util/system/tls.h>
#include <util/generic/ylimits.h>
#include <util/thread/singleton.h>

namespace {
    template <class T>
    struct TRndGen: public TMersenne<T> {
        inline TRndGen()
            : TMersenne<T>(EntropyPool())
        {
        }
    };

    template <class T>
    static inline TRndGen<T>* GetRndGen() {
        return FastTlsSingletonWithPriority<TRndGen<T>, 0>();
    }

    template <unsigned N>
    struct TToRealTypeBySize {
        typedef ui32 TResult;
    };

    template <>
    struct TToRealTypeBySize<8> {
        typedef ui64 TResult;
    };

    template <class T>
    struct TToRealType {
        typedef typename TToRealTypeBySize<sizeof(T)>::TResult TResult;
    };
}

#define DEF_RND(TY)\
    template <>\
    TY RandomNumber<TY>() {\
        return GetRndGen<TToRealType<TY>::TResult>()->GenRand();\
    } \
    \
    template <>\
    TY RandomNumber<TY>(TY n) {\
        return GenUniform(n, *GetRndGen<TToRealType<TY>::TResult>());\
    }

DEF_RND(char)
DEF_RND(unsigned char)
DEF_RND(unsigned int)
DEF_RND(unsigned long)
DEF_RND(unsigned short)
DEF_RND(unsigned long long)

template <>
bool RandomNumber<bool>() {
    return RandomNumber<ui8>() % 2 == 0;
}

template <>
float RandomNumber<float>() {
    float ret;

    do {
        ret = (float)GetRndGen<ui64>()->GenRandReal2();
    } while (ret >= 1);

    return ret;
}

template <>
double RandomNumber<double>() {
    return GetRndGen<ui64>()->GenRandReal4();
}

template <>
long double RandomNumber<long double>() {
    return RandomNumber<double>();
}
