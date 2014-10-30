#pragma once

#include <util/generic/ptr.h>
#include <util/lambda/function.h>

class IThreadPool {
    public:
        class IThreadAble {
            public:
                inline IThreadAble() throw () {
                }

                virtual ~IThreadAble() throw () {
                }

                inline void Execute() {
                    DoExecute();
                }

            private:
                virtual void DoExecute() = 0;
        };

        class IThread {
                friend class IThreadPool;
            public:
                inline IThread() throw () {
                }

                virtual ~IThread() throw () {
                }

                inline void Join() throw () {
                    DoJoin();
                }

            private:
                inline void Run(IThreadAble* func) {
                    DoRun(func);
                }

            private:
                // it's actually DoStart
                virtual void DoRun(IThreadAble* func) = 0;
                virtual void DoJoin() throw () = 0;
        };

        inline IThreadPool() throw () {
        }

        virtual ~IThreadPool() {
        }

        // XXX: rename to Start
        inline TAutoPtr<IThread> Run(IThreadAble* func) {
            TAutoPtr<IThread> ret(DoCreate());

            ret->Run(func);

            return ret;
        }

        TAutoPtr<IThread> Run(TFunction<void()> func);

    private:
        virtual IThread* DoCreate() = 0;
};

IThreadPool* SystemThreadPool();
void SetSystemThreadPool(IThreadPool* pool);
