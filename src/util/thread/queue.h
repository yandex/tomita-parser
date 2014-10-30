#pragma once

#include "pool.h"

#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>
#include <util/lambda/function.h>

class TDuration;

struct IObjectInQueue {
    virtual ~IObjectInQueue() {
    }

    virtual void Process(void* ThreadSpecificResource) = 0;
};

class TThreadPoolHolder {
    public:
        TThreadPoolHolder() throw ();

        inline TThreadPoolHolder(IThreadPool* pool) throw ()
            : Pool_(pool)
        {
        }

        inline ~TThreadPoolHolder() throw () {
        }

        inline IThreadPool* Pool() const throw () {
            return Pool_;
        }

    private:
        IThreadPool* Pool_;
};

typedef TFunction<void()> TThreadFunction;

/*
 * A queue processed simultaneously by several threads
 */
class IMtpQueue: public IThreadPool, public TNonCopyable {
    public:
        class TTsr {
            public:
                inline TTsr(IMtpQueue* q)
                    : Q_(q)
                    , Data_(Q_->CreateThreadSpecificResource())
                {
                }

                inline ~TTsr() throw () {
                    try {
                        Q_->DestroyThreadSpecificResource(Data_);
                    } catch (...) {
                    }
                }

                inline operator void* () throw () {
                    return Data_;
                }

            private:
                IMtpQueue* Q_;
                void* Data_;
        };

        virtual ~IMtpQueue() {
        }

        virtual void* CreateThreadSpecificResource() {
            return 0;
        }

        virtual void DestroyThreadSpecificResource(void* resource) {
            if (resource != 0) {
                YASSERT(resource == 0);
            }
        }

        void SafeAdd(IObjectInQueue* obj);
        void SafeAddFunc(TThreadFunction func);

        /// Add object to queue.
        /// Obj is not deleted after execution
        /// @return true of obj is successfully added to queue
        /// @return false if queue is full or shutting down
        virtual bool Add(IObjectInQueue* obj) _warn_unused_result = 0;
        bool AddFunc(TThreadFunction func) _warn_unused_result;
        virtual void Start(size_t threadCount, size_t queueSizeLimit = 0) = 0;
        /// Wait for completion of all scheduled object, and then exit
        virtual void Stop() throw () = 0;
        /// Number of tasks currently in queue
        virtual size_t Size() const throw () = 0;

    private:
        virtual IThread* DoCreate();
};

class TFakeMtpQueue: public IMtpQueue {
    public:
        virtual bool Add(IObjectInQueue* pObj) _warn_unused_result {
            TTsr tsr(this);
            pObj->Process(tsr);

            return true;
        }

        virtual void Start(size_t, size_t = 0) {
        }

        virtual void Stop() throw () {
        }

        virtual size_t Size() const throw () {
            return 0;
        }
};

/// queue processed by fixed size thread pool
class TMtpQueue: public IMtpQueue, public TThreadPoolHolder {
    public:
        TMtpQueue(bool blocking = false);
        TMtpQueue(IThreadPool* pool, bool blocking = false);
        virtual ~TMtpQueue();

        virtual bool Add(IObjectInQueue* obj) _warn_unused_result;
        /** @param queueSizeLimit means "unlimited" when = 0 */
        virtual void Start(size_t threadCount, size_t queueSizeLimit = 0);
        virtual void Stop() throw ();
        virtual size_t Size() const throw ();

    private:
        class TImpl;
        THolder<TImpl> Impl_;

        const bool Blocking;
};

class TAdaptiveMtpQueue: public IMtpQueue, public TThreadPoolHolder {
    public:
        TAdaptiveMtpQueue();
        TAdaptiveMtpQueue(IThreadPool* pool);
        virtual ~TAdaptiveMtpQueue();

        void SetMaxIdleTime(TDuration interval);

        virtual bool Add(IObjectInQueue* obj) _warn_unused_result;
        /// @param thrnum, @param maxque are ignored
        virtual void Start(size_t thrnum, size_t maxque = 0);
        virtual void Stop() throw ();
        virtual size_t Size() const throw ();

        class TImpl;
    private:
        THolder<TImpl> Impl_;
};

class TSimpleMtpQueue: public IMtpQueue, public TThreadPoolHolder {
    public:
        TSimpleMtpQueue();
        TSimpleMtpQueue(IThreadPool* pool);
        virtual ~TSimpleMtpQueue();

        virtual bool Add(IObjectInQueue* obj) _warn_unused_result;
        virtual void Start(size_t thrnum, size_t maxque = 0);
        virtual void Stop() throw ();
        virtual size_t Size() const throw ();

    private:
        THolder<IMtpQueue> Slave_;
};

template <class TQueue, class TSlave>
class TMtpQueueBinder: public TQueue {
    public:
        inline TMtpQueueBinder(TSlave* slave)
            : Slave_(slave)
        {
        }

        template <class T1>
        inline TMtpQueueBinder(TSlave* slave, const T1& t1)
            : TQueue(t1)
            , Slave_(slave)
        {
        }

        inline TMtpQueueBinder(TSlave& slave)
            : Slave_(&slave)
        {
        }

        virtual ~TMtpQueueBinder() {
            try {
                this->Stop();
            } catch (...) {
            }
        }

        virtual void* CreateThreadSpecificResource() {
            return Slave_->CreateThreadSpecificResource();
        }

        virtual void DestroyThreadSpecificResource(void* resource) {
            Slave_->DestroyThreadSpecificResource(resource);
        }

    private:
        TSlave* Slave_;
};

inline void Delete(TAutoPtr<IMtpQueue> q) {
    if (q.Get()) {
        q->Stop();
    }
}

// creates and starts TMtpQueue if threadsCount > 1, or TFakeMtpQueue otherwise
TAutoPtr<IMtpQueue> CreateMtpQueue(size_t threadsCount, size_t queueSizeLimit = 0);
