#include "mem.h"

#include <util/generic/yexception.h>

TMemoryInput::TMemoryInput() throw ()
    : Buf_(0)
    , Len_(0)
{
}

TMemoryInput::TMemoryInput(const void* buf, size_t len) throw ()
    : Buf_((const char*)buf)
    , Len_(len)
{
}

TMemoryInput::~TMemoryInput() throw () {
}

size_t TMemoryInput::DoRead(void* buf, size_t len) {
    const size_t to_read = Min(Len_, len);

    memcpy(buf, Buf_, to_read);
    Len_ -= to_read;
    Buf_ += to_read;

    return to_read;
}

bool TMemoryInput::DoNext(const void** ptr, size_t* len) {
    *ptr = (ui8*)Buf_;
    *len = Len_;
    Len_ = 0;

    return (bool)*len;
}

bool TMemoryInput::DoReadTo(Stroka& st, char to) {
    if (!Len_) {
        return false;
    }

    const char* end = Buf_ + Len_;
    const char* pos = (const char*)memchr((void*)Buf_, to, Len_);

    if (!pos) {
        pos = end;
    }

    st.AssignNoAlias(Buf_, pos);

    Buf_ = pos < end ? pos + 1 : end;
    Len_ = end - Buf_;

    return true;
}

size_t TMemoryInput::DoSkip(size_t len) throw() {
    len = Min(Len_, len);
    Len_ -= len;
    Buf_ += len;
    return len;
}

TMemoryOutput::~TMemoryOutput() throw () {
}

void TMemoryOutput::DoWrite(const void* buf, size_t len) {
    char* end = Buf_ + len;
    if (end > End_) {
        ythrow yexception() << "memory output stream exhausted";
    }

    memcpy(Buf_, buf, len);
    Buf_ = end;
}
