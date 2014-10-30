#include "ymath.h"
#include "stroka.h"
#include "buffer.h"

#include <util/memory/alloc.h>

TBuffer::TBuffer(size_t len)
    : Data_(0)
    , Len_(0)
    , Pos_(0)
{
    Reserve(len);
}

TBuffer::TBuffer(const Stroka& s)
    : Data_(0)
    , Len_(0)
    , Pos_(0)
{
    Append(~s, +s + 1);
}

TBuffer::TBuffer(const char* buf, size_t len)
    : Data_(0)
    , Len_(0)
    , Pos_(0)
{
    Append(buf, len);
}

void TBuffer::DoReserve(size_t realLen) {
    // FastClp2<T>(x) returns 0 on x from [Max<T>/2 + 2, Max<T>]
    const size_t len = Max<size_t>(FastClp2(realLen), realLen);

    YASSERT(realLen > Len_);
    YASSERT(len >= realLen);

    Realloc(len);
}

void TBuffer::Realloc(size_t len) {
    YASSERT(Pos_ <= len);

    Data_ = (char*)y_reallocate(Data_, len);
    Len_ = len;
}

TBuffer::~TBuffer() throw () {
    y_deallocate(Data_);
}

void TBuffer::AsString(Stroka& s) {
    s.assign(Data(), Size());
    Clear();
}
