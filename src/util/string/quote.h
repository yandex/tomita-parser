#pragma once

#include <util/generic/strbuf.h>
#include <util/charset/doccodes.h>

class Stroka;

char* CGIEscape(char* to, const char* from);
char* CGIEscape(char* to, const char* from, size_t len);
inline char* CGIEscape(char* to, const TStringBuf& from) {
    return CGIEscape(to, ~from, +from);
}
void CGIEscape(Stroka& url);
Stroka CGIEscapeRet(Stroka url);

inline TStringBuf CgiEscapeBuf(char* to, const TStringBuf& from) {
    return TStringBuf(to, CGIEscape(to, ~from, +from));
}
inline TStringBuf CgiEscape(void* tmp, const TStringBuf& s) {
    return CgiEscapeBuf(static_cast<char *>(tmp), s);
}

char* CGIUnescape(char* to, const char* from, ECharset To = CODES_UNKNOWN);
char* CGIUnescapeX(char* to, const char* from, ECharset To, ECharset From);
char* CGIUnescape(char* to, const char* from, size_t len, ECharset To = CODES_UNKNOWN);
void CGIUnescape(Stroka& url, ECharset To = CODES_UNKNOWN);

inline TStringBuf CgiUnescapeBuf(char* to, const TStringBuf& from, ECharset cs = CODES_UNKNOWN) {
    return TStringBuf(to, CGIUnescape(to, ~from, +from, cs));
}
inline TStringBuf CgiUnescape(void* tmp, const TStringBuf& s, ECharset To = CODES_UNKNOWN) {
    return CgiUnescapeBuf(static_cast<char *>(tmp), s, To);
}

char* Quote(char *to, const char* from, const char* safe = "/");
void Quote(Stroka& url, const char* safe = "/");

char* UrlEscape(char* to, const char* from);
void UrlEscape(Stroka& url);
char* UrlUnescape(char* to, TStringBuf from);
void UrlUnescape(Stroka& url);
void UrlDecode(const Stroka& url, Stroka& dest);

static inline size_t CgiEscapeBufLen(size_t len) throw () {
    return 3 * len + 1;
}

static inline size_t CgiUnescapeBufLen(size_t len) throw () {
    return len + 1;
}
