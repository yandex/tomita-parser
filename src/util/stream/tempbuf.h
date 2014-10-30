#pragma once

#include "output.h"

#include <util/memory/tempbuf.h>

class TTempBufOutput
    : public TOutputStream
    , public TTempBuf
{
public:
    TTempBufOutput()
    {}
    explicit TTempBufOutput(size_t size)
        : TTempBuf(size)
    {}

public:
    virtual void DoWrite(const void* data, size_t len) {
        Append(data, len);
    }
};

class TGrowingTempBufOutput
    : public TOutputStream
    , public TTempBuf
{
public:
    inline TGrowingTempBufOutput()
    {}

    explicit TGrowingTempBufOutput(size_t size)
        : TTempBuf(size)
    {}

private:
    static inline size_t Next(size_t size) throw () {
        return size * 2;
    }

    virtual void DoWrite(const void* data, size_t len) {
        if (EXPECT_TRUE(len <= Left())) {
            Append(data, len);
        } else {
            const size_t filled = Filled();

            TTempBuf buf(Next(filled + len));

            buf.Append(Data(), filled);
            buf.Append(data, len);

            static_cast<TTempBuf&>(*this) = buf;
        }
    }
};
