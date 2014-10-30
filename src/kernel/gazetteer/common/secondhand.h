#pragma once

#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/utility.h>
#include <util/generic/ptr.h>

#include <util/thread/lfstack.h>


template <typename T>
class TSimpleRecyclePool {
public:
    inline TIntrusivePtr<T> Pop() {
        TAutoPtr<T> free;
        return Free.Dequeue(&free) ? free.Release() : new T();
    }

    inline void Push(T* t) {
        if (t) {
            t->Reset();
            Free.Enqueue(t);
        }
    }

private:
    TLockFreeStack< TAutoPtr<T> > Free;
};

struct TRecycleDelete {
    template <class T>
    static inline void Destroy(T* t) throw () {
        Singleton<TSimpleRecyclePool<T> >()->Push(t);
    }
};

// Ref-counted object T which is recycled with Singleton<TSimpleRecyclePool<T> >
// when its ref-count decreases to 0.
template <typename T>
class TRecycleable: public TRefCounted<T, TAtomicCounter, TRecycleDelete> {
public:
    class TRecyclePtr: public TIntrusivePtr<T> {
    public:
        TRecyclePtr()
            : TIntrusivePtr<T>(Singleton<TSimpleRecyclePool<T> >()->Pop())
        {
        }
    };
};





template <typename T, typename TFactory>
class TRecyclePool: public TRefCounted< TRecyclePool<T, TFactory> > {

    class TItemHolder: public THolder<T> {
        typedef THolder<T> TBase;
        public:
            inline TItemHolder(T* obj, TRecyclePool* pool = NULL)
                : TBase(obj)
                , Parent(pool)
                , Counter(0)
            {
            }

            inline void Ref() {
                Counter.Inc();
            }

            inline void UnRef() {
                // on last UnRef an item is pushed back to its parent pool
                if (!Counter.Dec()) {
                    if (!!Parent)
                        Parent->Push(TBase::Release());
                }
            }

            inline void DecRef() throw () {
                Counter.Dec();
            }

            inline const TRecyclePool* Pool() const {
                return Parent.Get();
            }

        private:
            TIntrusivePtr<TRecyclePool> Parent;
            TAtomicCounter Counter;
    };

public:
    class TItemPtr: public TPointerBase<TItemPtr, T> {
    public:
        inline TItemPtr(T* obj = NULL, TRecyclePool* pool = NULL)
            : Item(obj ? new TItemHolder(obj, pool) : NULL)
        {
        }

        inline T* Get() const {
            return (Item.Get()) ? Item->Get() : NULL;
        }

        inline void Swap(TItemPtr& r) {
            Item.Swap(r.Item);
        }
/*
        inline T* Release() const {
            return (Item.Get()) ? Item->Release() : NULL;
        }
*/
        inline const TRecyclePool* Pool() const {
            return (Item.Get()) ? Item->Pool() : NULL;
        }

    private:
        TIntrusivePtr<TItemHolder> Item;
    };


    inline TRecyclePool(const TFactory* factory)
        : Factory(factory)
    {
    }

    inline TItemPtr Pop() {
        TAutoPtr<T> stored;
        T* t = Impl.Dequeue(&stored) ? stored.Release() : New();
        return TItemPtr(t, this);
    }

    inline void Push(T* t) {
        if (t)
            Impl.Enqueue(t);
    }

    inline T* New() const {
        return Factory->New();
    }

private:
    const TFactory* Factory;
    TLockFreeStack< TAutoPtr<T> > Impl;
};







template <typename T>
class TSwapDataCollector {
public:
    TSwapDataCollector()
        : Cache()
        , Last(Cache.begin())
    {
    }

    // @obj internal data is swapped into cache, without copying
    // @obj capacity becomes zero.
    inline void PutData(T& obj) {
        if (obj.capacity() > 0) {
            obj.clear();
            if (Last == Cache.end()) {
                Cache.push_back(T());
                ::DoSwap(Cache.back(), obj);
                Last = Cache.end();
            } else {
                ::DoSwap(*Last, obj);
                ++Last;
            }
        }
    }

    // @obj capacity is increased from one of the cached items, via swapping
    inline void GetData(T& obj) {
        if (obj.capacity() == 0 && Last != Cache.begin()) {
            --Last;
            ::DoSwap(*Last, obj);
        }
    }

protected:
    ylist<T> Cache;
    typename ylist<T>::iterator Last;
};

template <typename T>
class TVectorCollector: public TSwapDataCollector< yvector<T> > {
};

/*
template <typename T>
class TRefCollector {
public:
    inline TRefCollector() {
        Reset();
    }

    inline void Reserve(size_t count, size_t len) {
        for (size_t i = 0; i < count; ++i) {
            T t;
            t.reserve(len);
            Ref(t);
        }
        Revoke();
    }

    // @obj is copying in cache, its ref-count increases.
    // it is better to PushData after all modification of object,
    // to avoid its cloning on next write.
    inline void Ref(const T& obj) {
        if (obj.capacity() > 0) {
            if (RefIt == Refs.end()) {
                Refs.push_back(obj);       // ++ref on copying
                RefIt = Refs.end();
            } else {
                *RefIt = obj;
                ++RefIt;
            }
        }
    }

    inline void Revoke() {
        Refs.swap(Unrefs);
        Reset();
    }

    inline T Unref() {
        T obj;
        if (UnrefIt != Unrefs.end()) {
            ::DoSwap(*UnrefIt, obj);
            ++UnrefIt;
        }
        return obj;
    }

private:
    inline void Reset() {
        RefIt = Refs.begin();
        UnrefIt = Unrefs.begin();
    }

    ylist<T> Refs, Unrefs;
    typename ylist<T>::iterator RefIt, UnrefIt;

};


template <class T>
class TRefHolder: public T {
public:
    inline TRefHolder(TRefCollector<T>& collector)
        : Collector(&collector)
    {
        *AsT() = Collector->Unref();
    }

    inline ~TRefHolder() {
        Collector->Ref(*AsT());
    }

private:
    inline T* AsT() {
        return static_cast<T*>(this);
    }

    TRefCollector<T>* Collector;
    DECLARE_NOCOPY(TRefHolder);
};


typedef TRefCollector<Stroka> TStringCollector;
typedef TRefHolder<Stroka> TSecondHandString;
*/

