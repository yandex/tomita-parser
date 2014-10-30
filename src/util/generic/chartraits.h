#pragma once

#include "pair.h"
#include "utility.h"
#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <cstring>

#if defined(_64_) && !defined(NO_CITYHASH)
#  include <util/digest/city.h>
#  define CHAR_TRAITS_HASH CityHash64
#else
#  include <util/digest/murmur.h>
#  define CHAR_TRAITS_HASH MurmurHash<size_t>
#endif


wchar32 ToLower(wchar32 ch); // defined in unidata.h
class Stroka;
class Wtroka;

//! common implementation of char traits
template <typename TCharType>
class TCharTraitsBase {
public:
    struct TWhatString {
        typedef TPair<TCharType*, size_t> TString; //junk definition
    };
    static size_t GetLength(const TCharType* s) {
        YASSERT(s);
        const TCharType* sc = s;
        for (; *sc != 0; ++sc) {}
        return sc - s;
    }
    static size_t GetLength(const TCharType* s, size_t maxlen) {
        YASSERT(s || !maxlen);
        const TCharType c(0); // null char
        size_t i = 0;
        while (i < maxlen && s[i] != c)
            ++i;
        return i;
    }
    static TCharType* Move(TCharType* s1, const TCharType* s2, size_t n) {
        return (TCharType*)memmove(s1, s2, n * sizeof(TCharType));
    }
    static TCharType* Copy(TCharType* s1, const TCharType* s2, size_t n) {
        return (TCharType*)memcpy(s1, s2, n * sizeof(TCharType));
    }
    static TCharType* Assign(TCharType* s, size_t n, TCharType c) {
        for (TCharType* ptr = s; ptr < s + n; ++ptr) // see REVIEW:52714 for details
            *ptr = c;
        return s;
    }
    static int Compare(const TCharType* s1, const TCharType* s2) {
        int res;
        while (1) {
            if ((res = *s1 - *s2++) != 0 || !*s1++)
                break;
        }
        return res;
    }
    static int Compare(const TCharType* s1, const TCharType* s2, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            if (s1[i] != s2[i])
                return s1[i] < s2[i] ? -1 : 1;
        }
        return 0;
    }
    static const TCharType* Find(const TCharType* s, TCharType c) {
        for (;;) {
            if (*s == c)
                return s;
            if (!*(s++))
                break;
        }
        return NULL;
    }
    static const TCharType* Find(const TCharType* s, TCharType c, size_t n) {
        for (; n > 0 ; ++s, --n) {
            if (*s == c)
                return s;
        }
        return NULL;
    }
    static const TCharType* Find(const TCharType* s1, const TCharType* s2) {
        size_t n2 = GetLength(s2);
        if (!n2)
            return s1;
        size_t n1 = GetLength(s1);
        return Find(s1, n1, s2, n2);
    }
    static const TCharType* Find(const TCharType* s1, size_t l1, const TCharType* s2, size_t l2) {
        if (!l2)
            return s1;
        while (l1 >= l2) {
            l1--;
            if (!Compare(s1, s2, l2))
                return s1;
            s1++;
        }
        return NULL;
    }
    static const TCharType* RFind(const TCharType* s, TCharType c) {
        return RFind(s, c, GetLength(s));
    }
    static const TCharType* RFind(const TCharType* s, TCharType c, size_t n) {
        if (!n)
            return NULL;
        for (const TCharType* p = s + n - 1; p >= s; --p) {
            if (*p == c)
                return p;
        }
        return NULL;
    }
    static const TCharType* RFind(const TCharType* str1, size_t len1, const TCharType* str2, size_t len2, size_t pos) {
        if (len2 > len1)
            return NULL;
        for (pos = Min(pos, len1 - len2); pos != (size_t)-1; --pos) {
            if (Compare(str1 + pos, str2, len2) == 0)
                return str1 + pos;
        }
        return NULL;
    }
    static size_t FindFirstOf(const TCharType* s, const TCharType* set) {
        size_t n = 0;
        for (; *s; ++s, ++n) {
            if (Find(set, *s))
                break;
        }
        return n;
    }
    static const TCharType* FindFirstOf(const TCharType* s, size_t l1, const TCharType *set, size_t l2) {
        return FindBySet<true>(s, l1, set, l2);
    }
    static size_t FindFirstNotOf(const TCharType* s, const TCharType* set) {
        size_t n = 0;
        for (; *s != 0; ++s, ++n) {
            if (!Find(set, *s))
                break;
        }
        return n;
    }
    static const TCharType* FindFirstNotOf(const TCharType* s, size_t l1, const TCharType *set, size_t l2) {
        return FindBySet<false>(s, l1, set, l2);
    }
    static void Reverse(TCharType* s, size_t n) {
        TCharType* f = s;
        TCharType* l = s + n - 1;
        while (f < l) {
            DoSwap(*f++, *l--);
        }
    }
    static size_t GetHash(const TCharType* s, size_t n) {
        return CHAR_TRAITS_HASH((const char*)s, n * sizeof(TCharType));
    }
    static TCharType ToLower(TCharType c) {
        return ::ToLower((wchar32)c);
    }
private:
    template<bool shouldBeInSet>
    static const TCharType* FindBySet(const TCharType* s, size_t l1, const TCharType *set, size_t l2) {
        for (; l1 > 0; ++s, --l1) {
            const bool found = Find(set, *s, l2) != NULL;
            if (found == shouldBeInSet) {
                return s;
            }
        }
        return NULL;
    }
};

template<>
struct TCharTraitsBase<char>::TWhatString {
    typedef Stroka TString;
};

template<>
struct TCharTraitsBase<wchar16>::TWhatString {
    typedef Wtroka TString;
};

template <typename TCharType>
class TCharTraitsImpl: public TCharTraitsBase<TCharType> {
};

const char* FastFindFirstOf(const char* s, size_t len, const char* set, size_t setlen);
const char* FastFindFirstNotOf(const char* s, size_t len, const char* set, size_t setlen);

template <class T>
static inline T EndToZero(T r, T e) {
    if (r == e) {
        return 0;
    }

    return r;
}

//! specialization for @c char contains some OS accelerated functions
template <>
class TCharTraitsImpl<char>: public TCharTraitsBase<char> {
public:
    static size_t GetLength(const char* s) {
        YASSERT(s);
        return strlen(s);
    }
    static size_t GetLength(const char* s, size_t maxlen) {
        YASSERT(s || !maxlen);
        return strnlen(s, maxlen);
    }
    static char* Assign(char* s, size_t n, char c) {
        memset(s, c, n);
        return s;
    }
    static int Compare(const char* s1, const char* s2) {
        return strcmp(s1, s2);
    }
    static int Compare(const char* s1, const char* s2, size_t n) {
        return memcmp(s1, s2, n);
    }
    static const char* Find(const char* s, char c) {
        return strchr(s, c);
    }
    static const char* Find(const char* s, char c, size_t n) {
        return (const char*)memchr(s, c, n);
    }
    static const char* Find(const char* s1, const char* s2) {
        return strstr(s1, s2);
    }
    static const char* Find(const char* s1, size_t l1, const char* s2, size_t l2) {
#if defined(_glibc_) || defined(_freebsd_)
        return (const char*)memmem(s1, l1, s2, l2);
#else
        return TCharTraitsBase<char>::Find(s1, l1, s2, l2);
#endif
    }
    static const char* RFind(const char* s, char c) {
        return strrchr(s, c);
    }
    static const char* RFind(const char* s, char c, size_t n) {
        return TCharTraitsBase<char>::RFind(s, c, n);
    }
    static const char* RFind(const char* str1, size_t len1, const char* str2, size_t len2, size_t pos) {
        return TCharTraitsBase<char>::RFind(str1, len1, str2, len2, pos);
    }
    static size_t FindFirstOf(const char* s, const char* set) {
        return strcspn(s, set);
    }
    static const char* FindFirstOf(const char* s, size_t l1, const char *set, size_t l2) {
        return EndToZero(FastFindFirstOf(s, l1, set, l2), s + l1);
    }
    static size_t FindFirstNotOf(const char* s, const char* set) {
        return strspn(s, set);
    }
    static const char* FindFirstNotOf(const char* s, size_t l1, const char *set, size_t l2) {
        return EndToZero(FastFindFirstNotOf(s, l1, set, l2), s + l1);
    }
    static char ToLower(char c) {
        return (char)tolower((ui8)c);
    }
};

template <class TCharType>
class TCharTraits: public TCharTraitsImpl<TCharType> {
public:
    typedef TCharTraitsImpl<TCharType> TBase;

    static int Compare(const TCharType* s1, const TCharType* s2) {
        return TBase::Compare(s1, s2);
    }

    static int Compare(const TCharType* s1, const TCharType* s2, size_t n) {
        return TBase::Compare(s1, s2, n);
    }

    static int Compare(const TCharType* s1, size_t n1, const TCharType* s2, size_t n2) {
        const size_t n = n1 < n2 ? n1 : n2;
        const int result = Compare(s1, s2, n);
        return result ? result : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0));
    }
};

template <class T>
class TCharTraits<const T>: public TCharTraits<T> {
};
