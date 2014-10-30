#pragma once

#include <util/system/defaults.h>

inline bool IsAsciiSpace(char c) {
    extern const unsigned char asciiSpaces[256];
    return asciiSpaces[(unsigned char)c];
}

inline bool IsAsciiSpace(wchar16 c) {
    return (c < 127) && IsAsciiSpace((char)(unsigned char)c);
}
