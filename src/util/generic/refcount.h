#pragma once

#include <util/system/guard.h>
#include <util/system/atomic.h>
#include <util/system/defaults.h>
#include <util/system/thread.i>
#include <util/system/yassert.h>

template<class TCounterCheckPolicy>
class TSimpleCounterTemplate : public TCounterCheckPolicy {
    using TCounterCheckPolicy::Check;

    public:
        inline TSimpleCounterTemplate(long initial = 0) throw ()
            : Counter_(initial)
        {
        }

        inline ~TSimpleCounterTemplate() throw () {
            Check();
        }

        inline void Add(TAtomicBase d) throw () {
            Check();
            Counter_ += d;
        }

        inline void Inc() throw () {
            Add(1);
        }

        inline long Sub(TAtomicBase d) throw () {
            Check();
            return Counter_ -= d;
        }

        inline long Dec() throw () {
            return Sub(1);
        }

        inline long Val() const throw () {
            return Counter_;
        }

    private:
        long Counter_;
};

class TNoCheckPolicy {
protected:
    inline void Check() const {
    }
};

#if defined(SIMPLE_COUNTER_THREAD_CHECK)
class TCheckPolicy {
public:
    inline TCheckPolicy() {
        ThreadId = SystemCurrentThreadId();
    }

protected:
    inline void Check() const {
        VERIFY(ThreadId == SystemCurrentThreadId(), "incorrect usage of TSimpleCounter");
    }

private:
    size_t ThreadId;
};
#else
typedef TNoCheckPolicy TCheckPolicy;
#endif

// Use this one if access from multiple threads to your pointer is an error and you want to enforce thread checks
typedef TSimpleCounterTemplate<TCheckPolicy> TSimpleCounter;
// Use this one if you do want to share the pointer between threads, omit thread checks and do the synchronization yourself
typedef TSimpleCounterTemplate<TNoCheckPolicy> TExplicitSimpleCounter;

class TAtomicCounter {
    public:
        inline TAtomicCounter(long initial = 0) throw ()
            : Counter_(initial)
        {
        }

        inline ~TAtomicCounter() throw () {
        }

        inline void Add(TAtomicBase d) throw () {
            AtomicAdd(Counter_, d);
        }

        inline void Inc() throw () {
            Add(1);
        }

        inline TAtomicBase Sub(TAtomicBase d) throw () {
            return AtomicSub(Counter_, d);
        }

        inline TAtomicBase Dec() throw () {
            return Sub(1);
        }

        inline TAtomicBase Val() const throw () {
            return AtomicGet(Counter_);
        }

    private:
        TAtomic Counter_;
};

template<>
struct TCommonLockOps<TAtomicCounter> {
    static inline void Acquire(TAtomicCounter* t) throw () {
        t->Inc();
    }
    static inline void Release(TAtomicCounter* t) throw () {
        t->Dec();
    }
};

template <bool threadSafe>
struct TRefCounterTraits;

template <>
struct TRefCounterTraits<true> {
    typedef TAtomicCounter TResult;
};

template <>
struct TRefCounterTraits<false> {
    typedef TSimpleCounter TResult;
};

template <bool threadSafe>
class TRefCounter {
    public:
        inline TRefCounter(long initval = 0) throw ()
            : Counter_(initval)
        {
        }

        inline ~TRefCounter() throw () {
        }

        inline void Add(TAtomicBase d) throw () {
            Counter_.Add(d);
        }

        inline void Inc() throw () {
            Counter_.Inc();
        }

        inline TAtomicBase Sub(TAtomicBase d) throw () {
            return Counter_.Sub(d);
        }

        inline long Dec() throw () {
            return Counter_.Dec();
        }

        inline long Val() const throw () {
            return Counter_.Val();
        }

    private:
        typename TRefCounterTraits<threadSafe>::TResult Counter_;
};
