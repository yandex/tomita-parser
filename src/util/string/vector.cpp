#include "util.h"
#include "split.h"
#include "vector.h"

#include <util/system/defaults.h>
#include <util/generic/typehelpers.h>

template <class TConsumer, class TDelim, typename TChr>
static inline void DoSplit3(TConsumer& c, TDelim& d, const TChr* ptr, int) {
    SplitString(ptr, d, c);
}

template <class TConsumer, class TDelim, typename TChr>
static inline void DoSplit1(TConsumer& cc, TDelim& d, const TChr* ptr, int opts) {
    if (opts & KEEP_DELIMITERS) {
        TKeepDelimiters<TConsumer> kc(&cc);

        DoSplit2(kc, d, ptr, opts);
    } else {
        DoSplit2(cc, d, ptr, opts);
    }
}

template <class TConsumer, class TDelim, typename TChr>
static inline void DoSplit2(TConsumer& cc, TDelim& d, const TChr* ptr, int opts) {
    if (opts & KEEP_EMPTY_TOKENS) {
        DoSplit3(cc, d, ptr, opts);
    } else {
        TSkipEmptyTokens<TConsumer> sc(&cc);

        DoSplit3(sc, d, ptr, opts);
    }
}

template <class C, class TDelim, typename TChr>
static inline void DoSplit0(C* res, const TChr* ptr, TDelim& d, size_t maxFields, int options) {
    typedef typename TSelectType<TSameType<TChr, wchar16>::Result, Wtroka, Stroka>::TResult TString;
    res->clear();

    if (!ptr) {
        return;
    }

    typedef TContainerConsumer<C> TConsumer;
    TConsumer cc(res);

    if (maxFields) {
        TLimitingConsumer<TConsumer, const TChr> lc(maxFields, &cc);

        DoSplit1(lc, d, ptr, options);

        if (lc.Last) {
            res->push_back(TString(lc.Last));
        }
    } else {
        DoSplit1(cc, d, ptr, options);
    }
}

template <typename TChr>
static void SplitStroku(yvector<typename TSelectType<TSameType<TChr, wchar16>::Result, Wtroka, Stroka>::TResult>* res,
const TChr* ptr, const TChr* delim, size_t maxFields, int options)
{
    if (!*delim) {
        return;
    }

    if (*(delim + 1)) {
        TStringDelimiter<const TChr> d(delim, TCharTraits<TChr>::GetLength(delim));

        DoSplit0(res, ptr, d, maxFields, options);
    } else {
        TCharDelimiter<const TChr> d(*delim);

        DoSplit0(res, ptr, d, maxFields, options);
    }
}

void SplitStroku(VectorStrok* res, const char* ptr, const char* delim, size_t maxFields, int options) {
    return SplitStroku<char>(res, ptr, delim, maxFields, options);
}

void SplitStroku(VectorWtrok* res, const wchar16* ptr, const wchar16* delimiter, size_t maxFields, int options) {
    return SplitStroku<wchar16>(res, ptr, delimiter, maxFields, options);
}

template<class T>
void SplitStrokuBySetImpl(yvector<T>* res,const typename T::char_type* ptr, const typename T::char_type* delimiters, size_t maxFields, int options) {
    TSetDelimiter<const typename T::char_type> d(delimiters);
    DoSplit0(res, ptr, d, maxFields, options);
}

void SplitStrokuBySet(VectorStrok* res, const char* ptr, const char* delimiters, size_t maxFields, int options) {
    SplitStrokuBySetImpl<Stroka> (res, ptr, delimiters, maxFields, options);
}
void SplitStrokuBySet(VectorWtrok* res, const wchar16* ptr, const wchar16* delimiters, size_t maxFields, int options) {
    SplitStrokuBySetImpl<Wtroka> (res, ptr, delimiters, maxFields, options);
}

Wtroka JoinStroku(const VectorWtrok& v, const TWtringBuf& delim) {
    return JoinStroku(v.begin(), v.end(), delim);
}

Wtroka JoinStroku(const VectorWtrok& v, size_t index, size_t count, const TWtringBuf& delim) {
    const size_t f = Min(index, +v);
    const size_t l = f + Min(count, +v - f);

    return JoinStroku(v.begin() + f, v.begin() + l, delim);
}

size_t SplitStroku(char* str, char delim, char* tokens[], size_t maxCount) {
    if (!str)
        return 0;

    size_t i = 0;
    while (i < maxCount) {
        tokens[i++] = str;
        str = strchr(str, delim);
        if (!str)
            break;
        *str++ = 0;
    }

    return i;
}
