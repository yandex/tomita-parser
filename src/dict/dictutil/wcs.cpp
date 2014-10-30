#include "wcs.h"
#include "chars.h"

#include <cstdlib>
#include <string>
#include <assert.h>
#include <wctype.h>

////////////////////////////////////////////////////////////////////////////////
//
// wsc-functions
//

size_t wcslen(const wchar16* str) {
    const wchar16* end = str;
    while (*end)
        ++end;
    return end - str;
}

const wchar16* wcschr(const wchar16* str, wchar16 ch) {
    const wchar16* p = str;
    for (;; ++p) {
        if (*p == ch)
            break;
        if (*p == 0)
            return 0;
    }
    return p;
}

int wcscmp(const wchar16* str1, const wchar16* str2) {
    int cmp = 0;
    while ((cmp = *str1 - *str2) == 0 && *str1) {
        ++str1;
        ++str2;
    }
    return cmp;
}

int wcsncmp(const wchar16* str1, const wchar16* str2, size_t n) {
    int cmp = 0;
    while (n && (cmp = *str1 - *str2) == 0 && *str1) {
        ++str1;
        ++str2;
        --n;
    }
    return cmp;
}

////////////////////////////////////////////////////////////////////////////////
//
// wcstox
//

namespace {

union TVar {
    long L;
    double D;
};

inline int check(const ui32 mask[], wchar16 ch) {
    return mask[ch >> 5] & (1U << (ch & 0x1f));
}

static void read(const ui32 mask[], std::string& str, const wchar16* p) {
    while (*p < 0x80) {
        if (!check(mask, *p))
            break;
        str.push_back((char)*p++);
    }
}

void wcstox(char type, const wchar16* ptr, wchar16** endptr, int base, TVar* result) {
    static const ui32 maskInt[4] = { 0x0, 0x3ff2800, 0x7fffffe, 0x7fffffe }; // "0-9A-Za-z+-"
    static const ui32 maskFloat[4] = { 0x0, 0x3ff6800, 0x20, 0x20 }; // "0-9.Ee+-"

    // Skip space
    for (; iswspace(*ptr); ++ptr)
        ;

    std::string str;
    char* end = 0;
    char** pEnd = (endptr ? &end : 0);
    switch (type) {
        case 'l':
            read(maskInt, str, ptr);
            result->L = strtol(str.c_str(), pEnd, base);
            break;
        case 'd':
            read(maskFloat, str, ptr);
            result->D = strtod(str.c_str(), pEnd);
            break;
        default:
            assert(false);
    }
    if (endptr)
        *endptr = const_cast<wchar16*>(ptr + (end - str.c_str()));
}

} // anonymous namespace

long wcstol(const wchar16* ptr, wchar16** endptr, int base) {
    TVar result;
    wcstox('l', ptr, endptr, base, &result);
    return result.L;
}

double wcstod(const wchar16* ptr, wchar16** endptr) {
    TVar result;
    wcstox('d', ptr, endptr, 0, &result);
    return result.D;
}
