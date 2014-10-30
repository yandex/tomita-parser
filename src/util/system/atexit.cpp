#include "align.h"
#include "atexit.h"
#include "yassert.h"
#include "spinlock.h"

#include <util/system/thread.h>
#include <util/memory/smallobj.h>
#include <util/generic/ylimits.h>
#include <util/generic/avltree.h>
#include <util/generic/utility.h>

#include <cstdlib>

namespace {
    class TAtExit {
        typedef TAdaptiveLock TLock;
    public:
        class TMyAllocator: public TMemoryPool::TExpGrow, public TDefaultAllocator {
            public:
                inline TMyAllocator()
                    : FirstTime_(true)
                {
                }

                virtual ~TMyAllocator() throw () {
                }

                virtual TBlock Allocate(size_t len) {
                    if (FirstTime_) {
                        TBlock ret = {AlignedBuf(), AlignedBufLen()};

                        FirstTime_ = false;

                        return ret;
                    }

                    return TDefaultAllocator::Allocate(len);
                }

                virtual void Release(const TBlock& block) {
                    if (AlignedBuf() == block.Data) {
                        FirstTime_ = true;
                    } else {
                        TDefaultAllocator::Release(block);
                    }
                }

            private:
                inline char* AlignedBuf() const throw () {
                    return AlignUp((char*)Buf_);
                }

                inline size_t AlignedBufLen() const throw () {
                    return (Buf_ + sizeof(Buf_)) - AlignedBuf();
                }

            private:
                char Buf_[1024 * 4];
                bool FirstTime_;
        };

        struct TCmp {
            template <class T>
            static inline bool Compare(const T& l, const T& r) throw () {
                if (l.Priority < r.Priority) {
                    return true;
                }

                if (l.Priority == r.Priority) {
                    return l.Number < r.Number;
                }

                return false;
            }
        };

        struct TFunc: public TAvlTreeItem<TFunc, TCmp>, public TObjectFromPool<TFunc> {
            inline TFunc(TAtExitFunc func, void* ctx, size_t priority, size_t number)
                : Func(func)
                , Ctx(ctx)
                , Priority(priority)
                , Number(number)
            {
            }

            TAtExitFunc Func;
            void* Ctx;
            size_t Priority;
            size_t Number;
        };

    public:
        inline TAtExit()
            : Cnt_(0)
            , Pool_(&Alloc_, &Alloc_)
        {
        }

        inline void Finish() throw () {
            TGuard<TLock> guard(Lock_);

            while (!Items_.Empty()) {
                TFunc* c = &*Items_.Last();

                YASSERT(c);

                c->Unlink();

                {
                    TGuard<TLock, TInverseLockOps<TCommonLockOps<TLock> > > unguard(Lock_);

                    try {
                        c->Func(c->Ctx);
                    } catch (...) {
                    }
                }
            }
        }

        inline void Register(TAtExitFunc func, void* ctx, size_t priority) {
            TGuard<TLock> guard(Lock_);

            Items_.Insert(new (&Pool_) TFunc(func, ctx, priority, ++Cnt_));
        }

    private:
        size_t Cnt_;
        TLock Lock_;
        TMyAllocator Alloc_;
        TFunc::TPool Pool_;
        TAvlTree<TFunc, TCmp> Items_;
    };

    static TAtomic atExitLock = 0;
    static TAtExit* atExit = 0;
    static char atExitMem[sizeof(TAtExit) + PLATFORM_DATA_ALIGN];

    static void OnExit() {
        if (atExit) {
            atExit->Finish();
            atExit->~TAtExit();
            atExit = 0;
        }
    }

    static inline TAtExit* Instance() {
        if (!atExit) {
            TGuard<TAtomic> guard(atExitLock);

            if (!atExit) {
                atexit(OnExit);
                atExit = new (AlignUp(atExitMem)) TAtExit;
            }
        }

        return atExit;
    }
}

void AtExit(TAtExitFunc func, void* ctx, size_t priority) {
    Instance()->Register(func, ctx, priority);
}

void AtExit(TAtExitFunc func, void* ctx) {
    AtExit(func, ctx, Max<size_t>());
}

static void TraditionalCloser(void* ctx) {
    ((TTraditionalAtExitFunc)ctx)();
}

void AtExit(TTraditionalAtExitFunc func) {
    AtExit(TraditionalCloser, (void*)func);
}

void AtExit(TTraditionalAtExitFunc func, size_t priority) {
    AtExit(TraditionalCloser, (void*)func, priority);
}
