#pragma once

#include "base.h"

#include <util/system/pipe.h>
#include <util/generic/ptr.h>

/// @addtogroup Streams_Pipes
/// @{

/// Pipe к процессу.
/// @details Запускает системный процесс и открывает к нему канал (pipe).
/// @param[in] command - системная команда.
/// @param[in] mode - режим передачи данных через pipe: 'r' - чтение, 'w' - запись.
class TPipeBase {
    protected:
        TPipeBase(const char* command, const char* mode);
        virtual ~TPipeBase() throw ();

    protected:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// Pipe к процессу. Чтение данных.
/// @details Запускает процесс с командной строкой command и открывает pipe для чтения
/// данных из его потока вывода.
/// @n = TPipeBase (const char* command, 'r').
class TPipeInput: protected TPipeBase, public TInputStream {
    public:
        TPipeInput(const char* command);

    private:
        virtual size_t DoRead(void* buf, size_t len);
};

/// Pipe к процессу. Запись данных.
/// @details Запускает процесс с командной строкой command и открывает pipe для записи
/// данных в его поток ввода.
/// @n = TPipeBase (const char* command, 'w').
class TPipeOutput: protected TPipeBase, public TOutputStream {
    public:
        TPipeOutput(const char* command);
        void Close();

    private:
        virtual void DoWrite(const void* buf, size_t len);
};


/// Pipe системного уровня
/// @details базовая функциональность реализована через класс TPipeHandle
/// @param[in] fd - файловый дескриптор уже открытого пайпа
class TPipedBase {
    protected:
        TPipedBase(PIPEHANDLE fd);
        virtual ~TPipedBase() throw();

    protected:
        TPipeHandle Handle_;
};

/// Pipe системного уровня. Чтение данных
class TPipedInput: public TPipedBase, public TInputStream {
    public:
        TPipedInput(PIPEHANDLE fd);
        virtual ~TPipedInput() throw();

    private:
        virtual size_t DoRead(void* buf, size_t len);
};

/// Pipe системного уровня. Запись данных
class TPipedOutput: public TPipedBase, public TOutputStream {
    public:
        TPipedOutput(PIPEHANDLE fd);
        virtual ~TPipedOutput() throw();

    private:
        virtual void DoWrite(const void* buf, size_t len);
};

/// @}
