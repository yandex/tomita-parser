#pragma once

template <typename T> struct TAtomicTraits {
    enum {
        Castable = (TTypeTraits<T>::IsIntegral || TTypeTraits<T>::IsPointer) && sizeof(T) == sizeof(TAtomicBase) && !TTypeTraits<T>::IsConstant,
    };
    typedef TEnableIf<Castable, bool> TdBool;
};

template <typename T>
inline TAtomicBase AtomicGet(T const volatile& target
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicGet(*AsAtomicPtr(&target));
}

template <typename T>
inline void AtomicSet(T volatile& target, TAtomicBase v
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    AtomicSet(*AsAtomicPtr(&target), v);
}

template <typename T>
inline TAtomicBase AtomicIncrement(T volatile& target
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicIncrement(*AsAtomicPtr(&target));
}

template <typename T>
inline TAtomicBase AtomicDecrement(T volatile& target
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicDecrement(*AsAtomicPtr(&target));
}

template <typename T>
inline TAtomicBase AtomicAdd(T volatile& target, TAtomicBase value
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicAdd(*AsAtomicPtr(&target), value);
}

template <typename T>
inline TAtomicBase AtomicSub(T volatile& target, TAtomicBase value
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicSub(*AsAtomicPtr(&target), value);
}

template <typename T>
inline TAtomicBase AtomicSwap(T volatile& target, TAtomicBase exchange
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicSwap(*AsAtomicPtr(&target), exchange);
}

template <typename T>
inline TAtomicBase AtomicCas(T volatile* target, TAtomicBase exchange, TAtomicBase compare
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicCas(AsAtomicPtr(target), exchange, compare);
}

template <typename T>
inline TAtomicBase AtomicTryLock(T volatile* target
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicTryLock(AsAtomicPtr(target));
}

template <typename T>
inline TAtomicBase AtomicTryAndTryLock(T volatile* target
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    return AtomicTryAndTryLock(AsAtomicPtr(target));
}

template <typename T>
inline void AtomicUnlock(T volatile* target
    , typename TAtomicTraits<T>::TdBool::TResult = false)
{
    AtomicUnlock(AsAtomicPtr(target));
}
