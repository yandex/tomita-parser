#include "utf.h"
#include "codepage.h"

EUTF8Detect UTF8Detect(const char* s, size_t len) {
    const unsigned char *s0 = (const unsigned char *)s;
    const unsigned char *send = s0 + len;
    wchar32 rune;
    size_t rune_len;
    EUTF8Detect res = ASCII;

    while (s0 < send) {
        RECODE_RESULT rr = utf8_read_rune(rune, rune_len, s0, send);

        if (rr != RECODE_OK) {
            return NotUTF8;
        }

        if (rune_len > 1) {
            res = UTF8;
        }

        s0 += rune_len;
    }

    return res;
}

bool ToLowerUTF8Impl(const char* beg, size_t n, Stroka& newString) {
    const unsigned char* p = (const unsigned char*)beg;
    const unsigned char* const end = p + n;

    //first loop searches for the first character, which is changed by ToLower
    //if there is no changed character, we don't need reallocation/copy
    wchar32 cNew = 0;
    size_t cLen = 0;
    while (p < end) {
        wchar32 c;
        if (RECODE_OK != utf8_read_rune(c, cLen, p, end)) {
            ythrow yexception() << "failed to decode UTF-8 string at pos " << ((const char*)p - beg);
        }
        cNew = ToLower(c);

        if (cNew != c)
            break;
        p += cLen;
    }
    if (p == end) {
        return false;
    }

    //some character changed after ToLower. Write new string to newString.
    newString.resize(n);

    size_t written = (char*)p - beg;
    char* writePtr = newString.begin();
    memcpy(writePtr, beg, written);
    writePtr += written;
    size_t destSpace = n - written;

    //before each iteration (including the first one) variable 'cNew' contains unwritten symbol
    while (1) {
        size_t cNewLen;
        YASSERT((writePtr - ~newString) + destSpace == +newString);
        if (RECODE_EOOUTPUT == utf8_put_rune(cNew, cNewLen, (unsigned char*)writePtr, destSpace)) {
            destSpace += +newString;
            newString.resize(+newString * 2);
            writePtr = newString.begin() + (+newString - destSpace);
            continue;
        }
        destSpace -= cNewLen;
        writePtr += cNewLen;
        p += cLen;
        if (p == end) {
            newString.resize(+newString - destSpace);
            return true;
        }
        wchar32 c = 0;
        if (RECODE_OK != utf8_read_rune(c, cLen, p, end)) {
            ythrow yexception() << "failed to decode UTF-8 string at pos " << ((const char*)p - beg);
        }
        cNew = ToLower(c);
    }
    YASSERT(false);
}


Stroka ToLowerUTF8(const Stroka& s) {
    Stroka newString;
    bool changed = ToLowerUTF8Impl(~s, +s, newString);
    return changed ? newString : s;
}

Stroka ToLowerUTF8(TStringBuf s) {
    Stroka newString;
    bool changed = ToLowerUTF8Impl(~s, +s, newString);
    return changed ? newString : Stroka(~s, s);
}

Stroka ToLowerUTF8(const char *s) {
    return ToLowerUTF8(TStringBuf(s));
}

