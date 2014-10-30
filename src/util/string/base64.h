#pragma once

#include <util/system/defaults.h>
#include <util/generic/strbuf.h>

#define BASE64_DECODE_BUF_SIZE(len) ((len + 3) / 4 * 3)
#define BASE64_ENCODE_BUF_SIZE(len) ((len + 2) / 3 * 4 + 1)

//decode
inline size_t Base64DecodeBufSize(size_t len) {
    return BASE64_DECODE_BUF_SIZE(len);
}

size_t Base64Decode(void* dst, const char* b, const char* e);

inline TStringBuf Base64Decode(const TStringBuf& src, void* tmp) {
    return TStringBuf((const char*)tmp, Base64Decode(tmp, src.begin(), src.end()));
}

inline void Base64Decode(const TStringBuf& src, Stroka& dst) {
    dst.ReserveAndResize(Base64DecodeBufSize(src.size()));
    dst.resize(Base64Decode(src, dst.begin()).size());
}

inline Stroka Base64Decode(const TStringBuf& s) {
    Stroka ret;
    Base64Decode(s, ret);
    return ret;
}

//encode
inline size_t Base64EncodeBufSize(size_t len) {
    return BASE64_ENCODE_BUF_SIZE(len);
}

char* Base64Encode(char* outstr, const unsigned char* instr, size_t len);
char* Base64EncodeUrl(char* outstr, const unsigned char* instr, size_t len);

inline TStringBuf Base64Encode(const TStringBuf& src, void* tmp) {
    return TStringBuf((const char*)tmp, Base64Encode((char*)tmp, (const unsigned char*)~src, +src));
}

inline TStringBuf Base64EncodeUrl(const TStringBuf& src, void* tmp) {
    return TStringBuf((const char*)tmp, Base64EncodeUrl((char*)tmp, (const unsigned char*)~src, +src));
}

inline void Base64Encode(const TStringBuf& src, Stroka& dst) {
    dst.ReserveAndResize(Base64EncodeBufSize(src.size()));
    dst.resize(Base64Encode(src, dst.begin()).size());
}

inline void Base64EncodeUrl(const TStringBuf& src, Stroka& dst) {
    dst.ReserveAndResize(Base64EncodeBufSize(src.size()));
    dst.resize(Base64EncodeUrl(src, dst.begin()).size());
}

inline Stroka Base64Encode(const TStringBuf& s) {
    Stroka ret;
    Base64Encode(s, ret);
    return ret;
}

inline Stroka Base64EncodeUrl(const TStringBuf& s) {
    Stroka ret;
    Base64EncodeUrl(s, ret);
    return ret;
}
