#include "platform.h"

#ifndef _win_
    #include <stdexcept>
    #include <cxxabi.h>
#endif

#include "demangle.h"

const char* TCppDemangler::Demangle(const char* name) {
#ifdef _win_
    return name;
#else
    int status;
    TmpBuf_.Reset(__cxxabiv1::__cxa_demangle(name, 0, 0, &status));

    if (!TmpBuf_) {
        return name;
    }

    return TmpBuf_.Get();
#endif
}
