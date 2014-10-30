#include "yassert.h"
#include "src_root.h"

#include <util/stream/str.h>
#include <util/system/guard.h>
#include <util/string/util.h>
#include <util/stream/output.h>
#include <util/system/spinlock.h>
#include <util/system/backtrace.h>
#include <util/generic/stroka.h>
#include <util/generic/singleton.h>

#include <stdio.h>
#include <stdarg.h>

namespace {
    struct TPanicLockHolder: public TAdaptiveLock {
    };
}

void ::NPrivate::Panic(const TStaticBuf& file, int line, const char* function, const char* expr, const char* format, ...) throw () {
    try {
        // We care of panic of first failed thread only
        // Otherwise stderr could contain multiple messages and stack traces shuffled
        TGuard<TAdaptiveLock> guard(*Singleton<TPanicLockHolder>());

        Stroka errorMsg;
        va_list args;
        va_start(args, format);
        // format has " " prefix to mute GCC warning on empty format
        vsprintf(errorMsg, format[0] == ' ' ? format + 1 : format, args);
        va_end(args);

        Stroka r;
        TStringOutput o(r);
        if (expr) {
            o << "VERIFY failed: " << errorMsg << Endl;
        } else {
            o << "FAIL: " << errorMsg << Endl;
        }
        o << "  " << file.As<TStringBuf>() << ":" << line << Endl;
        if (expr) {
            o << "  " << function << "(): requirement " << expr << " failed" << Endl;
        } else {
            o << "  " << function << "() failed" << Endl;
        }
        Cerr << r;
#ifndef WITH_VALGRIND
        PrintBackTrace();
#endif
    } catch (...) {
        //nothing we can do here
    }

    abort();
}
