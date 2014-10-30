#pragma once

#include "guard.h"
#include "defaults.h"

#include <util/generic/ptr.h>

class TRWMutex {
    public:
        TRWMutex();
        ~TRWMutex() throw ();

        void AcquireRead() throw ();
        bool TryAcquireRead() throw ();
        void ReleaseRead() throw ();

        void AcquireWrite() throw ();
        bool TryAcquireWrite() throw ();
        void ReleaseWrite() throw ();

        void Release() throw ();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

struct TRWMutexReadOps {
     static inline void Acquire(TRWMutex* t) throw () {
         t->AcquireRead();
     }

     static inline void Release(TRWMutex* t) throw () {
         t->ReleaseRead();
     }
};

struct TRWMutexWriteOps {
     static inline void Acquire(TRWMutex* t) throw () {
         t->AcquireWrite();
     }

     static inline void Release(TRWMutex* t) throw () {
         t->ReleaseWrite();
     }
};

typedef TGuard<TRWMutex, TRWMutexReadOps> TReadGuard;
typedef TGuard<TRWMutex, TRWMutexWriteOps> TWriteGuard;
