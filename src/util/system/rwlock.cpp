#include "rwlock.h"

#include <util/generic/yexception.h>

#if defined(_win_)
    #include "mutex.h"
    #include "condvar.h"
#else
    #include <errno.h>
    #include <pthread.h>
#endif

#if defined(_win_)

class TRWMutex::TImpl {
    public:
        TImpl();
        ~TImpl() throw ();

        void AcquireRead() throw ();
        bool TryAcquireRead() throw ();
        void ReleaseRead() throw ();

        void AcquireWrite() throw ();
        bool TryAcquireWrite() throw ();
        void ReleaseWrite() throw ();

        void Release() throw ();

    private:
        TMutex Lock_;
        int State_;
        TCondVar ReadCond_;
        TCondVar WriteCond_;
        int BlockedWriters_;
};

TRWMutex::TImpl::TImpl()
    : State_(0)
    , BlockedWriters_(0)
{
}

TRWMutex::TImpl::~TImpl() throw () {
}

void TRWMutex::TImpl::AcquireRead() throw () {
    {
        TGuard<TMutex> g(Lock_);

        while (BlockedWriters_ || State_ < 0) {
            ReadCond_.Wait(Lock_);
        }

        State_++;
    }

    ReadCond_.Signal();
}

bool TRWMutex::TImpl::TryAcquireRead() throw () {
    TGuard<TMutex> g(Lock_);

    if (BlockedWriters_ || State_ < 0) {
        return false;
    }

    State_++;

    return true;
}

void TRWMutex::TImpl::ReleaseRead() throw () {
    Lock_.Acquire();

    if (--State_ > 0) {
        Lock_.Release();
    } else if (BlockedWriters_) {
        Lock_.Release();
        WriteCond_.Signal();
    } else {
        Lock_.Release();
    }
}

void TRWMutex::TImpl::AcquireWrite() throw () {
    TGuard<TMutex> g(Lock_);

    while (State_ != 0) {
        ++BlockedWriters_;
        WriteCond_.Wait(Lock_);
        --BlockedWriters_;
    }

    State_ = -1;
}

bool TRWMutex::TImpl::TryAcquireWrite() throw () {
    TGuard<TMutex> g(Lock_);

    if (State_ != 0) {
        return false;
    }

    State_ = -1;

    return true;
}

void TRWMutex::TImpl::ReleaseWrite() throw () {
    Lock_.Acquire();
    State_ = 0;

    if (BlockedWriters_) {
        Lock_.Release();
        WriteCond_.Signal();
    } else {
        Lock_.Release();
        ReadCond_.Signal();
    }
}

void TRWMutex::TImpl::Release() throw () {
    Lock_.Acquire();

    if (State_ > 0) {
        if (--State_ > 0) {
            Lock_.Release();
        } else if (BlockedWriters_) {
            Lock_.Release();
            WriteCond_.Signal();
        }
    } else {
        State_ = 0;

        if (BlockedWriters_) {
            Lock_.Release();
            WriteCond_.Signal();
        } else {
            Lock_.Release();
            ReadCond_.Signal();
        }
    }
}

#else /* POSIX threads */

class TRWMutex::TImpl {
    public:
        TImpl();
        ~TImpl() throw ();

        void AcquireRead() throw ();
        bool TryAcquireRead() throw ();
        void ReleaseRead() throw ();

        void AcquireWrite() throw ();
        bool TryAcquireWrite() throw ();
        void ReleaseWrite() throw ();

        void Release() throw ();

    private:
        pthread_rwlock_t Lock_;
};

TRWMutex::TImpl::TImpl() {
    int result = pthread_rwlock_init(&Lock_, 0);
    if (result != 0) {
        ythrow yexception() << "rwlock init failed (" << LastSystemErrorText(result) <<  ")";
    }
}

TRWMutex::TImpl::~TImpl() throw () {
    const int result = pthread_rwlock_destroy(&Lock_);
    VERIFY(result == 0, "rwlock destroy failed (%s)", LastSystemErrorText(result));
}

void TRWMutex::TImpl::AcquireRead() throw () {
    const int result = pthread_rwlock_rdlock(&Lock_);
    VERIFY(result == 0, "rwlock rdlock failed (%s)", LastSystemErrorText(result));
}

bool TRWMutex::TImpl::TryAcquireRead() throw () {
    const int result = pthread_rwlock_tryrdlock(&Lock_);
    VERIFY(result == 0 || result == EBUSY, "rwlock tryrdlock failed (%s)", LastSystemErrorText(result));
    return result == 0;
}

void TRWMutex::TImpl::ReleaseRead() throw () {
    const int result = pthread_rwlock_unlock(&Lock_);
    VERIFY(result == 0, "rwlock (read) unlock failed (%s)", LastSystemErrorText(result));
}

void TRWMutex::TImpl::AcquireWrite() throw () {
    const int result = pthread_rwlock_wrlock(&Lock_);
    VERIFY(result == 0, "rwlock wrlock failed (%s)", LastSystemErrorText(result));
}

bool TRWMutex::TImpl::TryAcquireWrite() throw () {
    const int result = pthread_rwlock_trywrlock(&Lock_);
    VERIFY(result == 0 || result == EBUSY, "rwlock trywrlock failed (%s)", LastSystemErrorText(result));
    return result == 0;
}

void TRWMutex::TImpl::ReleaseWrite() throw () {
    const int result = pthread_rwlock_unlock(&Lock_);
    VERIFY(result == 0, "rwlock (write) unlock failed (%s)", LastSystemErrorText(result));
}

void TRWMutex::TImpl::Release() throw () {
    const int result = pthread_rwlock_unlock(&Lock_);
    VERIFY(result == 0, "rwlock unlock failed (%s)", LastSystemErrorText(result));
}

#endif

TRWMutex::TRWMutex()
    : Impl_(new TImpl())
{
}

TRWMutex::~TRWMutex() throw () {
}

void TRWMutex::AcquireRead() throw () {
    Impl_->AcquireRead();
}
bool TRWMutex::TryAcquireRead() throw () {
    return Impl_->TryAcquireRead();
}
void TRWMutex::ReleaseRead() throw () {
    Impl_->ReleaseRead();
}

void TRWMutex::AcquireWrite() throw () {
    Impl_->AcquireWrite();
}
bool TRWMutex::TryAcquireWrite() throw () {
    return Impl_->TryAcquireWrite();
}
void TRWMutex::ReleaseWrite() throw () {
    Impl_->ReleaseWrite();
}

void TRWMutex::Release() throw () {
    Impl_->Release();
}
