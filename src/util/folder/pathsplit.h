#pragma once

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>

//do not own any data
struct TPathSplit: public yvector<TStringBuf> {
    TStringBuf Drive;
    bool IsAbsolute;

    inline TPathSplit()
        : IsAbsolute(false)
    {
    }

    inline TPathSplit(const TStringBuf& part)
        : IsAbsolute(false)
    {
        ParseFirstPart(part);
    }

    inline TPathSplit& AppendHint(size_t hint) {
        reserve(size() + hint);

        return *this;
    }

    inline TPathSplit& ParseFirstPart(const TStringBuf& part) {
        DoParseFirstPart(part);

        return *this;
    }

    inline TPathSplit& ParsePart(const TStringBuf& part) {
        DoParsePart(part);

        return *this;
    }

    Stroka Reconstruct() const;
    void AppendComponent(const TStringBuf& comp);

    TStringBuf Extension() const;

    template <class It>
    inline TPathSplit& AppendMany(It b, It e) {
        AppendHint(e - b);

        while (b != e) {
            AppendComponent(*b++);
        }

        return *this;
    }

private:
    void DoParseFirstPart(const TStringBuf& part);
    void DoParsePart(const TStringBuf& part);
};

Stroka JoinPaths(const TPathSplit& p1, const TPathSplit& p2);

TStringBuf CutExtension(const TStringBuf& fileName);
