#pragma once

#include <stlport/limits>

#if defined(max) || defined(min)
    #error "stop defining 'min' and 'max' macros, evil people"
#endif

template <class T>
class TNumericLimits: public NStl::numeric_limits<T> {
};

template <class T>
static inline T Max() throw () {
    return TNumericLimits<T>::max();
}

template <class T>
static inline T Min() throw () {
    return TNumericLimits<T>::min();
}

namespace NPrivate {
    struct TMax {
        template <class T>
        inline operator T() const {
            return Max<T>();
        }
    };

    struct TMin {
        template <class T>
        inline operator T() const {
            return Min<T>();
        }
    };
}

static inline ::NPrivate::TMax Max() {
    return ::NPrivate::TMax();
}

static inline ::NPrivate::TMin Min() {
    return ::NPrivate::TMin();
}
