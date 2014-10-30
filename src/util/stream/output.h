#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>

class Stroka;

/// @addtogroup Streams_Base
/// @{

/// Базовый поток вывода.
class TOutputStream {
    public:
        /// Блок данных для вывода в поток.
        struct TPart {
            inline TPart(const void* Buf, size_t Len) throw ()
                : buf(Buf)
                , len(Len)
            {
            }

            inline TPart(const TStringBuf& s) throw ()
                : buf(~s)
                , len(+s)
            {
            }

            inline TPart() throw ()
                : buf(0)
                , len(0)
            {
            }

            inline ~TPart() throw () {
            }

            static inline TPart CrLf() throw () {
                return TPart("\r\n", 2);
            }

            const void* buf;
            size_t      len;
        };

        TOutputStream() throw ();
        virtual ~TOutputStream() throw ();

        /// Записывает в поток len байт буфера buf.
        inline void Write(const void* buf, size_t len) {
            if (len) {
                DoWrite(buf, len);
            }
        }

        inline void Write(const TStringBuf& st) {
            Write(~st, +st);
        }

        /// Записывает в поток count блоков данных из массива parts.
        inline void Write(const TPart* parts, size_t count) {
            if (count) {
                DoWriteV(parts, count);
            }
        }

        /// Записывает в поток символ ch.
        inline void Write(char ch) {
            Write(&ch, 1);
        }

        /// Записывает содержимое буфера потока (сбрасывает буфер).
        inline void Flush() {
            DoFlush();
        }

        // flush and close
        /// Сообщает потоку, что запись в него завершена.
        inline void Finish() {
            DoFinish();
        }

    protected:
        virtual void DoWrite(const void* buf, size_t len) = 0;
        virtual void DoWriteV(const TPart* parts, size_t count);
        virtual void DoFlush();
        virtual void DoFinish();
};

template <class T>
void Out(TOutputStream& o, typename TTypeTraits<T>::TFuncParam t);

#define DECLARE_OUT_SPEC(MODIF, T, stream, value) \
    template <> MODIF void Out< T >(TOutputStream& stream, TTypeTraits< T >::TFuncParam value)

template <>
inline void Out<const char*>(TOutputStream& o, const char* t) {
    if (t) {
        o.Write(t);
    } else {
        o.Write("(null)");
    }
}

template <>
inline void Out<char*>(TOutputStream& o, char* t) {
    Out<const char*>(o, t);
}

void WriteString(TOutputStream& o, const wchar16* w, size_t n);

template <>
inline void Out<const wchar16*>(TOutputStream& o, const wchar16* w) {
    if (w) {
        WriteString(o, w, TCharTraits<wchar16>::GetLength(w));
    } else {
        o.Write("(null)");
    }
}

template <>
inline void Out<Wtroka>(TOutputStream& o, const Wtroka& w) {
    WriteString(o, w.c_str(), w.size());
}

typedef void (*TStreamManipulator)(TOutputStream&);

static inline TOutputStream& operator<< (TOutputStream& o, TStreamManipulator m) {
    m(o);

    return o;
}

static inline TOutputStream& operator<< (TOutputStream& o, const char* t) {
    Out<const char*>(o, t);

    return o;
}

static inline TOutputStream& operator<< (TOutputStream& o, char* t) {
    Out<const char*>(o, t);

    return o;
}

template <class T>
static inline TOutputStream& operator<< (TOutputStream& o, const T& t) {
    Out<T>(o, t);

    return o;
}

static inline TOutputStream& operator<< (TOutputStream& o, const wchar16* t) {
    Out<const wchar16*>(o, t);
    return o;
}

static inline TOutputStream& operator<< (TOutputStream& o, wchar16* t) {
    Out<const wchar16*>(o, t);
    return o;
}

/*
 * common streams
 */
TOutputStream& StdErrStream() throw ();
TOutputStream& StdOutStream() throw ();

#define Cout (StdOutStream())
#define Cerr (StdErrStream())
#define Clog Cerr

/*
 * end-of-line stream manipulator
 */
static inline void Endl(TOutputStream& o) {
    (o << '\n').Flush();
}

static inline void Flush(TOutputStream& o) {
    o.Flush();
}

// see also format.h for additional formats

#include "debug.h"

/// @}
