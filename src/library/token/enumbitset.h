#pragma once

#include <util/system/yassert.h>
#include <util/generic/yexception.h>
#include <util/generic/bitmap.h>
#include <util/ysaveload.h>
#include <util/string/util.h>

template <typename TEnum, TEnum begin, TEnum end>
class TEnumBitSet: private TBitMap<end - begin> {
public:
    static const TEnum Begin = begin;
    static const TEnum End = end;
    static const size_t BitsetSize = End - Begin;

    typedef TBitMap<BitsetSize> TParent;
    typedef TEnumBitSet<TEnum, begin, end> TThis;

    TEnumBitSet()
        : TParent (0)
    {}

    explicit TEnumBitSet (const TParent& p)
        : TParent (p)
    {}

    void Init(TEnum c) {
        Set(c);
    }

    template<class... R>
    void Init(TEnum c1, TEnum c2, R... r) {
        Set(c1);
        Init(c2, r...);
    }

    explicit TEnumBitSet(TEnum c) {
        Init(c);
    }

    template<class... R>
    TEnumBitSet(TEnum c1, TEnum c2, R... r) {
        Init(c1, c2, r...);
    }

    template <class TIt>
    TEnumBitSet(const TIt& begin_, const TIt& end_) {
        for (TIt p = begin_; p != end_; ++p)
            Set(*p);
    }

    static bool IsValid(TEnum c) {
        return c >= Begin && c < End;
    }

    bool Test(TEnum c) const {
        return TParent::Test(Pos(c));
    }

    TThis& Flip(TEnum c) {
        TParent::Flip(Pos(c));
        return *this;
    }

    TThis& Flip() {
        TParent::Flip();
        return *this;
    }

    TThis& Reset(TEnum c) {
        TParent::Reset(Pos(c));
        return *this;
    }

    TThis& Reset() {
        TParent::Clear();
        return *this;
    }

    TThis& Set(TEnum c) {
        TParent::Set(Pos(c));
        return *this;
    }

    TThis& Set(TEnum c, bool val) {
        if (val)
            Set(c);
        else
            Reset(c);
        return *this;
    }

    bool SafeTest(TEnum c) const {
        if (IsValid(c))
            return Test(c);
        return false;
    }

    TThis& SafeFlip(TEnum c) {
        if (IsValid(c))
            return Flip(c);
        return *this;
    }

    TThis& SafeReset(TEnum c) {
        if (IsValid(c))
            return Reset(c);
        return *this;
    }

    TThis& SafeSet(TEnum c) {
        if (IsValid(c))
            return Set(c);
        return *this;
    }

    TThis& SafeSet(TEnum c, bool val) {
        if (IsValid(c))
            return Set(c, val);
        return *this;
    }

    static TThis SafeConstruct(TEnum c) {
        TThis ret;
        ret.SafeSet(c);
        return ret;
    }

    bool operator < (const TThis& right) const {
        YASSERT(this->GetChunkCount() == right.GetChunkCount());
        for (size_t i = 0; i < this->GetChunkCount(); ++i) {
            if (this->GetChunks()[i] < right.GetChunks()[i])
                return true;
            else if (this->GetChunks()[i] > right.GetChunks()[i])
                return false;
        }
        return false;
    }

    bool operator != (const TThis& right) const {
        return !(TParent::operator == (right));
    }

    bool operator == (const TThis& right) const {
        return TParent::operator == (right);
    }

    TThis& operator &= (const TThis& right) {
        TParent::operator &= (right);
        return *this;
    }

    TThis& operator |= (const TThis& right) {
        TParent::operator |= (right);
        return *this;
    }

    TThis& operator ^= (const TThis& right) {
        TParent::operator ^= (right);
        return *this;
    }

    TThis operator ~ () const {
        TThis r = *this;
        r.Flip();
        return r;
    }

    TThis operator | (const TThis& right) const {
        TThis ret = *this;
        ret |= right;
        return ret;
    }

    TThis operator & (const TThis& right) const {
        TThis ret = *this;
        ret &= right;
        return ret;
    }

    TThis operator ^ (const TThis& right) const {
        TThis ret = *this;
        ret ^= right;
        return ret;
    }

    using TParent::Empty;
    using TParent::Count;

    void Swap(TThis& bitmap) {
        TParent::Swap(bitmap);
    }

    size_t GetHash() const {
        return this->Hash();
    }

    bool HasAny(const TThis& mask) const {
        return TParent::HasAny(mask);
    }

    bool HasAll(const TThis& mask) const {
        return TParent::HasAll(mask);
    }

    TEnum FindFirst() const {
        // finds first set item in bitset (or End if bitset is empty)
        return static_cast<TEnum> (this->FirstNonZeroBit() + Begin);
    }

    TEnum FindNext(TEnum c) const {
        // finds next set item after @c (or End if @c is the last set item)
        if (c < Begin)
            return FindFirst();
        return static_cast<TEnum> (this->NextNonZeroBit(c - Begin) + Begin);
    }

    //serialization to/from stream
    void Save(TOutputStream* buffer) const {
        ::Save(buffer, (ui32)Count());

        for(TEnum bit = FindFirst(); bit != End; bit = FindNext(bit)) {
            ::Save(buffer, (ui32)bit);
        }
    }

    void Load(TInputStream* buffer)    {
        Reset();

        ui32 sz = 0;
        ::Load(buffer, sz);

        for(ui32 t = 0; t < sz; t++) {
            ui32 bit = 0;
            ::Load(buffer, bit);

            Set((TEnum)bit);
        }
    }

    Stroka ToString() const {
        STATIC_ASSERT(sizeof(typename TParent::TChunk) <= sizeof(ui64));
        static const size_t chunkSize = sizeof(typename TParent::TChunk) * 8;
        static const ui64 numDig = chunkSize / 4;
        static const Stroka templ = Sprintf("%%0%lulX", numDig);
        Stroka ret;
        for (size_t pos = 0; pos < BitsetSize; pos += chunkSize) {
            ui64 t = 0;
            this->Export(pos, t);
            ret += Sprintf(~templ, t);
        }

        size_t n = 0;
        while (n + 1 < ret.length() && ret[n] == '0')
            ++n;
        ret.remove(0, n);
        return ret;
    }

    void FromString(const Stroka& s) {
        static const size_t chunkSize = sizeof(typename TParent::TChunk) * 8;
        static const size_t numDig = chunkSize / 4;

        Reset();
        for (size_t prev = s.length(), n = s.length() - numDig, pos = 0; prev; n -= numDig, pos += chunkSize) {
            if (pos >= BitsetSize)
                ythrow yexception() << "too many digits";
            if (n > prev)
                n = 0;
            typename TParent::TChunk t = IntFromString<typename TParent::TChunk, 16, TStringBuf>(s.substr(n, prev - n));
            this->Or(TParent(t), pos);
        }
    }

    bool any() const {      // obsolete
        return !Empty();
    }

    bool none() const {     // obsolete
        return Empty();
    }

    size_t count() const {  // obsolete
        return Count();
    }

private:
    static size_t Pos(TEnum c) {
        if (!IsValid(c))
            ythrow yexception() << "invalid value " << (int)c;
        return static_cast<size_t> (c - Begin);
    }
};

template <typename TEnum, TEnum begin, TEnum end>
class TSfEnumBitSet: public TEnumBitSet<TEnum, begin, end> {
public:
    typedef TEnumBitSet<TEnum, begin, end> TParent;

    TSfEnumBitSet()
        : TParent ()
    {}

    TSfEnumBitSet (const TParent& p)
        : TParent (p)
    {}

    //! unsafe initialization from ui64, value must be shifted according to TParent::Begin
    explicit TSfEnumBitSet(ui64 val)
        : TParent(typename TParent::TParent(val))
    {
        STATIC_ASSERT(TParent::BitsetSize <= 64);
    }

    void Init(TEnum c) {
        this->SafeSet(c);
    }

    template<class... R>
    void Init(TEnum c1, TEnum c2, R... r) {
        this->SafeSet(c1);
        Init(c2, r...);
    }

    TSfEnumBitSet(TEnum c) {
        Init(c);
    }

    template<class... R>
    TSfEnumBitSet(TEnum c1, TEnum c2, R... r) {
        Init(c1, c2, r...);
    }
};
