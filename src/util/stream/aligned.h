#pragma once

#include "base.h"

/// @addtogroup Streams
/// @{

class TAlignedInput: public TInputStream {
private:
    TInputStream* Stream;
    size_t Position;

    size_t DoRead(void* ptr, size_t len)
    {
        size_t ret = Stream->Read(ptr, len);
        Position += ret;
        return ret;
    }

public:
    TAlignedInput(TInputStream* s)
        : Stream(s)
        , Position(0)
    {
    }

    void Align(size_t divisor = sizeof(void*))
    {
        static char unused[sizeof(void*)*2];
        YASSERT(divisor <= sizeof(unused));
        if (Position & (divisor - 1))
            DoRead(unused, divisor - (Position & (divisor - 1)));
    }
};

class TAlignedOutput: public TOutputStream {
private:
    TOutputStream* Stream;
    size_t Position;

    void DoWrite(const void* ptr, size_t len)
    {
        Stream->Write(ptr, len);
        Position += len;
    }

public:
    TAlignedOutput(TOutputStream* s)
        : Stream(s)
        , Position(0)
    {
    }

    inline size_t GetCurrentOffset() const
    {
        return Position;
    }

    void Align(size_t divisor = sizeof(void*))
    {
        static char unused[sizeof(void*)*2];
        YASSERT(divisor <= sizeof(unused));
        if (Position & (divisor - 1))
            DoWrite(unused, divisor - (Position & (divisor - 1)));
    }
};

/// @}
