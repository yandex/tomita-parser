#pragma once

#include "output.h"

/// @addtogroup Streams
/// @{

/// Отладочный поток вывода.
/// @details Вывод в stderr.
class TDebugOutput: public TOutputStream {
    public:
        inline TDebugOutput() throw () {}
        virtual ~TDebugOutput() throw () {}

    private:
        virtual void DoWrite(const void* buf, size_t len);
};

/// Отладочный поток вывода.
/// @details Если переменная окружения DBGOUT = 1, вывод производится в Cerr,
/// в противном случае - в Cnull.
TOutputStream& StdDbgStream() throw ();

int StdDbgLevel() throw ();

#define Cdbg (StdDbgStream())

/// @}
