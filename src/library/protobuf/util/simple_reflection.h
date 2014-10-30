#pragma once

#include "traits.h"
#include "cast.h"

#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/message.h>

#include <util/generic/typetraits.h>
#include <util/generic/typehelpers.h>
#include <util/system/defaults.h>

namespace NProtoBuf {


class TConstField {
public:
    TConstField(const Message& msg, const FieldDescriptor* fd)
        : Msg(msg)
        , Fd(fd)
    {
        YASSERT(Fd && Fd->containing_type() == Msg.GetDescriptor());
    }

    const Message& Parent() const {
        return Msg;
    }

    const FieldDescriptor* Field() const {
        return Fd;
    }


    bool Has() const {
        return IsRepeated() ? Refl().FieldSize(Msg, Fd) > 0
                            : Refl().HasField(Msg, Fd);
    }

    size_t Size() const {
        return IsRepeated() ? Refl().FieldSize(Msg, Fd)
                            : (Refl().HasField(Msg, Fd) ? 1 : 0);
    }

    template <typename T>
    inline typename TSelectCppType<T>::T Get(size_t index = 0) const;

    template <typename TMsg>
    inline const TMsg* GetAs(size_t index = 0) const {
        // casting version of Get
        return IsMessageInstance<TMsg>() ? CheckedCast<const TMsg*>(Get<const Message*>(index)) : NULL;
    }


    template <typename T>
    bool IsInstance() const {
        return CppType() == TSelectCppType<T>::Result;
    }

    template <typename TMsg>
    bool IsMessageInstance() const {
        return IsMessage() && Fd->message_type() == TMsg::descriptor();
    }

    template<typename TMsg>
    bool IsInstance(typename TEnableIf<TIsDerived<Message, TMsg>::Result && !TSameType<Message, TMsg>::Result, void>::TResult* = NULL) const { // template will be selected when specifying Message children types
        return IsMessage() && Fd->message_type() == TMsg::descriptor();
    }

    bool IsString() const {
        return CppType() == FieldDescriptor::CPPTYPE_STRING;
    }

    bool IsMessage() const {
        return CppType() == FieldDescriptor::CPPTYPE_MESSAGE;
    }

protected:
    bool IsRepeated() const {
        return Fd->is_repeated();
    }

    FieldDescriptor::CppType CppType() const {
        return Fd->cpp_type();
    }

    const Reflection& Refl() const {
        return *Msg.GetReflection();
    }

    YNORETURN_B void RaiseUnknown() const YNORETURN {
        ythrow yexception() << "Unknown field cpp-type: " << (size_t)CppType();
    }

protected:
    const Message& Msg;
    const FieldDescriptor* Fd;
};

class TMutableField: public TConstField {
public:
    TMutableField(Message& msg, const FieldDescriptor* fd)
        : TConstField(msg, fd)
    {
    }

    Message* MutableParent() {
        return Mut();
    }

    template <typename T>
    inline void Set(T value, size_t index = 0);

    template <typename T>
    inline void Add(T value);

    inline void Clear() {
        Refl().ClearField(Mut(), Fd);
    }

    inline void RemoveLast() {
        YASSERT(Has());
        if (IsRepeated())
            Refl().RemoveLast(Mut(), Fd);
        else
            Clear();
    }

    Message* MutableMessage(size_t index = 0) {
        YASSERT(IsMessage() && index < Size());
        if (IsRepeated())
            return Refl().MutableRepeatedMessage(Mut(), Fd, index);
        else
            return Refl().MutableMessage(Mut(), Fd);
    }

    template <typename TMsg>
    inline TMsg& AddMessage() {
        YASSERT(IsMessage() && IsRepeated());
        return *CheckedCast<TMsg*>(Refl().AddMessage(Mut(), Fd));
    }

    inline Message* AddMessage() {
        YASSERT(IsMessage() && IsRepeated());
        return Refl().AddMessage(Mut(), Fd);
    }

private:
    Message* Mut() {
        return const_cast<Message*>(&Msg);
    }
};



// template implementations

template <typename T>
inline typename TSelectCppType<T>::T TConstField::Get(size_t index) const {
    YASSERT(index < Size());
    #define TMP_MACRO_FOR_CPPTYPE(CPPTYPE) case CPPTYPE: return CompatCast<CPPTYPE, TSelectCppType<T>::Result>(TSimpleFieldTraits<CPPTYPE>::Get(Msg, Fd, index));
    switch (CppType()) {
        APPLY_TMP_MACRO_FOR_ALL_CPPTYPES()
        default: RaiseUnknown();
    }
    #undef TMP_MACRO_FOR_CPPTYPE
}

template <typename T>
inline void TMutableField::Set(T value, size_t index) {
    YASSERT(index < Size());
    #define TMP_MACRO_FOR_CPPTYPE(CPPTYPE) case CPPTYPE: TSimpleFieldTraits<CPPTYPE>::Set(*Mut(), Fd, CompatCast<TSelectCppType<T>::Result, CPPTYPE>(value), index); break;
    switch (CppType()) {
        APPLY_TMP_MACRO_FOR_ALL_CPPTYPES()
        default: RaiseUnknown();
    }
    #undef TMP_MACRO_FOR_CPPTYPE
}

template <typename T>
inline void TMutableField::Add(T value) {
    #define TMP_MACRO_FOR_CPPTYPE(CPPTYPE) case CPPTYPE: TSimpleFieldTraits<CPPTYPE>::Add(*Mut(), Fd, CompatCast<TSelectCppType<T>::Result, CPPTYPE>(value)); break;
    switch (CppType()) {
        APPLY_TMP_MACRO_FOR_ALL_CPPTYPES()
        default: RaiseUnknown();
    }
    #undef TMP_MACRO_FOR_CPPTYPE
}



}   // namespace NProtoBuf
