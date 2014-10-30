#pragma once

#include "typetraits.h"

#include <util/cpp11.h>

//TODO: s/ForwardArg/Forward
template <class S>
static inline S&& ForwardArg(typename TRemoveReference<S>::TType& a) noexcept {
    return static_cast<S&&>(a);
}

template <class S>
static inline S&& ForwardArg(typename TRemoveReference<S>::TType&& a) noexcept {
    STATIC_ASSERT(!TTypeTraits<S>::IsLvalueReference);
    return static_cast<S&&>(a);
}
