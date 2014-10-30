#pragma once

#include "ios.h"

#include <util/stream/buffered.h>
#include <util/system/file.h>
#include <util/stream/base.h>

/// @addtogroup Streams_Files
/// @{

/// Чтение данных из файла.
/*
 * BFW: ввод небуферизованный.
 * ReadLine() будет ОЧЕНЬ медленным
 */
class TFileInput: public TInputStream {
    public:
        TFileInput(const TFile& file);
        TFileInput(const Stroka& path);

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        TFile File_;
};

/// Чтение данных из файла, отображенного в память.
class TMappedFileInput: public TMemoryInput {
    public:
        TMappedFileInput(const TFile& file);
        TMappedFileInput(const Stroka& path);
        virtual ~TMappedFileInput() throw ();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// Запись данных в файл.
class TFileOutput: public TOutputStream {
    public:
        TFileOutput(const Stroka& path);
        TFileOutput(const TFile& file);
        virtual ~TFileOutput() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);
        virtual void DoFlush();

    private:
        TFile File_;
};

/// Буферизованное чтение данных из файла.
class TBufferedFileInput: public TBuffered<TFileInput> {
    public:
        template <class T>
        inline TBufferedFileInput(const T& t, size_t buf = 1 << 13)
            : TBuffered<TFileInput>(buf, t)
        {
        }

        virtual ~TBufferedFileInput() throw () {
        }
};

typedef TBufferedFileInput TIFStream;

/// Буферизованная запись данных в файл.
class TBufferedFileOutput: public TBuffered<TFileOutput> {
    public:
        template <class T>
        inline TBufferedFileOutput(const T& t, size_t buf = 1 << 13)
            : TBuffered<TFileOutput>(buf, t)
        {
        }

        virtual ~TBufferedFileOutput() throw () {
        }
};

typedef TBufferedFileOutput TOFStream;

/// @}
