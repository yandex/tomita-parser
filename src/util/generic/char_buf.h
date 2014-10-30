#pragma once

#include <util/system/compat.h>
#include <util/system/yassert.h>

#if defined(REDUCE_STACK_USAGE)
    #include <new>
    #include "yptr.h"
#endif

template<int N>
class TCharBuf
{
#if defined(REDUCE_STACK_USAGE)
    THolder<void> sz;
#else
    char sz[N];
#endif
public:
    enum {
        BUF_LEN = N
    };
    TCharBuf() { allocate(); *data() = 0; }
    TCharBuf(const char *psz) { allocate(); strlcpy(data(), psz, N); }
    TCharBuf(const TCharBuf &a) { allocate(); strlcpy(data(), a.data(), N); }
    TCharBuf &operator=(const TCharBuf &a) { strlcpy(data(), a.data(), N); return *this; }
    TCharBuf &operator=(const char *psz) { strlcpy(data(), psz, N); return *this; }
    const char *c_str() const { return data(); }
    char &operator[](size_t i) { YASSERT(i <= length()); return *(data() + i); }
    char operator[](size_t i) const { YASSERT(i <= length()); return *(data() + i); }
    size_t length() const { return strlen(data()); }

private:
    inline const char* data() const {
        return ((TCharBuf<N>*)(this))->data();
    }

    inline char* data() {
#if defined(REDUCE_STACK_USAGE)
        return (char*)sz.Get();
#else
        return sz;
#endif
    }

    inline void allocate() {
#if defined(REDUCE_STACK_USAGE)
        sz.Reset(::operator new(N));
#endif
    }
};

template<int N>
    inline void strcpy(TCharBuf<N> &a, const char *psz) { a = psz; }

template<int N>
    inline void strcat(TCharBuf<N> &a, const char *psz) {
        size_t nPos = a.length();
        strlcpy(&a[nPos], psz, N - nPos);
    }

template<int N>
    inline int strcmp(const TCharBuf<N> &a, const TCharBuf<N> &b) { return strcmp(a.c_str(), b.c_str()); }
template<int N>
    inline int strcmp(const TCharBuf<N> &a, const char *b) { return strcmp(a.c_str(), b); }
template<int N>
    inline int strcmp(const char *a, const TCharBuf<N> &b) { return strcmp(a, b.c_str()); }
template<int N>
    inline bool operator==(const TCharBuf<N> &a, const TCharBuf<N> &b) { return strcmp(a.c_str(), b.c_str()) == 0; }
template<int N>
    inline bool operator!=(const TCharBuf<N> &a, const TCharBuf<N> &b) { return !(a==b); }
template<int N>
    inline bool operator<(const TCharBuf<N> &a, const TCharBuf<N> &b) { return strcmp(a,b) < 0; }
template<int N>
    inline TCharBuf<N> operator+(const TCharBuf<N> &a, const char *pszB) {
        TCharBuf<N> res(a);
        strcat(res, pszB);
        return res;
    }
template<int N>
    inline TCharBuf<N> operator+(const TCharBuf<N> &a, const TCharBuf<N> &b) {
        TCharBuf<N> res(a);
        strcat(res, b.c_str());
        return res;
    }
