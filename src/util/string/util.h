#pragma once

//THIS FILE A COMPAT STUB HEADER

#include <cstring>
#include <cstdarg>
#include <algorithm>

#include <util/system/defaults.h>
#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

#include "base64.h"
#include "encodexml.h"
#include "ascii.h"
#include "strip.h"
#include "subst.h"
#include "cat.h"
#include "type.h"
#include "pcdata.h"
#include "printf.h"
#include "cast.h"

/// @addtogroup Strings_Miscellaneous
/// @{

int a2i(const Stroka& s);

//Remove last character if it is equal to c.
template <class T>
inline void RemoveIfLast(T& s, int c) {
    const size_t length = s.length();
    if (length && s[length - 1] == c)
        s.remove(length - 1);

}

/// Добавляет в конец строки заданный символ, если этот символ уже не заканчивает строку.
inline void addIfNotLast(Stroka& s, int lastCh) {
    size_t len = s.length();
    if (!len || s[len-1] != lastCh) {
        s.append(char(lastCh));
    }
}

/// @details Если lastCh2 отсутствует в строке, и строка не завершается lastCh1, завершить строку lastCh1,
/// в противному случае, если lastCh2 не предпоследний, завершить строку lastCh2.
/// @todo ?? Определить, когда нужно применять эту функцию. Используется несколько раз для разбора URL.
inline void addIfAbsent(Stroka& s, char lastCh1, char lastCh2) {
    size_t pos = s.find(lastCh2);
    if (pos == Stroka::npos) {
        //s.append((char)lastCh1);
        addIfNotLast(s, lastCh1);
    } else if (pos < s.length()-1) {
        addIfNotLast(s, lastCh2);
    }
}

/// @}

/*
 * ------------------------------------------------------------------
 *
 * A fast implementation of glibc's functions;
 *      strspn, strcspn and strpbrk.
 *
 * ------------------------------------------------------------------
 */
struct ui8_256 {
    // forward chars table
    ui8 chars_table[256];
    // reverse (for c* functions) chars table
    ui8 c_chars_table[256];
};

class str_spn : public ui8_256 {
  public:
    explicit str_spn(const char* charset, bool extended = false) {
        // exteneded: if true, treat charset string more like
        // interior of brackets [ ], e.g. "a-z0-9"
        init(charset, extended);
    }

    /// Return first character in table, like strpbrk()
    /// That is, skip all characters not in table
    /// [DIFFERENCE FOR NOT_FOUND CASE: Returns end of string, not NULL]
    const char* brk(const char* s) const {
        while (c_chars_table[(ui8)*s])
            s++;
        return s;
    }

    const char* brk(const char* s, const char* e) const {
        while (s < e && c_chars_table[(ui8)*s])
            s++;
        return s;
    }

    /// Return first character not in table, like strpbrk() for inverted table.
    /// That is, skip all characters in table
    const char* cbrk(const char* s) const {
        while (chars_table[(ui8)*s])
            s++;
        return s;
    }

    const char* cbrk(const char* s, const char* e) const {
        while (s < e && chars_table[(ui8)*s])
            s++;
        return s;
    }

    /// Offset of the first character not in table, like strspn().
    size_t spn(const char* s) const {
        return cbrk(s) - s;
    }

    size_t spn(const char* s, const char* e) const {
        return cbrk(s, e) - s;
    }

    /// Offset of the first character in table, like strcspn().
    size_t cspn(const char* s) const {
        return brk(s) - s;
    }

    size_t cspn(const char* s, const char* e) const {
        return brk(s, e) - s;
    }

    char* brk(char* s) const {
        return const_cast<char*>(brk((const char*)s));
    }

    char* cbrk(char* s) const {
        return const_cast<char*>(cbrk((const char*)s));
    }

    /// See strsep [BUT argument is *&, not **]
    char *sep(char *&s) const {
        char sep_char; // unused;
        return sep(s, sep_char);
    }

    /// strsep + remember character that was destroyed
    char* sep(char*& s, char& sep_char) const {
        if (!s)
            return 0;
        char *ret = s;
        char *next = brk(ret);
        if (*next) {
            sep_char = *next;
            *next = 0;
            s = next + 1;
        } else {
            sep_char = 0;
            s = 0;
        }
        return ret;
    }

protected:
    void init(const char* charset, bool extended);
    str_spn() {}
};

template <typename T>
inline bool SepParam(char** s1, T& value) {
    char* s2 = strsep(s1, ".");
    if (s2)
        value = *s2 == 0 ? 0 : FromString<T>(s2);
    return (*s1 != 0);
}

// an analogue of tr/$from/$to/
class Tr {
public:
    Tr(const char *from, const char *to);

    char ConvertChar(char ch) const {
        return Map[(ui8)ch];
    }

    void Do(char *s) const {
        for (; *s; s++)
            *s = ConvertChar(*s);
    }
    void Do(const char *src, char *dst) const {
        for (; *src; src++)
            *dst++ = ConvertChar(*src);
        *dst = 0;
    }
    void Do(char *s, size_t l) const {
        for (size_t i = 0; i < l && s[i]; i++)
            s[i] = ConvertChar(s[i]);
    }
    void Do(Stroka& str) const;

private:
    char Map[256];

    size_t FindFirstChangePosition(const Stroka& str) const;
};

// Removes all occurrences of given character from string
template<typename TStroka>
void RemoveAll(TStroka& str, typename TStroka::char_type ch) {
    size_t pos = str.find(ch); // 'find' to avoid cloning of string in 'Stroka.begin()'
    if (pos == TStroka::npos)
        return;

    typename TStroka::iterator begin = str.begin();
    typename TStroka::iterator end = begin + str.length();
    typename TStroka::iterator it = NStl::remove(begin + pos, end, ch);
    str.erase(it, end);
}
