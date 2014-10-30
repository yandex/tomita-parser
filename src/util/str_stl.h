#pragma once

#include <util/memory/alloc.h>
#include <util/digest/numeric.h>
#include <util/generic/stroka.h>
#include <util/generic/pair.h>
#include <util/generic/strbuf.h>
#include <util/charset/codepage.h>

#include <stlport/functional>

namespace NStl {
    template <>
    struct less<const char*>: public NStl::binary_function<const char*, const char*, bool> {
        bool operator()(const char* x, const char* y) const {
            return strcmp(x, y) < 0;
        }
    };

    template <>
    struct equal_to<const char*>: public NStl::binary_function<const char*, const char*, bool> {
        bool operator()(const char* x, const char* y) const {
            return strcmp(x, y) == 0;
        }
        bool operator()(const char* x, const TFixedString<char> &y) const {
            return strlen(x) == y.Length && memcmp(x, y.Start, y.Length) == 0;
        }
    };
}

namespace NHashPrivate {
    template <class T, bool needNumericHashing>
    struct THashHelper {
        inline size_t operator() (const T& t) const throw () {
            return (size_t)t;
        }
    };

    template <class T>
    struct THashHelper<T, true> {
        inline size_t operator() (const T& t) const throw () {
            return NumericHash(t);
        }
    };
}

template <class T>
struct hash: public NHashPrivate::THashHelper<T, TTypeTraits<T>::IsPrimitive && !TTypeTraits<T>::IsIntegral> {
};

#if defined(_MSC_VER) && (_MSC_VER < 1500)
#   define QUAL ::
#else
#   define QUAL
#endif

template<typename T>
struct QUAL hash<const T*> {
    inline size_t operator() (const T* t) const throw () {
        return NumericHash(t);
    }
};

template <class T>
struct QUAL hash<T*>: public ::hash<const T*> {
};

template<>
struct QUAL hash<const char*> {
    // note that const char* is implicitly converted to TFixedString
    inline size_t operator()(const TFixedString<char> &s) const throw () {
        return Stroka::hashVal(s.Start, s.Length);
    }
};

struct sgi_hash {
    inline size_t operator()(const char* s) const {
        unsigned long h = 0;
        for (; *s; ++s)
            h = 5*h + *s;
        return size_t(h);
    }
    inline size_t operator()(const TFixedString<char> &x) const {
        unsigned long h = 0;
        for (const char *s = x.Start, *e = s + x.Length; s != e; ++s)
            h = 5*h + *s;
        return size_t(h);
    }
};

struct ci_less {
    inline bool operator()(const char* x, const char* y) const {
        return csYandex.stricmp(x, y) < 0;
    }
};

struct ci_equal_to {
    inline bool operator()(const char* x, const char* y) const {
        return csYandex.stricmp(x, y) == 0;
    }
    // this implementation is not suitable for strings with zero characters inside, sorry
    bool operator()(const TFixedString<char> &x, const TFixedString<char> &y) const {
        return x.Length == y.Length && csYandex.strnicmp(x.Start, y.Start, y.Length) == 0;
    }
};

struct ci_hash {
    inline size_t operator()(const char* s) const {
        return stroka::hashVal(s, strlen(s));
    }
    inline size_t operator()(const TFixedString<char> &s) const {
        return stroka::hashVal(s.Start, s.Length);
    }
};

struct ci_hash32 { // not the same as ci_hash under 64-bit
    inline ui32 operator()(const char* s) const {
        return (ui32)stroka::hashVal(s, strlen(s));
    }
};

template <>
struct QUAL hash<Stroka> {
    inline size_t operator()(const TStringBuf& s) const {
        return s.hash();
    }
};

template <>
struct QUAL hash<stroka> : public ci_hash {
};

template <>
struct QUAL hash<Wtroka> {
    inline size_t operator()(const TWtringBuf& s) const {
        return s.hash();
    }
};

template <class T>
struct THash: public ::hash<T> {
};

template <class TFirst, class TSecond>
struct QUAL hash< TPair<TFirst, TSecond> > {
private:
    THash<TFirst> FisrtHash;
    THash<TSecond> SecondHash;
public:
    inline size_t operator()(const TPair<TFirst, TSecond>& pair) const {
        return CombineHashes(FisrtHash(pair.first), SecondHash(pair.second));
    }
};

template <>
struct THash<TStringBuf> {
    inline size_t operator()(const TStringBuf& s) const {
        return s.hash();
    }
};

template <>
struct THash<TWtringBuf> {
    inline size_t operator()(const TWtringBuf& s) const {
        return s.hash();
    }
};

template <class Key>
struct hash32: public THash<Key> {
    inline size_t operator()(const Key& value) const {
        return (ui32)hash<Key>::operator()(value);
    }
};

struct sgi_hash32 : public sgi_hash {
    inline size_t operator()(const char* key) const {
        return (ui32)sgi_hash::operator()(key);
    }
};

typedef QUAL hash<Stroka> strokaHash;

template <class T>
struct TCIHash {
};

template <>
struct TCIHash<const char*> {
    inline size_t operator()(const TFixedString<char> &s) const {
        return stroka::hashVal(s.Start, s.Length);
    }
};

template <>
struct TCIHash<TStringBuf> {
    inline size_t operator()(const TStringBuf& s) const {
        return stroka::hashVal(~s, +s);
    }
};

template <>
struct TCIHash<Stroka> {
    inline size_t operator()(const Stroka& s) const {
        return stroka::hashVal(~s, +s);
    }
};

template <class T>
struct TEqualTo: public NStl::equal_to<T> {
};

template <>
struct TEqualTo<Stroka>: public TEqualTo<TStringBuf> {
};

template <>
struct TEqualTo<Wtroka>: public TEqualTo<TWtringBuf> {
};

template <>
struct TEqualTo<stroka>: public ci_equal_to {
};

template <class T>
struct TCIEqualTo {
};

template <>
struct TCIEqualTo<const char*> {
    inline size_t operator()(const char* a, const char* b) const {
        return stricmp(a, b) == 0;
    }
};

template <>
struct TCIEqualTo<TStringBuf> {
    inline size_t operator()(const TStringBuf& a, const TStringBuf& b) const {
        return +a == +b && strnicmp(~a, ~b, +a) == 0;
    }
};

template <>
struct TCIEqualTo<Stroka> {
    inline size_t operator()(const Stroka& a, const Stroka& b) const {
        return +a == +b && strnicmp(~a, ~b, +a) == 0;
    }
};

template <class T>
struct TLess: public NStl::less<T> {
};

template<>
struct TLess<Stroka> : public TLess<TStringBuf> {
};

template<>
struct TLess<Wtroka> : public TLess<TWtringBuf> {
};

template <class T>
struct TGreater: public NStl::greater<T> {
};

template<>
struct TGreater<Stroka> : public TGreater<TStringBuf> {
};

template<>
struct TGreater<Wtroka> : public TGreater<TWtringBuf> {
};

struct TLessT {
    template<typename TA, typename TB>
    bool operator() (const TA& a, const TB& b) const {
        return a < b;
    }
};

struct TGreaterT {
    template<typename TA, typename TB>
    bool operator() (const TA& a, const TB& b) const {
        return a > b;
    }
};

template <class T, class HashFcn = THash<const char*>, class EqualTo = TEqualTo<const char*>,
   class Alloc = DEFAULT_ALLOCATOR(const char*)>
class string_hash;

template <class T, class HashFcn = THash<const char*>, class EqualTo = TEqualTo<const char*> >
class segmented_string_hash;

template <class HashFcn = THash<const char*>, class EqualTo = TEqualTo<const char*> >
class atomizer;

typedef atomizer<ci_hash, ci_equal_to> ci_atomizer;

template <class T, class HashFcn = THash<const char*>, class EqualTo = TEqualTo<const char*> >
class super_atomizer;
