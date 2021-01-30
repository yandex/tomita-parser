#pragma once

#include <util/system/align.h>
#include <util/system/atexit.h>
#include <util/system/defaults.h>
#include <util/system/spinlock.h>

#include <new>

template <class T>
struct TSingletonTraits {
    static const size_t Priority = 65536;
};

namespace NPrivate {
    void FillWithTrash(void* ptr, size_t len);

    template <class T>
    void Destroyer(void* ptr) {
        UNUSED(ptr);
        ((T*)ptr)->~T();
        FillWithTrash(ptr, sizeof(T));
    }

    template <class T, size_t P>
    static inline T* SingletonInt() {
        static T* volatile ret;
        static char tBuf[sizeof(T) + PLATFORM_DATA_ALIGN];

        if (EXPECT_FALSE(!ret)) {
            static TAtomic lock;
            TGuard<TAtomic> guard(lock);

            if (!ret) {
                char* buf = AlignUp((char*)tBuf);
                T* res = ::new (buf) T();

                try {
                    AtExit(Destroyer<T>, res, P);
                } catch (...) {
                    Destroyer<T>(res);

                    throw;
                }

                ret = res;
            }
        }

        return (T*)ret;
    }

    template <class T>
    class TDefault {
    public:
        inline TDefault()
            : T_()
        {
        }

        inline const T* Get() const throw () {
            return &T_;
        }

    private:
        T T_;
    };
}

#define DECLARE_SINGLETON_FRIEND(T) friend T* ::NPrivate::SingletonInt<T, TSingletonTraits<T >::Priority>();

template <class T>
T* Singleton(T* value = NULL) {
    static T* ret;
    static thread_local T* local_ret;

    if (value) {
        local_ret = value;
    } else if (EXPECT_FALSE(!ret)) {
        ret = ::NPrivate::SingletonInt<T, TSingletonTraits<T>::Priority>();
    }

    if (local_ret != NULL) {
        return local_ret;
    }

    return ret;
}

template <class T, size_t P>
T* SingletonWithPriority() {
    static T* ret;

    if (EXPECT_FALSE(!ret)) {
        ret = ::NPrivate::SingletonInt<T, P>();
    }

    return ret;
}

template <class T>
const T& Default() {
    static const T* ret;

    if (EXPECT_FALSE(!ret)) {
        ret = ::NPrivate::SingletonInt<typename ::NPrivate::TDefault<T>, TSingletonTraits<T>::Priority>()->Get();
    }

    return *ret;
}
