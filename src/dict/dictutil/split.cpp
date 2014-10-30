#include "str.h"
#include "char_set.h"
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
//
// Tests
//

template<typename tchar_t>
inline int Test(const TCharSet& set, tchar_t ch) {
    return set.Test(ch);
}

template<typename tchar_t>
inline bool Test(const char* set, tchar_t ch) {
    for (; *set; ++set) {
        if ((wchar16)*set == (wchar16)ch)
            return true;
    }
    return false;
}

template<typename tchar_t>
inline bool Test(wchar16 sep, tchar_t ch) {
    return sep == (wchar16)ch;
}

////////////////////////////////////////////////////////////////////////////////
//
// FindSep
//

template<typename tchar_t, typename TSep>
inline const tchar_t* FindSep(TSep separator, bool plus,
    const tchar_t* s, const tchar_t* end, size_t* len)
{
    while (s != end && !Test(separator, *s))
        ++s;
    const tchar_t* sep = s;
    if (s != end) {
        ++s;
        if (plus) {
            while (s != end && Test(separator, *s))
                ++s;
        }
    }
    *len = s - sep;
    return sep;
}

////////////////////////////////////////////////////////////////////////////////
//
// CharSetSep
//

TCharSetSep::TCharSetSep(const TCharSet& set, char meta)
  : Set(set), Plus(meta == '+')
{}

const char* TCharSetSep::Find(const char* s, const char* end, size_t* len) const {
    return FindSep(Set, Plus, s, end, len);
}

const wchar16* TCharSetSep::Find(const wchar16* s, const wchar16* end, size_t* len) const {
    return FindSep(Set, Plus, s, end, len);
}

////////////////////////////////////////////////////////////////////////////////
//
// CharSep
//

TCharSep::TCharSep(char ch, char meta)
  : Char(ch), Plus(meta == '+')
{}

TCharSep::TCharSep(wchar16 ch, char meta)
  : Char(ch), Plus(meta == '+')
{}

const char* TCharSep::Find(const char* s, const char* end, size_t* len) const {
    const char* sep = std::find(s, end, Char);
    *len = 1;
    if (Plus) {
        const char* p = sep;
        while (p != end && *p == Char)
            ++p;
        *len = p - sep;
    }
    return sep;
}

const wchar16* TCharSep::Find(const wchar16* s, const wchar16* end, size_t* len) const {
    return FindSep(Char, Plus, s, end, len);
}

////////////////////////////////////////////////////////////////////////////////
//
// BySetSep
//

class TBySetSep : public ISep<char>, public ISep<wchar16> {
    DECLARE_NOCOPY(TBySetSep);

// Constructor
public:
    // @param meta '+' for 'one-or-more' separator mode
    TBySetSep(const char* set, char meta = 0);

// ISep methods
public:
    const char* Find(const char* s, const char* end, size_t* len) const;
    const wchar16* Find(const wchar16* s, const wchar16* end, size_t* len) const;

// Fields
private:
    const char* Set;
    bool Plus;
};

TBySetSep::TBySetSep(const char* set, char meta)
  : Set(set), Plus(meta == '+')
{}

const char* TBySetSep::Find(const char* s, const char* end, size_t* len) const {
    return FindSep(Set, Plus, s, end, len);
}

const wchar16* TBySetSep::Find(const wchar16* s, const wchar16* end, size_t* len) const {
    return FindSep(Set, Plus, s, end, len);
}

////////////////////////////////////////////////////////////////////////////////
//
// LineSep
//

class TLineSep : public ISep<char>, public ISep<wchar16> {
    DECLARE_NOCOPY(TLineSep);

// Constructor
public:
    TLineSep() {}

// ISep methods
public:
    const char* Find(const char* s, const char* end, size_t* len) const;
    const wchar16* Find(const wchar16* s, const wchar16* end, size_t* len) const;

// Instance
public:
    static TLineSep Instance;
};

template<typename tchar_t>
const tchar_t* FindCrLf(const tchar_t* s, const tchar_t* end, size_t* len) {
    while (s != end && *s != '\r' && *s != '\n')
        ++s;
    const tchar_t* sep = s;
    if (s != end) {
        ++s;
        if (s != end && s[-1] == '\r' && *s == '\n')
            ++s;
    }
    *len = s - sep;
    return sep;
}

TLineSep TLineSep::Instance;

const char* TLineSep::Find(const char* s, const char* end, size_t* len) const {
    return FindCrLf(s, end, len);
}

const wchar16* TLineSep::Find(const wchar16* s, const wchar16* end, size_t* len) const {
    return FindCrLf(s, end, len);
}

////////////////////////////////////////////////////////////////////////////////
//
// StrSep
//

class TStrSep : public ISep<char>, public ISep<wchar16> {
    DECLARE_NOCOPY(TStrSep);

// Constructor
public:
    TStrSep(const TStringBuf& sep) : Sep(sep) {}

// ISep methods
public:
    const char* Find(const char* s, const char* end, size_t* len) const {
        *len = Sep.length();
        return std::search(s, end, Sep.begin(), Sep.end());
    }

    const wchar16* Find(const wchar16* s, const wchar16* end, size_t* len) const {
        *len = Sep.length();
        return std::search(s, end, Sep.begin(), Sep.end());
    }

// Fields
private:
    const TStringBuf Sep;
};

////////////////////////////////////////////////////////////////////////////////
//
// Split
//

template<typename TStroka>
int SplitT(
    const typename TStroka::char_type* str, size_t len,
    const ISep<typename TStroka::char_type>& separator,
    yvector<TStroka>* result,
    int maxCount, int flags)
{
    typedef typename TStroka::char_type tchar_t;

    assert(maxCount >= 0);
    result->clear();

    const int keepEmpty = !(flags & REMOVE_EMPTY_TOKENS);
    const int keepDelim = (flags & KEEP_DELIMITERS);

    const tchar_t* end = str + len;
    const tchar_t* p = str;
    for (int i = 1; i != maxCount;) {
        size_t cchSep = 0;
        const tchar_t* sep = separator.Find(p, end, &cchSep);
        if (sep == end)
            break;

        if (p != sep || keepEmpty) {
            result->push_back(TStroka(p, sep));
            ++i;
        }

        if (keepDelim)
            result->push_back(TStroka(sep, cchSep));

        p = sep + cchSep;
    }

    if (p != end || keepEmpty)
        result->push_back(TStroka(p, end));

    return (int)result->size();
}

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

VectorStrok
Split(const Stroka& str, const ISep<char>& sep, int maxCount, int flags) {
    VectorStrok result;
    SplitT<Stroka>(~str, str.length(), sep, &result, maxCount, flags);
    return result;
}

VectorWStrok
Split(const WStroka& str, const ISep<wchar16>& sep, int maxCount, int flags) {
    VectorWStrok result;
    SplitT<WStroka>(~str, str.length(), sep, &result, maxCount, flags);
    return result;
}

VectorStrok
Split(const Stroka& str, char sepCh, int maxCount, int flags) {
    VectorStrok result;
    TCharSep sep(sepCh);
    SplitT<Stroka>(~str, str.length(), sep, &result, maxCount, flags);
    return result;
}

VectorWStrok
Split(const WStroka& str, wchar16 sepCh, int maxCount, int flags) {
    VectorWStrok result;
    TCharSep sep(sepCh);
    SplitT<WStroka>(~str, str.length(), sep, &result, maxCount, flags);
    return result;
}

int Split(const TStringBuf& str, const TStringBuf& sep, TStrBufVector* arr) {
    TStrSep strSep(sep);
    return SplitT<TStringBuf>(~str, str.length(), strSep, arr, 0, 0);
}

VectorStrok Split(const Stroka& str, const TStringBuf& sep) {
    VectorStrok result;
    TStrSep strSep(sep);
    SplitT<Stroka>(~str, str.length(), strSep, &result, 0, 0);
    return result;
}

VectorWStrok Split(const WStroka& str, const TStringBuf& sep) {
    VectorWStrok result;
    TStrSep strSep(sep);
    SplitT<WStroka>(~str, str.length(), strSep, &result, 0, 0);
    return result;
}

int Split(const TWStringBuf& str, wchar16 sep, TWStrBufVector* arr) {
    TCharSep strSep(sep);
    return SplitT<TWStringBuf>(~str, str.length(), strSep, arr, 0, 0);
}

static const TCharSet SPACE(" \t");
static const TCharSetSep SEP(SPACE, '+');

VectorStrok Split(const Stroka& str) {
    VectorStrok result;
    SplitT<Stroka>(~str, str.length(), SEP, &result, 0, REMOVE_EMPTY_TOKENS);
    return result;
}

VectorWStrok Split(const WStroka& str) {
    VectorWStrok result;
    SplitT<WStroka>(~str, str.length(), SEP, &result, 0, REMOVE_EMPTY_TOKENS);
    return result;
}

int Split(const TStringBuf& str, TStrBufVector* arr) {
    return SplitT<TStringBuf>(~str, str.length(), SEP, arr, 0, REMOVE_EMPTY_TOKENS);
}

VectorStrok
SplitBySet(const Stroka& str, const char* set, int maxCount, int flags) {
    VectorStrok result;
    TBySetSep sep(set, '+');
    SplitT<Stroka>(~str, str.length(), sep, &result, maxCount, flags);
    return result;
}

VectorWStrok
SplitBySet(const WStroka& str, const char* set, int maxCount, int flags) {
    VectorWStrok result;
    TBySetSep sep(set, '+');
    SplitT<WStroka>(~str, str.length(), sep, &result, maxCount, flags);
    return result;
}

int Split(char* str, const char* sep, char* arr[], int maxCount) {
    assert(sep && *sep);
    assert(maxCount >= 0);
    if (!str || maxCount <= 0)
        return 0;

    int i = 0;
    while (i < maxCount) {
        arr[i++] = str;
        str = strstr(str, sep);
        if (!str)
            break;
        *str = 0;
        str += strlen(sep);
    }

    return i;
}

int Split(const TStringBuf& str, const char* sep, TStringBuf arr[], int maxCount) {
    assert(sep && *sep);
    assert(maxCount >= 0);
    if (maxCount <= 0)
        return 0;

    TStringBuf currStr(str);
    const TStringBuf sepBuf(sep);
    int i = 0;
    for (; i < maxCount; ++i) {
        arr[i] = currStr;
        size_t pos = currStr.find(sepBuf);
        if (pos == TStringBuf::npos) {
            ++i;
            break;
        }
        arr[i].Trunc(pos);
        currStr.Skip(pos + sepBuf.length());
    }
    return i;
}
