#pragma once

#include "typetraits.h"

#include <util/cpp11.h>

template <class T>
static inline typename TRemoveReference<T>::TType&& MoveArg(T&& a) noexcept {
    typedef typename TRemoveReference<T>::TType&& TRvalRef;

    return static_cast<TRvalRef>(a);
}
