#pragma once

#include <util/string/cast.h>
#include <util/system/yassert.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/generic/algorithm.h>

#include <stlport/iterator>

class TDelimStrokaIter : public NStl::input_iterator<TStringBuf, int>
{
private:
    bool        IsValid;

    TStringBuf  Str;
    TStringBuf  Current;
    TStringBuf  Delim;

private:
    inline void UpdateCurrent()
    {
        // it is much faster than TStringBuf::find
        size_t pos = NStl::search(Str.begin(), Str.end(), Delim.begin(), Delim.end()) - Str.begin();
        Current = Str.Head(pos);
    }

public:
    TDelimStrokaIter(const char* begin, const char* strEnd, TStringBuf delim)
        : TDelimStrokaIter(TStringBuf(begin, strEnd), delim)
    {}

    TDelimStrokaIter(TStringBuf str, TStringBuf delim)
        : IsValid(true)
        , Str(str)
        , Delim(delim)
    {
        UpdateCurrent();
    }

    TDelimStrokaIter()
        : IsValid(false)
    {}

    // NOTE: this is a potentially unsafe operation (no overrun check)
    inline TDelimStrokaIter& operator++()
    {
        if (Current.end() != Str.end()) {
            Str.Skip(Current.length() + Delim.length());
            UpdateCurrent();
        } else {
            Str.Clear();
            Current.Clear();
            IsValid = false;
        }
        return *this;
    }

    void operator+=(size_t n)
    {
        for (;n > 0; --n) {
            ++(*this);
        }
    }

    bool operator==(const TDelimStrokaIter& rhs) const
    {
        return (IsValid == rhs.IsValid) && (!IsValid || (Current.begin() == rhs.Current.begin()));
    }

    bool operator!=(const TDelimStrokaIter& rhs) const
    {
        return !(*this == rhs);
    }

    TStringBuf operator*() const
    {
        return Current;
    }

    const TStringBuf* operator->() const
    {
        return &Current;
    }

    template<class T>
    bool TryNext(T& t)  // Get & advance
    {
        if (IsValid) {
            t = FromString<T>(Current);
            operator++();
            return true;
        } else {
            return false;
        }
    }

    template<class T>
    TDelimStrokaIter& Next(T& t) // Get & advance
    {
        if (!TryNext(t))
            ythrow yexception() << "No valid field";
        return *this;
    }

    template<class T>
    T GetNext()
    {
        T res;
        Next(res);
        return res;
    }

    char operator[](size_t i) const
    {
        YASSERT(i < Current.length());
        return Current[i];
    }

    const char* GetBegin() const
    {
        return Current.begin();
    }

    const char* GetEnd() const
    {
        return Current.end();
    }

    bool Valid() const
    {
        return IsValid;
    }

    // contents from next token to the end of string
    TStringBuf Cdr() const
    {
        return Str.SubStr(Current.length() + Delim.length());
    }

    TDelimStrokaIter IterEnd() const
    {
        return TDelimStrokaIter();
    }
};

inline TDelimStrokaIter begin_delim(const Stroka& str, TStringBuf delim)
{
    return TDelimStrokaIter(str, delim);
}

inline TDelimStrokaIter begin_delim(TStringBuf str, TStringBuf delim)
{
    return TDelimStrokaIter(str.begin(), str.end(), delim);
}

inline TDelimStrokaIter end_delim(const Stroka& /*str*/, TStringBuf /*delim*/)
{
    return TDelimStrokaIter();
}

