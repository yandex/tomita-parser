#pragma once

#include <util/system/guard.h>
#include <util/system/atomic.h>


struct TNoLockOps {
    template <class T>
    static inline void Acquire(T*) {
    }

    template <class T>
    static inline void Release(T*) {
    }
};

template <typename T, bool threadSafe>
struct TGuardSelector {
    typedef TGuard<T, TCommonLockOps<T> > TResult;
};

template <typename T>
struct TGuardSelector<T, false> {
    typedef TGuard<T, TNoLockOps> TResult;
};

template <bool threadSafe>
struct TAtomicCasSelector {
    template <class T>
    static inline bool Do(T* volatile* target, T* exchange, T* compare) {
        return ::AtomicCas(target, exchange, compare);
    }
};

template <>
struct TAtomicCasSelector<false> {
    template <class T>
    static inline bool Do(T* volatile* target, T* exchange, T* compare) {
        if (*target == compare) {
            *target = exchange;
            return true;
        }
        return false;
    }
};
