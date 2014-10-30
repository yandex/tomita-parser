#include "lz.h"

#include <util/system/yassert.h>
#include <util/system/byteorder.h>
#include <util/memory/addstorage.h>
#include <util/generic/utility.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

#include <contrib/libs/lz4/lz4.h>

static inline ui8 HostToLittle(ui8 t) throw () {
    return t;
}

static inline ui8 LittleToHost(ui8 t) throw () {
    return t;
}

struct TCommonData {
    static const size_t overhead = sizeof(ui16) + sizeof(ui8);
};

const size_t SIGNATURE_SIZE = 4;

template <class TCompressor, class TBase>
class TCompressorBase: public TAdditionalStorage<TCompressorBase<TCompressor, TBase> >
                     , public TCompressor, public TCommonData {
    public:
        inline TCompressorBase(TOutputStream* slave, ui16 blockSize)
            : Slave_(slave)
            , BlockSize_(blockSize)
        {
            /*
             * save signature
             */
            STATIC_ASSERT(sizeof(TCompressor::signature) - 1 == SIGNATURE_SIZE);
            Slave_->Write(TCompressor::signature, sizeof(TCompressor::signature) - 1);

            /*
             * save version
             */
            this->Save((ui32)1);

            /*
             * save block size
             */
            this->Save(BlockSize());
        }

        inline ~TCompressorBase() throw () {
        }

        inline void Write(const char* buf, size_t len) {
            while (len) {
                const ui16 toWrite = (ui16)Min<size_t>(len, this->BlockSize());

                this->WriteBlock(buf, toWrite);

                buf += toWrite;
                len -= toWrite;
            }
        }

        inline void Flush() {
        }

        inline void Finish() {
            this->Flush();
            this->WriteBlock(0, 0);
        }

        template <class T>
        static inline void Save(T t, TOutputStream* out) {
            t = HostToLittle(t);

            out->Write(&t, sizeof(t));
        }

        template <class T>
        inline void Save(T t) {
            Save(t, Slave_);
        }

    private:
        inline void* Block() const throw () {
            return this->AdditionalData();
        }

        inline ui16 BlockSize() const throw () {
            return BlockSize_;
        }

        inline void WriteBlock(const void* ptr, ui16 len) {
            YASSERT(len <= this->BlockSize());

            ui8 compressed = false;

            if (len) {
                const size_t out = this->Compress((const char*)ptr, len, (char*)Block());

                if (out < len || TCompressor::SaveIncompressibleChunks()) {
                    compressed = true;
                    ptr = Block();
                    len = (ui16)out;
                }
            }

            char tmp[overhead];
            TMemoryOutput header(tmp, sizeof(tmp));

            this->Save(len, &header);
            this->Save(compressed, &header);

            typedef TOutputStream::TPart TPart;
            const TPart parts[] = {
                TPart(tmp, sizeof(tmp)),
                TPart(ptr, len)
            };

            Slave_->Write(parts, sizeof(parts) / sizeof(*parts));
        }

    private:
        TOutputStream* Slave_;
        const ui16 BlockSize_;
};

template <class T>
static inline T GLoad(TInputStream* input) {
    T t;

    if (input->Load(&t, sizeof(t)) != sizeof(t)) {
        ythrow TDecompressorError() << "stream error";
    }

    return LittleToHost(t);
}

class TDecompressSignature {
    public:
        inline TDecompressSignature(TInputStream* input) {
            if (input->Load(Buffer_, SIGNATURE_SIZE) != SIGNATURE_SIZE) {
                ythrow TDecompressorError() << "can not load stream signature";
            }
        }

        template <class TDecompressor>
        inline bool Check() const {
            STATIC_ASSERT(sizeof(TDecompressor::signature) - 1 == SIGNATURE_SIZE);
            return memcmp(TDecompressor::signature, Buffer_, SIGNATURE_SIZE) == 0;
        }

    private:
        char Buffer_[SIGNATURE_SIZE];
};

template <class TDecompressor>
static inline TInputStream* ConsumeSignature(TInputStream* input) {
    TDecompressSignature sign(input);
    if (!sign.Check<TDecompressor>()) {
        ythrow TDecompressorError() << "incorrect signature";
    }
    return input;
}

template <class TDecompressor>
class TDecompressorBaseImpl: public TDecompressor, public TCommonData {
    public:
        inline TDecompressorBaseImpl(TInputStream* slave)
            : Slave_(slave)
            , Input_(0, 0)
            , Eof_(false)
            , Version_(Load<ui32>())
            , BlockSize_(Load<ui16>())
            , OutBufSize_(TDecompressor::Hint(BlockSize_))
            , Tmp_(::operator new(2 * OutBufSize_))
            , In_((char*)Tmp_.Get())
            , Out_(In_ + OutBufSize_)
        {
            this->InitFromStream(Slave_);
        }

        inline ~TDecompressorBaseImpl() throw () {
        }

        inline size_t Read(void* buf, size_t len) {
            size_t ret = Input_.Read(buf, len);

            if (ret) {
                return ret;
            }

            if (Eof_) {
                return 0;
            }

            this->FillNextBlock();

            ret = Input_.Read(buf, len);

            if (ret) {
                return ret;
            }

            Eof_ = true;

            return 0;
        }

        inline void FillNextBlock() {
            char tmp[overhead];

            if (Slave_->Load(tmp, sizeof(tmp)) != sizeof(tmp)) {
                ythrow TDecompressorError() << "can not read block header";
            }

            TMemoryInput header(tmp, sizeof(tmp));

            const ui16 len = GLoad<ui16>(&header);
            const ui8 compressed = GLoad<ui8>(&header);

            if (compressed > 1) {
                ythrow TDecompressorError() << "broken header";
            }

            if (Slave_->Load(In_, len) != len) {
                ythrow TDecompressorError() << "can not read data";
            }

            if (compressed) {
                const size_t ret = this->Decompress(In_, len, Out_, OutBufSize_);

                Input_.Reset(Out_, ret);
            } else {
                Input_.Reset(In_, len);
            }
        }

        template <class T>
        inline T Load() {
            return GLoad<T>(Slave_);
        }

    protected:
        TInputStream* Slave_;
        TMemoryInput Input_;
        bool Eof_;
        const ui32 Version_;
        const ui16 BlockSize_;
        const size_t OutBufSize_;
        THolder<void> Tmp_;
        char* In_;
        char* Out_;
};

template <class TDecompressor, class TBase>
class TDecompressorBase: public TDecompressorBaseImpl<TDecompressor> {
    public:
        inline TDecompressorBase(TInputStream* slave)
            : TDecompressorBaseImpl<TDecompressor>(ConsumeSignature<TDecompressor>(slave))
        {
        }

        inline ~TDecompressorBase() throw () {
        }
};

#define DEF_COMPRESSOR_COMMON(rname, name)\
    rname::~rname() throw () {\
        try {\
            Finish();\
        } catch (...) {\
        }\
    }\
    \
    void rname::DoWrite(const void* buf, size_t len) {\
        if (!Impl_) {\
            ythrow yexception() << "can not write to finalized stream";\
        }\
        \
        Impl_->Write((const char*)buf, len);\
    }\
    \
    void rname::DoFlush() {\
        if (!Impl_) {\
            ythrow yexception() << "can not flush finalized stream";\
        }\
        \
        Impl_->Flush();\
    }\
    \
    void rname::DoFinish() {\
        THolder<TImpl> impl(Impl_.Release());\
        \
        if (impl) {\
            impl->Finish();\
        }\
    }

#define DEF_COMPRESSOR(rname, name)\
    class rname::TImpl: public TCompressorBase<name, TImpl> {\
        public:\
            inline TImpl(TOutputStream* out, ui16 blockSize)\
                : TCompressorBase<name, TImpl>(out, blockSize)\
            {\
            }\
    };\
    \
    rname::rname(TOutputStream* slave, ui16 blockSize)\
        : Impl_(new (TImpl::Hint(blockSize)) TImpl(slave, blockSize))\
    {\
    }\
    \
    DEF_COMPRESSOR_COMMON(rname, name)

#define DEF_DECOMPRESSOR(rname, name)\
    class rname::TImpl: public TDecompressorBase<name, TImpl> {\
        public:\
            inline TImpl(TInputStream* in)\
                : TDecompressorBase<name, TImpl>(in)  \
            {\
            }\
    };\
    \
    rname::rname(TInputStream* slave)\
        : Impl_(new TImpl(slave))\
    {\
    }\
    \
    rname::~rname() throw () {\
    }\
    \
    size_t rname::DoRead(void* buf, size_t len) {\
        return Impl_->Read(buf, len);\
    }

template <size_t N>
class TFixedArray {
    public:
        inline TFixedArray() throw () {
            memset(WorkMem_, 0, sizeof(WorkMem_));
        }

    protected:
        char WorkMem_[N];
};

/*
 * LZ4
 */
class TLZ4 {
    public:
        static const char signature[];

        static inline size_t Hint(size_t len) throw () {
            return Max<size_t>((size_t)(len * 1.06), 100);
        }

        inline size_t Compress(const char* data, size_t len, char* ptr) {
            return LZ4_compress(data, ptr, len);
        }

        inline size_t Decompress(const char* data, size_t len, char* ptr, size_t max) {
            int res = LZ4_uncompress_unknownOutputSize(data, ptr, len, max);
            if (res < 0)
                ythrow TDecompressorError();
            return res;
        }

        inline void InitFromStream(TInputStream*) const throw () {
        }

        static inline bool SaveIncompressibleChunks() throw () {
            return false;
        }
};

const char TLZ4::signature[] = "LZ.4";

DEF_COMPRESSOR(TLz4Compress, TLZ4)
DEF_DECOMPRESSOR(TLz4Decompress, TLZ4)

// Decompressing input streams without signature verification
template <class TDecompressor>
class TLzDecompressInput: public TInputStream {
    public:
        inline TLzDecompressInput(TInputStream* in)
            : Impl_(in)
        {
        }

    private:
        virtual size_t DoRead(void* buf, size_t len) {
            return Impl_.Read(buf, len);
        }

    private:
        TDecompressorBaseImpl<TDecompressor> Impl_;
};


TAutoPtr<TInputStream> TryOpenLzDecompressor(const TDecompressSignature& s, TInputStream* input) {
    if (s.Check<TLZ4>())
        return new TLzDecompressInput<TLZ4>(input);

    return NULL;
}


TAutoPtr<TInputStream> TryOpenLzDecompressor(const TStringBuf& signature, TInputStream* input) {
    if (signature.size() == SIGNATURE_SIZE) {
        TMemoryInput mem(~signature, +signature);
        TDecompressSignature s(&mem);
        return TryOpenLzDecompressor(s, input);
    }

    return NULL;
}

TAutoPtr<TInputStream> OpenLzDecompressor(TInputStream* input) {
    TDecompressSignature s(input);

    TAutoPtr<TInputStream> ret = TryOpenLzDecompressor(s, input);
    if (!ret)
        ythrow TDecompressorError() << "Unknown compression format";

    return ret;
}
