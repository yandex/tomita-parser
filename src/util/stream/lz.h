#pragma once

#include "ios.h"

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

/// All *lz* compressors compress blocks. T*Compress.Write method splits input data into blocks, compressees each block
/// and then writes each compressed block to the underlying output stream. Thus T*Compress classes are not buffered.
/// MaxBlockSize parameter specified max allowed block size.
/// see http://altdevblogaday.com/2011/04/22/survey-of-fast-compression-algorithms-part-1/ for some comparison

struct TDecompressorError: public yexception {
};

/// @addtogroup Streams_Archs
/// @{

/*
 * Lz4 compression
 * http://code.google.com/p/lz4/
 */
class TLz4Compress: public TOutputStream {
    public:
        TLz4Compress(TOutputStream* slave, ui16 maxBlockSize = 1 << 15);
        virtual ~TLz4Compress() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);
        virtual void DoFlush();
        virtual void DoFinish();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

class TLz4Decompress: public TInputStream {
    public:
        TLz4Decompress(TInputStream* slave);
        virtual ~TLz4Decompress() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/*
 * LZO compress/decompress engines
 */ 
class TLzoCompress: public TOutputStream {
    public:
        TLzoCompress(TOutputStream* slave, ui16 maxBlockSize = 1 << 15);
        virtual ~TLzoCompress() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);
        virtual void DoFlush();
        virtual void DoFinish();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};


class TLzoDecompress: public TInputStream {
    public:
        TLzoDecompress(TInputStream* slave);
        virtual ~TLzoDecompress() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// @}

// Selects proper lz decompressor based on input stream signature.
TAutoPtr<TInputStream> OpenLzDecompressor(TInputStream* input);
TAutoPtr<TInputStream> TryOpenLzDecompressor(const TStringBuf& signature, TInputStream* input);
