#include "input.h"
#include "helpers.h"

#include <util/string/cast.h>
#include <util/memory/tempbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/yexception.h>
#include <util/charset/recyr.hh>
#include <util/generic/singleton.h>

TInputStream::TInputStream() throw () {
}

TInputStream::~TInputStream() throw () {
}

bool TInputStream::DoReadTo(Stroka& st, char to) {
    char ch;

    if (!Read(&ch, 1)) {
        return false;
    }

    st.clear();

    do {
        if (ch == to) {
            break;
        }

        st += ch;
    } while (Read(&ch, 1));

    return true;
}

size_t TInputStream::Load(void* buf_in, size_t len) {
    char* buf = (char*)buf_in;

    while (len) {
        const size_t ret = Read(buf, len);

        buf += ret;
        len -= ret;

        if (ret == 0) {
            break;
        }
    }

    return buf - (char*)buf_in;
}

void TInputStream::LoadOrFail(void* buf, size_t len) {
    const size_t realLen = Load(buf, len);
    if (EXPECT_FALSE(realLen != len)) {
        ythrow yexception() << "Failed to read required number of bytes from stream! Expected: " << len << ", gained: " << realLen << "!";
    }
}

bool TInputStream::ReadLine(Stroka& st) {
    const bool ret = ReadTo(st, '\n');

    if (ret && !st.empty() && st[st.length() - 1] == '\r') {
        st.pop_back();
    }

    return ret;
}

bool TInputStream::ReadLine(Wtroka& w) {
    Stroka s;
    if (!ReadLine(s))
        return false;

    UTF8ToWide(s, w);
    return true;
}

Stroka TInputStream::ReadLine() {
    Stroka ret;

    if (!ReadLine(ret)) {
        ythrow yexception() << "can not read line from stream";
    }

    return ret;
}

Stroka TInputStream::ReadTo(char ch) {
    Stroka ret;

    if (!ReadTo(ret, ch)) {
        ythrow yexception() << "can not read from stream";
    }

    return ret;
}

size_t TInputStream::Skip(size_t sz) {
    return DoSkip(sz);
}

size_t TInputStream::DoSkip(size_t sz) {
    TTempBuf buf;
    size_t total = 0;
    /* Read intentially is called at least once
       even if sz initially is zero
       in case Read do some checks, had side-effects or throws exceptions */
    do {
        size_t lresult = Read(buf.Data(), Min<size_t>(sz, buf.Size()));
        if (lresult == 0)
            return total;
        total += lresult;
        sz -= lresult;
    } while (sz);

    return total;
}

Stroka TInputStream::ReadAll() {
    Stroka ret;
    TStringOutput so(ret);

    TransferData(this, &so);

    return ret;
}

namespace {
    class TStdIn: public TInputStream {
    public:
        inline TStdIn() throw ()
            : F_(stdin)
        {
        }

        virtual ~TStdIn() throw () {
        }

    private:
        virtual size_t DoRead(void* buf, size_t len) {
            const size_t ret = fread(buf, 1, len, F_);

            if (ret < len && ferror(F_)) {
                ythrow TSystemError() << "can not read from stdin";
            }

            return ret;
        }

        virtual bool DoReadTo(Stroka& st, char ch) {
            if (ch != '\n') {
                return TInputStream::DoReadTo(st, ch);
            }

            st.clear();
            char line[256];
            while (fgets(line, ARRAY_SIZE(line), F_)) {
                st.append(line);
                if (st.back() == '\n') {
                    st.pop_back();
                    return true;
                }
            }
            if (ferror(F_)) {
                ythrow TSystemError() << "can not read from stdin";
            }
            return !st.empty();
        }

    private:
        FILE* F_;
    };
}

TInputStream& StdInStream() throw () {
    return *SingletonWithPriority<TStdIn, 4>();
}

// implementation of >> operator

// helper functions

static inline bool IsStdDelimiter(char c) {
    return (c == '\0') || (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t');
}

static void ReadUpToDelimiter(TInputStream& i, Stroka& s) {
    char c;
    while (i.ReadChar(c)) { // skip delimiters
        if (!IsStdDelimiter(c)) {
            s += c;
            break;
        }
    }
    while (i.ReadChar(c) && !IsStdDelimiter(c)) { // read data (with trailing delimiter)
        s += c;
    }
}

// specialization for string-related stuff

template <>
void In<Stroka>(TInputStream& i, Stroka& s) {
    s.resize(0);
    ReadUpToDelimiter(i, s);
}

template <>
void In<Wtroka>(TInputStream& i, Wtroka& w) {
    Stroka s;
    ReadUpToDelimiter(i, s);

    if (s.empty()) {
        w.erase();
    } else {
        Wtroka temp((int)s.size());
        size_t read = 0, written = 0;
        RecodeToUnicode(CODES_UTF8, s.c_str(), temp.begin(), s.size(), s.size(), read, written);
        temp.remove(written);
        w.assign(temp);
    }
}

// specialization for char types

#define SPEC_FOR_CHAR(T) \
template <> \
void In<T>(TInputStream& i, T& t) { \
    i.ReadChar((char&)t); \
}

SPEC_FOR_CHAR(char)
SPEC_FOR_CHAR(unsigned char)
SPEC_FOR_CHAR(signed char)

#undef SPEC_FOR_CHAR

// specialization for number types

#define SPEC_FOR_NUMBER(T) \
template <> \
void In<T>(TInputStream& i, T& t) { \
    char buf[128]; \
    size_t pos = 0; \
    while (i.ReadChar(buf[0])) { \
        if (!IsStdDelimiter(buf[0])) { \
            ++pos; \
            break; \
        } \
    } \
    while (i.ReadChar(buf[pos]) && !IsStdDelimiter(buf[pos]) && pos < 127) { \
        ++pos; \
    } \
    t = FromString<T, char>(buf, pos); \
}

SPEC_FOR_NUMBER(signed short)
SPEC_FOR_NUMBER(signed int)
SPEC_FOR_NUMBER(signed long int)
SPEC_FOR_NUMBER(signed long long int)
SPEC_FOR_NUMBER(unsigned short)
SPEC_FOR_NUMBER(unsigned int)
SPEC_FOR_NUMBER(unsigned long int)
SPEC_FOR_NUMBER(unsigned long long int)

SPEC_FOR_NUMBER(float)
SPEC_FOR_NUMBER(double)
SPEC_FOR_NUMBER(long double)

#undef SPEC_FOR_NUMBER
