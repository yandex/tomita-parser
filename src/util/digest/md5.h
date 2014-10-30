#pragma once

#include <util/system/defaults.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

class TInputStream;

class MD5 {
public:
    MD5() {
        Init();
    }

    void  Init();
    void  Update(const void* data, unsigned int len);
    void  Pad();
    unsigned char* Final(unsigned char [16]);

    // buf must be char[33];
    char* End(char* buf);

    // buf must be char[25];
    char* End_b64(char* buf);

    MD5& Update(TInputStream* in);

    // return hex-encoded data
    static char* File(const char* filename, char* buf);
    static char* Data(const unsigned char* data, unsigned int len, char* buf);
    static char* Stream(TInputStream* in, char* buf);

    static Stroka Calc(const TStringBuf& data);     // 32-byte hex-encoded
    static Stroka CalcRaw(const TStringBuf& data);  // 16-byte raw

private:
    ui32 state[4];    /* state (ABCD) */
    ui32 count[2];    /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64]; /* input buffer */
};
