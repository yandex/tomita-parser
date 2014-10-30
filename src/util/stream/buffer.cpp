#include "buffer.h"

#include <util/generic/buffer.h>

class TBufferOutput::TImpl {
public:
    inline TImpl(TBuffer& buf)
        : Data_(buf)
    {
    }

    virtual ~TImpl() {
    }

    inline TPart GetBuffer() const throw () {
        return TPart(~Data_, +Data_);
    }

    inline void DoWrite(const void* buf, size_t len) {
        Data_.Append((const char*)buf, len);
    }

    inline TBuffer& Buffer() const throw () {
        return Data_;
    }

private:
    TBuffer& Data_;
};

namespace {
    typedef TBufferOutput::TImpl TImpl;

    class TOwnedImpl: private TBuffer, public TImpl {
    public:
        inline TOwnedImpl(size_t buflen)
            : TBuffer(buflen)
            , TImpl(static_cast<TBuffer&>(*this))
        {
        }
    };
}

TBufferOutput::TBufferOutput(size_t buflen)
    : Impl_(new TOwnedImpl(buflen))
{
}

TBufferOutput::TBufferOutput(TBuffer& buffer)
    : Impl_(new TImpl(buffer))
{
}

TBufferOutput::~TBufferOutput() throw () {
}

TOutputStream::TPart TBufferOutput::GetBuffer() const throw () {
    return Impl_->GetBuffer();
}

void TBufferOutput::DoWrite(const void* buf, size_t len) {
    Impl_->DoWrite(buf, len);
}

TBuffer& TBufferOutput::Buffer() const throw () {
    return Impl_->Buffer();
}

size_t TBufferInput::DoRead(void* buf, size_t len) {
    const size_t forRead = Min(+Buf_ - Readed_, len);

    memcpy(buf, ~Buf_ + Readed_, forRead);
    Readed_ += forRead;

    return forRead;
}

size_t TBufferInput::DoSkip(size_t len) throw() {
    len = Min(len, Buf_.Size() - Readed_);
    Readed_ += len;
    return len;
}
