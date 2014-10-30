#include "quote.h"
#include "cstriter.h"

#include <util/memory/tempbuf.h>
#include <util/charset/utf.h>
#include <util/charset/recyr.hh>
#include <util/charset/codepage.h>

#include <cctype>

/* note: (x & 0xdf) makes x upper case */
#define GETXC do {                                                      \
    c *= 16;                                                            \
    c += (x[0] >= 'A' ? ((x[0] & 0xdf) - 'A') + 10 : (x[0] - '0'));     \
    ++x;                                                                \
} while (0)

namespace {
    class TFromHexZeroTerm {
    public:
        static inline char x2c(const char *&x) {
            if (!isxdigit((ui8)x[0]) || !isxdigit((ui8)x[1]))
                return '%';
            ui8 c = 0;

            GETXC; GETXC;
            return c;
        }

        static inline char x2c(TStringBuf &x) {
            if (!isxdigit((ui8)x[0]) || !isxdigit((ui8)x[1]))
                return '%';
            ui8 c = 0;

            GETXC; GETXC;
            return c;
        }

        static inline ui16 x2u(const char *&x) {
            if (!isxdigit((ui8)x[0]) || !isxdigit((ui8)x[1]) || !isxdigit((ui8)x[2]) || !isxdigit((ui8)x[3]))
                return '%';
            ui16 c = 0;
            GETXC; GETXC; GETXC; GETXC;
            return c? c : ' ';
        }
    };

    class TFromHexLenLimited {
    public:
        TFromHexLenLimited(const char* end)
            : End(end)
        {
        }

        inline char x2c(const char *&x) {
            if (x + 2 > End)
                return '%';
            return TFromHexZeroTerm::x2c(x);
        }

        inline ui16 x2u(const char *&x) {
            if (x + 4 > End)
                return '%';
            return TFromHexZeroTerm::x2u(x);
        }

    private:
        const char* End;
    };
}

static inline char d2x(unsigned x) {
    return (char)((x < 10) ? ('0'+x) : ('A'+x-10));
}

static inline const char* FixZero(const char* s) throw () {
    return s ? s : "";
}

// we escape:
// '\"', '|', '(', ')',
// '%',  '&', '+', ',',
// '#',  '<', '=', '>',
// '[',  '\\',']', '?',
//  ':', '{', '}',
// all below ' ' (0x20) and above '~' (0x7E).
// ' ' converted to '+'
static const bool chars_to_url_escape[256] = {
//  0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //0
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //1
    0, 0, 1, 1,  0, 1, 1, 1,  1, 1, 0, 1,  1, 0, 0, 0, //2
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0,  1, 1, 1, 1, //3

    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, //4
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 0, //5
    1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, //6
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 1, //7

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //8
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //9
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //A
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //B

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //C
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //D
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //E
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, //F
};

template <class It1, class It2, class It3>
static inline It1 Escape(It1 to, It2 from, It3 end, const bool* escape_map = chars_to_url_escape) {
    while (from != end) {
        if (escape_map[(unsigned char)*from]) {
            *to++ = '%';
            *to++ = d2x((unsigned char)*from >> 4);
            *to++ = d2x((unsigned char)*from & 0xF);
        } else {
            *to++ = (*from == ' ' ? '+' : *from);
        }

        from++;
    }

    *to = 0;

    return to;
}

template <class It1, class It2, class It3, class FromHex>
static inline It1 Unescape(It1 to, It2 from, It3 end, FromHex fromHex, ECharset To) {
    while (from != end) {
        switch (*from) {
        case '%':
            from++;

            if (from != end && *from == 'u') {
                from++;
                wchar32 c = fromHex.x2u(from);
                size_t in_readed = 0;
                size_t out_writed = 0;
                RecodeFromUnicode(To, &c, to, 1, 5, in_readed, out_writed);
                to += out_writed;
            } else {
                *to++ = fromHex.x2c(from);
            }
            break;
        case '+':
            *to++ = ' ';
            from++;
            break;
        default:
            *to++ = *from++;
        }
    }
    *to = 0;
    return to;
}

// CGIEscape returns pointer to the end of the result string
// so as it could be possible to populate single long buffer
// with several calls to CGIEscape in a row.
char* CGIEscape(char* to, const char* from) {
    return Escape(to, FixZero(from), TCStringEndIterator());
}

char* CGIEscape(char* to, const char* from, size_t len) {
    return Escape(to, from, from + len);
}

void CGIEscape(Stroka& url) {
    TTempBuf tempBuf(CgiEscapeBufLen(+url));
    char* to = tempBuf.Data();

    url.AssignNoAlias(to, CGIEscape(to, ~url, +url));
}

Stroka CGIEscapeRet(Stroka url) {
    CGIEscape(url);
    return url;
}

// More general version of CGIEscape. The optional safe parameter specifies
// additional characters that should not be quoted — its default value is '/'.

// Also returns pointer to the end of result string.
char* Quote(char* to, const char* from, const char* safe) {
    bool escape_map[256];
    memcpy(escape_map, chars_to_url_escape, 256);
    // RFC 3986 Uniform Resource Identifiers (URI): Generic Syntax
    // lists following reserved characters:
    const char* reserved = ":/?#[]@!$&\'()*+,;=";
    for (const char* p = reserved; *p; ++p) {
        escape_map[(unsigned char)*p] = 1;
    }
    // characters we think are safe at the moment
    for (const char* p = safe; *p; ++p) {
        escape_map[(unsigned char)*p] = 0;
    }

    return Escape(to, FixZero(from), TCStringEndIterator(), escape_map);
}

void Quote(Stroka& url, const char* safe) {
    TTempBuf tempBuf(CgiEscapeBufLen(+url));
    char* to = tempBuf.Data();

    url.AssignNoAlias(to, Quote(to, ~url, safe));
}

char* CGIUnescape(char *to, const char* from, ECharset To) {
    return Unescape(to, FixZero(from), TCStringEndIterator(), TFromHexZeroTerm(), To);
}

char* CGIUnescape(char* to, const char* from, size_t len, ECharset To) {
    return Unescape(to, from, from + len, TFromHexLenLimited(from + len), To);
}

char* CGIUnescapeX(char *to, const char* from, ECharset To, ECharset From) {
    from = FixZero(from);

    while (*from) {
        switch (*from) {
            case '%':
                from++;
                if (*from == 'u') {
                    from++;
                    wchar32 c = TFromHexZeroTerm::x2u(from);
                    size_t in_readed = 0;
                    size_t out_writed = 0;
                    RecodeFromUnicode(To, &c, to, 1, 5, in_readed, out_writed);
                    to += out_writed;
                } else {
                    size_t read = 0;
                    size_t written = 0;
                    char letter = TFromHexZeroTerm::x2c(from);
                    Recode(From, To, &letter, to, 1, 4, read, written);
                    to += written;
                }
                break;
            case '+':
                *to++ = ' ';
                from++;
                break;
            default: {
                size_t read = 0;
                size_t written = 0;
                Recode(From, To, from++, to, 1, 4, read, written);
                to += written;
            }
        }
    }
    *to = 0;
    return to;
}

void CGIUnescape(Stroka& url, ECharset To) {
    TTempBuf tempBuf(CgiUnescapeBufLen(+url));
    char* to = tempBuf.Data();

    url.AssignNoAlias(to, CGIUnescape(to, ~url, +url, To));
}

char* UrlUnescape(char* to, TStringBuf from) {
    while (!from.Empty())
    {
        char ch = from[0];
        ++from;
        if ('%' == ch && 2 <= from.length())
            ch = TFromHexZeroTerm::x2c(from);
        *to++ = ch;
    }

    *to = 0;

    return to;
}

void UrlUnescape(Stroka& url) {
    TTempBuf tempBuf(CgiUnescapeBufLen(+url));
    char* to = tempBuf.Data();
    url.AssignNoAlias(to, UrlUnescape(to, url));
}

char* UrlEscape(char* to, const char* from) {
    from = FixZero(from);

    while (*from) {
        if ((*from == '%' && !(*(from+1) && isxdigit(*(from+1)) && *(from+2) && isxdigit(*(from+2)))) ||
             (unsigned char)*from <= ' ' || (unsigned char)*from > '~')
        {
            *to++ = '%';
            *to++ = d2x((unsigned char)*from>>4);
            *to++ = d2x((unsigned char)*from&0xF);
        } else
            *to++ = *from;
        from++;
    }

    *to = 0;

    return to;
}

void UrlEscape(Stroka& url) {
    TTempBuf tempBuf(CgiEscapeBufLen(+url));
    char* to = tempBuf.Data();
    url.AssignNoAlias(to, UrlEscape(to, ~url));
}

void UrlDecode(const Stroka& url, Stroka& dest) {
    dest = url;
    CGIUnescape(dest, CODES_UTF8);
    EUTF8Detect utf = UTF8Detect(~dest, +dest);
    if (utf == UTF8)
        dest = RecodeToYandex(CODES_UTF8, dest);
}
