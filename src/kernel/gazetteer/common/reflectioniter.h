#pragma once

#include <library/protobuf/util/traits.h>

namespace NGzt
{


typedef NProtoBuf::Message TMessage;
typedef NProtoBuf::FieldDescriptor TFieldDescriptor;


template <typename TFieldType>
class TProtoFieldIterator {
    typedef NProtoBuf::TFieldTraits<NProtoBuf::TSelectCppType<TFieldType>::Result, true> TRepeated;
    typedef NProtoBuf::TFieldTraits<NProtoBuf::TSelectCppType<TFieldType>::Result, false> TSingle;
public:
    TProtoFieldIterator(const TMessage& article, const TFieldDescriptor* field)
        : Field(field)
        , Article(article)
        , FieldSize(0)
        , CurrentIndex(0)
        , CurrentValue()
        , IsDefault(false)
    {
        if (Field != NULL && Field->cpp_type() == NProtoBuf::TSelectCppType<TFieldType>::Result)
            Init();
        else
            SetInvalid();
    }

    TProtoFieldIterator(const TMessage& article, int fieldNumber)
        : Field(article.GetDescriptor()->FindFieldByNumber(fieldNumber))
        , Article(article)
        , FieldSize(0)
        , CurrentIndex(0)
        , CurrentValue()
        , IsDefault(false)
    {
        if (Field != NULL && Field->cpp_type() == NProtoBuf::TSelectCppType<TFieldType>::Result)
            Init();
        else
            SetInvalid();
    }

    inline void Reset() {
        SetInvalid();
    }

    inline bool Ok() const {
        return CurrentIndex < FieldSize;
    }

    inline void operator++() {
        ++CurrentIndex;
        ResetRepeated();
    }

    inline TFieldType operator* () const {
        return CurrentValue;
    }

    inline bool Valid() const {
        return Field != NULL;
    }

    inline bool Default() const {
        return IsDefault;
    }

protected:
    inline void SetInvalid() {
        Field = NULL;
        FieldSize = 0;
    }

    inline void Init() {
        if (Field->is_repeated()) {
            FieldSize = TRepeated::Size(Article, Field);
            ResetRepeated();
        } else {
            // if the field is empty we should still return a default value
            if (Field->is_optional() && !TSingle::Has(Article, Field))
                IsDefault = true;
            FieldSize = 1;
            CurrentValue = TSingle::Get(Article, Field);
        }
    }

private:

    inline void ResetRepeated() {
        if (Ok())
            CurrentValue = TRepeated::Get(Article, Field, CurrentIndex);
    }

protected:
    const TFieldDescriptor* Field;
private:
    const TMessage& Article;
    size_t FieldSize;
    size_t CurrentIndex;
    TFieldType CurrentValue;
    bool IsDefault;
};



template <typename TpMessage>
inline const TpMessage* Cast(const TMessage *msg, const TpMessage*& val
    , typename TEnableIf<TIsDerived<TMessage, TpMessage>::Result, void>::TResult* = NULL
) {
    val = NULL == msg || TpMessage::descriptor() != msg->GetDescriptor()
        ? NULL : static_cast<const TpMessage*>(msg);
    return val;
}

template <typename TpMessage>
inline const TpMessage* Cast(const TMessage* msg
    , typename TEnableIf<TIsDerived<TMessage, TpMessage>::Result, void>::TResult* = NULL
) {
    const TpMessage* val;
    return Cast(msg, val);
}

// special case (TMessage doesn't have descriptor()
inline const TMessage* Cast(const TMessage* msg, const TMessage*& val) {
    return val = msg;
}


} // namespace NGzt

