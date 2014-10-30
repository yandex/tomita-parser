#include "dictutil.h"
#include "chars.h"
#include "charset.h"
#include "str.h"

////////////////////////////////////////////////////////////////////////////////
//
// WideToChar
//

Stroka WideToChar(ECharset charset, const wchar16* wstr, size_t cch) {
    const TCharset* encoder = TCharset::ForCode(charset);

    if (cch == (size_t)-1)
        cch = wcslen(wstr);

    char bytes[1024];
    const wchar16* wptr = wstr;
    const wchar16* wend = wstr + cch;
    Stroka str;
    str.reserve(cch);
    while (wptr < wend) {
        size_t charsUsed = 0, bytesUsed = 0;
        encoder->Encode(wptr, wend - wptr, bytes, ARRAY_SIZE(bytes), &charsUsed, &bytesUsed);
        wptr += charsUsed;
        str.append(bytes, bytesUsed);
    }
    return str;
}

Stroka WideToChar(ECharset charset, const TWStringBuf& wstr) {
    return WideToChar(charset, wstr.c_str(), wstr.length());
}

void WideToChar(ECharset charset, const VectorWStrok& wstr, VectorStrok* result) {
    result->clear();
    result->reserve(wstr.size());
    for (int i = 0; i < (int)wstr.size(); ++i)
        result->push_back(WideToChar(charset, wstr[i]));
}

bool WideToCharEx(ECharset charset, const WStroka& str, Stroka* result) {
    static const char UNKNOWN_CHARS[] = { yaUNK_Up, yaUNK_Lo, yaIDEOGR, 0 };

    *result = WideToChar(charset, str);
    if (charset != CODES_UTF8) {
        Stroka::const_iterator it = result->begin();
        int cQuest = (int)std::count(it, result->end(), '?');
        if (cQuest && cQuest != (int)std::count(str.begin(), str.end(), '?'))
            return false;

        if (charset == CODES_YANDEX && FindAnyOf(*result, UNKNOWN_CHARS) != Stroka::npos)
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// CharToWide
//

WStroka CharToWide(ECharset charset, const char* str, size_t cb) {
    const TCharset* decoder = TCharset::ForCode(charset);

    if (cb == (size_t)-1)
        cb = strlen(str);

    wchar16 chars[512];
    const char* ptr = str;
    const char* end = str + cb;
    WStroka wstr;
    wstr.reserve(cb);
    while (ptr < end) {
        size_t bytesUsed = 0, charsUsed = 0;
        decoder->Decode(ptr, end - ptr, true, chars, ARRAY_SIZE(chars), &bytesUsed, &charsUsed);
        ptr += bytesUsed;
        wstr.append(chars, chars + charsUsed);
    }
    return wstr;
}

WStroka CharToWide(ECharset charset, const TStringBuf& str) {
    return CharToWide(charset, str.c_str(), str.length());
}

void CharToWide(ECharset charset, const VectorStrok& str, VectorWStrok* result) {
    result->clear();
    result->reserve(str.size());
    for (int i = 0; i < (int)str.size(); ++i)
        result->push_back(CharToWide(charset, str[i]));
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert
//

Stroka Convert(ECharset from, ECharset to, const Stroka& bytes) {
    return WideToChar(to, CharToWide(from, bytes));
}
