#pragma once

#include "input.h"

#include <util/generic/utility.h>

class TLengthLimitedInput: public TInputStream {
public:
    inline TLengthLimitedInput(TInputStream* slave, ui64 length) throw ()
        : Slave_(slave)
        , Length_(length)
    {
    }

    virtual ~TLengthLimitedInput() throw () {
    }

    inline ui64 Left() const throw () {
        return Length_;
    }

private:
    virtual size_t DoRead(void* buf, size_t len);

    virtual size_t DoSkip(size_t len);

private:
    TInputStream* Slave_;
    ui64 Length_;
};


class TCountingInput: public TInputStream {
public:
    inline TCountingInput(TInputStream* slave) throw ()
        : Slave_(slave)
        , Count_()
    {
    }

    virtual ~TCountingInput() throw () {
    }

    inline ui64 Counter() const throw () {
        return Count_;
    }

private:
    virtual size_t DoRead(void* buf, size_t len);

    virtual size_t DoSkip(size_t len);

private:
    TInputStream* Slave_;
    ui64 Count_;
};
