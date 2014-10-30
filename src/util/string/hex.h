#pragma once

#include <util/system/yassert.h>
#include <util/generic/stroka.h>
#include <util/generic/yexception.h>

inline static char DigitToChar(unsigned char digit) {
    if (digit < 10) {
        return (char)digit + '0';
    }

    return (char)(digit - 10) + 'A';
}

extern const char *Char2DigitTable;

inline static int Char2Digit(char ch) {
    char result = Char2DigitTable[(unsigned char)ch];
    ENSURE(result != '\xff', "invalid hex character " << (int)result);
    return result;
}

inline static int Stroka2Byte(const char* s) {
    return Char2Digit(*s) * 16 + Char2Digit(*(s + 1));
}

char* HexEncode(const void* in, size_t len, char* out);

void* HexDecode(const void* in, size_t len, void* ptr);

Stroka HexEncode(const Stroka& h);

Stroka HexEncode(const void* in, size_t len);

Stroka HexDecode(const Stroka& h);
