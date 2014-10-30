#pragma once

#include "pool.h"
#include "alloc.h"

#include <util/generic/utility.h>
#include <util/generic/intrlist.h>

class TFixedSizeAllocator {
    struct TAlloc: public TIntrusiveSListItem<TAlloc> {
        inline void* ToPointer() throw () {
            return this;
        }

        static inline TAlloc* FromPointer(void* ptr) throw () {
            return (TAlloc*)ptr;
        }

        static inline size_t EntitySize(size_t alloc) throw () {
            return Max(sizeof(TAlloc), alloc);
        }

        static inline TAlloc* Construct(void* ptr) throw () {
            return (TAlloc*)ptr;
        }
    };

public:
    typedef TMemoryPool::IGrowPolicy IGrowPolicy;

    inline TFixedSizeAllocator(size_t allocSize, IAllocator* alloc)
        : Pool_(allocSize, TMemoryPool::TExpGrow::Instance(), alloc)
        , AllocSize_(allocSize)
    {
    }

    inline TFixedSizeAllocator(size_t allocSize, IGrowPolicy* grow, IAllocator* alloc)
        : Pool_(allocSize, grow, alloc)
        , AllocSize_(allocSize)
    {
    }

    inline void* Allocate() {
        if (EXPECT_FALSE(Free_.Empty())) {
            return Pool_.Allocate(AllocSize_);
        }

        return Free_.PopFront()->ToPointer();
    }

    inline void Release(void* ptr) throw () {
        Free_.PushFront(TAlloc::FromPointer(ptr));
    }

    inline size_t Size() const throw () {
        return AllocSize_;
    }

private:
    TMemoryPool Pool_;
    const size_t AllocSize_;
    TIntrusiveSList<TAlloc> Free_;
};

template <class T>
class TSmallObjAllocator {
public:
    typedef TFixedSizeAllocator::IGrowPolicy IGrowPolicy;

    inline TSmallObjAllocator(IAllocator* alloc)
        : Alloc_(sizeof(T), alloc)
    {
    }

    inline TSmallObjAllocator(IGrowPolicy* grow, IAllocator* alloc)
        : Alloc_(sizeof(T), grow, alloc)
    {
    }

    inline T* Allocate() {
        return (T*)Alloc_.Allocate();
    }

    inline void Release(T* t) throw () {
        Alloc_.Release(t);
    }

private:
    TFixedSizeAllocator Alloc_;
};

template <class T>
class TObjectFromPool {
public:
    typedef TSmallObjAllocator<T> TPool;

    inline void SetPool(TPool* pool) throw () {
        PoolParent_ = pool;
    }

    inline TPool* Pool() const throw () {
        YASSERT(PoolParent_);

        return PoolParent_;
    }

    inline void* operator new(size_t, TPool* pool) {
        T* ret = pool->Allocate();

        /*
         * UB, object not constructed here
         */
        ret->SetPool(pool);

        return ret;
    }

    inline void operator delete(void* ptr, size_t) throw () {
        DoDelete(ptr);
    }

    inline void operator delete(void* ptr, TPool*) throw () {
        /*
         * this delete operator can be called automagically by compiler
         */

        DoDelete(ptr);
    }

private:
    static inline void DoDelete(void* ptr) throw () {
        ((T*)ptr)->Pool()->Release((T*)ptr);
    }

private:
    TPool* PoolParent_;
};
