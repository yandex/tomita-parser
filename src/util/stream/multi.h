#pragma once

#include "input.h"

/// @addtogroup Streams_Multi
/// @{

/*
 * merge two input streams in one
 */
/// Объединение двух потоков ввода в один.
/// @details При чтении данных первым обрабатывается поток f.
class TMultiInput: public TInputStream {
    public:
        TMultiInput(TInputStream* f, TInputStream* s) throw ();
        virtual ~TMultiInput() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        TInputStream* C_;
        TInputStream* N_;
};

/// @}
