#include "datetime.h"
#include "defaults.h"

#include <cstdio>

#include "event.h"
#include "mutex.h"
#include "condvar.h"

#ifdef _win_
    #include "winint.h"
#endif

class Event::TEvImpl {
    public:
#ifdef _win_
        inline TEvImpl(ResetMode rmode) {
            cond = CreateEvent(NULL, rmode == rManual ? true : false, false, NULL);
        }

        inline ~TEvImpl() throw () {
            CloseHandle(cond);
        }

        inline void Reset() throw () {
            ResetEvent(cond);
        }

        inline void Signal() throw () {
            SetEvent(cond);
        }

        inline bool WaitD(TInstant deadLine) throw () {
            if (deadLine == TInstant::Max()) {
                return WaitForSingleObject(cond, INFINITE) == WAIT_OBJECT_0;
            }

            const TInstant now = Now();

            if (now < deadLine) {
                //TODO
                return WaitForSingleObject(cond, (deadLine - now).MilliSeconds()) == WAIT_OBJECT_0;
            }

            return false;
        }
#else
        inline TEvImpl(ResetMode rmode)
            : Signaled(false)
            , Manual(rmode == rManual ? true : false)
        {
        }

        inline void Signal() throw () {
            if (Manual && Signaled) {
                return; // shortcut
            }
            {
                TGuard<TMutex> guard(Mutex);
                Signaled = true;
            }
            if (Manual)
                Cond.BroadCast();
            else
                Cond.Signal();
        }

        inline void Reset() throw () {
            Signaled = false;
        }

        inline bool WaitD(TInstant deadLine) throw () {
            if (Manual && Signaled)
                return true; // shortcut

            Mutex.Acquire();

            bool resSignaled = true;

            while (!Signaled) {
                if (!Cond.WaitD(Mutex, deadLine)) {
                    resSignaled = Signaled; // timed out, but Signaled could have been set
                    break;
                }
            }

            if (!Manual) {
                Signaled = false;
            }

            Mutex.Release();

            return resSignaled;
        }
#endif

    private:
#ifdef _win_
        HANDLE cond;
#else
        TCondVar Cond;
        TMutex Mutex;
        volatile bool Signaled;
        bool Manual;
#endif
};

Event::Event(ResetMode rmode)
    : EvImpl_(new TEvImpl(rmode))
{
}

Event::~Event() throw () {
}

void Event::Reset() throw () {
    EvImpl_->Reset();
}

void Event::Signal() throw () {
    EvImpl_->Signal();
}

bool Event::WaitD(TInstant deadLine) throw () {
    return EvImpl_->WaitD(deadLine);
}
