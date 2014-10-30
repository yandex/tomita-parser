#pragma once

#include "ios.h"

#include <util/system/defaults.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

/// @addtogroup Streams_Archs
/// @{

struct TZLibError: public yexception {
};

struct TZLibCompressorError: public TZLibError {
};

struct TZLibDecompressorError: public TZLibError {
};

#define ZLIB_BUF_LEN (8 * 1024)

namespace ZLib {
    enum StreamType {
        //for decompress only
        Auto = 0,
        //for compress && decompress
        ZLib = 1,
        GZip = 2,
        Raw  = 3
    };
}

/**
 * Non-buffered ZLib decompressor.
 *
 * @note: Please don't use @c TZLibDecompress if you read text data
 * from stream using @p ReadLine, it is VERY slow (approx 10 times slower,
 * according to syntetic benchmark). For fast buffered ZLib
 * stream reading use @c TBufferedZLibDecompress aka @c TZDecompress.
 **/
class TZLibDecompress: public TInputStream {
public:
    TZLibDecompress(TInputStream* input, ZLib::StreamType type = ZLib::Auto, size_t buflen = ZLIB_BUF_LEN);
    TZLibDecompress(IZeroCopyInput* input, ZLib::StreamType type = ZLib::Auto);
    virtual ~TZLibDecompress() throw ();

private:
    virtual size_t DoRead(void* buf, size_t size);

public:
    class TImpl;
    THolder<TImpl> Impl_;
};

class TZLibCompress: public TOutputStream {
public:
    struct TParams {
        inline TParams(TOutputStream* out)
            : Out(out)
            , Type(ZLib::ZLib)
            , CompressionLevel(6)
            , BufLen(ZLIB_BUF_LEN)
        {
        }

        inline TParams& SetType(ZLib::StreamType type) throw () {
            Type = type;

            return *this;
        }

        inline TParams& SetCompressionLevel(size_t level) throw () {
            CompressionLevel = level;

            return *this;
        }

        inline TParams& SetBufLen(size_t buflen) throw () {
            BufLen = buflen;

            return *this;
        }

        inline TParams& SetDict(const TStringBuf& dict) throw () {
            Dict = dict;

            return *this;
        }

        TOutputStream* Out;
        ZLib::StreamType Type;
        size_t CompressionLevel;
        size_t BufLen;
        TStringBuf Dict;
    };

    inline TZLibCompress(const TParams& params) {
        Init(params);
    }

    //compat constructors
    inline TZLibCompress(TOutputStream* out, ZLib::StreamType type) {
        Init(TParams(out).SetType(type));
    }

    inline TZLibCompress(TOutputStream* out, ZLib::StreamType type, size_t compression_level) {
        Init(TParams(out).SetType(type).SetCompressionLevel(compression_level));
    }

    inline TZLibCompress(TOutputStream* out, ZLib::StreamType type, size_t compression_level, size_t buflen) {
        Init(TParams(out).SetType(type).SetCompressionLevel(compression_level).SetBufLen(buflen));
    }

    virtual ~TZLibCompress() throw ();

private:
    void Init(const TParams& opts);

    virtual void DoWrite(const void* buf, size_t size);
    virtual void DoFlush();
    virtual void DoFinish();

public:
    class TImpl;

    //to allow inline constructors
    struct TDestruct {
        static void Destroy(TImpl* impl);
    };

    THolder<TImpl, TDestruct> Impl_;
};

/// buffered version, supports efficient ReadLine calls and similar "reading in small pieces" usage patterns
class TBufferedZLibDecompress: public TBuffered<TZLibDecompress> {
    public:
        template <class T>
        inline TBufferedZLibDecompress(T* in, ZLib::StreamType type = ZLib::Auto, size_t buf = 1 << 13)
            : TBuffered<TZLibDecompress>(buf, in, type)
        {
        }

        virtual ~TBufferedZLibDecompress() throw () {
        }
};

typedef TBufferedZLibDecompress TZDecompress;

/// @}

/*
 * undef some shit...
 */
#undef ZLIB_BUF_LEN
