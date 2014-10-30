#include "str.h"

TStringInput::~TStringInput() throw () {
}

size_t TStringInput::DoRead(void* buf, size_t len) {
    if (Pos_ < +S_) {
        const size_t toRead = Min(len, +S_ - Pos_);

        memcpy(buf, ~S_ + Pos_, toRead);
        Pos_ += toRead;

        return toRead;
    }

    return 0;
}

TStringOutput::~TStringOutput() throw () {
}

void TStringOutput::DoWrite(const void* buf, size_t len) {
    S_.append((const char*)buf, len);
}

TStringStream::~TStringStream() throw () {
}
