#pragma once

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/descriptor.h>

#include "common/reflectioniter.h"
#include "common/secondhand.h"
#include "protopool.h"


// forward declarations from base.proto
class TSearchKey;
class TRef;


namespace NGzt
{

// Defined in this file:
class TArticlePtr;
class TCustomKeyIterator;
class TRefIterator;


// Forward declarations
class TArticlePool;


typedef NProtoBuf::Message TMessage;
typedef NProtoBuf::Descriptor TDescriptor;
typedef NProtoBuf::FieldDescriptor TFieldDescriptor;

template <typename T, typename D = TDelete>
class TLazyInitPtr: public TPointerBase<TLazyInitPtr<T, D>, T> {
    struct TData: public TRefCounted<TData, TAtomicCounter>, public THolder<T, D> {
        TData(T* t)
            : TRefCounted<TData, TAtomicCounter>(1)
            , THolder<T, D>(t)
        {
        }
    };
public:
    // non-initialized, InitOnce() should be called before Get().
    TLazyInitPtr()
        : Data(NULL)
    {
    }

    // pre-initialized
    TLazyInitPtr(T* t)
        : Data(new TData(t))
    {
    }

    ~TLazyInitPtr() {
        if (Data)
            Data->UnRef();
    }

    TLazyInitPtr(const TLazyInitPtr& p)
        : Data(p.AtomicData())
    {
        if (Data)
            Data->Ref();
    }

    TLazyInitPtr& operator=(TLazyInitPtr p) {
        Swap(p);
        return *this;
    }

    bool Inited() const {
        return AtomicData();
    }

    // Works only once (takes ownership on @t in any case)
    void InitOnce(T* t) const {
        THolder<T> h(t);
        if (!Inited()) {
            THolder<TData> d(new TData(h.Release()));
            if (AtomicCas(&Data, d.Get(), (TData*)NULL))
                d.Release();
        }
    }

    T* Get() const {
        TData* d = AtomicData();
        return d ? d->Get() : NULL;
    }

    T* GetInited() const {
        YASSERT(Inited());
        return Data->Get();
    }

    void Swap(TLazyInitPtr& p) {
        TData* tmp = Data;
        Data = p.Data;
        p.Data = tmp;
    }

private:
    TData* AtomicData() const {
        return reinterpret_cast<TData*>(::AtomicGet(*AsAtomicPtr(&Data)));
    }

    mutable TData* volatile Data;
};


// Holder for NProtoBuf::Message* (pointer to protobuf article)
// + article title and several helper methods
// + lazy deserialization of protobuf
class TArticlePtr
{
    friend class TCustomKeyIterator;

public:
    // empty pointer
    inline TArticlePtr()
        : ArticlePool(NULL)
        , Offset(Max<ui32>())
        , Descriptor(NULL)
        , Body(NULL)    // initialized with NULL
    {
    }

    TArticlePtr(ui32 offset, const TArticlePool& pool)
        : ArticlePool(&pool)
        , Offset(offset)
        , Descriptor(NULL)
    {
        DeserializeHead();
    }

    TArticlePtr(ui32 offset, const TArticlePtr& article)
        : ArticlePool(article.ArticlePool)
        , Offset(offset)
        , Descriptor(NULL)
    {
        if (ArticlePool.Get())
            DeserializeHead();
    }

    // Special constructor for external articles not from article pool.
    // The caller should supply unique @id for it, which is not valid article pool offset.
    static TArticlePtr MakeExternalArticle(ui32 id, const TMessage& object, const Wtroka& title = Wtroka());

    inline const TMessage* Get() const {
        if (!Body.Inited())
            DeserializeBody();  // lazy deserialization
        return Body.GetInited();
    }

#ifdef GZT_DEBUG
    inline bool Deserialized() const {
        return Body.Inited();
    }
#endif

    // Returned value can be safely used as unique ID (within same gazetteer)
    inline ui32 GetId() const {
        return Offset;
    }

    inline const Wtroka& GetTitle() const {
        return Title;
    }

    inline const TDescriptor* GetType() const {
        return Descriptor;
    }

    // Sometimes it is useful to have a descriptor type name as wide string
    // To avoid re-encoding CharToWide(GetType()->name()), use this method
    Wtroka GetTypeName() const;

    // Same as Get(), but expects the found article to have specified @TGeneratedProtoType.
    // This method can only be used for compiled-in gzt-types, known at compile time.
    template <class TGeneratedProtoType>
    inline const TGeneratedProtoType* As() const {
        return Cast<TGeneratedProtoType>(GetIf<TGeneratedProtoType>());
    }
    template <class TGeneratedProtoType>
    inline const TGeneratedProtoType* Get(const TGeneratedProtoType*& val) const {
        return Cast(GetIf<TGeneratedProtoType>(), val);
    }

    // Binary compatible conversion
    template <class TGeneratedProtoType>
    inline bool ConvertTo(TGeneratedProtoType& res) const {
        TBlob b = GetBinary();
        return res.ParseFromArray(b.Data(), b.Size());
    }

    // Returns serialized article binary data, which could be later de-serialized using standard Protocol Buffers procedures.
    // This blob points directly to corresponding block from mapped gzt-binary in memory,
    // but does not own this data, so it is valid only during gazetteer life-time.
    // You should not modify returned blob content.
    TBlob GetBinary() const;


    // pointer-like operators
    inline const TMessage* operator->() const {
        return Get();
    }

    inline const TMessage& operator*() const {
        YASSERT(!IsNull());
        return *Get();
    }

    inline bool IsNull() const {
        return Descriptor == NULL;
    }

    inline bool operator! () const {
         return IsNull();
    }

    const TArticlePool* GetArticlePool() const {
        // NULL for artificial (non-pool) articles
        return ArticlePool.Get();
    }

    inline bool IsFromPool() const {
        return GetArticlePool() != NULL;
    }

    inline bool IsArtificial() const {
        return !IsFromPool();
    }


    inline bool operator == (const TArticlePtr& article) const {
        return GetId() == article.GetId();
    }

    inline bool operator < (const TArticlePtr& article) const {
        return GetId() < article.GetId();
    }

    inline size_t hash() const {
        return GetId();
    }

    // Returns true if @this article's type is equal or derived from specified @type
    bool IsInstance(const TDescriptor* type) const;

    // same for statically known (builtin) proto-types
    template <typename TGeneratedProtoType>
    bool IsInstance() const {
        return IsInstance(TGeneratedProtoType::descriptor());
    }



    // Field value getters ---------------------------------------------


    // TFieldRef in templates below could be:
    //     const TFieldDescriptor&      - ready descriptor (returns itself)
    //     const TFieldDescriptor*
    //
    //     size_t                       - number of field
    //
    //     const Stroka&                - name of field (this method of referencing field is much slower!)
    //     const char*
    //     const TStringBuf&


    // Returns field descriptor corresponding to @field or NULL if there is no such field in Article.
    inline const TFieldDescriptor* FindField(const TFieldDescriptor& field) const;
    inline const TFieldDescriptor* FindField(const TFieldDescriptor* field) const;
    inline const TFieldDescriptor* FindField(size_t field) const;
    inline const TFieldDescriptor* FindField(const Stroka& field) const;
    inline const TFieldDescriptor* FindField(const TStringBuf& field) const;



    // Return generic iterator over values of field specified by field-descriptor
    // or field number (e.g. for " int repeated F = 3; " number of field F is 3), or field name
    // Iteration will fail if the field does not have specified type T
    // or there is no field with such number or name
    template <typename T, typename TFieldRef>
    inline TProtoFieldIterator<T> IterField(TFieldRef field) const {
        return TProtoFieldIterator<T>(*Get(), FindField(field));
    }


    // Puts into @result first value corresponding to @field
    // If there is no corresponding values for the field, returns false.
    template <typename T, typename TFieldRef>
    inline bool GetField(TFieldRef field, T& result) const;

    // This will fail if there is no value for specified field
    template <typename T, typename TFieldRef>
    inline T GetField(TFieldRef field) const;

    // Iterator over articles referred by @field
    template <typename TFieldRef>
    inline TRefIterator IterRefs(TFieldRef field) const;


    // Suitable for retrieving any numeric type value (including bool and enums)
    // Return false, if there is not such field in article, or the field is not of numeric type
    template <typename TNumeric, typename TFieldRef>
    bool GetNumericField(TFieldRef field, TNumeric& result) const;



    // Some miscellaneous stuff ---------------------------------------------


    // Helper methods for loading articles without explicit ArticlePool usage
    // Return empty article-ptr if no such article is found in the pool
    TArticlePtr LoadByOffset(ui32 offset) const;
    TArticlePtr LoadByRef(const TRef& ref) const;
    TArticlePtr LoadByTitle(const TWtringBuf& title) const;

    // True if the article has at least one key with non-custom type (i.e. not CUSTOM)
    bool HasNonCustomKey() const;

private:
    void DeserializeHead();
    void DeserializeBody() const;
    void NoValueError(const TFieldDescriptor*) const;

    bool GetNumericFieldAsDouble(const TFieldDescriptor* field, double& result) const;

    template <typename TGeneratedProtoType>
    inline const TMessage* GetIf() const {
        // laziest getter: first check if descriptor is ok.
        return IsInstance<TGeneratedProtoType>() ? Get() : NULL;
    }

private:
    // ArticlPool should outlive this TArticlePtr
    TIntrusivePtr<const TArticlePool> ArticlePool;

    ui32 Offset;

    const TDescriptor* Descriptor;
    Wtroka Title;

    //typedef TRecyclePool<TMessage, TMessage>::TItemPtr TMessagePtr;
    //typedef TSharedPtr<TMessage, TAtomicCounter> TMessagePtr;

    typedef TLazyInitPtr<TMessage> TMessagePtr;
    TMessagePtr Body;   // deserialized lazily
};



class TRefIterator {
public:
    template <typename TFieldRef>
    TRefIterator(const TArticlePtr& article, TFieldRef fieldRef)
        : RefIter(*article, article.FindField(fieldRef))
        , SourceArticle(article)
        , Offset(0)
    {
        ResetCurrent();
    }

    inline bool Ok() const {
        return RefIter.Ok();
    }

    inline void operator++() {
        ++RefIter;
        ResetCurrent();
    }

    inline ui32 operator* () const {
        return GetId();
    }

    TArticlePtr LoadArticle() const {
        YASSERT(Ok());
        return TArticlePtr(Offset, SourceArticle);
    }

    inline ui32 GetId() const {
        return Offset;
    }

private:
    void ResetCurrent();

    TProtoFieldIterator<const TMessage*> RefIter;
    const TArticlePtr& SourceArticle;
    ui32 Offset;
};


class TCustomKeyIterator
{
public:
    TCustomKeyIterator(const TArticlePtr& article, const TStringBuf& prefix);
    TCustomKeyIterator(const TMessage* article, const TProtoPool& descriptors, const Stroka& prefix);

    ~TCustomKeyIterator();

    inline bool Ok() const {
        return Ok_;
    }

    inline void operator++() {
        Ok_ = FindNext();
    }

    inline const Stroka& operator*() const {
        return KeyTextWithoutPrefix;
    }

private:
    bool FindNext();

    TStringBuf Prefix;
    THolder<TSearchKey> TmpKey;
    TSearchKeyIterator It;
    size_t KeyTextIndex;
    Stroka KeyTextWithoutPrefix;
    bool Ok_;
};



// inlined and template definitions

const TFieldDescriptor* TArticlePtr::FindField(const TFieldDescriptor& field) const {
    return &field;
}

const TFieldDescriptor* TArticlePtr::FindField(const TFieldDescriptor* field) const {
    return field;
}

const TFieldDescriptor* TArticlePtr::FindField(size_t field) const {
    return GetType()->FindFieldByNumber(field);
}

const TFieldDescriptor* TArticlePtr::FindField(const Stroka& field) const {
    return GetType()->FindFieldByName(field);
}

const TFieldDescriptor* TArticlePtr::FindField(const TStringBuf& field) const {
    return FindField(::ToString(field));
}


template <typename T, typename TFieldRef>
inline bool TArticlePtr::GetField(TFieldRef field, T& result) const {
    TProtoFieldIterator<T> iter = IterField<T, TFieldRef>(field);
    if (!iter.Ok())
        return false;
    result = *iter;
    return true;
}

template <typename T, typename TFieldRef>
inline T TArticlePtr::GetField(TFieldRef field) const {
    T res = T();
    if (!GetField<T, TFieldRef>(field, res))
        NoValueError(FindField(field));
    return res;
}

template <typename TFieldRef>
inline TRefIterator TArticlePtr::IterRefs(TFieldRef field) const {
    YASSERT(!IsNull());
    return TRefIterator(*this, field);
}

template <typename TNumeric, typename TFieldRef>
bool TArticlePtr::GetNumericField(TFieldRef field, TNumeric& result) const {
    const TFieldDescriptor* descr = FindField(field);
    double tmp = 0;
    if (descr == NULL || !GetNumericFieldAsDouble(descr, tmp))
        return false;
    result = static_cast<TNumeric>(tmp);
    return true;
}



} // namespace NGzt

