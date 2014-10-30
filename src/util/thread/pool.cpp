#include "pool.h"

#include <util/system/thread.h>
#include <util/generic/singleton.h>

typedef IThreadPool::IThread IThread;

namespace {
    class TSystemThreadPool: public IThreadPool {
    public:
        class TPoolThread: public IThread {
            public:
                virtual ~TPoolThread() throw () {
                    if (Thr_) {
                        Thr_->Detach();
                    }
                }

                virtual void DoRun(IThreadAble* func) {
                    Thr_.Reset(new TThread(ThreadProc, func));

                    Thr_->Start();
                }

                virtual void DoJoin() throw () {
                    if (!Thr_) {
                        return;
                    }

                    Thr_->Join();
                    Thr_.Destroy();
                }

            private:
                static void* ThreadProc(void* func) {
                    ((IThreadAble*)(func))->Execute();

                    return 0;
                }

            private:
                THolder<TThread> Thr_;
        };

        inline TSystemThreadPool() throw () {
        }

        virtual IThread* DoCreate() {
            return new TPoolThread;
        }
    };

    class TThrFuncObj: public IThreadPool::IThreadAble {
        public:
            TThrFuncObj(const TFunction<void()>& func)
                : Func(func)
            {
            }
            void DoExecute() {
                THolder<TThrFuncObj> self(this);
                Func();
            }
        private:
            TFunction<void()> Func;
    };
}

TAutoPtr<IThread> IThreadPool::Run(TFunction<void()> func) {
    TAutoPtr<IThread> ret(DoCreate());

    ret->Run(new ::TThrFuncObj(func));

    return ret;
}

static IThreadPool* SystemThreadPoolImpl() {
    return Singleton<TSystemThreadPool>();
}

static IThreadPool* systemPool = 0;

IThreadPool* SystemThreadPool() {
    if (systemPool) {
        return systemPool;
    }

    return SystemThreadPoolImpl();
}

void SetSystemThreadPool(IThreadPool* pool) {
    systemPool = pool;
}
