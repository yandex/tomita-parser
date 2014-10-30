#pragma once

#include "memfun.h"
#include "function_traits.h"

#include <util/generic/static_assert.h>

/// Use Bind* functions below to perform the binding
/// See bind_ut.cpp for info, how this can be used

/// Implementation

template <typename TpOp, typename TpArg>
struct TBinderBase {
    typedef typename TResultOf<TpOp>::TType TResult;
    typedef typename TFunctionVar<TpOp>::TType TOperationVar;

    TOperationVar mSelf;
    TpArg mParam;

    inline TBinderBase(const TOperationVar& self, const TpArg& param)
        : mSelf(self)
        , mParam(param)
    {
    }
};

template <typename TpOp, typename TpArg, size_t ParamPos, size_t ParamCount = TFunctionParamCount<TpOp>::Value>
struct TBinder;

template <typename TpOp, typename TpArg1, size_t ParamPos>
struct TFunctionType<TBinder<TpOp, TpArg1, ParamPos> > {
    typedef ::TBinder<TpOp, TpArg1, ParamPos> TBinder;

    typedef typename TBinder::TType TType;
};

template <size_t ParamPos, typename TpOp, typename TpArg>
static inline TBinder<TpOp, TpArg, ParamPos> Bind(const TpOp& self, const TpArg& param) {
    return TBinder<TpOp, TpArg, ParamPos>(self, param);
}

/// bind first arg
template <typename TpOp, typename TpArg1>
static inline TBinder<TpOp, TpArg1, 1> BindFirst(const TpOp& self, const TpArg1& param) {
    return Bind<1, TpOp, TpArg1>(self, param);
}

#include "generated/bind.h"

template <typename TpOp, typename TpArg2>
static inline TBinder<TpOp, TpArg2, 2> BindSecond(const TpOp& self, const TpArg2& param) {
    return Bind<2, TpOp, TpArg2>(self, param);
}

template <typename TpOp, typename TpArg>
static inline TBinder<TpOp, TpArg, TFunctionParamCount<TpOp>::Value> BindLast(const TpOp& self, const TpArg& param) {
    return Bind<TFunctionParamCount<TpOp>::Value, TpOp, TpArg>(self, param);
}
