#pragma once

#include "kmp.h"

#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <cstdio>

template<typename T>
struct TNumPair
{
    T Begin;
    T End;

    TNumPair()
    {
    }

    TNumPair(T begin, T end)
        : Begin(begin)
        , End(end)
    {
        YASSERT(begin <= end);
    }

    T Length() const
    {
        return End - Begin + 1;
    }

    bool operator==(const TNumPair& r) const
    {
        return (Begin == r.Begin) && (End == r.End);
    }

    bool operator!=(const TNumPair& r) const
    {
        return (Begin != r.Begin) || (End != r.End);
    }
};

typedef TNumPair<size_t> TSizeTRegion;
typedef TNumPair<ui32> TUi32Region;

template <>
inline Stroka ToString(const TUi32Region& r) {
    return Sprintf("(%" PRIu32 ", %" PRIu32 ")", r.Begin, r.End);
}

template <>
inline TUi32Region FromString(const Stroka& s) {
    TUi32Region result;
    sscanf(~s, "(%" PRIu32 ", %" PRIu32 ")", &result.Begin, &result.End);
    return result;
}

class TSplitDelimitersBit
{
private:
    ui32 Delims[8];

public:
    TSplitDelimitersBit(const char* s);

    FORCED_INLINE bool IsDelimiter(ui8 ch) const
    {
        return Delims[ch >> 5] & (1 << (ch & 31));
    }
};

class TSplitDelimiters
{
private:
    bool Delims[256];

public:
    TSplitDelimiters(const char* s);

    FORCED_INLINE bool IsDelimiter(ui8 ch) const
    {
        return Delims[ch];
    }
};

template<class Split>
class TSplitIterator;

class TSplitBase
{
protected:
    const char* Str;
    size_t Len;

public:
    TSplitBase(const char* str, size_t length);
    TSplitBase(const Stroka& s);

    FORCED_INLINE const char* GetString() const
    {
        return Str;
    }

    FORCED_INLINE size_t GetLength() const
    {
        return Len;
    }
};

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4512 )
#endif

class TDelimitersSplit : public TSplitBase
{
private:
    const TSplitDelimiters& Delimiters;

public:
    typedef TSplitIterator<TDelimitersSplit> TIterator;
    friend class TSplitIterator<TDelimitersSplit>;

    TDelimitersSplit(const char* str, size_t length, const TSplitDelimiters& delimiters);
    TDelimitersSplit(const Stroka& s, const TSplitDelimiters& delimiters);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

class TDelimitersStrictSplit : public TSplitBase
{
private:
    const TSplitDelimiters& Delimiters;

public:
    typedef TSplitIterator<TDelimitersStrictSplit> TIterator;
    friend class TSplitIterator<TDelimitersStrictSplit>;

    TDelimitersStrictSplit(const char* str, size_t length, const TSplitDelimiters& delimiters);
    TDelimitersStrictSplit(const Stroka& s, const TSplitDelimiters& delimiters);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

class TScreenedDelimitersSplit : public TSplitBase
{
private:
    const TSplitDelimiters& Delimiters;
    const TSplitDelimiters& Screens;

public:
    typedef TSplitIterator<TScreenedDelimitersSplit> TIterator;
    friend class TSplitIterator<TScreenedDelimitersSplit>;

    TScreenedDelimitersSplit(const Stroka& s, const TSplitDelimiters& delimiters, const TSplitDelimiters& screens);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

class TDelimitersSplitWithoutTags : public TSplitBase
{
private:
    const TSplitDelimiters& Delimiters;
    size_t SkipTag(size_t pos) const;
    size_t SkipDelimiters(size_t pos) const;

public:
    typedef TSplitIterator<TDelimitersSplitWithoutTags> TIterator;
    friend class TSplitIterator<TDelimitersSplitWithoutTags>;

    TDelimitersSplitWithoutTags(const char* str, size_t length, const TSplitDelimiters& delimiters);
    TDelimitersSplitWithoutTags(const Stroka& s, const TSplitDelimiters& delimiters);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

class TCharSplit : public TSplitBase
{
public:
    typedef TSplitIterator<TCharSplit> TIterator;
    friend class TSplitIterator<TCharSplit>;

    TCharSplit(const char* str, size_t length);
    TCharSplit(const Stroka& s);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

class TCharSplitWithoutTags : public TSplitBase
{
private:
    size_t SkipTag(size_t pos) const;
    size_t SkipDelimiters(size_t pos) const;

public:
    typedef TSplitIterator<TCharSplitWithoutTags> TIterator;
    friend class TSplitIterator<TCharSplitWithoutTags>;

    TCharSplitWithoutTags(const char* str, size_t length);
    TCharSplitWithoutTags(const Stroka& s);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

class TSubstringSplitDelimiter
{
public:
    TKMPMatcher Matcher;
    size_t Len;

    TSubstringSplitDelimiter(const Stroka& s);
};

class TSubstringSplit : public TSplitBase
{
private:
    const TSubstringSplitDelimiter& Delimiter;

public:
    typedef TSplitIterator<TSubstringSplit> TIterator;
    friend class TSplitIterator<TSubstringSplit>;

    TSubstringSplit(const char* str, size_t length, const TSubstringSplitDelimiter& delimiter);
    TSubstringSplit(const Stroka& str, const TSubstringSplitDelimiter& delimiter);
    TIterator Iterator() const;
    TSizeTRegion Next(size_t& pos) const;
    size_t Begin() const;
};

template<class TSplit>
class TSplitIterator
{
protected:
    const TSplit& Split;
    size_t Pos;
    Stroka* CurrentStroka;

public:
    TSplitIterator(const TSplit& split)
        : Split(split)
        , Pos(Split.Begin())
        , CurrentStroka(NULL)
    {

    }

    virtual ~TSplitIterator() {
        delete CurrentStroka;
    }

    inline TSizeTRegion Next() {
        if (Eof())
            ythrow yexception() << "eof reached";
        return Split.Next(Pos);
    }

    TStringBuf NextTok() {
        if (Eof())
            return TStringBuf();
        TSizeTRegion region = Next();
        return TStringBuf(Split.Str + region.Begin, region.End - region.Begin);
    }

    const Stroka& NextStroka() {
        if (!CurrentStroka)
            CurrentStroka = new Stroka();
        TSizeTRegion region = Next();
        CurrentStroka->assign(Split.Str, region.Begin, region.Length() - 1);
        return *CurrentStroka;
    }

    inline bool Eof() const {
        return Pos >= Split.Len;
    }

    Stroka GetTail() const {
        return Stroka(Split.Str + Pos);
    }

    void Skip(size_t count) {
        for (size_t i = 0; i < count; ++i)
            Next();
    }
};

typedef yvector<Stroka> TSplitTokens;

template<typename TSplit>
void Split(const TSplit& split, TSplitTokens* words)
{
    words->clear();
    TSplitIterator<TSplit> it(split);
    while (!it.Eof())
        words->push_back(it.NextStroka());
}
