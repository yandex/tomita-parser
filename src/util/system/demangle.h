#pragma once

#include <util/generic/ptr.h>
#include <util/generic/stroka.h>

class TCppDemangler {
    public:
        const char* Demangle(const char* name);

    private:
        THolder<char, TFree> TmpBuf_;
};

inline Stroka CppDemangle(const Stroka& name) {
    return TCppDemangler().Demangle(~name);
}
