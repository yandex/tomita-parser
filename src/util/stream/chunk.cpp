#include "chunk.h"

#include <util/string/cast.h>
#include <util/system/byteorder.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>

static inline size_t ParseHex(const Stroka& s) {
    if (s.empty()) {
        ythrow yexception() << "can not parse chunk length(empty string)";
    }

    size_t ret = 0;

    for (Stroka::const_iterator c = s.begin(); c != s.end(); ++c) {
        const char ch = *c;

        if (ch >= '0' && ch <= '9') {
            ret *= 16;
            ret += ch - '0';
        } else if (ch >= 'a' && ch <= 'f') {
            ret *= 16;
            ret += 10 + ch - 'a';
        } else if (ch >= 'A' && ch <= 'F') {
            ret *= 16;
            ret += 10 + ch - 'A';
        } else if (ch == ';') {
            break;
        } else if (isspace(ch)) {
            continue;
        } else {
            ythrow yexception() << "can not parse chunk length(" <<  ~s << ")";
        }
    }

    return ret;
}

static inline char* ToHex(size_t len, char* buf) {
    do {
        const size_t val = len % 16;

        *--buf = (val < 10) ? (val + '0') : (val - 10 + 'a');
        len /= 16;
    } while (len);

    return buf;
}

class TChunkedInput::TImpl {
    public:
        inline TImpl(TInputStream* slave)
            : Slave_(slave)
            , Pending_(0)
            , LastChunkReaded_(false)
        {
        }

        inline ~TImpl() throw () {
        }

        inline size_t Read(void* buf, size_t len) {
            if (!HavePendingData()) {
                return 0;
            }

            const size_t toRead = Min(Pending_, len);

            if (toRead) {
                const size_t readed = Slave_->Read(buf, toRead);

                if (!readed) {
                    ythrow yexception() << "malformed http chunk";
                }

                Pending_ -= readed;

                return readed;
            }

            return 0;
        }

    private:
        inline bool HavePendingData() {
            if (LastChunkReaded_) {
                return false;
            }

            if (!Pending_) {
                if (!ProceedToNextChunk()) {
                    return false;
                }
            }

            return true;
        }

        inline bool ProceedToNextChunk() {
            Stroka len(Slave_->ReadLine());

            if (len.empty()) {
                /*
                 * skip crlf from previous chunk
                 */

                len = Slave_->ReadLine();
            }

            Pending_ = ParseHex(len);

            if (Pending_) {
                return true;
            }

            Slave_->ReadLine(); //skip last crlf
            LastChunkReaded_ = true;

            return false;
        }

    private:
        TInputStream* Slave_;
        size_t Pending_;
        bool LastChunkReaded_;
};

TChunkedInput::TChunkedInput(TInputStream* slave)
    : Impl_(new TImpl(slave))
{
}

TChunkedInput::~TChunkedInput() throw () {
}

size_t TChunkedInput::DoRead(void* buf, size_t len) {
    return Impl_->Read(buf, len);
}

class TChunkedOutput::TImpl {
        typedef TOutputStream::TPart TPart;
    public:
        inline TImpl(TOutputStream* slave)
            : Slave_(slave)
        {
        }

        inline ~TImpl() throw () {
        }

        inline void Write(const void* buf, size_t len) {
            const char* ptr = (const char*)buf;

            while (len) {
                const size_t portion = Min<size_t>(len, 1024 * 16);

                WriteImpl(ptr, portion);

                ptr += portion;
                len -= portion;
            }
        }

        inline void WriteImpl(const void* buf, size_t len) {
            char tmp[32];
            char* e = tmp + sizeof(tmp);
            char* b = ToHex(len, e);

            const TPart parts[] = {
                TPart(b, e - b),
                TPart::CrLf(),
                TPart(buf, len),
                TPart::CrLf(),
            };

            Slave_->Write(parts, sizeof(parts) / sizeof(*parts));
        }

        inline void Flush() {
            Slave_->Flush();
        }

        inline void Finish() {
            Slave_->Write("0\r\n\r\n", 5);

            Flush();
        }

    private:
        TOutputStream* Slave_;
};

TChunkedOutput::TChunkedOutput(TOutputStream* slave)
    : Impl_(new TImpl(slave))
{
}

TChunkedOutput::~TChunkedOutput() throw () {
    try {
        Finish();
    } catch (...) {
    }
}

void TChunkedOutput::DoWrite(const void* buf, size_t len) {
    if (Impl_.Get()) {
        Impl_->Write(buf, len);
    } else {
        ythrow yexception() << "can not write to finished stream";
    }
}

void TChunkedOutput::DoFlush() {
    if (Impl_.Get()) {
        Impl_->Flush();
    }
}

void TChunkedOutput::DoFinish() {
    if (Impl_.Get()) {
        Impl_->Finish();
        Impl_.Destroy();
    }
}

class TBinaryChunkedInput::TImpl {
    public:
        inline TImpl(TInputStream* input)
            : Slave_(input)
            , Pending_(Proceed())
        {
        }

        inline ~TImpl() throw () {
        }

        inline size_t Read(void* buf, size_t len) {
            if (!Pending_) {
                return 0;
            }

            const size_t toread = Min(Pending_, len);
            const size_t readed = Slave_->Read(buf, toread);

            Pending_ -= readed;

            if (!Pending_) {
                Pending_ = Proceed();
            }

            return readed;
        }

        inline size_t Proceed() {
            ui16 ret = 0;

            if (Slave_->Load(&ret, sizeof(ret)) != sizeof(ret)) {
                ythrow yexception() << "malformed binary stream";
            }

            return LittleToHost(ret);
        }

    private:
        TInputStream* Slave_;
        size_t Pending_;
};

class TBinaryChunkedOutput::TImpl {
    public:
        inline TImpl(TOutputStream* slave)
            : Slave_(slave)
        {
        }

        inline ~TImpl() throw () {
        }

        inline void Write(const void* buf_in, size_t len) {
            const char* buf = (const char*)buf_in;

            while (len) {
                const size_t to_write = Min<size_t>(len, 16 * 1024);

                WriteImpl(buf, to_write);

                buf += to_write;
                len -= to_write;
            }
        }

        inline void Finish() {
            WriteImpl("", 0);
        }

    private:
        inline void WriteImpl(const void* buf, size_t len) {
            const ui16 llen = HostToLittle((ui16)len);

            const TPart parts[] = {
                TPart(&llen, sizeof(llen)),
                TPart(buf, len)
            };

            Slave_->Write(parts, sizeof(parts) / sizeof(*parts));
        }

    private:
        TOutputStream* Slave_;
};

TBinaryChunkedInput::TBinaryChunkedInput(TInputStream* input)
    : Impl_(new TImpl(input))
{
}

TBinaryChunkedInput::~TBinaryChunkedInput() throw () {
}

size_t TBinaryChunkedInput::DoRead(void* buf, size_t len) {
    return Impl_->Read(buf, len);
}

TBinaryChunkedOutput::TBinaryChunkedOutput(TOutputStream* output)
    : Impl_(new TImpl(output))
{
}

TBinaryChunkedOutput::~TBinaryChunkedOutput() throw () {
    try {
        Finish();
    } catch (...) {
    }
}

void TBinaryChunkedOutput::DoWrite(const void* buf, size_t len) {
    if (Impl_.Get()) {
        Impl_->Write(buf, len);
    } else {
        ythrow yexception() << "can not write to finished stream";
    }
}

void TBinaryChunkedOutput::DoFinish() {
    if (Impl_.Get()) {
        Impl_->Finish();
        Impl_.Destroy();
    }
}
