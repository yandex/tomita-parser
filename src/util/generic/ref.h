#pragma once

#include "ptr.h"
#include "typetraits.h"

/* a potentially unsafe class (but no more so than TStringBuf) which can
 * be initialized either by references (without ownership) or by smart
 * or allocated pointers (with ownership and reference counting) */

template <typename TPSmartPtr, typename T = typename TPSmartPtr::TValueType>
struct TSmartPtrTraits {
    enum {
        IsSmartPtr = TIsDerived<TPointerBase<TPSmartPtr, T>, TPSmartPtr>::Result,
    };
};

template <typename TPSmartPtr>
class TSmartRef: public TPointerBase<TSmartRef<TPSmartPtr>, typename TEnableIf<TSmartPtrTraits<TPSmartPtr>::IsSmartPtr, typename TPSmartPtr::TValueType>::TResult> {
public:
    typedef typename TPSmartPtr::TValueType TDVal;
    typedef TPSmartPtr TDSmartPtr;
    typedef TSmartRef TDSelf;

private:
    TDSmartPtr SmartPtr_;
    TDVal* Ptr_;

protected:
    // don't use when non-trivial copy semantics apply to TDSmartPtr
    TSmartRef(TDVal* ptr, const TDSmartPtr& sptr)
        : SmartPtr_(sptr)
        , Ptr_(ptr)
    {
    }

public:
    TSmartRef(TDVal& ref)
        : Ptr_(&ref)
    {
    }

    TSmartRef(TDVal* ptr = 0)
        : SmartPtr_(ptr)
        , Ptr_(ptr)
    {
    }

    TSmartRef(const TPSmartPtr& ptr)
        : SmartPtr_(ptr)
        , Ptr_(SmartPtr_.Get())
    {
    }

    // copy constructor
    TSmartRef(const TDSelf& obj)
        : SmartPtr_(obj.SmartPtr_)
        , Ptr_(SmartPtr_.Get() ? SmartPtr_.Get() : obj.Ptr_)
    {
    }

    // make sure TPOther is a smart pointer and its value type derives from TDVal
    template <typename TPOther, typename TPOtherVal = typename TPOther::TValueType>
    struct TOtherTraits {
        enum {
            IsCompatType = TIsDerived<TDVal, TPOtherVal>::Result,
            IsCompatSmartPtr = IsCompatType && TSmartPtrTraits<TPOther, TPOtherVal>::IsSmartPtr,
        };
    };

    template <typename TPOther>
    TSmartRef(const TPOther& ptr, typename TEnableIf<TOtherTraits<TPOther>::IsCompatSmartPtr, bool>::TResult = false)
        : SmartPtr_(ptr)
        , Ptr_(SmartPtr_.Get())
    {
    }

    template <typename TPOther>
    friend class TSmartRef;

    template <typename TPOther>
    TSmartRef(const TSmartRef<TPOther>& obj, typename TEnableIf<TOtherTraits<TPOther>::IsCompatType, bool>::TResult = false)
        : SmartPtr_(obj.SmartPtr_)
        , Ptr_(SmartPtr_.Get() ? SmartPtr_.Get() : obj.Ptr_)
    {
    }

    // assignment operator
    TDSelf& operator=(TDSelf ref) {
        Swap(ref);
        return *this;
    }

    // TPointerCommon
    TDVal* Get() const {
        return Ptr_;
    }

    const TDSmartPtr& SmartPtr() const {
        return SmartPtr_;
    }

    // other
    void Reset() {
        Reset(TDSelf());
    }

    void Reset(const TDSelf& ref) {
        *this = ref;
    }

    void Swap(TDSelf& obj) {
        SmartPtr_.Swap(obj.SmartPtr_);
        DoSwap(Ptr_, obj.Ptr_);
    }
};

// template "typedef"
template <typename T, typename C = TSimpleCounter, typename D = TDelete>
struct TSharedRef: public TSmartRef<TSharedPtr<T, C, D> > {
    typedef TSharedPtr<T, C, D> TDSmartPtr;
    typedef TSmartRef<TDSmartPtr> TDSmartRef;

    TSharedRef(T& ref)
        : TDSmartRef(ref)
    {
    }

    TSharedRef(T* ptr = 0)
        : TDSmartRef(ptr)
    {
    }

    TSharedRef(const TDSmartPtr& ptr)
        : TDSmartRef(ptr)
    {
    }

    TSharedRef(const TDSmartRef& obj)
        : TDSmartRef(obj.Get(), obj.SmartPtr())
    {
    }

    TSharedRef(const TAutoPtr<T, D>& ptr)
        : TDSmartRef(ptr)
    {
    }

    // make sure U derives from T

    template <typename U>
    TSharedRef(const TSharedPtr<U, C, D>& ptr, typename TEnableIf<TIsDerived<T, U>::Result, bool>::TResult = false)
        : TDSmartRef(ptr)
    {
    }

    template <typename U>
    TSharedRef(const TSmartRef<TSharedPtr<U, C, D> >& obj, typename TEnableIf<TIsDerived<T, U>::Result, bool>::TResult = false)
        : TDSmartRef(obj.Get(), obj.SmartPtr())
    {
    }
};
