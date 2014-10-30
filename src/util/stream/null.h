#pragma once

#include "base.h"

/// @addtogroup Streams
/// @{

/// Нулевой поток ввода.
class TNullInput: public TInputStream, public IZeroCopyInput {
    public:
        TNullInput() throw ();
        virtual ~TNullInput() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);
        virtual bool DoNext(const void** ptr, size_t* len);
};

/// Нулевой поток вывода.
class TNullOutput: public TOutputStream {
    public:
        TNullOutput() throw ();
        virtual ~TNullOutput() throw ();

    private:
        virtual void DoWrite(const void* /*buf*/, size_t /*len*/);
};

/// Нулевой поток ввода-вывода.
class TNullIO: public TNullInput, public TNullOutput {
    public:
        TNullIO() throw ();
        virtual ~TNullIO() throw ();
};

/*
 * common streams
 */
TNullIO& StdNullStream() throw ();

#define Cnull (StdNullStream())

/// @}
