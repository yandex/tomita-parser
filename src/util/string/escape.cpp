#include "escape.h"

#include <util/system/defaults.h>
#include <util/string/cast.h>
#include <util/charset/wide.h>

/* REFEREBCES FOR ESCAPE SEQUENCE INTERPRETATION:
 *   C99 p. 6.4.3   Universal character names.
 *   C99 p. 6.4.4.4 Character constants.
 *
 * <simple-escape-sequence> ::= {
 *      \' , \" , \? , \\ ,
 *      \a , \b , \f , \n , \r , \t , \v
 * }
 *
 * <octal-escape-sequence>       ::= \  <octal-digit> {1, 3}
 * <hexadecimal-escape-sequence> ::= \x <hexadecimal-digit> +
 * <universal-character-name>    ::= \u <hexadecimal-digit> {4}
 *                                || \U <hexadecimal-digit> {8}
 *
 * NOTE (6.4.4.4.7):
 * Each octal or hexadecimal escape sequence is the longest sequence of characters that can
 * constitute the escape sequence.
 *
 * THEREFORE:
 *  - Octal escape sequence spans until rightmost non-octal-digit character.
 *  - Octal escape sequence always terminates after three octal digits.
 *  - Hexadecimal escape sequence spans until rightmost non-hexadecimal-digit character.
 *  - Universal character name consists of exactly 4 or 8 hexadecimal digit.
 *
 */
template <typename TChar>
static inline char HexDigit(TChar value) {
    YASSERT(value < 16);
    if (value < 10)
        return '0' + value;
    else
        return 'A' + value - 10;
}

template <typename TChar>
static inline char OctDigit(TChar value) {
    YASSERT(value < 8);
    return '0' + value;
}

template <typename TChar>
static inline bool IsPrintable(TChar c) {
    return c >= 32 && c <= 126;
}

template <typename TChar>
static inline bool IsHexDigit(TChar c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

template <typename TChar>
static inline bool IsOctDigit(TChar c) {
    return  c >= '0' && c <= '7';
}

template <typename TChar>
struct TEscapeUtil;

template <>
struct TEscapeUtil<char> {
    static const size_t ESCAPE_C_BUFFER_SIZE = 4;

    template <typename TNextChar, typename TBufferChar>
    static inline size_t EscapeC(unsigned char c, TNextChar next, TBufferChar r[ESCAPE_C_BUFFER_SIZE]) {
        // (1) Printable characters go as-is, except backslash and double quote.
        // (2) Characters \r, \n, \t and \0 ... \7 replaced by their simple escape characters (if possible).
        // (3) Otherwise, character is encoded using hexadecimal escape sequence (if possible), or octal.
        if (c == '\"') {
            r[0] = '\\';
            r[1] = '\"';
            return 2;
        } else if (c == '\\') {
            r[0] = '\\';
            r[1] = '\\';
            return 2;
        } else if (IsPrintable(c)) {
            r[0] = c;
            return 1;
        } else if (c == '\r') {
            r[0] = '\\';
            r[1] = 'r';
            return 2;
        } else if (c == '\n') {
            r[0] = '\\';
            r[1] = 'n';
            return 2;
        } else if (c == '\t') {
            r[0] = '\\';
            r[1] = 't';
            return 2;
       } else if (c < 8 && !IsOctDigit(next)) {
            r[0] = '\\';
            r[1] = OctDigit(c);
            return 2;
        } else if (!IsHexDigit(next)) {
            r[0] = '\\';
            r[1] = 'x';
            r[2] = HexDigit((c & 0xF0) >> 4);
            r[3] = HexDigit((c & 0x0F) >> 0);
            return 4;
        } else {
            r[0] = '\\';
            r[1] = OctDigit((c & 0700) >> 6);
            r[2] = OctDigit((c & 0070) >> 3);
            r[3] = OctDigit((c & 0007) >> 0);
            return 4;
        }
    }
};

template <>
struct TEscapeUtil<wchar16> {
    static const size_t ESCAPE_C_BUFFER_SIZE = 6;

    template <typename TNextChar, typename TBufferChar>
    static inline size_t EscapeC(wchar16 c, TNextChar next, TBufferChar r[ESCAPE_C_BUFFER_SIZE]) {
        if (c < 0x100) {
            return TEscapeUtil<char>::EscapeC(char(c), next, r);
        } else {
            r[0] = '\\';
            r[1] = 'u';
            r[2] = HexDigit((c & 0xF000) >> 12);
            r[3] = HexDigit((c & 0x0F00) >> 8);
            r[4] = HexDigit((c & 0x00F0) >> 4);
            r[5] = HexDigit((c & 0x000F) >> 0);
            return 6;
        }
    }
};

template <class TChar>
typename TCharStringTraits<TChar>::TString& EscapeCImpl(const TChar* str, size_t len, typename TCharStringTraits<TChar>::TString& r) {
    typedef ::TEscapeUtil<TChar> TEscapeUtil;

    TChar buffer[TEscapeUtil::ESCAPE_C_BUFFER_SIZE];

    size_t i, j;
    for (i = 0, j = 0; i < len; ++i) {
        size_t rlen = TEscapeUtil::EscapeC(str[i], (i + 1 < len ? str[i + 1] : 0), buffer);

        if (rlen > 1) {
            r.append(str + j, i - j);
            j = i + 1;
            r.append(buffer, rlen);
        }
    }

    if (j > 0) {
        r.append(str + j, len - j);
    } else {
        r.append(str, len);
    }

    return r;
}

template Stroka& EscapeCImpl<Stroka::TChar>(const Stroka::TChar* str, size_t len, Stroka& r);
template Wtroka& EscapeCImpl<Wtroka::TChar>(const Wtroka::TChar* str, size_t len, Wtroka& r);

inline void AppendUnicode(Stroka& s, ui32 v) {
    const size_t oldsz = s.size();
    s.resize(oldsz + 4);
    size_t sz = 0;
    NDetail::WriteUTF8Char(v, sz, (ui8*) s.end() - 4);
    s.resize(oldsz + sz);
}

inline void AppendUnicode(Wtroka& s, ui32 v) {
    WriteSymbol(v, s);
}

template <ui32 sz, typename TChar>
inline size_t CountHex(const TChar* p, const TChar* pe) {
    ui32 maxsz = Min<size_t>(sz, pe - p);

    for (ui32 i = 0; i < maxsz; ++i, ++p) {
        if (!IsHexDigit(*p))
            return i;
    }

    return maxsz;
}

template <ui32 sz, typename TChar>
inline size_t CountOct(const TChar* p, const TChar* pe) {
    ui32 maxsz = Min<size_t>(sz, pe - p);

    if (3 == sz && 3 == maxsz && !(*p >= '0' && *p <= '3'))
        maxsz = 2;

    for (ui32 i = 0; i < maxsz; ++i, ++p) {
        if (!IsOctDigit(*p))
            return i;
    }

    return maxsz;
}

template <class Char>
typename TCharStringTraits<Char>::TString& UnescapeCImpl(const Char* p, size_t sz,
                                                        typename TCharStringTraits<Char>::TString& res) {
    const Char* pe = p + sz;

    for (;p != pe; ++p) {
        if ('\\' == *p) {
            ++p;

            if (p == pe)
                return res;

            switch (*p) {
            default:
                res.append(*p);
                break;
            case 'b':
                res.append('\b');
                break;
            case 'f':
                res.append('\f');
                break;
            case 'n':
                res.append('\n');
                break;
            case 'r':
                res.append('\r');
                break;
            case 't':
                res.append('\t');
                break;
            case 'u':
                if (CountHex<4>(p + 1, pe) != 4) {
                    res.append(*p);
                } else {
                    AppendUnicode(res, IntFromString<ui32, 16>(p + 1, 4));
                    p += 4;
                }
                break;
            case 'U':
                if (CountHex<8>(p + 1, pe) != 8)
                    res.append(*p);
                else
                    AppendUnicode(res, IntFromString<ui32, 16>(p + 1, 8));
                break;
            case 'x':
                if (ui32 v = CountHex<2>(p + 1, pe)) {
                    res.append((Char)IntFromString<ui32, 16>(p + 1, v));
                    p += v;
                } else {
                    res.append(*p);
                }

                break;
            case '0':
            case '1':
            case '2':
            case '3':
                if (ui32 v = CountOct<3>(p, pe)) {
                    res.append((Char)IntFromString<ui32, 8>(p, v));
                    p += v - 1;
                } else {
                    res.append(*p);
                }

                break;
            case '4':
            case '5':
            case '6':
            case '7':
                if (ui32 v = CountOct<2>(p, pe)) {
                    res.append((Char)IntFromString<ui32, 8>(p, v));
                    p += v - 1;
                } else {
                    res.append(*p);
                }

                break;
            }
        } else
            res.append(*p);
    }

    return res;
}

template Stroka& UnescapeCImpl<Stroka::TChar>(const Stroka::TChar* str, size_t len, Stroka& r);
template Wtroka& UnescapeCImpl<Wtroka::TChar>(const Wtroka::TChar* str, size_t len, Wtroka& r);

Stroka& EscapeC(const Stroka& str, Stroka& s) {
    return EscapeC(~str, +str, s);
}

Wtroka& EscapeC(const Wtroka& str, Wtroka& w) {
    return EscapeC(~str, +str, w);
}

Stroka EscapeC(const Stroka& str) {
    return EscapeC(~str, +str);
}

Wtroka EscapeC(const Wtroka& str) {
    return EscapeC(~str, +str);
}

Stroka& UnescapeC(const Stroka& str, Stroka& s) {
    return UnescapeC(~str, +str, s);
}

Wtroka& UnescapeC(const Wtroka& str, Wtroka& w) {
    return UnescapeC(~str, +str, w);
}

Stroka UnescapeC(const Stroka& str) {
    return UnescapeC(~str, +str);
}

Wtroka UnescapeC(const Wtroka& str) {
    return UnescapeC(~str, +str);
}
