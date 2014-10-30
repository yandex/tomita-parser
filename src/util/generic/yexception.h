#pragma once

#include "strbuf.h"
#include "stroka.h"
#include "utility.h"

#include <util/system/error.h>
#include <util/system/compat.h>
#include <util/system/src_root.h>
#include <util/stream/tempbuf.h>

#include <stlport/exception>

#include <cstdio>
#include <cstdarg>

#if defined (_MSC_VER)
    // it's the known bug in Visual Studio 2008
    // if we ignore the warning the exception is caught just fine
#   pragma warning(disable:4673) /*throwing 'exception class name' the following types will not be considered at the catch site*/
#endif

namespace NPrivateException {
    class yexception: public NStl::exception {
    public:
        virtual const char* what() const throw ();

        template <class T>
        inline void Append(const T& t) const {
            (TOutputStream&)Buf_ << t;
        }

        TStringBuf AsStrBuf() const {
            return TStringBuf(Buf_.Data(), Buf_.Filled());
        }

    private:
        mutable TTempBufOutput Buf_;
    };

    template <class E, class T>
    static inline const E& operator<< (const E& e, const T& t) {
        e.Append(t);

        return e;
    }

    struct TLineInfo {
        inline TLineInfo(const TStringBuf& f, int l)
            : File(f)
            , Line(l)
        {
        }

        TStringBuf File;
        int Line;
    };

    template <class T>
    static inline const T& operator+ (const TLineInfo& li, const T& t) {
        return (t << li);
    }
}

class yexception: public NPrivateException::yexception {
};

DECLARE_OUT_SPEC(inline, yexception, stream, value) {
    stream << value.AsStrBuf();
}

namespace NPrivate {
    class TSystemErrorStatus {
    public:
        inline TSystemErrorStatus()
            : Status_(LastSystemError())
        {
        }

        inline TSystemErrorStatus(int status)
            : Status_(status)
        {
        }

        inline int Status() const throw () {
            return Status_;
        }

    private:
        int Status_;
    };
}

class TSystemError: public virtual NPrivate::TSystemErrorStatus, public virtual yexception {
public:
    inline TSystemError(int status)
        : TSystemErrorStatus(status)
    {
        Init();
    }

    inline TSystemError() {
        Init();
    }

private:
    void Init();
};

struct TIoException: public virtual yexception {
};

class TIoSystemError: public TSystemError, public TIoException {
};

class TFileError: public TIoSystemError {
};

struct TBadCastException: public virtual yexception {
};

#define ythrow throw NPrivateException::TLineInfo(__SOURCE_FILE__, __LINE__) +

static inline void fputs(const std::exception &e, FILE *f = stderr) {
    char message[256];
    size_t len = Min(sizeof(message) - 2, strlcpy(message, e.what(), sizeof(message) - 1));
    message[len++] = '\n';
    message[len] = 0;
    fputs(message, f);
}

Stroka CurrentExceptionMessage();
bool UncaughtException() throw ();

void ThrowBadAlloc();
void ThrowLengthError(const char* descr);
void ThrowRangeError(const char* descr);



#define ENSURE_EX(CONDITION, THROW_EXPRESSION) do { if (EXPECT_FALSE(!(CONDITION))) { ythrow THROW_EXPRESSION;  } } while(0)

#define ENSURE(CONDITION, MESSAGE) ENSURE_EX(CONDITION, yexception() << MESSAGE)
