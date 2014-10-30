#include "base64.h"

#include <util/generic/yexception.h>

static const char base64_etab_std[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64_bkw[] =
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\76\0\76\0\77\64\65\66\67\70\71\72\73\74\75\0\0\0\0\0\0"
"\0\0\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31\0\0\0\0\77"
"\0\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\60\61\62\63\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

// Base64 for url encoding, RFC3548
static const char base64_etab_url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

template <bool urlVersion>
static inline char* Base64EncodeImpl(char* outstr, const unsigned char* instr, size_t len) {
    const char* base64_etab = (urlVersion ? base64_etab_url : base64_etab_std);
    const char last = (urlVersion ? ',' : '=');

    size_t idx = 0;

    while (idx < len) {
        if (idx % 3 == 0) {
            *outstr++ = base64_etab[(int)(instr[idx] >> 2)];
        } else if (idx % 3 == 1) {
            *outstr++ = base64_etab[(int)(((instr[idx - 1] << 4) & 0x30) | ((instr[idx] >> 4) & 0x0f))];
            char c3 = idx + 1 < len ? instr[idx + 1] : '\0';
            *outstr++ = base64_etab[(int)(((instr[idx] << 2) & 0x3c) | ((c3 >> 6) & 0x03))];
        } else {
            *outstr++ = base64_etab[(int)(instr[idx] & 0x3f)];
        }

        idx++;
    }

    if (idx % 3 == 1) {
        *outstr++ = base64_etab[(int)((instr[idx - 1] << 4) & 0x30)];
        *outstr++ = last;
        *outstr++ = last;
    } else if (idx % 3 == 2) {
        *outstr++ = last;
    }

    *outstr = 0;

    return outstr;
}

char* Base64Encode(char* outstr, const unsigned char* instr, size_t len) {
    return Base64EncodeImpl<false>(outstr, instr, len);
}

char* Base64EncodeUrl(char* outstr, const unsigned char* instr, size_t len) {
    return Base64EncodeImpl<true>(outstr, instr, len);
}

inline void uudecode_1(char *dst, unsigned char *src) {
    dst[0] = char((base64_bkw[src[0]] << 2) | (base64_bkw[src[1]] >> 4));
    dst[1] = char((base64_bkw[src[1]] << 4) | (base64_bkw[src[2]] >> 2));
    dst[2] = char((base64_bkw[src[2]] << 6) | base64_bkw[src[3]]);
}

size_t Base64Decode(void* dst, const char* b, const char* e) {
    size_t n = 0;

    if ((e - b) % 4) {
        ythrow yexception() << "incorrect input length for base64 decode";
    }

    while (b < e) {
        uudecode_1((char*)dst + n, (unsigned char*)b);

        b += 4;
        n += 3;
    }

    if (n > 0) {
        if (b[-1] == ','  || b[-1] == '=') {
            n--;

            if (b[-2] == ',' || b[-2] == '=') {
                n--;
            }
        }
    }

    return n;
}
