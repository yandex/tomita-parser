#pragma once

#include <util/generic/fwd.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>

#include <util/memory/pool.h>
#include <util/memory/blob.h>



template <typename T>
static inline size_t ByteSize(const T& t);

template <typename T>
static inline size_t ArrayByteSize(const T* t, size_t n);


#define DEBUG_BYTESIZE(x) "\t" << #x << ": " << ::ByteSize(x)/(1024*1024) << " Mb\n"


template <typename T>
struct TByteSizeTypeTraits {
    enum {
        IsPod = TTypeTraits<T>::IsPod
    };
};

#define DECLARE_BYTESIZE_PODTYPE(type) \
    template <>\
    struct TByteSizeTypeTraits<type> {\
        enum {\
            IsPod = true\
        };\
    }

#define DECLARE_BYTESIZE_PODTYPE_METHOD(type) \
    size_t ByteSize() const { return sizeof(type); }



template <class T>
struct TDefaultArrayByteSize {
    static inline size_t ArrayByteSize(const T* t, size_t n) {
        size_t ret = 0;
        for (const T* stop = t + n; t != stop; ++t)
            ret += ByteSize(*t);
        return ret;
    }
};


// POD selector
template <typename T>
static inline size_t PodByteSize(const T&) {
    return sizeof(T);
}

template <typename T>
static inline size_t PodArrayByteSize(const T*, size_t n) {
    return sizeof(T) * n;
}

template <class T, bool isPod>
struct TByteSizePodSelector {
    static inline size_t ByteSize(const T& t) {
        return ::PodByteSize(t);
    }

    static inline size_t ArrayByteSize(const T* t, size_t n) {
        return ::PodArrayByteSize(t, n);
    }
};

template <class T>
struct TByteSizePodSelector<T, false>: public TDefaultArrayByteSize<T> {
    static inline size_t ByteSize(const T& t) {
        return t.ByteSize();
    }
};


// Ptr selector
template <typename T>
static inline size_t PtrByteSize(const T* t) {
    return PodByteSize(t) + (t != NULL ? ByteSize(*t) : 0);
}

template <typename T>
static inline size_t PtrArrayByteSize(const T** t, size_t n) {
    size_t ret = 0;
    const T* e = *t + n;
    for (const T* b = *t; b != e; ++b)
        ret += PtrByteSize(b);
    return ret;
}


template <class T, bool isPtr>
struct TByteSizePtrSelector {
    // T is a pointer
    static inline size_t ByteSize(const T& t) {
        return ::PtrByteSize(t);
    }

    static inline size_t ArrayByteSize(const T* t, size_t n) {
        return ::PtrArrayByteSize(t, n);
    }
};

template <class T>
struct TByteSizePtrSelector<T, false>: public TByteSizePodSelector<T, TByteSizeTypeTraits<T>::IsPod> {
};


// Most generic
template <class T>
struct TByteSize: public TByteSizePtrSelector<T, TTypeTraits<T>::IsPointer> {
};

template <typename T>
static inline size_t ByteSize(const T& t) {
    return TByteSize<T>::ByteSize(t);
}

template <typename T>
static inline size_t ArrayByteSize(const T* t, size_t n) {
    return TByteSize<T>::ArrayByteSize(t, n);
}


template <typename T>
static inline size_t OuterByteSize(const T& t) {
    size_t total = ByteSize(t);
    return total > sizeof(T) ? total - sizeof(T) : 0;
}

template <typename T>
static inline size_t OuterArrayByteSize(const T* t, size_t n) {
    size_t total = ArrayByteSize(t, n);
    size_t inner = PodArrayByteSize(t, n);
    return total > inner ? total - inner : 0;
}





// Range
template <typename It>
static inline size_t IterRangeByteSize(It b, It e) {
    size_t ret = 0;
    while (b != e)
        ret += ByteSize(*b++);
    return ret;
}

template <class It, bool isPtr>
struct TRangeByteSize {
    // b:e is a pointer range, i.e. an array
    static inline size_t ByteSize(It b, It e) {
        return ArrayByteSize(b, e - b);
    }
};

template <class It>
struct TRangeByteSize<It, false> {
    static inline size_t ByteSize(It b, It e) {
        return IterRangeByteSize(b, e);
    }
};


template <class It>
static inline size_t RangeByteSize(It b, It e) {
    return TRangeByteSize<It, TTypeTraits<It>::IsPointer>::ByteSize(b, e);
}




// const char*
template <>
struct TByteSize<const char*>: public TDefaultArrayByteSize<const char*> {
    static inline size_t ByteSize(const char* t) {
        size_t ret = sizeof(const char*);
        if (t != NULL)
            ret += (TCharTraits<char>::GetLength(t) + 1) * sizeof(char);
        return ret;
    }
};



// any collection with begin() and end()
template <class TCollection>
struct TStdCollectionByteSize: public TDefaultArrayByteSize<TCollection> {
    static inline size_t ByteSize(const TCollection& t) {
        // this is approximate size!
        return sizeof(TCollection) + RangeByteSize(t.begin(), t.end());
    }
};

// same + count unused capacity
template <class TCollection, class TValue>
struct TStdReserveCollectionByteSize: public TDefaultArrayByteSize<TCollection> {
    static inline size_t ByteSize(const TCollection& t) {
        // approximate!
        return TStdCollectionByteSize<TCollection>::ByteSize(t)
            + sizeof(TValue) * (t.capacity() - t.size());
    }
};

template <class T, class A>
struct TByteSize< yvector<T, A> >: public TStdReserveCollectionByteSize< yvector<T, A>, T > {
};

template <>
struct TByteSize<Stroka>: public TStdReserveCollectionByteSize<Stroka, char> {
};

template <>
struct TByteSize<Wtroka>: public TStdReserveCollectionByteSize<Wtroka, wchar16> {
};


// stringbufs do not own their buffers, so they should be treated as POD-types
template <class TChr>
struct TByteSize< TStringBufImpl<TChr> >: public TByteSizePodSelector< TStringBufImpl<TChr>, true > {
};




template <class TRbTreeCollection>
struct TRbTreeByteSize: public TDefaultArrayByteSize<TRbTreeCollection> {
    static inline size_t ByteSize(const TRbTreeCollection& t) {
        // approximate!
        return TStdCollectionByteSize<TRbTreeCollection>::ByteSize(t) + (sizeof(bool) + sizeof(void*)*3) * t.size() + sizeof(TRbTreeCollection);
    }
};

template <class K, class C, class A>
struct TByteSize< yset<K, C, A> >: public TRbTreeByteSize< yset<K, C, A> > {
};

template <class K, class V, class C, class A>
struct TByteSize< ymap<K, V, C, A> >: public TRbTreeByteSize< ymap<K, V, C, A> > {
};


template <class THashTable>
struct THashTableByteSize: public TDefaultArrayByteSize<THashTable> {
    static inline size_t ByteSize(const THashTable& t) {
        // very approximate!
        typedef typename THashTable::value_type TValue;
        typedef __yhashtable_node<TValue> TNode;
        STATIC_ASSERT(sizeof(TNode) >= sizeof(TValue));

        const size_t itemOverhead = sizeof(TNode) - sizeof(TValue);
        const size_t bucketsOverhead = t.bucket_count() * sizeof(TNode*);
        const size_t valuesOverhead = t.size() * itemOverhead;
        const size_t valueSize = RangeByteSize(t.begin(), t.end());

        return valueSize + valuesOverhead + bucketsOverhead + sizeof(THashTable);
    }
};


template <class T1, class T2, class T3, class T4, class T5>
struct TByteSize< yhash<T1, T2, T3, T4, T5> >: public THashTableByteSize< yhash<T1, T2, T3, T4, T5> > {
};

template <class T1, class T2, class T3, class T4>
struct TByteSize< yhash_set<T1, T2, T3, T4> >: public THashTableByteSize< yhash_set<T1, T2, T3, T4> > {
};



template <class A, class B>
struct TByteSize< NStl::pair<A, B> >: public TDefaultArrayByteSize< NStl::pair<A, B> > {
    static inline size_t ByteSize(const NStl::pair<A, B>& t) {
        return sizeof(t) + OuterByteSize(t.first) + OuterByteSize(t.second);
    }
};

template <class A, class B>
struct TByteSize< TPair<A, B> >: public TByteSize< NStl::pair<A, B> > {
};




// specific memory holders

static inline size_t ByteSize(const TMemoryPool& pool) {
    return pool.MemoryAllocated() + pool.MemoryWaste();
}

static inline size_t ByteSize(const TBlob& blob) {
    return blob.Size();
}

static inline size_t ByteSize(const TBuffer& buffer) {
    return buffer.Capacity();
}
