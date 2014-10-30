#include "str.h"
#include "char_set.h"
#include "dictutil.h"
#include <time.h>
#include <util/string/cast.h>
#include <util/string/escape.h>
#include <util/string/strip.h>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
//
// WStrokaFor
//

WStroka WStrokaFor(const char* str, size_t len) {
    if (len == (size_t)-1)
        len = strlen(str);
    WStroka result(len, 0);
    WStroka::iterator it = result.begin();
    const char* end = str + len;
    while (str != end) {
        char ch = *str++;
        *it++ = ch < 0 ? '?' : ch;
    }
    return result;
}

WStroka WStrokaFor(TStringBuf str) {
    return WStrokaFor(str.c_str(), str.length());
}

////////////////////////////////////////////////////////////////////////////////
//
// Length
//

inline size_t Length(const char* str) {
    return strlen(str);
}

inline size_t Length(const wchar16* str) {
    return wcslen(str);
}

template<typename TStroka>
size_t Length(const yvector<TStroka>& text) {
    size_t len = 0;
    for (typename yvector<TStroka>::const_iterator it = text.begin(); it != text.end(); ++it)
        len += it->length();
    return len;
}

template size_t Length<Stroka>(const VectorStrok& text);
template size_t Length<WStroka>(const VectorWStrok& text);

////////////////////////////////////////////////////////////////////////////////
//
// i2a, i2w, w2i, hex2i
//

char* i2a(int value, char* str) {
    static const char all[] = "9876543210123456789";
    static const char* digits = all + 9;

    if (!value) {
        str[0] = '0';
        str[1] = 0;
        return str;
    }

    char* p = str;
    for (int v = value; v; v /= 10)
        *p++ = digits[v % 10];
    if (value < 0)
        *p++ = '-';
    *p-- = 0;

    // reverse
    for (char* s = str; s < p; ++s, --p) {
        char t = *s; *s = *p; *p = t;
    }

    return str;
}

Stroka i2a(int i) {
    char str[32] = "";
    i2a(i, str);
    return str;
}

WStroka i2w(int i) {
    char str[32] = "";
    i2a(i, str);
    return WStrokaFor(str);
}

int w2i(const TWStringBuf& wstr) {
    char str[32] = "";
    size_t len = wstr.length();
    if (len >= ARRAY_SIZE(str))
        len = ARRAY_SIZE(str) - 1;

    for (size_t i = 0; i < len; ++i) {
        if (wstr[i] > 0x7f)
            return 0;
        str[i] = (char)wstr[i];
    }
    str[len] = 0;

    return atoi(str);
}

Stroka d2a(double value) {
    char str[64] = "";
    ToStringImpl<double>(value, str, ARRAY_SIZE(str));
    return str;
}

WStroka d2w(double value) {
    char str[64] = "";
    ToStringImpl<double>(value, str, ARRAY_SIZE(str));
    return WStrokaFor(str);
}

double a2d(const TStringBuf& str) {
    char buf[64] = "";
    TStringBuf sBuf = StripString(str);
    if (sBuf.empty() || sBuf.length() >= ARRAY_SIZE(buf))
        return 0;
    memcpy(buf, sBuf.Data(), sBuf.length());
    buf[sBuf.length()] = 0;
    return strtod(buf, 0);
}

i8 mapHex2i[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

template<typename tchar_t>
tchar_t* i2hex(int value, tchar_t* str, int len) {
    str[len] = 0;
    for (; len > 0; value >>= 4)
        str[--len] = "0123456789abcdef"[value & 0x0f];
    return str;
}

template char* i2hex<char>(int value, char* str, int len);
template wchar16* i2hex<wchar16>(int value, wchar16* str, int len);

char* i2Hex(int value, char* str, int len) {
    str[len] = 0;
    for (; len > 0; value >>= 4)
        str[--len] = "0123456789ABCDEF"[value & 0x0f];
    return str;
}

////////////////////////////////////////////////////////////////////////////////
//
// Join
//

template <typename TStroka>
typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString
Join(TStringBuf sep, const TStroka* text, int index, int count) {
    typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString str;

    assert(index >= 0 && count >= 0);
    if (!count)
        return str;

    size_t cch = 0;
    for (int i = 0; i < count; ++i)
        cch += text[index + i].length();
    cch += (count - 1) * sep.length();

    str.reserve(cch);
    for (int i = 0; i < count; ++i) {
        if (i)
            Append(str, ~sep, sep.length());
        str.append(text[index + i]);
    }

    return str;
}

template <typename TStroka>
typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString
Join(TStringBuf sep, const TBufPtr<const TStroka>& buf) {
    return Join(sep, buf.data(), 0, buf.size());
}

template <typename TStroka>
typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString
Join(TStringBuf sep, const yvector<TStroka>& text, int index, int count) {
    assert(index >= 0);
    if (index > (int)text.size())
        index = (int)text.size();
    if (count < 0)
        count = (int)text.size() - index;
    count = Min(index + count, (int)text.size()) - index;
    return Join(sep, text.data(), index, count);
}

template Stroka Join<Stroka>(TStringBuf sep, const VectorStrok& text, int index, int count);
template WStroka Join<WStroka>(TStringBuf sep, const VectorWStrok& text, int index, int count);
template WStroka
    Join<TWStringBuf>(TStringBuf sep, const TWStrBufVector& text, int index, int count);

////////////////////////////////////////////////////////////////////////////////
//
// Append
//

template<typename TStroka>
TStroka& Append(TStroka& str, const char* add, size_t cch) {
    if (cch == (size_t)-1)
        cch = strlen(add);
    str.resize(str.length() + cch);
    typename TStroka::iterator it = str.begin() + str.length() - cch;
    for (; cch; --cch)
        *it++ = (ui8)*add++;
    return str;
}

template Stroka& Append<Stroka>(Stroka& str, const char* add, size_t len);
template WStroka& Append<WStroka>(WStroka& str, const char* add, size_t len);

WStroka& Append(WStroka& str, const Stroka& add) {
    return Append(str, add.c_str(), add.length());
}

////////////////////////////////////////////////////////////////////////////////
//
// Find
//

template<typename TStroka, typename tchar_t>
inline size_t FindT(const TStroka& str, const tchar_t* what, size_t pos) {
    return str.find(what, pos);
}

template<>
inline size_t FindT(const WStroka& str, const char* what, size_t pos) {
    if (*what == 0)
        return pos;

    const wchar16* begin = str.c_str();
    const wchar16* end = begin + str.length();
    const wchar16* p = begin + pos;

    for (; p < end; ++p) {
        if (*p != (ui8)*what)
            continue;
        const ui8* w = (const ui8*)what;
        const wchar16* t = p;
        for (++w, ++t; t < end && *w != 0; ++w, ++t) {
            if (*w != *t)
                break;
        }
        if (*w == 0)
            break;
    }
    if (p >= end)
        return WStroka::npos;
    return p - begin;
}

size_t Find(const WStroka& str, const char* what, size_t pos) {
    return FindT(str, what, pos);
}

////////////////////////////////////////////////////////////////////////////////
//
// FindAnyOf
//

template<typename tchar_t>
inline int TestCharT(const TCharSet& set, tchar_t ch) {
    return set.Test(ch);
}

template<typename TSet, typename tchar_t>
inline bool TestCharT(const TSet* set, const tchar_t ch) {
    for (; *set; ++set) {
        if (ch == *set)
            return true;
    }
    return false;
}

template<typename TStroka, typename TSet>
size_t FindAnyOfT(const TStroka& str, TSet set, size_t pos) {
    for (typename TStroka::const_iterator p = str.begin() + pos; p != str.end(); ++p, ++pos) {
        if (TestCharT(set, *p))
            return pos;
    }
    return (size_t)TStroka::npos;
}

size_t FindAnyOf(const WStroka& str, const char* set, size_t pos) {
    return FindAnyOfT(str, (const ui8*)set, pos);
}

size_t FindAnyOf(const Stroka& str, const char* set, size_t pos) {
    return FindAnyOfT(str, set, pos);
}

size_t FindAnyOf(const Stroka& str, const TCharSet& set, size_t pos) {
    return FindAnyOfT(str, set, pos);
}

size_t FindAnyOf(const WStroka& str, const TCharSet& set, size_t pos) {
    return FindAnyOfT(str, set, pos);
}

////////////////////////////////////////////////////////////////////////////////
//
// FindLastOf
//

size_t FindLastOf(const WStroka& str, const TCharSet& set) {
    size_t pos = str.length();
    for (WStroka::const_iterator it = str.end(); it != str.begin();) {
        --it; --pos;
        if (set.Test(*it))
            return pos;
    }
    return WStroka::npos;
}

////////////////////////////////////////////////////////////////////////////////
//
// Replace
//

template<typename TStroka, typename tchar_t>
inline void ReplaceT(
    TStroka& str, size_t off, size_t oldLen, const tchar_t* newStr, size_t)
{
    str.replace(off, oldLen, newStr);
}

template<>
inline void ReplaceT(
    WStroka& str, size_t off, size_t oldLen, const char* newStr, size_t newLen)
{
    str.replace(off, oldLen, newLen, 0);
    WStroka::iterator it = str.begin() + off;
    const char* end = newStr + newLen;
    for (const char* p = newStr; p < end; ++p)
        *it++ = (ui8)*p;
}

////////////////////////////////////////////////////////////////////////////////
//
// RemoveAll
//

template<typename TStroka, typename TChr>
void RemoveAllT(TStroka& str, const TChr* remove, size_t startPos) {
    assert(startPos <= str.length());
    size_t i = startPos;
    size_t removeLen = Length(remove);
    while ((i = FindT(str, remove, i)) != Stroka::npos)
        str.erase(i, removeLen);
}

void RemoveAll(Stroka& str, const char* remove, size_t startPos) {
    RemoveAllT(str, remove, startPos);
}

void RemoveAll(WStroka& str, const wchar16* remove, size_t startPos) {
    RemoveAllT(str, remove, startPos);
}

void RemoveAll(WStroka& str, const char* remove, size_t startPos) {
    RemoveAllT(str, remove, startPos);
}

////////////////////////////////////////////////////////////////////////////////
//
// RemoveAnyOf
//

template<typename TStr, typename TChr>
void RemoveAnyOfT(TStr& str, const TChr* charsToRemove) {
    size_t i = 0;
    while ((i = FindAnyOfT(str, charsToRemove, i)) != Stroka::npos)
        str.erase(i, 1);
}

void RemoveAnyOf(Stroka& str, const char* remove) {
    RemoveAnyOfT<Stroka, char>(str, remove);
}

void RemoveAnyOf(WStroka& str, const wchar16* remove) {
    RemoveAnyOfT<WStroka, wchar16>(str, remove);
}

////////////////////////////////////////////////////////////////////////////////
//
// ReplaceAll
//

template<typename TStroka, typename TChar, typename TChar2>
void ReplaceAll(TStroka& str, const TChar* oldStr, const TChar2* newStr) {
    if (*oldStr == 0)
        return;

    size_t oldLen = Length(oldStr);
    size_t newLen = Length(newStr);
    for (size_t off = 0; (off = FindT(str, oldStr, off)) != TStroka::npos; off += newLen)
        ReplaceT(str, off, oldLen, newStr, newLen);
}

template void ReplaceAll<Stroka, char, char>(Stroka& s, const char* old, const char* newS);
template void ReplaceAll<WStroka, char, char>(WStroka& s, const char* old, const char* newS);
template void ReplaceAll<WStroka, wchar16, wchar16>(WStroka& s, const wchar16*, const wchar16*);

template<typename TStroka, typename TChar, typename TStroka2>
void ReplaceAll(TStroka& str, const TChar* oldStr, const TStroka2& newStr) {
    if (*oldStr == 0)
        return;

    size_t oldLen = Length(oldStr);
    size_t newLen = newStr.length();
    for (size_t off = 0; (off = FindT(str, oldStr, off)) != WStroka::npos; off += newLen)
        ReplaceT(str, off, oldLen, newStr.c_str(), newLen);
}

template void ReplaceAll<Stroka, char, Stroka>(Stroka& s, const char* old, const Stroka& newS);
template void ReplaceAll<WStroka, char, Stroka>(WStroka& s, const char* old, const Stroka& newS);
template void ReplaceAll<WStroka, char, WStroka>(WStroka& s, const char* old, const WStroka& newS);

template<typename TStroka, typename TChar>
void ReplaceAllT(TStroka& str, TChar oldChr, TChar newChr) {
    for (size_t off = 0; (off = str.find(oldChr, off)) != TStroka::npos; ++off)
        str.replace(off, 1, 1, newChr);
}

void ReplaceAll(Stroka& str, char oldChr, char newChr) {
    ReplaceAllT(str, oldChr, newChr);
}

void ReplaceAll(WStroka& str, wchar16 oldChr, wchar16 newChr) {
    ReplaceAllT(str, oldChr, newChr);
}

////////////////////////////////////////////////////////////////////////////////
//
// ReplaceAnyOf
//

template<typename TStr, typename TChr>
void ReplaceAnyOfT(TStr& str, const TChr* oldChars, TChr newChar) {
    for (size_t off = 0; (off = FindAnyOfT(str, oldChars, off)) != TStr::npos; ++off)
        str.replace(off, 1, 1, newChar);
}

void ReplaceAnyOf(Stroka& str, const char* oldChars, char newChar) {
    ReplaceAnyOfT(str, oldChars, newChar);
}

void ReplaceAnyOf(WStroka& str, const wchar16* oldChars, wchar16 newChar) {
    ReplaceAnyOfT(str, oldChars, newChar);
}

void ReplaceAnyOf(WStroka& str, const char* oldChars, char newChar) {
    ReplaceAnyOfT(str, oldChars, newChar);
}

////////////////////////////////////////////////////////////////////////////////
//
// RemoveCrLf
//

void RemoveCrLf(char* s) {
    assert(s != 0);
    size_t len = strlen(s);
    if (len && s[len - 1] == '\n')
        s[--len] = 0;
    if (len && s[len - 1] == '\r')
        s[--len] = 0;
}

void RemoveCrLf(Stroka& s) {
    size_t len = s.length();
    if (len && s[len - 1] == '\n')
        s.erase(--len, 1);
    if (len && s[len - 1] == '\r')
        s.erase(--len, 1);
}

////////////////////////////////////////////////////////////////////////////////
//
// SplitNameValue
//

template<typename TStroka> int SplitNameValueT(
    const TStroka& str, typename TStroka::char_type sep, TStroka* name, TStroka* value)
{
    const TStroka srcStr(str);
    size_t pos = srcStr.find(sep);
    if (pos == TStroka::npos) {
        name->assign(srcStr);
        StripString(*name, *name);
        value->clear();
    }
    else {
        name->assign(srcStr, 0, pos);
        value->assign(srcStr, pos + 1);
        StripString(*name, *name);
        StripString(*value, *value);
    }
    return (int)pos;
}

int SplitNameValue(const Stroka& str, char sep, Stroka* name, Stroka* value) {
    return SplitNameValueT(str, sep, name, value);
}

int SplitNameValue(const WStroka& str, wchar16 sep, WStroka* name, WStroka* value) {
    return SplitNameValueT(str, sep, name, value);
}

////////////////////////////////////////////////////////////////////////////////
//
// Compare
//

template<typename TChr1, typename TChr2>
int CompareT(const TChr1* str1, size_t len1, const TChr2* str2, size_t len2) {
    for (size_t n = Min(len1, len2); n; --n) {
        int cmp = *str1++ - *str2++;
        if (cmp)
            return cmp;
    }
    return (int)(len1 - len2);
}

template<typename TChr1, typename TChr2>
int CompareIgnoreCaseT(const TChr1* str1, size_t len1, const TChr2* str2, size_t len2) {
    for (size_t n = Min(len1, len2); n; --n) {
        int cmp = ToLower(*str1++) - ToLower(*str2++);
        if (cmp)
            return cmp;
    }
    return (int)(len1 - len2);
}

bool Equals(const Stroka& str1, const char* str2, int flags) {
    size_t len2 = strlen(str2);
    if (flags & IGNORE_CASE) {
        return CompareIgnoreCaseT((const ui8*)str1.c_str(), str1.length()
            , (const ui8*)str2, len2) == 0;
    }
    return CompareT((const ui8*)str1.c_str(), str1.length(), (const ui8*)str2, len2) == 0;
}

bool Equals(const WStroka& str, const char* asciiStr, int flags) {
    size_t asciiLen = strlen(asciiStr);
    assert(IsAscii(TStringBuf(asciiStr, asciiLen)));
    if (flags & IGNORE_CASE)
        return CompareIgnoreCaseT(str.c_str(), str.length(), asciiStr, asciiLen) == 0;
    return CompareT(str.c_str(), str.length(), (const ui8*)asciiStr, asciiLen) == 0;
}

bool Equals(const WStroka& str1, const WStroka& str2, int flags) {
    if (flags & IGNORE_CASE)
        return CompareIgnoreCaseT(str1.c_str(), str1.length(), str2.c_str(), str2.length()) == 0;
    return CompareT(str1.c_str(), str1.length(), str2.c_str(), str2.length()) == 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Starts/EndsWith
//

bool StartsWith(const WStroka& str, const char* prefix) {
    assert(prefix != 0);
    size_t n = strlen(prefix);
    if (str.length() < n)
        return false;
    return CompareT(str.c_str(), n, (const ui8*)prefix, n) == 0;
}

bool EndsWith(const WStroka& str, const char* suffix) {
    assert(suffix != 0);
    size_t n = strlen(suffix);
    if (str.length() < n)
        return false;
    return CompareT(str.c_str() + str.length() - n, n, (const ui8*)suffix, n) == 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// String functions
//

template<typename TStroka>
size_t CommonPrefix(const TStroka& s1, const TStroka& s2) {
    size_t n = Min(s1.length(), s2.length());
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i])
            return i;
    }
    return n;
}

template size_t CommonPrefix<Stroka>(const Stroka&, const Stroka&);
template size_t CommonPrefix<WStroka>(const WStroka&, const WStroka&);

template<typename TStroka>
size_t CommonSuffix(const TStroka& s1, const TStroka& s2) {
    size_t n = Min(s1.length(), s2.length());
    const typename TStroka::char_type* p1 = s1.c_str() + s1.length();
    const typename TStroka::char_type* p2 = s2.c_str() + s2.length();
    for (size_t i = 0; i < n; ++i) {
        if (*--p1 != *--p2)
            return i;
    }
    return n;
}

template size_t CommonSuffix<Stroka>(const Stroka&, const Stroka&);
template size_t CommonSuffix<WStroka>(const WStroka&, const WStroka&);

bool IsNumber(const WStroka& s) {
    if (s.empty())
        return false;

    for (size_t i = 0; i < s.length(); ++i) {
        if (!IsDigit(s[i]))
            return false;
    }
    return true;
}

template<typename TStroka>
bool IsHexT(const TStroka& s) {
    if (s.empty())
        return false;
    for (int i = 0; i < (int)s.length(); ++i) {
        typename TStroka::char_type ch = s[i];
        if (ch < '0' || ch > 'f' || mapHex2i[(ui8)ch] < 0)
            return false;
    }
    return true;
}

bool IsHex(const TStringBuf& s) { return IsHexT(s); }
bool IsHex(const TWStringBuf& s) { return IsHexT(s); }

////////////////////////////////////////////////////////////////////////////////
//
// Ascii
//

template<typename TStroka>
bool IsAsciiT(const TStroka& s) {
    for (int i = 0; i < (int)s.length(); ++i) {
        if ((wchar32)s[i] >= 0x80)
            return false;
    }
    return true;
}

bool IsAscii(const TStringBuf& s) {
    return IsAsciiT(s);
}

bool IsAscii(const TWStringBuf& s) {
    return IsAsciiT(s);
}

template<typename TStroka>
Stroka ToAscii(const TStroka& s) {
    Stroka ascii(s.length());
    Stroka::iterator it(ascii.begin());
    for (int i = 0; i < (int)s.length(); ++i) {
        wchar16 ch = s[i];
        *it++ = (ch < 0x80 ? (char)ch : '?');
    }
    return ascii;
}

template Stroka ToAscii<Stroka>(const Stroka& s);
template Stroka ToAscii<WStroka>(const WStroka& s);

Stroka Escape(const WStroka& str) {
    return ToAscii(EscapeC(str));
}

////////////////////////////////////////////////////////////////////////////////
//
// FormatTime
//

Stroka FormatTime(time_t time, ETimeFormat format) {
#ifdef _win_
    #define FMT_YANDEX  "[%Y-%m-%d %H:%M:%S]"
    #define FMT_ISO8601 "%Y-%m-%dT%H:%M:%S"
#else
    #define FMT_YANDEX  "[%Y-%m-%d %H:%M:%S] %z"
    #define FMT_ISO8601 "%Y-%m-%dT%H:%M:%S%z"
#endif

    char buf[30] = "";
    switch (format) {
        case TIME_ISO8601:
            strftime(buf, ARRAY_SIZE(buf), FMT_ISO8601, localtime(&time));
            break;
        case TIME_YANDEX:
            strftime(buf, ARRAY_SIZE(buf), FMT_YANDEX, localtime(&time));
            break;
        case TIME_UNIX:
            sprintf(buf, "%ld", (long)time);
            break;
        default:
            break;
    }
    return buf;
}
