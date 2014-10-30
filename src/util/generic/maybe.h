#pragma once

#include <util/generic/typehelpers.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>
#include <util/system/align.h>
#include <new>

namespace NMaybePrivate {
    template <class T>
    struct TAlignedType {
        typedef union {
            char Data[sizeof(T)];
            TAligner Alignment;
        } TResult;
    };

    template <class T>
    struct TDataType
        : TSelectType<TTypeTraits<T>::IsPod, T, typename TAlignedType<T>::TResult>
    {
    };

    template <class T, class TFrom>
    struct TGetDataPtr {
        inline static const T* Get(const TFrom* from) {
            return reinterpret_cast<const T*>(from);
        }

        inline static T* Get(TFrom* from) {
            return reinterpret_cast<T*>(from);
        }
    };

    template <class T>
    struct TGetDataPtr<T, typename TAlignedType<T>::TResult> {
        inline static const T* Get(const typename TAlignedType<T>::TResult* from){
            return reinterpret_cast<const T*>(from->Data);
        }

        inline static T* Get(typename TAlignedType<T>::TResult* from){
            return reinterpret_cast<T*>(from->Data);
        }
    };
}

template <class T>
class TMaybe {
public:
    inline TMaybe()
        : Defined_(false)
    {
    }

    inline TMaybe(const T& right)
        : Defined_(false)
    {
        Init(right);
    }

    inline TMaybe(const TMaybe& right)
        : Defined_(false)
    {
        if (right.Defined()) {
            Init(*right.Data());
        }
    }

    inline ~TMaybe() {
        Clear();
    }

    inline TMaybe<T>& operator=(const T& right) {
        if (Defined()) {
            *Data() = right;
        } else {
            Init(right);
        }

        return *this;
    }

    inline TMaybe<T>& operator=(const TMaybe& right) {
        if (right.Defined()) {
            operator=(*right.Data());
        } else {
            Clear();
        }

        return *this;
    }

    inline void Clear() {
        if (Defined()) {
            Destroy();
        }
    }

    inline bool Defined() const throw () {
        return Defined_;
    }

    inline bool Empty() const throw () {
        return !Defined();
    }

    inline void CheckDefined() const {
        if (!Defined()) {
            ythrow yexception() << AsStringBuf("TMaybe is empty");
        }
    }

    const T* Get() const throw () {
        return Defined() ? Data() : 0;
    }

    T* Get() throw () {
        return Defined() ? Data() : 0;
    }

    inline const T& GetRef() const {
        CheckDefined();
        return *Data();
    }

    inline T& GetRef() {
        CheckDefined();
        return *Data();
    }

    inline const T& operator*() const {
        return GetRef();
    }

    inline T& operator*() {
        return GetRef();
    }

    const T* operator->() const {
        return &GetRef();
    }

    inline T* operator->() {
        return &GetRef();
    }

    const T& GetOrElse(const T& elseValue) const {
        return Defined() ? *Data() : elseValue;
    }

    T& GetOrElse(T& elseValue) {
        return Defined() ? *Data() : elseValue;
    }

    const TMaybe<T>& OrElse(const TMaybe& elseValue) const throw () {
        return Defined() ? *this : elseValue;
    }

    TMaybe<T>& OrElse(TMaybe& elseValue) {
        return Defined() ? *this : elseValue;
    }

    inline bool operator==(const TMaybe& that) const {
        if (Empty() || that.Empty()) {
            return Empty() == that.Empty();
        }
        return *Data() == *that.Data();
    }

    inline bool operator!=(const TMaybe& that) const {
        return !operator==(that);
    }

    inline explicit operator bool () const throw () {
        return Defined();
    }

private:
    bool Defined_;
    typename NMaybePrivate::TDataType<T>::TResult Data_;

    inline const T* Data() const throw () {
        return NMaybePrivate::TGetDataPtr<T, typename NMaybePrivate::TDataType<T>::TResult>::Get(&Data_);
    }

    inline T* Data() throw () {
        return NMaybePrivate::TGetDataPtr<T, typename NMaybePrivate::TDataType<T>::TResult>::Get(&Data_);
    }

    inline void Init(const T& right) {
        new (Data()) T(right);
        Defined_ = true;
    }

    inline void Destroy() {
        Defined_ = false;
        Data()->~T();
    }
};

namespace NPrivate {
    class TNothing {
    public:
        inline TNothing() {
        }

        template <typename T>
        inline operator TMaybe<T>() const {
            return TMaybe<T>();
        }
    };
}

inline ::NPrivate::TNothing Nothing() {
    return ::NPrivate::TNothing();
}
