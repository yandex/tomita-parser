/*
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 *
 * This code is the same as the code published by RSA Inc.  It has been
 * edited for clarity and style only.
 */

/* c++ class derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm */

#include <cstring>
#include <cstdlib>

#include "md5.h"

#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/string/base64.h>

namespace {
    struct TMd5Stream: public TOutputStream {
        inline TMd5Stream(MD5* md5)
            : M_(md5)
        {
        }

        virtual void DoWrite(const void* buf, size_t len) {
            M_->Update(buf, len);
        }

        MD5* M_;
    };
}

char* MD5::File(const char* filename, char* buf) {
    try {
        TFileInput fi(filename);

        return Stream(&fi, buf);
    } catch (...) {
    }

    return 0;
}

char* MD5::Data(const unsigned char* data, unsigned int len, char* buf) {
    MD5 md5;
    md5.Update(data, len);
    return md5.End(buf);
}

char* MD5::Stream(TInputStream* in, char* buf) {
    return MD5().Update(in).End(buf);
}

MD5& MD5::Update(TInputStream* in) {
    TMd5Stream md5(this);

    TransferData(in, &md5);

    return *this;
}

static void MD5Transform(ui32 [4], const unsigned char [64]);

/*
 * Encodes input (ui32) into output (unsigned char). Assumes len is
 * a multiple of 4.
 */

static void Encode(unsigned char* output, ui32* input, unsigned int len) {
    unsigned int i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (unsigned char)(input[i] & 0xff);
        output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
        output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
        output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
    }
}

/*
 * Decodes input (unsigned char) into output (ui32). Assumes len is
 * a multiple of 4.
 */

static void Decode(ui32* output, const unsigned char* input, unsigned int len)
{
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((ui32)input[j]) | (((ui32)input[j+1]) << 8) |
            (((ui32)input[j+2]) << 16) | (((ui32)input[j+3]) << 24);
}

static unsigned char PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/*
 * FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 * Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
    (a) += F ((b), (c), (d)) + (x) + (ui32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#define GG(a, b, c, d, x, s, ac) { \
    (a) += G ((b), (c), (d)) + (x) + (ui32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#define HH(a, b, c, d, x, s, ac) { \
    (a) += H ((b), (c), (d)) + (x) + (ui32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#define II(a, b, c, d, x, s, ac) { \
    (a) += I ((b), (c), (d)) + (x) + (ui32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }

/* MD5 initialization. Begins an MD5 operation, writing a new context. */

void MD5::Init() {
    count[0] = count[1] = 0;
    /* Load magic initialization constants.  */
    state[0] = 0x67452301;
    state[1] = 0xefcdab89;
    state[2] = 0x98badcfe;
    state[3] = 0x10325476;
}

/*
 * MD5 block update operation. Continues an MD5 message-digest
 * operation, processing another message block, and updating the
 * context.
 */

void MD5::Update(const void* inputPtr, unsigned int inputLen) {
    const unsigned char* input = (const unsigned char*)inputPtr;
    unsigned int i, index, partLen;
    /* Compute number of bytes mod 64 */
    index = (unsigned int)((count[0] >> 3) & 0x3F);
    /* Update number of bits */
    if ((count[0] += ((ui32)inputLen << 3))
        < ((ui32)inputLen << 3))
        count[1]++;
    count[1] += ((ui32)inputLen >> 29);
    partLen = 64 - index;
    /* Transform as many times as possible. */
    if (inputLen >= partLen) {
        memcpy((void *)&buffer[index], (const void *)input, partLen);
        MD5Transform (state, buffer);
        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5Transform (state, &input[i]);
        index = 0;
    } else
        i = 0;
    /* Buffer remaining input */
    memcpy ((void *)&buffer[index], (const void *)&input[i], inputLen-i);
}

/*
 * MD5 padding. Adds padding followed by original length.
 */

void MD5::Pad() {
    unsigned char bits[8];
    unsigned int index, padLen;
    /* Save number of bits */
    Encode (bits, count, 8);
    /* Pad out to 56 mod 64. */
    index = (unsigned int)((count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    Update (PADDING, padLen);
    /* Append length (before padding) */
    Update (bits, 8);
}

/*
 * MD5 finalization. Ends an MD5 message-digest operation, writing the
 * the message digest and zeroizing the context.
 */

unsigned char* MD5::Final(unsigned char digest[16]) {
    /* Do padding. */
    Pad();
    /* Store state in digest */
    Encode(digest, state, 16);
    /* Zeroize sensitive information. */
    memset ((void *)this, 0, sizeof (*this));

    return digest;
}

char* MD5::End(char* buf) {
    int i;
    unsigned char digest[16];
    static const char hex[] = "0123456789abcdef";
    if (!buf)
        buf = (char*)malloc(33);
    if (!buf)
        return 0;
    Final(digest);
    for (i = 0; i < 16; i++) {
        buf[i+i] = hex[digest[i] >> 4];
        buf[i+i+1] = hex[digest[i] & 0x0f];
    }
    buf[i+i] = '\0';
    return buf;
}

char* MD5::End_b64(char* buf) {
    unsigned char digest[16];
    if (!buf)
        buf = (char*)malloc(25);
    if (!buf)
        return 0;
    Final(digest);
    Base64Encode(buf, digest, 16);
    return buf;
}

/* MD5 basic transformation. Transforms state based on block. */

static void MD5Transform(ui32 state[4], const unsigned char block[64]) {
    ui32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    Decode (x, block, 64);

    /* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
    FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
    GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
    HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
    II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    /* Zeroize sensitive information. */
    memset ((void *)x, 0, sizeof (x));
}

Stroka MD5::Calc(const TStringBuf& data) {
    Stroka result;
    result.resize(32);
    MD5 md5;
    md5.Update(~data, +data);
    md5.End(result.begin());
    return result;
}

Stroka MD5::CalcRaw(const TStringBuf& data) {
    Stroka result;
    result.resize(16);
    MD5 md5;
    md5.Update(~data, +data);
    md5.Final(reinterpret_cast<unsigned char*>(result.begin()));
    return result;
}
