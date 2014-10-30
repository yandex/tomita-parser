#pragma once

namespace NCmp {

enum EResult { EQUAL, BETTER, WORSE };

inline EResult MoreIsWorse(size_t a1, size_t a2) {
    if (a1 > a2)
        return WORSE;
    else if (a1 == a2)
        return EQUAL;
    else
        return BETTER;
}

inline EResult MoreIsBetter(size_t a1, size_t a2) {
    return MoreIsWorse(a2, a1);
}

inline EResult TrueIsBetter(bool a1, bool a2) {
    if (a1)
        return a2 ? EQUAL : BETTER;
    else
        return a2 ? BETTER : EQUAL;
}

inline EResult NullIsWorse(const void* p1, const void* p2) {
    return TrueIsBetter(p1, p2);
}


}   // namespace NCmp
