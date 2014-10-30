#include "execpath.h"
#include "progname.h"

#include <util/folder/dirut.h>
#include <util/generic/singleton.h>

static const char* Argv0;

namespace {
struct TProgramNameHolder {
    inline TProgramNameHolder()
        : ProgName(GetFileNameComponent(Argv0 ? Argv0 : ~GetExecPath()))
        , ProgDir(StripFileComponent(Argv0 ? Argv0 : ~GetExecPath()))
    {
    }

    Stroka ProgName;
    Stroka ProgDir;
};
}

const Stroka& GetProgramName() {
    return Singleton<TProgramNameHolder>()->ProgName;
}

const Stroka& GetProgramDir() {
    return Singleton<TProgramNameHolder>()->ProgDir;
}

void SetProgramName(const char* argv0) {
    Argv0 = argv0;
}
