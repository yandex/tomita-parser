#pragma once

#include <util/generic/noncopyable.h>

template <class T>
struct TCommonLockOps {
    static inline void Acquire(T* t) throw () {
        t->Acquire();
    }

    static inline void Release(T* t) throw () {
        t->Release();
    }
};

template <class T>
struct TTryLockOps: TCommonLockOps<T> {
    static inline bool TryAcquire(T* t) throw () {
        return t->TryAcquire();
    }
};

//must be used with great care
template <class TOps>
struct TInverseLockOps: TOps {
    template <class T>
    static inline void Acquire(T* t) throw () {
        TOps::Release(t);
    }

    template <class T>
    static inline void Release(T* t) throw () {
        TOps::Acquire(t);
    }
};

template <class T, class TOps = TCommonLockOps<T> >
class TGuard: public TNonCopyable {
    public:
        inline TGuard(const T& t) throw () {
            Init(&t);
        }

        inline TGuard(const T* t) throw () {
            Init(t);
        }

        inline ~TGuard() throw () {
            Release();
        }

        inline void Release() throw () {
            if (WasAcquired()) {
                TOps::Release(T_);
                T_ = 0;
            }
        }

        inline bool WasAcquired() const throw () {
            return T_ != 0;
        }

    private:
        inline void Init(const T* t) throw () {
            T_ = const_cast<T*>(t); TOps::Acquire(T_);
        }

    private:
        T* T_;
};

template <class T, class TOps = TCommonLockOps<T> >
class TInverseGuard: public TGuard<T, TInverseLockOps<TOps> > {
public:
    inline TInverseGuard(const T& t) throw()
        : TGuard<T, TInverseLockOps<TOps> >(t)
    {
    }
};

template <class T, class TOps = TTryLockOps<T> >
class TTryGuard: public TNonCopyable {
    public:
        inline TTryGuard(const T& t) throw () {
            Init(&t);
        }

        inline TTryGuard(const T* t) throw () {
            Init(t);
        }

        inline ~TTryGuard() throw () {
            Release();
        }

        inline void Release() throw () {
            if (WasAcquired()) {
                TOps::Release(T_);
                T_ = 0;
            }
        }

        inline bool WasAcquired() const throw () {
            return T_ != 0;
        }

    private:
        inline void Init(const T* t) throw () {
            T_ = 0;
            T* tMutable = const_cast<T*>(t);
            if (TOps::TryAcquire(tMutable)) {
                T_ = tMutable;
            }
        }

    private:
        T* T_;
};
