#pragma once

template <class T1, class T2>
struct TSameType {
    enum {
        Result = false
    };
};

template <class T>
struct TSameType<T, T> {
    enum {
        Result = true
    };
};

template <class T1, class T2>
struct TIsDerived {
    struct RT1 {
        char t[2];
    };

    typedef char RT2;

    static RT1 Func(T1*);
    static RT2 Func(...);
    static T2* GetT2();

    /*
     * Result == (T2 derived from T1)
     */
    enum {
        Result = (sizeof(Func(GetT2())) != sizeof(RT2))
    };
};

template <bool val, class T1, class T2>
struct TSelectType {
    typedef T1 TResult;
};

template <class T1, class T2>
struct TSelectType<false, T1, T2> {
    typedef T2 TResult;
};
