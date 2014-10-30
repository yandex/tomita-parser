#pragma once

#include "function_traits.h"

#include <util/generic/typetraits.h>

template <typename TMethod>
struct TMemFun;

template <typename T, typename R, typename... Params>
struct TMemFun<R (T::*)(Params...)> {
    typedef R (T::*TMethod)(Params...);

    const TMethod mMethod;

    TMemFun(const TMethod& method)
        : mMethod(method)
    {
    }

    inline R operator() (T& thiz, Params... params) const {
        return (thiz.*mMethod)(params...);
    }

    template <typename TThis>
    inline R operator() (TThis thiz, Params... params) const {
        return this->operator ()(*thiz, params...);
    }
};

template <typename T, typename R, typename... Params>
struct TMemFun<R (T::*)(Params...) const> {
    typedef R (T::*TMethod)(Params...) const;

    const TMethod mMethod;

    TMemFun(const TMethod& method)
        : mMethod(method)
    {
    }

    inline R operator() (const T& thiz, Params... params) const {
        return (thiz.*mMethod)(params...);
    }

    template <typename TThis>
    inline R operator() (TThis thiz, Params... params) const {
        return this->operator ()(*thiz, params...);
    }
};

template <typename TMethod>
struct TFunctionType<TMemFun<TMethod> > {
    typedef typename TFunctionType<TMethod>::TType TType;
};

template <typename TMethod>
struct TFunctionVar<TMethod, true> {
    typedef TMemFun<TMethod> TType;
};

template <typename TMethod>
TMemFun<TMethod> MemFun(TMethod method) {
    return TMemFun<TMethod>(method);
}
