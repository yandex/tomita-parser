#include "yarchive.h"

#include <util/stream/zlib.h>
#include <util/memory/tempbuf.h>
#include <util/system/byteorder.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/ysaveload.h>

namespace {
    class TLengthCalc: public TOutputStream {
    public:
        inline TLengthCalc(TOutputStream* slave)
            : Length_(0)
            , Slave_(slave)
        {
        }

        virtual ~TLengthCalc() throw () {
        }

        virtual void DoWrite(const void* buf, size_t len) {
            Slave_->Write(buf, len);
            Length_ += len;
        }

        inline ui64 Length() const throw () {
            return Length_;
        }

    private:
        ui64 Length_;
        TOutputStream* Slave_;
    };
}

template <class T>
static inline void ESSave(TOutputStream* out, const T& t_in) {
    T t = HostToLittle(t_in);

    out->Write((const void*)&t, sizeof(t));
}

static inline void ESSave(TOutputStream* out, const Stroka& s) {
    ESSave(out, (ui32)+s);
    out->Write(~s, +s);
}

template <class T>
static inline T ESLoad(TInputStream* in) {
    T t = T();

    if (in->Load(&t, sizeof(t)) != sizeof(t)) {
        ythrow TSerializeException() << "malformed archive";
    }

    return LittleToHost(t);
}

template <>
inline Stroka ESLoad<Stroka>(TInputStream* in) {
    size_t len = ESLoad<ui32>(in);
    Stroka ret;
    TTempBuf tmp;

    while (len) {
        const size_t toread = Min(len, tmp.Size());
        const size_t readed = in->Read(tmp.Data(), toread);

        if (!readed) {
            ythrow TSerializeException() << "malformed archive";
        }

        ret.append(tmp.Data(), readed);
        len -= readed;
    }

    return ret;
}

namespace {
    class TArchiveRecordDescriptor: public TRefCounted<TArchiveRecordDescriptor> {
    public:
        inline TArchiveRecordDescriptor(ui64 off, ui64 len, const Stroka& name)
            : Off_(off)
            , Len_(len)
            , Name_(name)
        {
        }

        inline TArchiveRecordDescriptor(TInputStream* in)
            : Off_(ESLoad<ui64>(in))
            , Len_(ESLoad<ui64>(in))
            , Name_(ESLoad<Stroka>(in))
        {
        }

        inline ~TArchiveRecordDescriptor() throw () {
        }

        inline void SaveTo(TOutputStream* out) const {
            ESSave(out, Off_);
            ESSave(out, Len_);
            ESSave(out, Name_);
        }

        inline const Stroka& Name() const throw () {
            return Name_;
        }

        inline ui64 Length() const throw () {
            return Len_;
        }

        inline ui64 Offset() const throw () {
            return Off_;
        }

    private:
        ui64 Off_;
        ui64 Len_;
        Stroka Name_;
    };

    typedef TIntrusivePtr<TArchiveRecordDescriptor> TArchiveRecordDescriptorRef;
}

class TArchiveWriter::TImpl {
        typedef yhash<Stroka, TArchiveRecordDescriptorRef> TDict;
    public:
        inline TImpl(TOutputStream& out, bool compress)
            : Off_(0)
            , Out_(&out)
            , UseCompression(compress)
        {
        }

        inline ~TImpl() throw () {
        }

        inline void Flush() {
            Out_->Flush();
        }

        inline void Finish() {
            TLengthCalc out(Out_);

            {
                TZLibCompress compress(&out);

                ESSave(&compress, (ui32)Dict_.size());

                for (TDict::const_iterator it = Dict_.begin(); it != Dict_.end(); ++it) {
                    it->second->SaveTo(&compress);
                }

                ESSave(&compress, static_cast<ui8>(UseCompression));

                compress.Finish();
            }

            ESSave(Out_, out.Length());

            Out_->Flush();
        }

        inline void Add(const Stroka& key, TInputStream* src) {
            if (Dict_.find(key) != Dict_.end()) {
                ythrow yexception() << "key " <<  ~key << " already stored";
            }

            TLengthCalc out(Out_);
            if (UseCompression) {
                TZLibCompress compress(&out);
                TransferData(src, &compress);
                compress.Finish();
            } else {
                TransferData(src, &out);
                out.Finish();
            }

            TArchiveRecordDescriptorRef descr(new TArchiveRecordDescriptor(Off_, out.Length(), key));

            Dict_[key] = descr;
            Off_ += out.Length();
        }

    private:
        ui64 Off_;
        TOutputStream* Out_;
        TDict Dict_;
        const bool UseCompression;
};

TArchiveWriter::TArchiveWriter(TOutputStream* out, bool compress)
    : Impl_(new TImpl(*out, compress))
{
}

TArchiveWriter::~TArchiveWriter() throw () {
    try {
        Finish();
    } catch (...) {
    }
}

void TArchiveWriter::Flush() {
    if (Impl_.Get()) {
        Impl_->Flush();
    }
}

void TArchiveWriter::Finish() {
    if (Impl_.Get()) {
        Impl_->Finish();
        Impl_.Destroy();
    }
}

void TArchiveWriter::Add(const Stroka& key, TInputStream* src) {
    if (!Impl_.Get()) {
        ythrow yexception() << "archive already closed";
    }

    Impl_->Add(key, src);
}

namespace {
    class TArchiveInputStreamBase {
    public:
        inline TArchiveInputStreamBase(const TBlob& b)
            : Blob_(b)
            , Input_(b.Data(), b.Size())
        {
        }

    protected:
        TBlob Blob_;
        TMemoryInput Input_;
    };

    class TArchiveInputStream: public TArchiveInputStreamBase, public TZLibDecompress {
    public:
        inline TArchiveInputStream(const TBlob& b)
            : TArchiveInputStreamBase(b)
            , TZLibDecompress((IZeroCopyInput*)&Input_)
        {
        }

        virtual ~TArchiveInputStream() throw () {
        }
    };
}

class TArchiveReader::TImpl {
        typedef yhash<Stroka, TArchiveRecordDescriptorRef> TDict;
    public:
        inline TImpl(const TBlob& blob)
            : Blob_(blob)
            , UseDecompression(true)
        {
            ReadDict();
        }

        inline ~TImpl() throw () {
        }

        inline void ReadDict() {
            if (Blob_.Size() < sizeof(ui64)) {
                ythrow yexception() << "too small blob";
            }

            const char* end = (const char*)Blob_.End();
            const char* ptr = end - sizeof(ui64);
            ui64 dictlen = 0;
            memcpy(&dictlen, ptr, sizeof(ui64));
            dictlen = LittleToHost(dictlen);

            if (dictlen > Blob_.Size() - sizeof(ui64)) {
                ythrow yexception() << "bad blob";
            }

            const char* beg = ptr - dictlen;
            TMemoryInput mi(beg, dictlen);
            TZLibDecompress d((IZeroCopyInput*)&mi);
            const ui32 count = ESLoad<ui32>(&d);

            for (size_t i = 0; i < count; ++i) {
                TArchiveRecordDescriptorRef descr(new TArchiveRecordDescriptor(&d));

                Recs_.push_back(descr);
                Dict_[descr->Name()] = descr;
            }

            try {
                UseDecompression = static_cast<bool>(ESLoad<ui8>(&d));
            } catch (const TSerializeException&) {
                // that's ok - just old format
                UseDecompression = true;
            }
        }

        inline size_t Count() const throw () {
            return Recs_.size();
        }

        inline Stroka KeyByIndex(size_t n) const {
            if (n < Count()) {
                return Recs_[n]->Name();
            }

            ythrow yexception() << "incorrect index";
        }

        inline bool Has(const Stroka& key) const {
            return Dict_.has(key);
        }

        inline TAutoPtr<TInputStream> ObjectByKey(const Stroka& key) const {
            TBlob subBlob = BlobByKey(key);

            if (UseDecompression) {
                return new TArchiveInputStream(subBlob);
            } else {
                return new TMemoryInput(subBlob.Data(), subBlob.Length());
            }
        }

        inline TBlob ObjectBlobByKey(const Stroka& key) const {
            TBlob subBlob = BlobByKey(key);

            if (UseDecompression) {
                TArchiveInputStream st(subBlob);
                return TBlob::FromStream(st);
            } else {
                return subBlob;
            }
        }

        inline TBlob BlobByKey(const Stroka& key) const {
            TDict::const_iterator it = Dict_.find(key);

            if (it == Dict_.end()) {
                ythrow yexception() << "key " <<  ~key << " not found";
            }

            const size_t off = it->second->Offset();
            const size_t len = it->second->Length();

            /*
             * TODO - overflow check
             */

            return Blob_.SubBlob(off, off + len);
        }

        inline bool Compressed() const {
            return UseDecompression;
        }

    private:
        TBlob Blob_;
        yvector<TArchiveRecordDescriptorRef> Recs_;
        TDict Dict_;
        bool UseDecompression;
};

TArchiveReader::TArchiveReader(const TBlob& data)
    : Impl_(new TImpl(data))
{
}

TArchiveReader::~TArchiveReader() throw () {
}

size_t TArchiveReader::Count() const throw () {
    return Impl_->Count();
}

Stroka TArchiveReader::KeyByIndex(size_t n) const {
    return Impl_->KeyByIndex(n);
}

bool TArchiveReader::Has(const Stroka& key) const {
    return Impl_->Has(key);
}

TAutoPtr<TInputStream> TArchiveReader::ObjectByKey(const Stroka& key) const {
    return Impl_->ObjectByKey(key);
}

TBlob TArchiveReader::ObjectBlobByKey(const Stroka& key) const {
    return Impl_->ObjectBlobByKey(key);
}

TBlob TArchiveReader::BlobByKey(const Stroka& key) const {
    return Impl_->BlobByKey(key);
}

bool TArchiveReader::Compressed() const {
    return Impl_->Compressed();
}
