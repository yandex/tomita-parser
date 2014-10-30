#pragma once

#include <util/generic/region.h>
#include <util/memory/blob.h>
#include <util/memory/tempbuf.h>
#include <util/generic/pair.h>
#include <util/generic/array.h>
#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/char_buf.h>
#include <util/generic/typetraits.h>
#include <util/generic/typehelpers.h>

#include <string>

template <typename T>
struct TMemoryTraits {
    enum {
        SimpleMemory = TTypeTraits<T>::IsArithmetic,
        ContinuousMemory = SimpleMemory,
        OwnsMemory = SimpleMemory,
    };

    typedef T TElementType;
};

template <typename T, size_t n>
struct TMemoryTraits<T[n]> {
    enum {
        SimpleMemory = TMemoryTraits<T>::SimpleMemory,
        ContinuousMemory = SimpleMemory,
        OwnsMemory = SimpleMemory,
    };

    typedef T TElementType;
};

template <typename T, size_t n>
struct TMemoryTraits<NTr1::array<T, n> > {
    enum {
        SimpleMemory = TMemoryTraits<T>::SimpleMemory,
        ContinuousMemory = SimpleMemory,
        OwnsMemory = SimpleMemory,
    };

    typedef T TElementType;
};

template <typename T, size_t n>
struct TMemoryTraits<yarray<T, n> > {
    enum {
        SimpleMemory = TMemoryTraits<T>::SimpleMemory,
        ContinuousMemory = SimpleMemory,
        OwnsMemory = SimpleMemory,
    };

    typedef T TElementType;
};

template <typename A, typename B>
struct TMemoryTraits<NStl::pair<A, B> > {
    enum {
        SimpleMemory = TMemoryTraits<A>::SimpleMemory && TMemoryTraits<B>::SimpleMemory,
        ContinuousMemory = SimpleMemory,
        OwnsMemory = SimpleMemory,
    };

    typedef NStl::pair<A, B> TElementType;
};

template <typename A, typename B>
struct TMemoryTraits<TPair<A, B> > {
    enum {
        SimpleMemory = TMemoryTraits<A>::SimpleMemory && TMemoryTraits<B>::SimpleMemory,
        ContinuousMemory = SimpleMemory,
        OwnsMemory = SimpleMemory,
    };

    typedef TPair<A, B> TElementType;
};

template <>
struct TMemoryTraits<TBuffer> {
    enum {
        SimpleMemory = false,
        ContinuousMemory = true,
        OwnsMemory = true,
    };

    typedef char TElementType;
};

template <>
struct TMemoryTraits<TTempBuf> {
    enum {
        SimpleMemory = false,
        ContinuousMemory = true,
        OwnsMemory = true,
    };

    typedef char TElementType;
};

template <>
struct TMemoryTraits< ::TBlob> {
    enum {
        SimpleMemory = false,
        ContinuousMemory = true,
        OwnsMemory = true,
    };

    typedef char TElementType;
};

template <int N>
struct TMemoryTraits<TCharBuf<N> > {
    enum {
        SimpleMemory = false,
        ContinuousMemory = true,
        OwnsMemory = true,
    };

    typedef char TElementType;
};

template <typename T>
struct TElementDependentMemoryTraits {
    enum {
        SimpleMemory = false,
        ContinuousMemory = TMemoryTraits<T>::SimpleMemory,
    };

    typedef T TElementType;
};

template <typename T, typename TAlloc>
struct TMemoryTraits<NStl::vector<T, TAlloc> > : public TElementDependentMemoryTraits<T> {
    enum {
        OwnsMemory = TMemoryTraits<T>::OwnsMemory
    };
};

template <typename T, typename TAlloc>
struct TMemoryTraits<yvector<T, TAlloc> > : public TMemoryTraits<NStl::vector<T, TAlloc> > {
};

template <typename T>
struct TMemoryTraits<TTempArray<T> >  : public TElementDependentMemoryTraits<T> {
    enum {
        OwnsMemory = TMemoryTraits<T>::OwnsMemory
    };
};

template <typename T, typename TCharTraits, typename TAlloc>
struct TMemoryTraits<NStl::basic_string<T, TCharTraits, TAlloc> > : public TElementDependentMemoryTraits<T> {
    enum {
        OwnsMemory = TMemoryTraits<T>::OwnsMemory
    };
};

template <>
struct TMemoryTraits<Stroka> : public TElementDependentMemoryTraits<char> {
    enum {
        OwnsMemory = true
    };
};

template <>
struct TMemoryTraits<stroka> : public TMemoryTraits<Stroka> {
};

template <>
struct TMemoryTraits<Wtroka> : public TElementDependentMemoryTraits<TChar> {
    enum {
        OwnsMemory = true
    };
};

template <typename T>
struct TMemoryTraits<TRegion<T> > : public TElementDependentMemoryTraits<T> {
    enum {
        OwnsMemory = false
    };
};

template <typename TCharType, typename TCharTraits>
struct TMemoryTraits<TFixedString<TCharType, TCharTraits> > : public TElementDependentMemoryTraits<TCharType> {
    enum {
        OwnsMemory = false
    };
};

template <>
struct TMemoryTraits<TStringBuf> : public TElementDependentMemoryTraits<char> {
    enum {
        OwnsMemory = false
    };
};

template <>
struct TMemoryTraits<TWtringBuf> : public TElementDependentMemoryTraits<TChar> {
    enum {
        OwnsMemory = false
    };
};
