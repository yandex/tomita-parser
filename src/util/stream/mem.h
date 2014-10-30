#pragma once

#include "input.h"
#include "output.h"
#include "zerocopy.h"

#include <util/generic/strbuf.h>

/// @addtogroup Streams_Memory
/// @{

/// Чтение данных из памяти.
/// @details Cоздает поток ввода для считывания данных из памяти.
class TMemoryInput
    : public TInputStream
    , public IZeroCopyInput
{
public:
    TMemoryInput() throw ();

    /// @param[in] buf - размещенные в памяти данные.
    /// @param[in] len - количество байт, доступных для считывания с помощью потока.
    TMemoryInput(const void* buf, size_t len) throw ();
    virtual ~TMemoryInput() throw ();

    /// Заново инициализирует поток ввода для считывания данных из памяти.
    /// @param[in] buf - размещенные в памяти данные.
    /// @param[in] len - количество байт, доступных для считывания с помощью потока.
    inline void Reset(const void* buf, size_t len) throw () {
        Buf_ = (const char*)buf;
        Len_ = len;
    }

    /// Проверяет, есть ли доступная для считывания с помощью потока память.
    inline bool Exhausted() const throw () {
        return !Avail();
    }

    /// Возвращает количество памяти, доступной для считывания с помощью потока.
    inline size_t Avail() const throw () {
        return Len_;
    }

    /// Возвращает указатель на текущую позицию в буфере данных для чтения
    inline const char* Buf() const throw () {
        return Buf_;
    }

    /// Инициализирует поток данными из zs.
    inline void Fill(IZeroCopyInput* zs) {
        if (!zs->Next(&Buf_, &Len_)) {
            Reset(0, 0);
        }
    }

private:
    virtual size_t DoRead(void* buf, size_t len);
    virtual size_t DoSkip(size_t len) throw();
    virtual bool DoReadTo(Stroka& st, char ch);
    virtual bool DoNext(const void** ptr, size_t* len);

private:
    const char* Buf_;
    size_t Len_;
};

/// Запись данных в память.
/// @details Создает поток вывода для записи данных в память.
class TMemoryOutput
    : public TOutputStream
{
public:
    /// @param[in] buf - указатель на буфер для размещения в памяти данных.
    /// @param[in] len - количество байт, доступных для записи с помощью потока.
    TMemoryOutput(void* buf, size_t len) throw ()
        : Buf_(static_cast<char *>(buf))
        , End_(Buf_ + len)
    {}
    virtual ~TMemoryOutput() throw ();

    inline void Reset(void* buf, size_t len) throw () {
        Buf_ = static_cast<char *>(buf);
        End_ = Buf_ + len;
    }

    /// Проверяет, есть ли доступная для записи с помощью потока память.
    inline bool Exhausted() const throw () {
        return !Avail();
    }

    /// Возвращает количество памяти, доступной для записи с помощью потока.
    inline size_t Avail() const throw () {
        return End_ - Buf_;
    }

    /// Возвращает указатель на текущую позицию в буфере данных для записи
    inline char* Buf() const throw () {
        return Buf_;
    }

    char* End() const {
        return End_;
    }

private:
    virtual void DoWrite(const void* buf, size_t len);

protected:
    char* Buf_;
    char* End_;
};

class TMemoryWriteBuffer
    : public TMemoryOutput
{
public:
    /// @param[in] buf - размещенные в памяти данные.
    /// @param[in] len - количество байт, доступных для записи с помощью потока.
    TMemoryWriteBuffer(void* buf, size_t len)
        : TMemoryOutput(buf, len)
        , Beg_(Buf_)
    {}

    void Reset(void* buf, size_t len) {
        TMemoryOutput::Reset(buf, len);
        Beg_ = Buf_;
    }

    size_t Len() const {
        return Buf() - Beg();
    }

    size_t Empty() const {
        return Buf() == Beg();
    }

    /// returns the string which has been written so far
    TStringBuf Str() const {
        return TStringBuf(Beg(), Buf());
    }

    char* Beg() const {
        return Beg_;
    }

    /// @{
    /// repositions the current writing cursor
    void SetPos(char* ptr) {
        YASSERT(Beg_ <= ptr);
        SetPosImpl(ptr);
    }

    void SetPos(size_t pos) {
        SetPosImpl(Beg_ + pos);
    }
    /// @}

protected:
    void SetPosImpl(char* ptr) {
        YASSERT(End_ >= ptr);
        Buf_ = ptr;
    }

protected:
    char* Beg_;
};

/// @}
