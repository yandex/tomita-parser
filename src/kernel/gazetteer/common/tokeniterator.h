#pragma once

#include <util/generic/strbuf.h>
#include <util/charset/unidata.h>


namespace NGzt {

// Iterates over non-empty tokens in given string.
// A token can be either alpha characters sequence (e.g. "moscow") or digit sequence (e.g. "2012"), without mixing.
// Both types could have unicode diacritic symbols inside (not in the beginning).

class TTokenIterator {
public:
    TTokenIterator()
        : Alpha(false)
    {
    }

    TTokenIterator(TWtringBuf text)
        : Tail(text)
        , Alpha(true)
    {
        NextToken();
    }

    bool Ok() const {
        return !CurrentToken.empty();
    }

    void operator++() {
        NextToken();
    }

    const TWtringBuf& operator*() const {
        return CurrentToken;
    }

    const TWtringBuf* operator->() const {
        return &CurrentToken;
    }

    bool IsAlpha() const {
        return Alpha;
    }

    bool IsDigits() const {
        return !Alpha;
    }

private:
    void NextToken() {
        size_t start = 0;
        Alpha = true;
        for (; start < Tail.size(); ++start)
            if (::IsAlpha(Tail[start]))
                break;
            else if (::IsNumeric(Tail[start])) {
                Alpha = false;
                break;
            }

        size_t stop = FindTokenEnd(start);
        CurrentToken = Tail.SubStr(start, stop - start);
        Tail.Skip(stop);
    }

    size_t FindTokenEnd(size_t start) const {
        size_t stop = start + 1;
        for (; stop < Tail.size(); ++stop)
            if (::IsAlpha(Tail[stop])) {
                if (!Alpha)
                    break;  // 1st
            } else if (::IsNumeric(Tail[stop])) {
                if (Alpha)
                    break;  // abc123
            } else if (!::IsCombining(Tail[stop]))
                break;      // xxx-yyy
        return stop;
    }

protected:
    TWtringBuf CurrentToken;

private:
    TWtringBuf Tail;
    bool Alpha;
};

// Iterates only sub-tokens, i.e. if given string is a whole single token, an iterator will be empty.
class TSubtokenIterator: public TTokenIterator {
public:
    TSubtokenIterator() {
    }

    TSubtokenIterator(TWtringBuf text)
        : TTokenIterator(text)
    {
        if (TTokenIterator::CurrentToken.size() == text.size())
            TTokenIterator::CurrentToken = TWtringBuf();
    }
};

}   // namespace NGzt
