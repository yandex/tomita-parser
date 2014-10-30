#include "dynlib.h"
#include "demangle.h"
#include "platform.h"
#include "backtrace.h"

#include <util/stream/ios.h>
#include <util/generic/singleton.h>
#include <util/generic/stroka.h>

#if !defined(HAVE_BACKTRACE) && (defined(_glibc_) || defined(_darwin_)) && !defined(_musl_)
    #define USE_GLIBC_BACKTRACE
    #define HAVE_BACKTRACE
#endif

#if !defined(HAVE_BACKTRACE) && defined(__GNUC__)
    #define USE_GCC_BACKTRACE
    #define HAVE_BACKTRACE
#endif

#if defined(USE_GLIBC_BACKTRACE)
#include <execinfo.h>

size_t BackTrace(void** p, size_t len) {
    return (size_t)backtrace(p, len);
}
#endif

#if defined(USE_GCC_BACKTRACE)
extern "C" {
    struct _Unwind_Context;

    typedef enum {
        _URC_NO_REASON = 0,
        _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
        _URC_FATAL_PHASE2_ERROR = 2,
        _URC_FATAL_PHASE1_ERROR = 3,
        _URC_NORMAL_STOP = 4,
        _URC_END_OF_STACK = 5,
        _URC_HANDLER_FOUND = 6,
        _URC_INSTALL_CONTEXT = 7,
        _URC_CONTINUE_UNWIND = 8
    } _Unwind_Reason_Code;

    typedef _Unwind_Reason_Code (*_Unwind_Trace_Fn)(struct _Unwind_Context*, void*);
    typedef void* _Unwind_Ptr;

    _Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn, void*);
    _Unwind_Ptr _Unwind_GetIP(struct _Unwind_Context*);
};

namespace {
    class TGccBackTraceImpl {
    public:
        inline size_t BackTrace(void** p, size_t len) {
            if (len >= 1) {
                TBackTraceContext bt = {p, 0, len};

                _Unwind_Backtrace(Helper, &bt);

                return bt.cnt - 1;
            }

            return 0;
        }

    private:
        struct TBackTraceContext {
            void** sym;
            size_t cnt;
            size_t size;
        };

        static _Unwind_Reason_Code Helper(struct _Unwind_Context* c, void* h) {
            TBackTraceContext* bt = (TBackTraceContext*)h;

            if (bt->cnt != 0) {
                bt->sym[bt->cnt - 1] = (void*)_Unwind_GetIP(c);
            }

            if (bt->cnt == bt->size) {
                return _URC_END_OF_STACK;
            }

            ++bt->cnt;

            return _URC_NO_REASON;
        }
    };
}

size_t BackTrace(void** p, size_t len) {
    return Singleton<TGccBackTraceImpl>()->BackTrace(p, len);
}
#endif

#if !defined(HAVE_BACKTRACE)
size_t BackTrace(void**, size_t) {
    return 0;
}
#endif

#if defined(_unix_)
#include <util/generic/strfcpy.h>

#include <dlfcn.h>

#if defined(_darwin_)
    #include <execinfo.h>
#endif

static inline const char* CopyTo(const char* from, char* buf, size_t len) {
    strfcpy(buf, from, len);

    return buf;
}

TResolvedSymbol ResolveSymbol(void* sym, char* buf, size_t len) {
    TResolvedSymbol ret = {
        "??",
        sym
    };

    Dl_info dli;

    Zero(dli);

    if (dladdr(sym, &dli) && dli.dli_sname) {
        ret.Name = CopyTo(TCppDemangler().Demangle(dli.dli_sname), buf, len);
        ret.NearestSymbol = dli.dli_saddr;
    }

    return ret;
}
#else
TResolvedSymbol ResolveSymbol(void* sym, char*, size_t) {
    TResolvedSymbol ret = {
        "??",
        sym
    };

    return ret;
}
#endif

void FormatBackTrace(TOutputStream* out, void* backtrace[], size_t backtraceSize) {
    char tmpBuf[1024];

    for (size_t i = 0; i < backtraceSize; ++i) {
        TResolvedSymbol rs = ResolveSymbol(backtrace[i], tmpBuf, sizeof(tmpBuf));

        *out << rs.Name << "+" << ((long)backtrace[i] - (long)rs.NearestSymbol) << Endl;
    }
}

void FormatBackTrace(TOutputStream* out) {
    void* array[300];
    const size_t s = BackTrace(array, ARRAY_SIZE(array));
    FormatBackTrace(out, array, s);
}

void PrintBackTrace() {
    FormatBackTrace(&Cerr);
}

TBackTrace::TBackTrace()
    : Size(0)
{
}

void TBackTrace::Capture() {
    Size = BackTrace(Data, CAPACITY);
}

// copy-paste of FormatBackTrace from util
void TBackTrace::PrintTo(TOutputStream& out) const {
    char tmpBuf[1024];

    for (size_t i = 0; i < Size; ++i) {
        TResolvedSymbol rs = ResolveSymbol(Data[i], tmpBuf, sizeof(tmpBuf));

        out << rs.Name << "+" << ((long)Data[i] - (long)rs.NearestSymbol) << Endl;
    }
}

Stroka TBackTrace::PrintToString() const {
    TStringStream ss;
    PrintTo(ss);
    return ss;
}
