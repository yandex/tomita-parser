#pragma once

#include <util/generic/stroka.h>

/// Заменяет в строке одни подстроки на другие.
template <class Troka>
size_t SubstGlobal(Troka& s, const Troka& from, const Troka& to, size_t fromPos = 0) {
    if (from.empty())
        return 0;

    size_t result = 0;
    for (size_t off = fromPos; (off = s.find(from, off)) != Troka::npos; off += to.length()) {
        s.replace(off, from.length(), to);
        ++result;
    }
    return result;
}

inline size_t SubstGlobal(Stroka& s, const Stroka& from, const Stroka& to, size_t fromPos = 0) {
    return SubstGlobal<Stroka>(s, from, to, fromPos);
}

/// Заменяет в строке одни подстроки на другие.
inline size_t SubstGlobal(Stroka& s, char from, char to, size_t fromPos = 0) {
    if (fromPos >= s.size())
        return 0;
    size_t result = 0;
    for (char* it = s.begin() + fromPos; it != s.end(); ++it) {
        if (*it == from) {
            *it = to;
            ++result;
        }
    }
    return result;
}
