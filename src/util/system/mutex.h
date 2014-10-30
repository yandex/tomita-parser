#pragma once

#include "guard.h"
#include "defaults.h"

#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>

class TFakeMutex: public TNonCopyable {
    public:
        inline void Acquire() throw () {
        }

        inline bool TryAcquire() throw () {
            return true;
        }

        inline void Release() throw () {
        }

        ~TFakeMutex() throw () {
        }
};

class TSysMutex {
    public:
        TSysMutex();
        ~TSysMutex() throw ();

        void Acquire() throw ();
        bool TryAcquire() throw ();
        void Release() throw ();

        //return opaque pointer to real handler
        void* Handle() const throw ();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

class TMutex: public TSysMutex {
};
