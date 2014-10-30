#include "chartraits.h"

#include <util/string/strspn.h>

static inline const char* FindChr(const char* s, char c, size_t len) throw () {
    const char* ret = TCharTraits<char>::Find(s, c, len);

    return ret ? ret : (s + len);
}

static inline const char* FindTwo(const char* s, const char* c, size_t len) throw () {
    const char* e = s + len;

    while (s != e && *s != c[0] && *s != c[1]) {
        ++s;
    }

    return s;
}

const char* FastFindFirstOf(const char* s, size_t len, const char* set, size_t setlen) {
    switch (setlen) {
        case 0:
            return s + len;

        case 1:
            return FindChr(s, *set, len);

        case 2:
            return FindTwo(s, set, len);

        default:
            return TCompactStrSpn(set, set + setlen).FindFirstOf(s, s + len);
    }
}

const char* FastFindFirstNotOf(const char* s, size_t len, const char* set, size_t setlen) {
    return TCompactStrSpn(set, set + setlen).FindFirstNotOf(s, s + len);
}
