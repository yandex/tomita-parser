#pragma once

#include "bind.h"

#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>

// Functor wraps any function object or pointer to member
template <typename Func>
struct TFunctor;

template <typename TFuncProto, typename TFunc>
class TFuncHandler;

template <typename TFuncProto>
class TFunction;

template <typename TFuncProto>
class TFunctionBase {
protected:
    typedef TFunctor<TFuncProto> TImpl;
    TIntrusivePtr<TImpl> Impl;

public:
    typedef typename TImpl::TResult TResult;

    inline TFunctionBase() throw () {
    }

    inline TFunctionBase(const TFunctionBase<TFuncProto>& lhs)
        : Impl(lhs.Impl)
    {
    }

    template <typename TFunc>
    inline TFunctionBase(const TFunc& func)
        : Impl(new TFuncHandler<TFuncProto, TFunc>(func))
    {
    }

    inline explicit operator bool () const throw () {
        return Impl.Get() != nullptr;
    }
};

template <typename R, typename... Params>
struct TFunctor<R (Params...)>: public TThrRefBase {
    typedef R TResult;
    virtual R operator()(Params...) = 0;
};

template <typename TFunc, typename R, typename... Params>
class TFuncHandler<R (Params...), TFunc>: public TFunctor<R (Params...)> {
public:
    typedef TFunctor<R (Params...)> TBase;

    TFuncHandler(const TFunc& func)
        : Func(func)
    {
    }

    R operator()(Params... params) {
        return Func(params...);
    }

private:
    TFunc Func;
};

template <typename R, typename... Params>
class TFuncHandler<R (Params...), R (Params...)>: public TFunctor<R (Params...)> {
public:
    typedef TFunctor<R (Params...)> TBase;

    TFuncHandler(R func(Params...))
        : Func(func)
    {
    }

    R operator()(Params... params) {
        return (*Func)(params...);
    }

private:
    R (*Func)(Params...);
};

template <typename R, typename... Params>
class TFunction<R (Params...)>: public TFunctionBase<R (Params...)> {
    typedef TFunctionBase<R (Params...)> TBase;
public:

    TFunction() {
    }

    TFunction(const TFunction<R (Params...)>& lhs)
        : TBase((const TBase&) lhs)
    {
    }

    template <typename TFunc>
    TFunction(const TFunc& func)
        : TBase(func)
    {
    }

    template <typename Host, typename TFunc>
    TFunction(const Host& host, const TFunc& func)
        : TBase(BindFirst(func, host))
    {
    }

    typename TBase::TResult operator()(Params... params) const {
        return (*this->Impl)(params...);
    }
};

/// defined so TFunction can be used in BindFirst.
template <typename TSig>
struct TFunctionType<TFunction<TSig> > {
    typedef TSig TType;
};
