#pragma once

#include "mersenne64.h"
#include "mersenne32.h"

#include <util/system/defaults.h>
#include <util/stream/input.h>
#include <util/memory/tempbuf.h>

namespace NPrivate {
    template <class T>
    struct TMersenneTraits;

    template <>
    struct TMersenneTraits<ui64> {
        typedef TMersenne64 TRealization;

        enum {
            Seed = ULL(19650218)
        };
    };

    template <>
    struct TMersenneTraits<ui32> {
        typedef TMersenne32 TRealization;

        enum {
            Seed = 19650218UL
        };
    };

    TTempBuf ReadRandData(TInputStream* pool, size_t len);
}

template <class T>
class TMersenne {
    public:
        inline TMersenne(T seed = ::NPrivate::TMersenneTraits<T>::Seed) throw ()
            : R_(seed)
        {
        }

        inline TMersenne(TInputStream* pool)
            : R_((const T*)NPrivate::ReadRandData(pool, 128 * sizeof(T)).Data(), 128)
        {
        }

        inline TMersenne(const T keys[], size_t len) throw ()
            : R_(keys, len)
        {
        }

        inline T GenRand() throw () {
            return R_.GenRand();
        }

        inline T RandMax() throw () {
            return Max<T>();
        }

        /* generates a random number on [0, 1]-real-interval */
        inline double GenRandReal1() throw () {
            return R_.GenRandReal1();
        }

        /* generates a random number on [0, 1)-real-interval */
        inline double GenRandReal2() throw () {
            return R_.GenRandReal2();
        }

        /* generates a random number on (0, 1)-real-interval */
        inline double GenRandReal3() throw () {
            return R_.GenRandReal3();
        }

        /* generates a random number on [0, 1) with 53-bit resolution */
        inline double GenRandReal4() throw () {
            return R_.GenRandReal4();
        }

    private:
        typename ::NPrivate::TMersenneTraits<T>::TRealization R_;
};
