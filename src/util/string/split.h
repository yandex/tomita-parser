#pragma once

#include "strspn.h"

#include <util/system/compat.h>
#include <util/system/defaults.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/ylimits.h>
#include <util/string/cast.h>

template <class I, class TDelim, class TConsumer>
static inline void SplitString(I b, I e, const TDelim& d, TConsumer& c) {
    I l, i;

    do {
        l = b;
        i = d.Find(b, e);
    } while (c.Consume(l, i, b) && (b != i));
}

template <class I, class TDelim, class TConsumer>
static inline void SplitString(I b, const TDelim& d, TConsumer& c) {
    I l, i;

    do {
        l = b;
        i = d.Find(b);
    } while (c.Consume(l, i, b) && (b != i));
}

template <class I1, class I2>
static inline I1* FastStrChr(I1* str, I2 f) throw () {
    I1* ret = (I1*)TCharTraits<I1>::Find(str, f);

    if (!ret) {
        ret = str + TCharTraits<I1>::GetLength(str);
    }

    return ret;
}

template <class I>
static inline I* FastStrStr(I* str, I* f, size_t l) throw () {
    (void)l;
    I* ret = (I*)TCharTraits<I>::Find(str, *f);

    if (ret) {
        ret = (I*)TCharTraits<I>::Find(ret, f);
    }

    if (!ret) {
        ret = str + TCharTraits<I>::GetLength(str);
    }

    return ret;
}

template <class I>
struct TStringDelimiter {
    inline TStringDelimiter(I* delim) throw ()
        : Delim(delim)
        , Len(TCharTraits<I>::GetLength(delim))
    {
    }

    inline TStringDelimiter(I* delim, size_t len) throw ()
        : Delim(delim)
        , Len(len)
    {
    }

    inline I* Find(I*& b, I* e) const throw () {
        I* ret = (I*)TCharTraits<I>::Find(b, e - b, Delim, Len);

        if (ret) {
            b = ret + Len;
            return ret;
        }

        return (b = e);
    }

    inline I* Find(I*& b) const throw () {
        I* ret = FastStrStr(b, Delim, Len);

        if (*ret) {
            b = ret + Len;
        } else {
            b = ret;
        }

        return ret;
    }

    I* Delim;
    const size_t Len;
};

template <class I>
struct TCharDelimiter {
    inline TCharDelimiter(I ch) throw ()
        : Ch(ch)
    {
    }

    inline I* Find(I*& b, I* e) const throw () {
        I* ret = (I*)TCharTraits<I>::Find(b, Ch, e - b);

        if (ret) {
            b = ret + 1;
            return ret;
        }

        return (b = e);
    }

    inline I* Find(I*& b) const throw () {
        I* ret = FastStrChr(b, Ch);

        if (*ret) {
            b = ret + 1;
        } else {
            b = ret;
        }

        return ret;
    }

    I Ch;
};

template <class I>
struct TFindFirstOf {
    inline TFindFirstOf(I* set)
        : Set(set)
    {
    }

    //TODO
    inline I* FindFirstOf(I* b, I* e) const throw () {
        I* ret = b;
        for (; ret != e; ++ret) {
            if (TCharTraits<I>::Find(Set, *ret))
                break;
        }
        return ret;
    }

    inline I* FindFirstOf(I* b) const throw () {
        return b + TCharTraits<I>::FindFirstOf(b, Set);
    }

    I* Set;
};

template <>
struct TFindFirstOf<const char>: public TCompactStrSpn {
    inline TFindFirstOf(const char* set)
        : TCompactStrSpn(set)
    {
    }
};

template <class I>
struct TSetDelimiter: private TFindFirstOf<const I> {
    inline TSetDelimiter(const I* d)
        : TFindFirstOf<const I>(d)
    {
    }

    inline I* Find(I*& b, I* e) const throw () {
        I* ret = const_cast<I*>(this->FindFirstOf(b, e));

        if (ret != e) {
            b = ret + 1;
            return ret;
        }

        return (b = e);
    }

    inline I* Find(I*& b) const throw () {
        I* ret = const_cast<I*>(this->FindFirstOf(b));

        if (*ret) {
            b = ret + 1;
            return ret;
        }

        return (b = ret);
    }
};

namespace NSplitTargetHasPushBack {
    HAS_MEMBER_NAMED(push_back, PushBack);
}

template <class T, bool hasPushBack>
struct TConsumerBackInserter {
    static void DoInsert(T* C, const typename T::value_type& i);
};

template <class T>
struct TConsumerBackInserter<T, true> {
    static void DoInsert(T* C, const typename T::value_type& i)
    {
        C->push_back(i);
    }
};

template <class T>
struct TConsumerBackInserter<T, false> {
    static void DoInsert(T* C, const typename T::value_type& i)
    {
        C->insert(C->end(), i);
    }
};

template <class T>
struct TContainerConsumer {
    inline TContainerConsumer(T* c) throw ()
        : C(c)
    {
    }

    template <class I>
    inline bool Consume(I* b, I* d, I* /*e*/) {
        TConsumerBackInserter<T, NSplitTargetHasPushBack::TClassHasPushBack<T>::Result>::DoInsert(C, typename T::value_type(b, d));

        return true;
    }

    T* C;
};

template <class T>
struct TContainerConvertingConsumer {
    inline TContainerConvertingConsumer(T* c) throw ()
        : C(c)
    {
    }

    template <class I>
    inline bool Consume(I* b, I* d, I* /*e*/) {
        TConsumerBackInserter<T, NSplitTargetHasPushBack::TClassHasPushBack<T>::Result>::DoInsert(C, FromString<typename T::value_type>(TStringBuf(b, d)));

        return true;
    }

    T* C;
};

template <class S, class I>
struct TLimitingConsumer {
    inline TLimitingConsumer(size_t cnt, S* slave) throw ()
        : Cnt(cnt ? cnt - 1 : Max<size_t>())
        , Slave(slave)
        , Last(0)
    {
    }

    inline bool Consume(I* b, I* d, I* e) {
        if (!Cnt) {
            Last = b;

            return false;
        }

        --Cnt;

        return Slave->Consume(b, d, e);
    }

    size_t Cnt;
    S* Slave;
    I* Last;
};

template <class S>
struct TSkipEmptyTokens {
    inline TSkipEmptyTokens(S* slave) throw ()
        : Slave(slave)
    {
    }

    template <class I>
    inline bool Consume(I* b, I* d, I* e) {
        if (b != d) {
            return Slave->Consume(b, d, e);
        }

        return true;
    }

    S* Slave;
};

template <class S>
struct TKeepDelimiters {
    inline TKeepDelimiters(S* slave) throw ()
        : Slave(slave)
    {
    }

    template <class I>
    inline bool Consume(I* b, I* d, I* e) {
        if (Slave->Consume(b, d, d)) {
            if (d != e) {
                return Slave->Consume(d, e, e);
            }

            return true;
        }

        return false;
    }

    S* Slave;
};

template <class TConsumer, class I, class C>
static inline void SplitRangeToImpl(I* b, I* e, I d, C* c) {
    TCharDelimiter<I> delim(d);
    TConsumer consumer(c);

    SplitString(b, e, delim, consumer);
}

template <class TConsumer, class I, class C>
static inline void SplitRangeBySetToImpl(I* b, I* e, I* d, C* c) {
    TSetDelimiter<I> delim(d);
    TConsumer consumer(c);

    SplitString(b, e, delim, consumer);
}

template <class TConsumer, class I, class C>
static inline void SplitRangeToImpl(I* b, I* e, I* d, C* c) {
    TStringDelimiter<I> delim(d);
    TConsumer consumer(c);

    SplitString(b, e, delim, consumer);
}

template <class TConsumer, class S, class C>
static inline void SplitStringToImpl(const S& s, const S& d, C* c) {
    SplitRangeToImpl<TConsumer, const char, C>(~s, ~s + +s, d.c_str(), c);
}

// Split functions
template <class I, class C>
static inline void SplitRangeTo(I* b, I* e, I d, C* c) {
    SplitRangeToImpl< TContainerConsumer<C>, I, C >(b, e, d, c);
}

template <class I, class C>
static inline void SplitRangeBySetTo(I* b, I* e, I* d, C* c) {
    SplitRangeBySetToImpl< TContainerConsumer<C> >(b, e, d, c);
}

template <class S, class C>
static inline void SplitStringTo(const S& s, char delim, C* c) {
    SplitRangeTo<const char, C>(~s, ~s + +s, delim, c);
}

template <class S, class C>
static inline void SplitStringBySetTo(const S& s, const char* delim, C* c) {
    SplitRangeBySetTo<const char, C>(~s, ~s + +s, delim, c);
}

template <class I, class C>
static inline void SplitRangeTo(I* b, I* e, I* d, C* c) {
    SplitRangeToImpl< TContainerConsumer<C> >(b, e, d, c);
}

template <class S, class C>
static inline void SplitStringTo(const S& s, const S& d, C* c) {
    SplitRangeTo<const char, C>(~s, ~s + +s, d.c_str(), c);
}

// Split and Convert functions
template <class I, class C>
static inline void SplitConvertRangeTo(I* b, I* e, I d, C* c) {
    SplitRangeToImpl< TContainerConvertingConsumer<C>, I, C >(b, e, d, c);
}

template <class I, class C>
static inline void SplitConvertRangeBySetTo(I* b, I* e, I* d, C* c) {
    SplitRangeToImpl< TContainerConvertingConsumer<C> >(b, e, d, c);
}

template <class S, class C>
static inline void SplitConvertStringTo(const S& s, char delim, C* c) {
    SplitConvertRangeTo<const char, C>(~s, ~s + +s, delim, c);
}

template <class S, class C>
static inline void SplitConvertStringBySetTo(const S& s, const char* delim, C* c) {
    SplitRangeBySetTo<TContainerConvertingConsumer<C>, const char, C>(~s, ~s + +s, delim, c);
}

template <class I, class C>
static inline void SplitConvertRangeTo(I* b, I* e, I* d, C* c) {
    SplitRangeToImpl< TContainerConvertingConsumer<C> >(b, e, d, c);
}

template <class S, class C>
static inline void SplitConvertStringTo(const S& s, const S& d, C* c) {
    SplitConvertRangeTo<const char, C>(~s, ~s + +s, d.c_str(), c);
}

template <class T>
struct TSimplePusher {
    inline bool Consume(char* b, char* d, char*) {
        *d = 0;
        C->push_back(b);

        return true;
    }

    T* C;
};

template <class T>
static inline void Split(char* buf, T* res) {
    Split(buf, '\t', res);
}

template <class T>
static inline void Split(char* buf, char ch, T* res) {
    res->resize(0);
    if (*buf == 0)
        return;

    TCharDelimiter<char> delim(ch);
    TSimplePusher<T> pusher = {res};

    SplitString(buf, delim, pusher);
}

/// Split string into res vector. Res vector is cleared before split.
/// Old good slow split function.
/// Field delimter is any number of symbols specified in delim (no empty strings in res vector)
/// @return number of elements created
size_t Split(const char* in, const char* delim, yvector<Stroka>& res);
size_t Split(const char* in, const char* delim, yvector<TStringBuf>& res);
size_t Split(const Stroka& in, const Stroka& delim, yvector<Stroka>& res);

/// Old split reimplemented for TStringBuf using the new code
/// Note that delim can be constructed from char* automatically (it is not cheap though)
inline size_t Split(const TStringBuf& s, const TSetDelimiter<const char> &delim, yvector<TStringBuf>& res) {
    res.clear();
    TContainerConsumer< yvector<TStringBuf> > res1(&res);
    TSkipEmptyTokens< TContainerConsumer< yvector<TStringBuf> > > consumer(&res1);
    SplitString(~s, ~s + +s, delim, consumer);
    return res.size();
}

template<class P, class D>
void GetNext(TStringBuf& s, D delim, P& param) {
    TStringBuf next = s.NextTok(delim);
    ENSURE(next.IsInited(), "Split: number of fields less than number of Split output arguments");
    param = FromString<P>(next);
}

// example:
// Split(TStringBuf("Sherlock,2014,36.6"), ',', name, year, temperature);
template<class D, class P1, class P2>
void Split(TStringBuf s, D delim, P1& p1, P2& p2) {
    GetNext(s, delim, p1);
    GetNext(s, delim, p2);
    ENSURE(!s.IsInited(), "Split: number of fields more than number of Split output arguments");
}

template<class D, class P1, class P2, class... Other>
void Split(TStringBuf s, D delim, P1& p1, P2& p2, Other&... other) {
    GetNext(s, delim, p1);
    Split(s, delim, p2, other...);
}

