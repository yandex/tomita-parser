#pragma once

#include "ios.h"

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

#define BZIP_BUF_LEN (8 * 1024)
#define BZIP_COMPRESSION_LEVEL 6

/// @addtogroup Streams_Archs
/// @{

class TBZipException: public yexception {};
class TBZipDecompressError: public TBZipException {};
class TBZipCompressError: public TBZipException {};

class TBZipDecompress: public TInputStream {
    public:
        TBZipDecompress(TInputStream* input, size_t bufLen = BZIP_BUF_LEN);
        virtual ~TBZipDecompress() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t size);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

class TBZipCompress: public TOutputStream {
    public:
        TBZipCompress(TOutputStream* out,
            size_t compressionLevel = BZIP_COMPRESSION_LEVEL,
            size_t bufLen = BZIP_BUF_LEN);
        virtual ~TBZipCompress() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t size);
        virtual void DoFlush();
        virtual void DoFinish();

    public:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// @}
