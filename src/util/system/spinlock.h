#pragma once

#include "atomic.h"


class TSpinLockBase {
    protected:
        inline TSpinLockBase() throw ()
            : Val_(0)
        {
        }

    public:
        inline bool IsLocked() throw () {
            return AtomicGet(Val_);
        }

        inline bool TryAcquire() throw () {
            return AtomicTryLock(&Val_);
        }

    protected:
        TAtomic Val_;
};


static inline void SpinLockPause() {
#if defined(__GNUC__) && (defined(_i386_) || defined(_x86_64_))
    __asm __volatile("pause");
#endif
}

static inline void AcquireSpinLock(TAtomic* l) {
    if (!AtomicTryLock(l)) {
        do {
            SpinLockPause();
        } while (!AtomicTryAndTryLock(l));
    }
}

static inline void ReleaseSpinLock(TAtomic* l) {
    AtomicUnlock(l);
}

class TSpinLock : public TSpinLockBase {
    public:
        inline void Release() throw () {
            ReleaseSpinLock(&Val_);
        }

        inline void Acquire() throw () {
            AcquireSpinLock(&Val_);
        }
};

static inline void AcquireAdaptiveLock(TAtomic* l) {
    extern void AcquireAdaptiveLockSlow(TAtomic* l);

    if (!AtomicTryLock(l)) {
        AcquireAdaptiveLockSlow(l);
    }
}

static inline void ReleaseAdaptiveLock(TAtomic* l) {
    AtomicUnlock(l);
}

class TAdaptiveLock : public TSpinLockBase {
    public:
        inline void Release() throw () {
            ReleaseAdaptiveLock(&Val_);
        }

        inline void Acquire() throw () {
            AcquireAdaptiveLock(&Val_);
        }
};

#include "guard.h"

template <>
struct TCommonLockOps<TAtomic> {
    static inline void Acquire(TAtomic* v) throw () {
        AcquireAdaptiveLock(v);
    }

    static inline bool TryAcquire(TAtomic* v) throw () {
        return AtomicTryLock(v);
    }

    static inline void Release(TAtomic* v) throw () {
        ReleaseAdaptiveLock(v);
    }
};
