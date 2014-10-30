#pragma once

#include "output.h"

/// @addtogroup Streams_Multi
/// @{

/// Вывод данных в два потока одновременно.
class TTeeOutput: public TOutputStream {
    public:
        TTeeOutput(TOutputStream* l, TOutputStream* r) throw ();
        virtual ~TTeeOutput() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);
        virtual void DoFlush();
        virtual void DoFinish();

    private:
        TOutputStream* L_;
        TOutputStream* R_;
};

/// @}
