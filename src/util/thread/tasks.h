#pragma once

#include <util/system/mutex.h>
#include <util/generic/ptr.h>
#include <util/generic/intrlist.h>
#include <util/generic/vector.h>

class IMtpQueue;

class ITaskToMultiThreadProcessing {
    public:
        inline ITaskToMultiThreadProcessing() throw () {
        }

        virtual ~ITaskToMultiThreadProcessing() throw () {
        }

        virtual void Process(void* threadContext) = 0;
};

class TMtpTask {
    public:
        TMtpTask(IMtpQueue* slave = 0);
        ~TMtpTask() throw ();

        void Destroy() throw ();
        void Process(ITaskToMultiThreadProcessing* task, void** taskContexts, size_t contextCount);

    private:
        class TImpl;
        IMtpQueue* Queue_;
        THolder<TImpl> Impl_;
};

class TMtpTasksPool {
    public:
        class TMtpTaskFromPool: public TMtpTask
                              , public TRefCounted<TMtpTaskFromPool, TSimpleCounter, TMtpTaskFromPool>
                              , public TIntrusiveListItem<TMtpTaskFromPool> {
            public:
                inline TMtpTaskFromPool(TMtpTasksPool* parent)
                    : TMtpTask(parent->Queue_)
                    , Parent_(parent)
                {
                }

                static inline void Destroy(TMtpTaskFromPool* This) throw () {
                    This->DeActivate();
                }

            private:
                inline void DeActivate() throw () {
                    Parent_->Release(this);
                }

            private:
                TMtpTasksPool* Parent_;
        };

        typedef TIntrusivePtr<TMtpTaskFromPool> TMtpTaskFromPoolRef;

        inline TMtpTasksPool(IMtpQueue* slave = 0)
            : Queue_(slave)
        {
        }

        inline ~TMtpTasksPool() throw () {
            while (!List_.Empty()) {
                delete List_.PopFront();
            }
        }

        inline TMtpTaskFromPoolRef Acquire() {
            return AcquireImpl();
        }

    private:
        inline void Release(TMtpTaskFromPool* task) throw () {
            TGuard<TMutex> lock(Lock_);

            List_.PushFront(task);
        }

        inline TMtpTaskFromPool* AcquireImpl() {
            {
                TGuard<TMutex> lock(Lock_);

                if (!List_.Empty()) {
                    return List_.PopFront();
                }
            }

            return new TMtpTaskFromPool(this);
        }

    private:
        IMtpQueue* Queue_;
        TIntrusiveList<TMtpTaskFromPool> List_;
        TMutex Lock_;
};

typedef TMtpTasksPool::TMtpTaskFromPoolRef TMtpTaskFromPool;

template <class T>
class TTaskToMultiThreadProcessing: public ITaskToMultiThreadProcessing {
    public:
        inline TTaskToMultiThreadProcessing() throw () {
        }

        virtual ~TTaskToMultiThreadProcessing() throw () {
        }

        virtual void Process(void* threadContext) {
            ((T*)threadContext)->ProcessTask();
        }
};


template <typename T>
class TSimpleMtpTask {
public:
    inline TSimpleMtpTask(IMtpQueue* queue)
        : Slave_(queue)
    {
    }

    inline ~TSimpleMtpTask() {
        Reset();
    }

    inline void Reset() {
        for (size_t i = 0; i < Tasks_.size(); ++i)
            delete Tasks_[i];
        Tasks_.clear();
    }

    template <typename TT>
    inline void Add(TT task) {
        AddAndOwn(new T(task));
    }

    inline void AddAndOwn(TAutoPtr<T> task) {
        Tasks_.push_back(task.Get());
        task.Release();
    }

    inline void Process() {
        TTaskToMultiThreadProcessing<T> executor;
        Slave_.Process(&executor, (void**)~Tasks_, +Tasks_);
        Reset();
    }

private:
    TMtpTask Slave_;
    yvector<T*> Tasks_;
};
