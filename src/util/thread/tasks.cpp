#include "tasks.h"
#include "queue.h"

#include <util/system/yassert.h>
#include <util/system/datetime.h>
#include <util/stream/ios.h>
#include <util/system/event.h>
#include <util/system/condvar.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

class TMtpTask::TImpl {
        class TProcessor: public IObjectInQueue, public TRefCounted<TProcessor> {
            public:
                inline TProcessor()
                    : Task_(0)
                    , Ctx_(0)
                {
                }

                virtual ~TProcessor() throw () {
                }

                inline void SetData(ITaskToMultiThreadProcessing* task, void* ctx) throw () {
                    YASSERT(Task_ == 0);
                    YASSERT(Ctx_ == 0);

                    Task_ = task;
                    Ctx_ = ctx;
                }

                virtual void Process(void* /*tsr*/) {
                    try {
                        Task_->Process(Ctx_);
                    } catch (...) {
                        Cdbg << "[mtp task] " << CurrentExceptionMessage() << Endl;
                    }

                    Done();
                }

                inline void Wait() throw () {
                    TGuard<TMutex> guard(Mutex_);

                    while (Task_) {
                        CondVar_.Wait(Mutex_);
                    }
                }

            private:
                inline void Done() throw () {
                    TGuard<TMutex> guard(Mutex_);

                    Task_ = 0;
                    Ctx_ = 0;

                    CondVar_.Signal();
                }

            private:
                ITaskToMultiThreadProcessing* Task_;
                void* Ctx_;
                TMutex Mutex_;
                TCondVar CondVar_;
        };

        typedef TIntrusivePtr<TProcessor> TProcessorRef;
        typedef yvector<TProcessorRef> TProcessors;

    public:
        inline TImpl(IMtpQueue* slave, size_t cnt)
            : Queue_(slave)
        {
            if (!Queue_) {
                MyQueue_.Reset(new TAdaptiveMtpQueue());
                MyQueue_->Start(cnt);
                Queue_ = MyQueue_.Get();
            }

            YASSERT(Queue_);
        }

        inline ~TImpl() throw () {
            MyQueue_.Destroy();
        }

        inline void Process(ITaskToMultiThreadProcessing* task, void** ctx, size_t cnt) {
            Reserve(cnt);

            for (size_t i = 0; i < cnt; ++i) {
                Procs_[i]->SetData(task, ctx[i]);

                if ((/*last task - in current thread*/i == cnt - 1) || !Queue_->Add(Procs_[i].Get())) {
                    Procs_[i]->Process(0);
                }
            }

            for (size_t i = 0; i < cnt; ++i) {
                Procs_[i]->Wait();
            }
        }

    private:
        inline void Reserve(size_t cnt) {
            while (Procs_.size() < cnt) {
                Procs_.push_back(new TProcessor());
            }
        }

    private:
        IMtpQueue* Queue_;
        THolder<IMtpQueue> MyQueue_;
        TProcessors Procs_;
};

TMtpTask::TMtpTask(IMtpQueue* slave)
    : Queue_(slave)
{
}

TMtpTask::~TMtpTask() throw () {
    Destroy();
}

void TMtpTask::Destroy() throw () {
    Impl_.Destroy();
}

void TMtpTask::Process(ITaskToMultiThreadProcessing* task, void** ctx, size_t cnt) {
    if (cnt == 0) {
        return;
    }

    if (cnt == 1) {
        /*
         * megaoptimization
         */
        task->Process(*ctx);

        return;
    }

    if (!Impl_) {
        /*
         * megaoptimization, too
         */

        Impl_.Reset(new TImpl(Queue_, cnt));
    }

    Impl_->Process(task, ctx, cnt);
}
