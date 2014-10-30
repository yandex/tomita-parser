#pragma once

#include "ascii.h"

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

template <class It>
struct TIsAsciiSpaceAdapter {
    bool operator()(const It& it) const throw () {
        return IsAsciiSpace(*it);
    }
};

template <class It>
TIsAsciiSpaceAdapter<It> IsAsciiSpaceAdapter(It) {
    return TIsAsciiSpaceAdapter<It>();
}

template <class TChar>
struct TEqualsStripAdapter {
    TEqualsStripAdapter(TChar ch)
        : Ch(ch)
    {
    }
    template <class It>
    bool operator()(const It& it) const throw () {
        return *it == Ch;
    }

    TChar Ch;
};

template <class TChar>
TEqualsStripAdapter<TChar> EqualsStripAdapter(TChar ch) {
    return TEqualsStripAdapter<TChar>(ch);
}

template <class It, class StripCriterion>
inline void StripRangeBegin(It& b, const It& e, const StripCriterion& criterion) throw () {
    while (b < e && criterion(b)) {
        ++b;
    }
}

template <class It>
inline void StripRangeBegin(It& b, const It& e) throw () {
    StripRangeBegin(b, e, IsAsciiSpaceAdapter(b));
}

template <class It, class StripCriterion>
inline void StripRangeEnd(const It& b, It& e, const StripCriterion& criterion) throw () {
    while (b < e && criterion(e - 1)) {
        --e;
    }
}

template <class It>
inline void StripRangeEnd(const It& b, It& e) throw () {
    StripRangeEnd(b, e, IsAsciiSpaceAdapter(b));
}

template <bool stripBeg, bool stripEnd>
struct TStripImpl {
    template <class It, class StripCriterion>
    static inline bool StripRange(It& b, It& e, const StripCriterion& criterion) throw () {
        const size_t oldLen = e - b;

        if (stripBeg) {
            StripRangeBegin(b, e, criterion);
        }

        if (stripEnd) {
            StripRangeEnd(b, e, criterion);
        }

        const size_t newLen = e - b;
        return newLen != oldLen;
    }

    template <class T, class StripCriterion>
    static inline bool StripString(const T& from, T& to, const StripCriterion& criterion) {
        const typename T::TChar* b = from.begin();
        const typename T::TChar* e = from.end();

        if (StripRange(b, e, criterion)) {
            to = T(b, e);

            return true;
        }

        to = from;

        return false;
    }

    template <class T, class StripCriterion>
    static inline T StripString(const T& from, const StripCriterion& criterion) {
        T ret;
        StripString(from, ret, criterion);
        return ret;
    }

    template <class T>
    static inline T StripString(const T& from) {
        return StripString(from, IsAsciiSpaceAdapter(from.begin()));
    }
};




template <class It, class StripCriterion>
inline bool StripRange(It& b, It& e, const StripCriterion& criterion) throw () {
    return TStripImpl<true, true>::StripRange(b, e, criterion);
}

template <class It>
inline bool StripRange(It& b, It& e) throw () {
    return StripRange(b, e, IsAsciiSpaceAdapter(b));
}

template <class It, class StripCriterion>
inline bool Strip(It& b, size_t& len, const StripCriterion& criterion) throw () {
    It e = b + len;

    if (StripRange(b, e, criterion)) {
        len = e - b;

        return true;
    }

    return false;
}

template <class It>
inline bool Strip(It& b, size_t& len) throw () {
    return Strip(b, len, IsAsciiSpaceAdapter(b));
}


template <class T, class StripCriterion>
static inline bool StripString(const T& from, T& to, const StripCriterion& criterion) {
    return TStripImpl<true, true>::StripString(from, to, criterion);
}

template <class T>
static inline bool StripString(const T& from, T& to) {
    return StripString(from, to, IsAsciiSpaceAdapter(from.begin()));
}

template <class T, class StripCriterion>
static inline T StripString(const T& from, const StripCriterion& criterion) {
    return TStripImpl<true, true>::StripString(from, criterion);
}

template <class T>
static inline T StripString(const T& from) {
    return TStripImpl<true, true>::StripString(from);
}

template <class T>
static inline T StripStringLeft(const T& from) {
    return TStripImpl<true, false>::StripString(from);
}

template <class T>
static inline T StripStringRight(const T& from) {
    return TStripImpl<false, true>::StripString(from);
}

/// Удаляет пробельные символы с краев строки.
static inline bool Strip(const Stroka& from, Stroka& to) {
    return StripString(from, to);
}
// deprecated version, use Strip instead
static inline bool strip(const Stroka& from, Stroka& to) {
    return StripString(from, to);
}

/// Удаляет пробельные символы с краев строки.
inline Stroka& Strip(Stroka& s) {
    Strip(s, s);
    return s;
}
// deprecated version, use Strip(Stroka& s) instead
inline Stroka& strip(Stroka& s) {
    Strip(s, s);
    return s;
}

/// Удаляет пробельные символы с краев строки.
inline Stroka Strip(const Stroka& s) {
    Stroka ret = s;
    Strip(ret, ret);
    return ret;
}
// deprecated version, use Stroka Strip(const Stroka& s) instead
inline Stroka strip(const Stroka& s) {
    return Strip(s);
}

bool Collapse(const Stroka& from, Stroka& to, size_t maxLen = 0);
// deprecated version, use Collapse instead
inline bool collapse(const Stroka& from, Stroka& to, size_t maxLen = 0) {
    return Collapse(from, to, maxLen);
}

/// Заменяет последовательно идущие подряд пробельные символы одним пробелом (обрабатывая не более maxLen байт)
inline Stroka& Collapse(Stroka& s, size_t maxLen = 0) {
    Collapse(s, s, maxLen);
    return s;
}
// deprecated version, use Collapse instead
inline Stroka& collapse(Stroka& s, size_t maxLen = 0) {
    Collapse(s, s, maxLen);
    return s;
}

/// Заменяет последовательно идущие подряд пробельные символы одним пробелом (обрабатывая не более maxLen байт)
inline Stroka Collapse(const Stroka& s, size_t maxLen = 0) {
    Stroka ret;
    Collapse(s, ret, maxLen);
    return ret;
}
// deprecated version, use Collapse instead
inline Stroka collapse(const Stroka& s, size_t maxLen = 0) {
    return Collapse(s, maxLen);
}

void CollapseText(const Stroka& from, Stroka& to, size_t maxLen);

/// То же самое, что Collapse() + обрезает строку до заданной длины.
/// @details В конец обрезанной строки вставляется многоточие.
inline void CollapseText(Stroka& s, size_t maxLen) {
    Stroka to;
    CollapseText(s, to, maxLen);
    s = to;
}
