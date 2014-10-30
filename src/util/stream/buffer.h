#pragma once

#include "base.h"

#include <util/generic/ptr.h>

class TBuffer;
/// @addtogroup Streams_Buffers
/// @{

/// Запись данных в буфер - собственный или внешний.
class TBufferOutput: public TOutputStream {
public:
    class TImpl;

    /*
     * construct own buffer
     */
    TBufferOutput(size_t buflen = 1024);

    /*
     * use external buffer
     */
    TBufferOutput(TBuffer& buffer);

    virtual ~TBufferOutput() throw ();

    TPart GetBuffer() const throw ();

    /// Возвращает буфер, в который записываются данные потока.
    TBuffer& Buffer() const throw ();

private:
    virtual void DoWrite(const void* buf, size_t len);

private:
    THolder<TImpl> Impl_;
};

/// Чтение данных из внешнего буфера buf.
class TBufferInput: public TInputStream {
    public:
        inline TBufferInput(const TBuffer& buf)
            : Buf_(buf)
            , Readed_(0)
        {
        }

        inline void Rewind() throw () {
            Readed_ = 0;
        }

        virtual ~TBufferInput() throw () {
        }

    private:
        virtual size_t DoRead(void* buf, size_t len);
        virtual size_t DoSkip(size_t len) throw();

    private:
        const TBuffer& Buf_;
        size_t Readed_;
};

/// Поток ввода/вывода для манипулирования данными буфера - собственного или внешнего.
class TBufferStream: public TBufferOutput,
                     public TBufferInput {
    public:
        /*
         * construct own buffer
         */
        inline TBufferStream(size_t buflen = 1024)
            : TBufferOutput(buflen)
            , TBufferInput(Buffer())
        {
        }

        /*
         * use external buffer
         */
        inline TBufferStream(TBuffer& buffer)
            : TBufferOutput(buffer)
            , TBufferInput(Buffer())
        {
        }

        virtual ~TBufferStream() throw () {
        }
};

/// @}
