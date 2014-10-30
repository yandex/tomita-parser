#include "file.h"

#include <util/memory/blob.h>
#include <util/generic/yexception.h>

TFileInput::TFileInput(const Stroka& path)
    : File_(path, OpenExisting | RdOnly | Seq)
{
    if (!File_.IsOpen()) {
        ythrow TIoException() << "file " << path <<" not open";
    }
}

TFileInput::TFileInput(const TFile& file)
    : File_(file)
{
    if (!File_.IsOpen()) {
        ythrow TIoException() << "file (" << file.GetName() << ") not open";
    }
}

size_t TFileInput::DoRead(void* buf, size_t len) {
    return File_.Read(buf, len);
}

TFileOutput::TFileOutput(const Stroka& path)
    : File_(path, CreateAlways | WrOnly | Seq)
{
    if (!File_.IsOpen()) {
        ythrow TFileError() << "can not open " << path;
    }
}

TFileOutput::TFileOutput(const TFile& file)
    : File_(file)
{
    if (!File_.IsOpen()) {
        ythrow TIoException() << "closed file(" << file.GetName() << ") passed";
    }
}

TFileOutput::~TFileOutput() throw () {
}

void TFileOutput::DoWrite(const void* buf, size_t len) {
    File_.Write(buf, len);
}

void TFileOutput::DoFlush() {
    if (File_.IsOpen()) {
        File_.Flush();
    }
}

class TMappedFileInput::TImpl: public TBlob {
    public:
        inline TImpl(TFile file)
            : TBlob(TBlob::FromFile(file))
        {
        }

        inline ~TImpl() throw () {
        }
};

TMappedFileInput::TMappedFileInput(const TFile& file)
    : TMemoryInput(0, 0)
    , Impl_(new TImpl(file))
{
    Reset(Impl_->Data(), Impl_->Size());
}

TMappedFileInput::TMappedFileInput(const Stroka& path)
    : TMemoryInput(0, 0)
    , Impl_(new TImpl(TFile(path, OpenExisting | RdOnly)))
{
    Reset(Impl_->Data(), Impl_->Size());
}

TMappedFileInput::~TMappedFileInput() throw () {
}
