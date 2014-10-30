#pragma once

#include "mutex.h"

#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>
#include <util/datetime/base.h>

class TCondVar {
    public:
        TCondVar();
        ~TCondVar() throw ();

        void BroadCast() throw ();
        void Signal() throw ();

        /*
         * returns false if failed by timeout
         */
        bool WaitD(TMutex& m, TInstant deadLine) throw ();

        /*
         * returns false if failed by timeout
         */
        inline bool WaitT(TMutex& m, TDuration timeOut) throw () {
            return WaitD(m, timeOut.ToDeadLine());
        }

        /*
         * infinite wait
         */
        inline void WaitI(TMutex& m) throw () {
            WaitD(m, TInstant::Max());
        }

        //deprecated
        inline void Wait(TMutex& m) throw () {
            WaitI(m);
        }

        inline bool TimedWait(TMutex& m, TDuration timeOut) throw () {
            return WaitT(m, timeOut);
        }

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};
