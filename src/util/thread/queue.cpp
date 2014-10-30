#include <util/system/defaults.h>

#if defined(_unix_)
    #include <pthread.h>
#endif

#include <util/generic/vector.h>
#include <util/generic/intrlist.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>
#include <util/generic/singleton.h>
#include <util/generic/fastqueue.h>

#include <util/stream/ios.h>

#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/atomic.h>
#include <util/system/condvar.h>

#include <util/datetime/base.h>

#include "pool.h"
#include "queue.h"

TThreadPoolHolder::TThreadPoolHolder() throw ()
    : Pool_(SystemThreadPool())
{
}

class TMtpQueue::TImpl: public TIntrusiveListItem<TImpl>
                      , public IThreadPool::IThreadAble {
        typedef IMtpQueue::TTsr TTsr;
        typedef TFastQueue<IObjectInQueue*> TJobQueue;
        typedef TAutoPtr<IThreadPool::IThread> TThreadRef;
    public:
        inline TImpl(TMtpQueue* parent, size_t thrnum, size_t maxqueue, bool blocking)
            : Parent_(parent)
            , Blocking(blocking)
            , ShouldTerminate(true)
            , MaxQueueSize(0)
            , ThreadCountExpected(0)
            , ThreadCountReal(0)
            , Forked(false)
        {
            TAtforkQueueRestarter::Get().RegisterObject(this);
            Start(thrnum, maxqueue);
        }

        inline ~TImpl() throw () {
            try {
                Stop();
            } catch (...) {
            }

            TAtforkQueueRestarter::Get().UnregisterObject(this);
            YASSERT(Tharr.empty());
        }

        inline bool Add(IObjectInQueue* obj) {
            if (ShouldTerminate) {
                return false;
            }

            if (Tharr.empty()) {
                TTsr tsr(Parent_);
                obj->Process(tsr);

                return true;
            }

            {
                TGuard<TMutex> g(QueueMutex);

                while (MaxQueueSize > 0 && Queue.Size() >= MaxQueueSize && !ShouldTerminate) {
                    if (!Blocking) {
                        return false;
                    }
                    QueuePopCond.Wait(QueueMutex);
                }

                if (ShouldTerminate) {
                    return false;
                }

                Queue.Push(obj);
            }

            QueuePushCond.Signal();

            return true;
        }

        inline size_t Size() const throw () {
            TGuard<TMutex> g(QueueMutex);

            return Queue.Size();
        }

        inline size_t GetMaxQueueSize() const throw () {
            return MaxQueueSize;
        }

        inline size_t GetThreadCountExpected() const throw () {
            return ThreadCountExpected;
        }

        inline void AtforkAction() throw () {
            Forked = true;
        }

        inline bool NeedRestart() const throw () {
            return Forked;
        }

    private:
        inline void Start(size_t num, size_t maxque) {
            ShouldTerminate = false;
            MaxQueueSize = maxque;
            ThreadCountExpected = num;

            for (size_t i = 0; i < num; ++i) {
                Tharr.push_back(Parent_->Pool()->Run(this));
            }

            ThreadCountReal = Tharr.size();
        }

        inline void Stop() {
            ShouldTerminate = true;

            {
                TGuard<TMutex> g(QueueMutex);
                QueuePopCond.BroadCast();
            }

            if (!NeedRestart()) {
                WaitForComplete();
            }

            Tharr.clear();
            ThreadCountExpected = 0;
            MaxQueueSize = 0;
        }

        inline void WaitForComplete() throw () {
            TGuard<TMutex> g(StopMutex);

            while (ThreadCountReal) {
                {
                    TGuard<TMutex> g2(QueueMutex);
                    QueuePushCond.Signal();
                }
                StopCond.Wait(StopMutex);
            }
        }

        virtual void DoExecute() {
            THolder<TTsr> tsr(new TTsr(Parent_));

            while (true) {
                IObjectInQueue* job = 0;

                {
                    TGuard<TMutex> g(QueueMutex);

                    while (Queue.Empty() && !ShouldTerminate) {
                        QueuePushCond.Wait(QueueMutex);
                    }

                    if (ShouldTerminate && Queue.Empty()) {
                        tsr.Destroy();

                        break;
                    }

                    job = Queue.Pop();
                }

                QueuePopCond.Signal();

                try {
                    try {
                        job->Process(*tsr);
                    } catch (...) {
                        Cdbg << "[mtp queue] " << CurrentExceptionMessage() << Endl;
                    }
                } catch (...) {
                }
            }

            FinishOneThread();
        }

        inline void FinishOneThread() throw () {
            TGuard<TMutex> g(StopMutex);

            --ThreadCountReal;
            StopCond.Signal();
        }

    private:
        TMtpQueue* Parent_;
        const bool Blocking;
        mutable TMutex QueueMutex;
        mutable TMutex StopMutex;
        TCondVar QueuePushCond;
        TCondVar QueuePopCond;
        TCondVar StopCond;
        TJobQueue Queue;
        yvector<TThreadRef> Tharr;
        bool ShouldTerminate;
        size_t MaxQueueSize;
        size_t ThreadCountExpected;
        size_t ThreadCountReal;
        bool Forked;

        class TAtforkQueueRestarter {
            public:
                static TAtforkQueueRestarter& Get() {
                    return *SingletonWithPriority<TAtforkQueueRestarter, 256>();
                }

                inline void RegisterObject(TImpl* obj) {
                    TGuard<TMutex> g(ActionMutex);

                    RegisteredObjects.PushBack(obj);
                }

                inline void UnregisterObject(TImpl* obj) {
                    TGuard<TMutex> g(ActionMutex);

                    obj->Unlink();
                }

            private:
                void ChildAction() {
                    TGuard<TMutex> g(ActionMutex);

                    for (TIntrusiveList<TImpl>::TIterator it = RegisteredObjects.Begin(); it != RegisteredObjects.End(); ++it) {
                        it->AtforkAction();
                    }
                }

                static void ProcessChildAction() {
                    Get().ChildAction();
                }

                TIntrusiveList<TImpl> RegisteredObjects;
                TMutex ActionMutex;

            public:
                inline TAtforkQueueRestarter() {
#if defined(_unix_)
    #if !(defined(_freebsd_) && (__FreeBSD__ < 5 ))
                    pthread_atfork(NULL, NULL, ProcessChildAction);
    #endif
#endif
                }
        };
};

TMtpQueue::TMtpQueue(bool blocking)
    : Blocking(blocking)
{
}

TMtpQueue::TMtpQueue(IThreadPool* pool, bool blocking)
    : TThreadPoolHolder(pool)
    , Blocking(blocking)
{
}

TMtpQueue::~TMtpQueue() {
}

size_t TMtpQueue::Size() const throw () {
    if (!Impl_.Get()) {
        return 0;
    }

    return Impl_->Size();
}

bool TMtpQueue::Add(IObjectInQueue* obj) {
    if (!Impl_.Get()) {
        ythrow yexception() << "mtp queue not started";
    }

    if (Impl_->NeedRestart()) {
        Start(Impl_->GetThreadCountExpected(), Impl_->GetMaxQueueSize());
    }

    return Impl_->Add(obj);
}

void TMtpQueue::Start(size_t thrnum, size_t maxque) {
    Impl_.Reset(new TImpl(this, thrnum, maxque, Blocking));
}

void TMtpQueue::Stop() throw () {
    Impl_.Destroy();
}

static TAtomic mtp_queue_counter = 0;

class TAdaptiveMtpQueue::TImpl {
    public:
        class TThread: public IThreadPool::IThreadAble {
            public:
                inline TThread(TImpl* parent)
                    : Impl_(parent)
                    , Thread_(Impl_->Parent_->Pool()->Run(this))
                {
                }

                inline ~TThread() throw () {
                    Impl_->DecThreadCount();
                }

            private:
                virtual void DoExecute() throw () {
                    THolder<TThread> This(this);

                    {
                        TTsr tsr(Impl_->Parent_);
                        IObjectInQueue* obj;

                        while ((obj = Impl_->WaitForJob()) != 0) {
                            try {
                                try {
                                    obj->Process(tsr);
                                } catch (...) {
                                    Cdbg << Impl_->Name() << " " << CurrentExceptionMessage() << Endl;
                                }
                            } catch (...) {
                            }
                        }
                    }
                }

            private:
                TImpl* Impl_;
                TAutoPtr<IThreadPool::IThread> Thread_;
        };

        inline TImpl(TAdaptiveMtpQueue* parent)
            : Parent_(parent)
            , ThrCount_(0)
            , AllDone_(false)
            , Obj_(0)
            , Free_(0)
            , IdleTime_(TDuration::Max())
        {
            sprintf(Name_, "[mtp queue %ld]", (long)AtomicAdd(mtp_queue_counter, 1));
        }

        inline ~TImpl() throw () {
            Stop();
        }

        inline void SetMaxIdleTime(TDuration idleTime) {
            IdleTime_ = idleTime;
        }

        inline const char* Name() const throw () {
            return Name_;
        }

        inline void Add(IObjectInQueue* obj) {
            {
                TGuard<TMutex> lock(Mutex_);

                while (Obj_ != 0) {
                    CondFree_.Wait(Mutex_);
                }

                if (Free_ == 0) {
                    AddThreadNoLock();
                }

                Obj_ = obj;

                if (AllDone_) {
                    ythrow yexception() << "shit happen";
                }
            }

            CondReady_.Signal();
        }

        inline void AddThreads(size_t n) {
            TGuard<TMutex> lock(Mutex_);

            while (n) {
                AddThreadNoLock();

                --n;
            }
        }

        inline size_t Size() const throw () {
            return (size_t)ThrCount_;
        }

    private:
        inline void IncThreadCount() throw () {
            AtomicAdd(ThrCount_, 1);
        }

        inline void DecThreadCount() throw () {
            AtomicAdd(ThrCount_, -1);
        }

        inline void AddThreadNoLock() {
            IncThreadCount();

            try {
                new TThread(this);
            } catch (...) {
                DecThreadCount();

                throw;
            }
        }

        inline void Stop() throw () {
            Mutex_.Acquire();

            AllDone_ = true;

            while (ThrCount_) {
                Mutex_.Release();
                CondReady_.Signal();
                Mutex_.Acquire();
            }

            Mutex_.Release();
        }

        inline IObjectInQueue* WaitForJob() throw () {
            Mutex_.Acquire();

            ++Free_;

            while (!Obj_ && !AllDone_) {
                if (!CondReady_.TimedWait(Mutex_, IdleTime_)) {
                    break;
                }
            }

            IObjectInQueue* ret = Obj_; Obj_ = 0;

            --Free_;

            Mutex_.Release();
            CondFree_.Signal();

            return ret;
        }

    private:
        TAdaptiveMtpQueue* Parent_;
        TAtomic ThrCount_;
        TMutex Mutex_;
        TCondVar CondReady_;
        TCondVar CondFree_;
        bool AllDone_;
        IObjectInQueue* Obj_;
        size_t Free_;
        char Name_[64];
        TDuration IdleTime_;
};

TAdaptiveMtpQueue::TAdaptiveMtpQueue() {
}

TAdaptiveMtpQueue::TAdaptiveMtpQueue(IThreadPool* pool)
    : TThreadPoolHolder(pool)
{
}

TAdaptiveMtpQueue::~TAdaptiveMtpQueue() {
}

bool TAdaptiveMtpQueue::Add(IObjectInQueue* obj) {
    if (!Impl_.Get()) {
        ythrow yexception() << "mtp queue not started";
    }

    Impl_->Add(obj);

    return true;
}

void TAdaptiveMtpQueue::Start(size_t, size_t) {
    Impl_.Reset(new TImpl(this));
}

void TAdaptiveMtpQueue::Stop() throw () {
    Impl_.Destroy();
}

size_t TAdaptiveMtpQueue::Size() const throw () {
    if (Impl_.Get()) {
        return Impl_->Size();
    }

    return 0;
}

void TAdaptiveMtpQueue::SetMaxIdleTime(TDuration interval) {
    if (!Impl_.Get()) {
        ythrow yexception() << "mtp queue not started";
    }

    Impl_->SetMaxIdleTime(interval);
}

TSimpleMtpQueue::TSimpleMtpQueue() {
}

TSimpleMtpQueue::TSimpleMtpQueue(IThreadPool* pool)
    : TThreadPoolHolder(pool)
{
}

TSimpleMtpQueue::~TSimpleMtpQueue() {
    try {
        Stop();
    } catch (...) {
    }
}

bool TSimpleMtpQueue::Add(IObjectInQueue* obj) {
    if (!Slave_.Get()) {
        ythrow yexception() << "mtp queue not started";
    }

    return Slave_->Add(obj);
}

void TSimpleMtpQueue::Start(size_t thrnum, size_t maxque) {
    THolder<IMtpQueue> tmp;
    TAdaptiveMtpQueue* adaptive(0);

    if (thrnum) {
        tmp.Reset(new TMtpQueueBinder<TMtpQueue, TSimpleMtpQueue>(this, Pool()));
    } else {
        adaptive = new TMtpQueueBinder<TAdaptiveMtpQueue, TSimpleMtpQueue>(this, Pool());
        tmp.Reset(adaptive);
    }

    tmp->Start(thrnum, maxque);

    if (adaptive) {
        adaptive->SetMaxIdleTime(TDuration::Seconds(100));
    }

    Slave_.Swap(tmp);
}

void TSimpleMtpQueue::Stop() throw () {
    Slave_.Destroy();
}

size_t TSimpleMtpQueue::Size() const throw () {
    if (Slave_.Get()) {
        return Slave_->Size();
    }

    return 0;
}

namespace {
    class TThrFuncObj: public IObjectInQueue {
        public:
            TThrFuncObj(const TThreadFunction& func)
                : Func(func)
            {
            }
            void Process(void*) {
                THolder<TThrFuncObj> self(this);
                Func();
            }
        private:
            TThreadFunction Func;
    };
}

void IMtpQueue::SafeAdd(IObjectInQueue* obj) {
    if (!Add(obj)) {
        ythrow yexception() << "can not add object to queue";
    }
}

void IMtpQueue::SafeAddFunc(TThreadFunction func) {
    if (!AddFunc(func)) {
        ythrow yexception() << "can not add function to queue";
    }
}

bool IMtpQueue::AddFunc(TThreadFunction func) {
    return Add(new ::TThrFuncObj(func));
}

typedef IThreadPool::IThread IThread;
typedef IThreadPool::IThreadAble IThreadAble;

namespace {
    class TPoolThread: public IThread {
            class TThreadImpl: public IObjectInQueue, public TRefCounted<TThreadImpl, TAtomicCounter> {
                public:
                    inline TThreadImpl(IThreadAble* func)
                        : Func_(func)
                    {
                    }

                    virtual ~TThreadImpl() throw () {
                    }

                    inline void WaitForStart() throw () {
                        StartEvent_.Wait();
                    }

                    inline void WaitForComplete() throw () {
                        CompleteEvent_.Wait();
                    }

                private:
                    virtual void Process(void* /*tsr*/) {
                        TThreadImplRef This(this);

                        {
                            StartEvent_.Signal();

                            try {
                                Func_->Execute();
                            } catch (...) {
                            }

                            CompleteEvent_.Signal();
                        }
                    }

                private:
                    IThreadAble* Func_;
                    Event CompleteEvent_;
                    Event StartEvent_;
            };

            typedef TIntrusivePtr<TThreadImpl> TThreadImplRef;

        public:
            inline TPoolThread(IMtpQueue* parent)
                : Parent_(parent)
            {
            }

            virtual ~TPoolThread() throw () {
                if (Impl_) {
                    Impl_->WaitForStart();
                }
            }

        private:
            virtual void DoRun(IThreadAble* func) {
                TThreadImplRef impl(new TThreadImpl(func));

                Parent_->SafeAdd(impl.Get());
                Impl_.Swap(impl);
            }

            virtual void DoJoin() throw () {
                if (Impl_) {
                    Impl_->WaitForComplete();
                    Impl_ = 0;
                }
            }

        private:
            IMtpQueue* Parent_;
            TThreadImplRef Impl_;
    };
}

IThread* IMtpQueue::DoCreate() {
    return new TPoolThread(this);
}

TAutoPtr<IMtpQueue> CreateMtpQueue(size_t threadsCount, size_t queueSizeLimit) {
    THolder<IMtpQueue> queue;
    if (threadsCount > 1) {
        queue.Reset(new TMtpQueue());
    } else {
        queue.Reset(new TFakeMtpQueue());
    }
    queue->Start(threadsCount, queueSizeLimit);
    return queue;
}
