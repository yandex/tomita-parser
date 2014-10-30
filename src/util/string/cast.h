#pragma once

#include <util/system/defaults.h>
#include <util/stream/str.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>

/*
 * specialized for all arithmetic types
 */

template <class T>
size_t ToStringImpl(T t, char* buf, size_t len);

/**
  * Converts @c t to string writing not more than @c len bytes to output buffer @c buf.
  * No NULL terminator appended! Throws exception on buffer overflow.
  * @return number of bytes written
  **/
template <class T>
inline size_t ToString(const T& t, char* buf, size_t len) {
    typedef typename TTypeTraits<T>::TFuncParam TParam;

    return ToStringImpl<TParam>(t, buf, len);
}

namespace NPrivate {
    template <class T, bool isSimple>
    struct TToString {
        static inline Stroka Cvt(const T& t) {
            char buf[256];

            return Stroka(buf, ToString<T>(t, buf, sizeof(buf)));
        }
    };

    template <class T>
    struct TToString<T, false> {
        static inline Stroka Cvt(const T& t) {
            TStringStream s;
            s << t;
            return s.Str();
        }
    };
}

/*
 * some clever implementations...
 */
template <class T>
inline Stroka ToString(const T& t) {
    typedef typename TTypeTraits<T>::TNonQualified TR;

    return ::NPrivate::TToString<TR, TTypeTraits<TR>::IsArithmetic>::Cvt((const TR&)t);
}

inline const Stroka& ToString(const Stroka& s) throw () {
    return s;
}

inline const Stroka& ToString(Stroka& s) throw () {
    return s;
}

inline Stroka ToString(const char* s) {
    return s;
}

inline Stroka ToString(char* s) {
    return s;
}

/*
 * Wrapper for wide strings.
 */
template<class T>
inline Wtroka ToWtring(const T& t) {
    return Wtroka::FromAscii(ToString(t));
}

inline const Wtroka& ToWtring(const Wtroka& w) {
    return w;
}

inline const Wtroka& ToWtring(Wtroka& w) {
    return w;
}

/*
 * specialized for:
 *  bool
 *  short
 *  unsigned short
 *  int
 *  unsigned int
 *  long
 *  unsigned long
 *  long long
 *  unsigned long long
 *  float
 *  double
 *  long double
 */

struct TFromStringException: public TBadCastException {
};

template <typename T, typename TChar>
T FromStringImpl(const TChar* data, size_t len);

template <typename T, typename TChar>
inline T FromString(const TChar* data, size_t len) {
    return ::FromStringImpl<T>(data, len);
}

template <typename T, typename TChar>
inline T FromString(const TChar* data) {
    return ::FromString<T>(data, TCharTraits<TChar>::GetLength(data));
}

template <class T>
inline T FromString(const TStringBuf& s) {
    return ::FromString<T>(~s, +s);
}

template <class T>
inline T FromString(const Stroka& s) {
    return ::FromString<T>(~s, +s);
}

template <>
inline Stroka FromString<Stroka>(const Stroka& s) {
    return s;
}

template <class T>
inline T FromString(const TWtringBuf& s) {
    return ::FromString<T, typename TWtringBuf::char_type>(~s, +s);
}

template <class T>
inline T FromString(const Wtroka& s) {
    return ::FromString<T, typename Wtroka::char_type>(~s, +s);
}

namespace NPrivate {
    template <typename TChar>
    class TFromString {
        const TChar* const Data;
        const size_t Len;

    public:
        inline TFromString(const TChar* data, size_t len)
            : Data(data)
            , Len(len)
        {
        }

        template <typename T>
        inline operator T() const {
            return FromString<T, TChar>(Data, Len);
        }
    };
}

template <typename TChar>
inline ::NPrivate::TFromString<TChar> FromString(const TChar* data, size_t len) {
    return ::NPrivate::TFromString<TChar>(data, len);
}

template <typename TChar>
inline ::NPrivate::TFromString<TChar> FromString(const TChar* data) {
    return ::NPrivate::TFromString<TChar>(data, TCharTraits<TChar>::GetLength(data));
}

template <typename T>
inline ::NPrivate::TFromString<typename T::TChar> FromString(const T& s) {
    return ::NPrivate::TFromString<typename T::TChar>(~s, +s);
}

// Conversion exception free versions
template <typename T, typename TChar>
bool TryFromStringImpl(const TChar* data, size_t len, T& result);

/**
 * @param data Source string buffer pointer
 * @param len Source string length, in characters
 * @param result Place to store conversion result value.
 * If conversion error occurs, no value stored in @c result
 * @return @c true in case of successful conversion, @c false otherwise
 **/
template <typename T, typename TChar>
inline bool TryFromString(const TChar* data, size_t len, T& result) {
    return TryFromStringImpl<T>(data, len, result);
}

template <typename T, typename TChar>
inline bool TryFromString(const TChar* data, T& result) {
    return TryFromString<T>(data, TCharTraits<TChar>::GetLength(data), result);
}

template <class T>
inline bool TryFromString(const TStringBuf& s, T& result) {
    return TryFromString<T>(~s, +s, result);
}

template <class T>
inline bool TryFromString(const Stroka& s, T& result) {
    return TryFromString<T>(~s, +s, result);
}

template <class T>
inline bool TryFromString(const TWtringBuf& s, T& result) {
    return TryFromString<T>(~s, +s, result);
}

template <class T>
inline bool TryFromString(const Wtroka& s, T& result) {
    return TryFromString<T>(~s, +s, result);
}

double StrToD(const char* b, char** se);
double StrToD(const char* b, const char* e, char** se);

template <int base, class T>
size_t IntToString(T t, char* buf, size_t len);

template <int base, class T>
inline Stroka IntToString(T t) {
    STATIC_ASSERT(TTypeTraits<typename TTypeTraits<T>::TNonQualified>::IsArithmetic);

    char buf[256];

    return Stroka(buf, IntToString<base>(t, buf, sizeof(buf)));
}

template <class TInt, int base, class TChar>
TInt IntFromString(const TChar* str, size_t len);

template <class TInt, int base, class TChar>
inline TInt IntFromString(const TChar* str) {
    return IntFromString<TInt, base>(str, TCharTraits<TChar>::GetLength(str));
}

template <class TInt, int base, class Troka>
inline TInt IntFromString(const Troka& str) {
    return IntFromString<TInt, base>(~str, +str);
}

/* Lite functions with no error check */
template <class T>
inline T strtonum_u(const char* s) throw () { // lite 10-based unguarded
    char cs;
    do {
        cs = *s++;
    } while (cs && (ui8)cs <= 32);
    bool neg;
    if ((neg = (cs == '-')) || cs == '+')
        cs = *s++;
    int c = (int)(ui8)cs - '0';
    T acc = 0;
    for (; c >= 0 && c <= 9; c = (int)(ui8)*s++ - '0')
        acc = acc * 10 + c;
    if (neg)
        acc = -acc;
    return acc;
}

ui32 strtoui32(const char* s) throw (); // strtonum_u<ui32>
int yatoi(const char* s) throw (); // strtonum_u<long>
