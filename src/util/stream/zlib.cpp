#include "zlib.h"

#include <util/memory/addstorage.h>
#include <util/generic/utility.h>

#include <contrib/libs/zlib/zlib.h>

#include <cstdio>
#include <cstring>

namespace {
    static const int opts[] = {
        //Auto
        15 + 32,
        //ZLib
        15 + 0,
        //GZip
        15 + 16,
        //Raw
        -15
    };

    class TZLibCommon {
    public:
        inline TZLibCommon() throw () {
            memset(Z(), 0, sizeof(*Z()));
        }

        inline ~TZLibCommon() throw () {
        }

        inline const char* GetErrMsg() const throw () {
            return Z()->msg != 0 ? Z()->msg : "unknown error";
        }

        inline z_stream* Z() const throw () {
            return (z_stream*)(&Z_);
        }

    private:
        z_stream Z_;
    };

    static inline ui32 MaxPortion(size_t s) throw () {
        return (ui32)Min<size_t>(Max<ui32>(), s);
    }

    struct TChunkedZeroCopyInput {
        inline TChunkedZeroCopyInput(IZeroCopyInput* in)
            : In(in)
            , Buf(0)
            , Len(0)
        {
        }

        template <class P, class T>
        inline bool Next(P** buf, T* len) {
            if (!Len) {
                if (!In->Next(&Buf, &Len)) {
                    return false;
                }
            }

            const T toread = (T)Min((size_t)Max<T>(), Len);

            *len = toread;
            *buf = (P*)Buf;

            Buf += toread;
            Len -= toread;

            return true;
        }

        IZeroCopyInput* In;
        const char* Buf;
        size_t Len;
    };
}

class TZLibDecompress::TImpl: private TZLibCommon, public TChunkedZeroCopyInput {
public:
    inline TImpl(IZeroCopyInput* in, ZLib::StreamType type)
        : TChunkedZeroCopyInput(in)
    {
        if (inflateInit2(Z(), opts[type]) != Z_OK) {
            ythrow TZLibDecompressorError() << "can not init inflate engine";
        }
    }

    virtual ~TImpl() {
        inflateEnd(Z());
    }

    inline size_t Read(void* buf, size_t size) {
        Z()->next_out = (unsigned char*)buf;
        Z()->avail_out = size;

        while (true) {
            if (Z()->avail_in == 0) {
                if (!FillInputBuffer()) {
                    return 0;
                }
            }

            switch (inflate(Z(), Z_SYNC_FLUSH)) {
                case Z_STREAM_END: {
                    if (inflateReset(Z()) != Z_OK) {
                        ythrow TZLibDecompressorError() << "inflate reset error(" << GetErrMsg() << ")";
                    }
                }

                case Z_OK: {
                    const size_t processed = size - Z()->avail_out;

                    if (processed) {
                        return processed;
                    }

                    break;
                }

                default:
                    ythrow TZLibDecompressorError() << "inflate error(" << GetErrMsg() << ")";
            }
        }
    }

private:
    inline bool FillInputBuffer() {
        return Next(&Z()->next_in, &Z()->avail_in);
    }
};

namespace {
    class TDecompressStream: public IZeroCopyInput, public TZLibDecompress::TImpl, public TAdditionalStorage<TDecompressStream> {
    public:
        inline TDecompressStream(TInputStream* input, ZLib::StreamType type)
            : TZLibDecompress::TImpl(this, type)
            , Stream_(input)
        {
        }

    private:
        virtual bool DoNext(const void** ptr, size_t* len) {
            void* buf = AdditionalData();

            *ptr = buf;
            *len = Stream_->Read(buf, AdditionalDataLength());

            return *len;
        }

    private:
        TInputStream* Stream_;
    };

    typedef TZLibDecompress::TImpl TZeroCopyDecompress;
}

class TZLibCompress::TImpl: public TAdditionalStorage<TImpl>, private TZLibCommon {
    template <class T>
    static inline T Type(T type) {
        if (type == ZLib::Auto) {
            return ZLib::ZLib;
        }

        return type;
    }

public:
    inline TImpl(const TParams& p)
        : Stream_(p.Out)
    {
        if (deflateInit2(Z(), Min<size_t>(9, p.CompressionLevel), Z_DEFLATED, opts[Type(p.Type)], 8, Z_DEFAULT_STRATEGY)) {
            ythrow TZLibCompressorError() << "can not init inflate engine";
        }

        if (+p.Dict) {
            if (deflateSetDictionary(Z(), (const Bytef*)~p.Dict, +p.Dict)) {
                ythrow TZLibCompressorError() << "can not set deflate dictionary";
            }
        }

        Z()->next_out = TmpBuf();
        Z()->avail_out = TmpBufLen();
    }

    inline ~TImpl() throw () {
        deflateEnd(Z());
    }

    inline void Write(const void* buf, size_t size) {
        const Bytef* b = (const Bytef*)buf;
        const Bytef* e = b + size;

        do {
            b = WritePart(b, e);
        } while (b < e);
    }

    inline const Bytef* WritePart(const Bytef* b, const Bytef* e) {
        Z()->next_in = const_cast<Bytef*>(b);
        Z()->avail_in = MaxPortion(e - b);

        while (Z()->avail_in) {
            const int ret = deflate(Z(), Z_NO_FLUSH);

            switch (ret) {
                case Z_OK:
                    continue;

                case Z_BUF_ERROR:
                    FlushBuffer();

                    break;

                default:
                    ythrow TZLibCompressorError() << "deflate error(" <<  GetErrMsg() << ")";
            }
        }

        return Z()->next_in;
    }

    inline void Flush() {
    }

    inline void FlushBuffer() {
        Stream_->Write(TmpBuf(), TmpBufLen() - Z()->avail_out);
        Z()->next_out = TmpBuf();
        Z()->avail_out = TmpBufLen();
    }

    inline void Finish() {
        int ret = deflate(Z(), Z_FINISH);

        while (ret == Z_OK || ret == Z_BUF_ERROR) {
            FlushBuffer();
            ret = deflate(Z(), Z_FINISH);
        }

        if (ret == Z_STREAM_END) {
            Stream_->Write(TmpBuf(), TmpBufLen() - Z()->avail_out);
        } else {
            ythrow TZLibCompressorError() << "deflate error(" << GetErrMsg() << ")";
        }
    }

private:
    inline unsigned char* TmpBuf() throw () {
        return (unsigned char*)AdditionalData();
    }

    inline size_t TmpBufLen() const throw () {
        return AdditionalDataLength();
    }

private:
    TOutputStream* Stream_;
};

TZLibDecompress::TZLibDecompress(TInputStream* input, ZLib::StreamType type, size_t buflen)
    : Impl_(new (buflen) TDecompressStream(input, type))
{
}

TZLibDecompress::TZLibDecompress(IZeroCopyInput* input, ZLib::StreamType type)
    : Impl_(new TZeroCopyDecompress(input, type))
{
}

TZLibDecompress::~TZLibDecompress() throw () {
}

size_t TZLibDecompress::DoRead(void* buf, size_t size) {
    return Impl_->Read(buf, MaxPortion(size));
}

void TZLibCompress::Init(const TParams& opts) {
    Impl_.Reset(new (opts.BufLen) TImpl(opts));
}

void TZLibCompress::TDestruct::Destroy(TImpl* impl) {
    delete impl;
}

TZLibCompress::~TZLibCompress() throw () {
    try {
        Finish();
    } catch (...) {
    }
}

void TZLibCompress::DoWrite(const void* buf, size_t size) {
    if (!Impl_) {
        ythrow TZLibCompressorError() << "can not write to finished zlib stream";
    }

    Impl_->Write(buf, size);
}

void TZLibCompress::DoFlush() {
    if (Impl_) {
        Impl_->Flush();
    }
}

void TZLibCompress::DoFinish() {
    THolder<TImpl> impl(Impl_.Release());

    if (impl) {
        impl->Finish();
    }
}
