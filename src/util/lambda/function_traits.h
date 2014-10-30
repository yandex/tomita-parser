#pragma once

#include <util/system/defaults.h>

/// function type

/// function is something that has operator().
/// Partial specialization with TType must exist in order have Bind* working
template <typename TFunction>
struct TFunctionType {
    // it is not a function by default
};

template <typename TAny>
struct TResultOf {
    typedef typename TResultOf<typename TFunctionType<TAny>::TType >::TType TType;
};

template <typename TAny>
struct TFunctionParamCount {
    static const size_t Value = TFunctionParamCount<typename TFunctionType<TAny>::TType>::Value;
};

template <typename TSomething>
struct TIsMethod {
    static const bool Value = false;
};

template <typename TFunction, bool IsMethod = TIsMethod<TFunction>::Value>
struct TFunctionVar {
    typedef TFunction TType;
};

template <typename R, typename... Params>
struct TFunctionType<R (Params...)> {
    typedef R TType(Params...);
};

template <typename R, typename... Params>
struct TFunctionType<R (*)(Params...)> {
    typedef R TType(Params...);
};

template <typename T, typename R, typename... Params>
struct TFunctionType<R (T::*)(Params...)> {
    typedef R TType(T&, Params...);
};

template <typename T, typename R, typename... Params>
struct TFunctionType<R (T::*)(Params...) const> {
    typedef R TType(const T&, Params...);
};

template <typename R, typename... Params>
struct TResultOf<R (Params...)> {
    typedef R TType;
};

template <typename R, typename... Params>
struct TFunctionParamCount<R (Params...)> {
    static const size_t Value = sizeof... (Params);
};

template <typename R, typename T, typename... Params>
struct TIsMethod<R (T::*)(Params...)> {
    static const bool Value = true;
};

template <typename R, typename T, typename... Params>
struct TIsMethod<R (T::*)(Params...) const> {
    static const bool Value = true;
};

template <size_t N, typename R, typename... Params>
struct TFunctionParamImpl;

template <typename FirstParam, typename R, typename... Params>
struct TFunctionParamImpl<1, R (FirstParam, Params...)> {
    typedef FirstParam TType;
};

template <size_t N, typename R, typename FirstParam, typename... Params>
struct TFunctionParamImpl<N, R (FirstParam, Params...)> {
    typedef typename TFunctionParamImpl<N - 1, R (Params...)>::TType TType;
};

template <typename TAny, size_t N>
struct TFunctionParam {
    typedef typename TFunctionParamImpl<N, typename TFunctionType<TAny>::TType>::TType TType;
};

template <typename R, typename... Params>
struct TFunctionVar<R (Params...), false> {
    typedef typename TFunctionType<R (Params...)>::TType* TType;
};
