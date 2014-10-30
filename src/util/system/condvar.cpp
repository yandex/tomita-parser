#include "event.h"
#include "mutex.h"
#include "yassert.h"
#include "condvar.h"
#include "datetime.h"
#include "spinlock.h"

#include <util/generic/ylimits.h>
#include <util/generic/intrlist.h>
#include <util/generic/yexception.h>

#include <cstdlib>

#if defined(_unix_)
    #include <sys/time.h>
    #include <pthread.h>
    #include <cerrno>
#endif

namespace {
class TCondVarImpl {
        typedef TAdaptiveLock TLock;

        struct TWaitEvent: public TIntrusiveListItem<TWaitEvent>, public Event {
        };

        typedef TIntrusiveList<TWaitEvent> TWaitEvents;
    public:
        inline ~TCondVarImpl() throw () {
            YASSERT(Events_.Empty());
        }

        inline void Signal() throw () {
            TGuard<TLock> guard(Lock_);

            if (!Events_.Empty()) {
                Events_.PopFront()->Signal();
            }
        }

        inline void BroadCast() throw () {
            TGuard<TLock> guard(Lock_);

            //TODO
            while (!Events_.Empty()) {
                Events_.PopFront()->Signal();
            }
        }

        inline bool WaitD(TMutex& m, TInstant deadLine) throw () {
            TWaitEvent event;

            {
                TGuard<TLock> guard(Lock_);

                Events_.PushBack(&event);
            }

            m.Release();

            const bool signalled = event.WaitD(deadLine);

            m.Acquire();

            {
                TGuard<TLock> guard(Lock_);

                event.Unlink();
            }

            return signalled;
        }

    private:
        TWaitEvents Events_;
        TLock Lock_;
};
}

#if defined(_win_)
class TCondVar::TImpl: public TCondVarImpl {
};
#else
class TCondVar::TImpl {
    public:
        inline TImpl() {
            if (pthread_cond_init(&Cond_, NULL)) {
                ythrow yexception() << "can not create condvar(" << LastSystemErrorText() << ")";
            }
        }

        inline ~TImpl() throw () {
            int ret = pthread_cond_destroy(&Cond_);
            VERIFY(ret == 0, "pthread_cond_destroy failed: %s", LastSystemErrorText(ret));
        }

        inline void Signal() throw () {
            int ret = pthread_cond_signal(&Cond_);
            VERIFY(ret == 0, "pthread_cond_signal failed: %s", LastSystemErrorText(ret));
        }

        inline bool WaitD(TMutex& lock, TInstant deadLine) throw () {
            if (deadLine == TInstant::Max()) {
                int ret = pthread_cond_wait(&Cond_, (pthread_mutex_t*)lock.Handle());
                VERIFY(ret == 0, "pthread_cond_wait failed: %s", LastSystemErrorText(ret));
                return true;
            } else {
                struct timespec spec;

                Zero(spec);

                spec.tv_sec = deadLine.Seconds();
                spec.tv_nsec = deadLine.NanoSecondsOfSecond();

                int ret = pthread_cond_timedwait(&Cond_, (pthread_mutex_t*)lock.Handle(), &spec);

                VERIFY(ret == 0 || ret == ETIMEDOUT, "pthread_cond_timedwait failed: %s", LastSystemErrorText(ret));

                return ret == 0;
            }
        }

        inline void BroadCast() throw () {
            int ret = pthread_cond_broadcast(&Cond_);
            VERIFY(ret == 0, "pthread_cond_broadcast failed: %s", LastSystemErrorText(ret));
        }

    private:
        pthread_cond_t Cond_;
};
#endif

TCondVar::TCondVar()
    : Impl_(new TImpl)
{
}

TCondVar::~TCondVar() throw () {
}

void TCondVar::BroadCast() throw () {
    Impl_->BroadCast();
}

void TCondVar::Signal() throw () {
    Impl_->Signal();
}

bool TCondVar::WaitD(TMutex& mutex, TInstant deadLine) throw () {
    return Impl_->WaitD(mutex, deadLine);
}
