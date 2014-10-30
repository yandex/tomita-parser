#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>

enum EUTF8Detect {
      NotUTF8
    , UTF8
    , ASCII
};

EUTF8Detect UTF8Detect(const char* s, size_t len);

inline EUTF8Detect UTF8Detect(const TStringBuf& input) {
    return UTF8Detect(~input, +input);
}

inline bool IsUtf(const char* input, size_t len) {
    return UTF8Detect(input, len) != NotUTF8;
}

inline bool IsUtf(const TStringBuf& input) {
    return IsUtf(~input, +input);
}

//! returns true, if result is not the same as input, and put it in newString
//! returns false, if result is unmodified
bool ToLowerUTF8Impl(const char* beg, size_t n, Stroka& newString);

Stroka ToLowerUTF8(const Stroka& s);
Stroka ToLowerUTF8(TStringBuf s);
Stroka ToLowerUTF8(const char *s);
