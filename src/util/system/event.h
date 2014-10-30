#pragma once

#include <util/generic/ptr.h>
#include <util/datetime/base.h>

class Event {
    public:
        enum ResetMode {
            rAuto,   // the state will be nonsignaled after Wait() returns
            rManual, // we need call Reset() to set the state to nonsignaled.
        };

        Event(ResetMode rmode = rManual);
        ~Event() throw ();

        void Reset() throw ();
        void Signal() throw ();

        /*
         * return true if signaled, false if timed out.
         */
        bool WaitD(TInstant deadLine) throw ();

        /*
         * return true if signaled, false if timed out.
         */
        inline bool WaitT(TDuration timeOut) throw () {
            return WaitD(timeOut.ToDeadLine());
        }

        /*
         * wait infinite time
         */
        inline void WaitI() throw () {
            WaitD(TInstant::Max());
        }

        //return true if signaled, false if timed out.
        inline bool Wait(ui32 timer) throw () {
            return WaitT(TDuration::MilliSeconds(timer));
        }

        inline bool Wait() throw () {
            WaitI();

            return true;
        }

    private:
        class TEvImpl;
        THolder<TEvImpl> EvImpl_;
};
