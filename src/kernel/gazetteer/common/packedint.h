#pragma once

#include <library/comptrie/compact_vector.h>

#include "vector2d.h"



namespace NGzt {


template <typename T>
class TPackedIntOps {
public:
    template <typename TByteCollector>
    static inline void Pack(T item, TByteCollector& dst) {
        while (item > 0x7F) {
            dst.AppendByte((item & 0x7F) | 0x80);
            item >>= 7;
        }
        dst.AppendByte(static_cast<ui8>(item));
    }

    typedef const ui8* TBytePtr;

    static T Unpack(TBytePtr& begin) {
        ui8 byte = *begin++;
        T value = byte & 0x7F;
        for (T shift = 7; byte & 0x80; shift += 7) {
            byte = *begin++;
            value |= (byte & 0x7F) << shift;
        }
        return value;
    }

    template <typename TByteCollector>
    static inline void PackDiff(T item, T first, TByteCollector& dst) {
        if (item >= first)
            Pack((item - first) << 1, dst);
        else
            Pack(((first - item) << 1) | 1, dst);   // sign is stored in least significant bit
    }

    static inline T UnpackDiff(T first, TBytePtr& begin) {
        T diff = Unpack(begin);
        return (diff & 1) ? first - (diff >> 1) : first + (diff >> 1);
    }

    static size_t PackedSize(TBytePtr begin, TBytePtr end) {
        // count number of finalizing bytes
        size_t count = 0;
        for (; begin != end; ++begin)
            if (!(*begin & 0x80))
                ++count;
        return count;
    }
};


template <typename T>
class TPackedIntVector {
    typedef TPackedIntOps<T> TOps;
public:
    // no empty state, always start with single item

    explicit TPackedIntVector(T first = 0)
        : First(first)
    {
    }
/*
    size_t Size() const {
        // not very efficient
        return 1 + TOps::PackedSize(Bytes.begin(), Bytes.end());
    }
*/
    void PushBack(T item) {
        TOps::PackDiff(item, First, *this);
    }


    class TIterator {
    public:
        TIterator()
            : Begin(NULL)
            , End(NULL)
            , First(0)
            , Next(0)
            , Ok_(false)
        {
        }

        TIterator(T first, const ui8* begin, const ui8* end)
            : Begin(begin)
            , End(end)
            , First(first)
            , Next(first)
            , Ok_(true)
        {
        }

        inline bool Ok() const {
            return Ok_;
        }

        inline void operator++() {
            if (Begin != End)
                Next = TOps::UnpackDiff(First, Begin);
            else
                Ok_ = false;
        }

        inline T operator* () const {
            return Next;
        }

    private:
        const ui8 *Begin, *End;
        T First, Next;
        bool Ok_;
    };

    TIterator Iter() const {
        return TIterator(First, Bytes.begin(), Bytes.end());
    }

    // serialize as byte range (compatible with TPackedIntVectorsVector::Load)
    void SaveRange(TOutputStream* output) const {
        ::Save<T>(output, First);
        ::SaveArray<ui8>(output, Bytes.begin(), Bytes.size());
    }

    bool HasSingleItem() const {
        return Bytes.empty();
    }

    T GetFirst() const {
        return First;
    }

    size_t SaveRangeSize() const {
        return sizeof(T) + Bytes.size();
    }

    size_t ByteSize() const {
        size_t ret = sizeof(*this);
        if (!Bytes.empty())
            ret += Bytes.Capacity() * sizeof(ui8) + sizeof(size_t)*2;
        return ret;
    }

private:
    friend class TPackedIntOps<T>;

    inline void AppendByte(ui8 byte) {
        Bytes.PushBack(byte);
    }

private:
    TCompactVector<ui8> Bytes;
    T First;
};

template <typename T>
class TPackedIntVectorsVector: private TVectorsVector<ui8> {
    typedef TVectorsVector<ui8> TBase;
    typedef TPackedIntVector<T> TSubVector;
public:
    using TBase::TIndex;

    typedef typename TSubVector::TIterator TSubVectorIter;

    static TSubVectorIter IterRawRange(const ui8* begin, const ui8* end)  {
        if (begin >= end)
            return TSubVectorIter();

        if (EXPECT_FALSE(begin + sizeof(T) > end))
            ythrow yexception() << "Invalid range supplied for TPackedIntVector::TIterator.";

        return TSubVectorIter(ReadFirst(begin), begin + sizeof(T), end);
    }

    inline TSubVectorIter operator[](size_t index) const {
        TBase::TRange range = TBase::SubVector(index);
        return IterRawRange(range.first, range.second);
    }

    using TBase::Size;

    using TBase::Save;
    using TBase::Load;
    using TBase::SaveToProto;
    using TBase::LoadFromProto;

    using TBase::ByteSize;

private:
    static T ReadFirst(const ui8* data) {
        T ret = 0;
        memcpy(&ret, data, sizeof(T));      // as TMemoryInput does
        return ret;
    }

};


template <typename T>
struct TPackedIntVectorTraits {
    typedef TPackedIntVector<T> TVector;
    typedef typename TPackedIntVector<T>::TIterator TIterator;

    static inline TIterator Iterate(const TVector& v) {
        return v.Iter();
    }

    static inline void PushBack(TVector& v, const T& t) {
        v.PushBack(t);
    }

    // hope RVO works here
    static inline TVector MakeSingularVector(const T& t) {
        return TVector(t);
    }

    static inline void SaveRange(TOutputStream* output, const TVector& v) {
        v.SaveRange(output);
    }

    static inline size_t SaveRangeSize(const TVector& v) {
        return v.SaveRangeSize();
    }
};

template <typename T>
class TPackedIntVectorsVectorBuilder: public TVectorsVectorBuilder<T, TPackedIntVectorTraits<T> > {
/*
    typedef TVectorsVectorBuilder<T, TPackedIntVectorTraits<T> > TBase;
    using TBase::Add;
    using TBase::AddTo;
public:
    typedef typename TBase::TIndex TIndex;

    // a little optimization: check previously added subvector
    TIndex Add(const T& item) {
        TIndex idx = TBase::NextIndex();
        if (EXPECT_TRUE(idx > 1)) {
            TIndex prevIdx = idx - 1;
            const typename TBase::TSubVector& prev = TBase::SubVector(prevIdx);
            if (prev.HasSingleItem() && prev.GetFirst() == item)
                return prevIdx;
        }
        return TBase::Add(item);
    }

    TIndex AddTo(TIndex index, const T& item) {
        if (index == TBase::EmptyIndex)
            return TBase::Add(item);
        const typename TBase::TSubVector& dst = TBase::SubVector(index);
        if (dst.HasSingleItem())
            index = TBase::Add(dst.GetFirst());     // make a copy for appending second item
        return TBase::AddTo(index, item);
    }
*/
};



}   // namespace NGzt


