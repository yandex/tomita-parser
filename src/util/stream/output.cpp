#include "ios.h"
#include "output.h"

#include <util/string/cast.h>
#include <util/stream/format.h>
#include <util/memory/tempbuf.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/charset/wide.h>
#include <util/charset/recyr.hh>

#include <cerrno>
#include <string>
#include <cstdio>

TOutputStream::TOutputStream() throw () {
}

TOutputStream::~TOutputStream() throw () {
}

void TOutputStream::DoFlush() {
    /*
     * do nothing
     */
}

void TOutputStream::DoFinish() {
    Flush();
}

void TOutputStream::DoWriteV(const TPart* parts, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const TPart& part = parts[i];

        DoWrite(part.buf, part.len);
    }
}

template <>
void Out<Stroka>(TOutputStream& o, const Stroka& p) {
    o.Write(~p, +p);
}

template <>
void Out<stroka>(TOutputStream& o, const stroka& p) {
    o.Write(~p, +p);
}

template <>
void Out<std::string>(TOutputStream& o, const std::string& p) {
    o.Write(p.data(), p.length());
}

template <>
void Out<TFixedString<char> >(TOutputStream& o, const TFixedString<char>& p) {
    o.Write(p.Start, p.Length);
}

template <>
void Out<TFixedString<wchar16> >(TOutputStream& o, const TFixedString<wchar16>& p) {
    WriteString(o, p.Start, p.Length);
}

void WriteString(TOutputStream& o, const wchar16* w, size_t n) {
    const size_t buflen = (n * 4); // * 4 because the conversion functions can convert unicode character into maximum 4 bytes of UTF8
    TTempBuf buffer(buflen + 1);
    char* const data = buffer.Data();
    size_t written = 0;
    WideToUTF8(w, n, data, written);
    data[written] = 0;
    o.Write(data, written);
}

#define DEF_CONV_DEFAULT(type)                  \
    template <>                                 \
    void Out<type>(TOutputStream& o, type p) {  \
        o << ToString(p);                       \
    }

#define DEF_CONV_CHR(type)                      \
    template <>                                 \
    void Out<type>(TOutputStream& o, type p) {  \
        o.Write((const char*)&p, 1);            \
    }

#define DEF_CONV_NUM(type)                                              \
    template <>                                                         \
    void Out<type>(TOutputStream& o, type p) {                          \
        char buf[256];                                                  \
        o.Write(buf, ToString(p, buf, sizeof(buf)));                    \
    } \
    \
    template <>                                                         \
    void Out<volatile type>(TOutputStream& o, volatile type p) {        \
        Out<type>(o, p); \
    }


DEF_CONV_NUM(bool)
DEF_CONV_CHR(char)
DEF_CONV_CHR(signed char)
DEF_CONV_NUM(signed short)
DEF_CONV_NUM(signed int)
DEF_CONV_NUM(signed long int)
DEF_CONV_NUM(signed long long int)
DEF_CONV_CHR(unsigned char)
DEF_CONV_NUM(unsigned short)
DEF_CONV_NUM(unsigned int)
DEF_CONV_NUM(unsigned long int)
DEF_CONV_NUM(unsigned long long int)
DEF_CONV_NUM(float)
DEF_CONV_NUM(double)
DEF_CONV_NUM(long double)

template <>
void Out<const void*>(TOutputStream& o, const void* t) {
    o << Hex(size_t(t));
}

template <>
void Out<void*>(TOutputStream& o, void* t) {
    Out<const void*>(o, t);
}

namespace {
    class TStdOutput: public TOutputStream {
    public:
        inline TStdOutput(FILE* f) throw ()
            : F_(f)
        {
        }

        virtual ~TStdOutput() throw () {
        }

    private:
        virtual void DoWrite(const void* buf, size_t len) {
            if (len != fwrite(buf, 1, len, F_)) {
#ifdef _win_
                // On Windows, if 'F_' is console -- 'fwrite' returns count of written characters.
                // If, for example, console output codepage is UTF-8, then returned value is
                // not equal to 'len'. But 'errno' and 'GetLastError' return success (zero code).
                if (errno == 0 && isatty(fileno(F_)))
                    return;
#endif // _win_
                ythrow TSystemError() << "write failed";
            }
        }

        virtual void DoFlush() {
            fflush(F_);
        }

    private:
        FILE* F_;
    };

    class TStdErr: public TStdOutput {
    public:
        inline TStdErr()
            : TStdOutput(stderr)
        {
        }

        virtual ~TStdErr() throw () {
        }
    };

    class TStdOut: public TStdOutput {
    public:
        inline TStdOut()
            : TStdOutput(stdout)
        {
        }

        virtual ~TStdOut() throw () {
        }
    };
}

TOutputStream& StdErrStream() throw () {
    return *SingletonWithPriority<TStdErr, 4>();
}

TOutputStream& StdOutStream() throw () {
    return *SingletonWithPriority<TStdOut, 4>();
}
