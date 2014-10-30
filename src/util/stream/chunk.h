#pragma once

#include "ios.h"

#include <util/generic/ptr.h>

/// @addtogroup Streams_Chunked
/// @{

/// Ввод данных порциями.
/// @details Последовательное чтение блоков данных. Предполагается, что
/// данные записаны в виде <длина блока><блок данных>.
class TChunkedInput: public TInputStream {
    public:
        TChunkedInput(TInputStream* slave);
        virtual ~TChunkedInput() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// Вывод данных порциями.
/// @details Вывод данных блоками в виде <длина блока><блок данных>. Если объем
/// данных превышает 64K, они записываются в виде n блоков по 64K + то, что осталось.
class TChunkedOutput: public TOutputStream {
    public:
        TChunkedOutput(TOutputStream* slave);
        virtual ~TChunkedOutput() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);
        virtual void DoFlush();
        virtual void DoFinish();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// Ввод бинарных данных порциями.
/// @details Аналогично TChunkedInput, но для бинарных данных.
class TBinaryChunkedInput: public TInputStream {
    public:
        TBinaryChunkedInput(TInputStream* input);
        virtual ~TBinaryChunkedInput() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// Вывод бинарных данных порциями.
/// @details Аналогично TChunkedOutput, но для бинарных данных.
class TBinaryChunkedOutput: public TOutputStream {
    public:
        TBinaryChunkedOutput(TOutputStream* output);
        virtual ~TBinaryChunkedOutput() throw ();

    private:
        virtual void DoWrite(const void* buf_in, size_t len);
        virtual void DoFinish();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// @}
