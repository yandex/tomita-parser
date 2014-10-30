#include "pathsplit.h"

#include <util/stream/output.h>
#include <util/generic/yexception.h>

static inline bool IsPathSep(char c) {
#ifdef _win_
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

template <class T>
static inline size_t ToReserve(const T& t) {
    size_t ret = t.size() + 5;

    for (typename T::const_iterator it = t.begin(); it != t.end(); ++it) {
        ret += it->size();
    }

    return ret;
}

void TPathSplit::DoParseFirstPart(const TStringBuf& part0) {
    TStringBuf part(part0);

    if (part == STRINGBUF(".")) {
        push_back(STRINGBUF("."));

        return;
    }

#ifdef _win_
    if (part.length() >= 2 && part[1] == ':') {
        Drive = part.SubStr(0, 2);
        part = part.SubStr(2);
    }
#endif

    if (part && IsPathSep(part[0])) {
        IsAbsolute = true;
    }

    ParsePart(part);
}

void TPathSplit::DoParsePart(const TStringBuf& part0) {
    AppendHint(+part0 / 8);

#if defined(_unix_)
    TStringBuf next(part0);
    TStringBuf part;

    while (TStringBuf(next).SplitImpl('/', part, next)) {
        AppendComponent(part);
    }

    AppendComponent(next);
#else
    size_t pos = 0;
    TStringBuf part(part0);

    while (pos < +part) {
        while (pos < +part && IsPathSep(part[pos])) {
            ++pos;
        }

        const char* begin = ~part + pos;

        while (pos < +part && !IsPathSep(part[pos])) {
            ++pos;
        }

        AppendComponent(TStringBuf(begin, ~part + pos));
    }
#endif
}

Stroka TPathSplit::Reconstruct() const {
    Stroka r;

    r.reserve(ToReserve(*this));

    if (IsAbsolute) {
        r.AppendNoAlias(Drive);
        r.AppendNoAlias(STRINGBUF("/"));
    }

    for (const_iterator i = begin(); i != end(); ++i) {
        if (i != begin()) {
            r.AppendNoAlias(STRINGBUF("/"));
        }

        r.AppendNoAlias(*i);
    }

    return r;
}

void TPathSplit::AppendComponent(const TStringBuf& comp) {
    if (!comp || comp == STRINGBUF(".")) {
        ; // ignore
    } else if (comp == STRINGBUF("..") && !empty() && back() != STRINGBUF("..")) {
        pop_back();
    } else {
        // push back first .. also
        push_back(comp);
    }
}

TStringBuf TPathSplit::Extension() const {
    return size() > 0 ? CutExtension(back()) : TStringBuf();
}

template <>
void Out<TPathSplit>(TOutputStream& o, const TPathSplit& ps) {
    o << ps.Reconstruct();
}

Stroka JoinPaths(const TPathSplit& p1, const TPathSplit& p2) {
    if (p2.IsAbsolute) {
        ythrow yexception() << "can not join " << p1 << " and " << p2;
    }

    return TPathSplit(p1).AppendMany(p2.begin(), p2.end()).Reconstruct();
}

TStringBuf CutExtension(const TStringBuf& fileName) {
    if (fileName.Empty()) {
        return fileName;
    }

    TStringBuf name;
    TStringBuf extension;
    fileName.RSplit('.', name, extension);
    if (name.Empty()) {
        // dot at a start or not found
        return name;
    } else {
        return extension;
    }
}
