#pragma once

#include "mem.h"
#include "output.h"

#include <util/string/cast.h>
#include <util/memory/tempbuf.h>
#include <util/generic/strbuf.h>

enum EHexFlags {
    HF_FULL = 0x01,  // Write hex number with leading zeros
    HF_ADDX = 0x02,  // Put '0x' before hex digits
};

namespace NFormatPrivate {
    static inline void WriteChars(TOutputStream& os, char c, size_t count) {
        if (count == 0)
            return;
        TTempBuf buf(count);
        memset(buf.Data(), c, count);
        os.Write(buf.Data(), count);
    }

    template <typename T>
    struct TLeftPad {
        T Value;
        size_t Width;
        char Padc;

        inline TLeftPad(const T& value, size_t width, char padc)
            : Value(value)
            , Width(width)
            , Padc(padc)
        {
        }
    };

    template <typename T>
    TOutputStream& operator<<(TOutputStream& o, const TLeftPad<T>& lp) {
        TTempBuf buf;
        TMemoryOutput ss(buf.Data(), buf.Size());
        ss << lp.Value;
        size_t written = buf.Size() - ss.Avail();
        if (lp.Width > written) {
            WriteChars(o, lp.Padc, lp.Width - written);
        }
        o.Write(buf.Data(), written);
        return o;
    }

    template <typename T>
    struct TRightPad {
        T Value;
        size_t Width;
        char Padc;

        inline TRightPad(const T& value, size_t width, char padc)
            : Value(value)
            , Width(width)
            , Padc(padc)
        {
        }
    };

    template <typename T>
    TOutputStream& operator<<(TOutputStream& o, const TRightPad<T>& lp) {
        TTempBuf buf;
        TMemoryOutput ss(buf.Data(), buf.Size());
        ss << lp.Value;
        size_t written = buf.Size() - ss.Avail();
        o.Write(buf.Data(), written);
        if (lp.Width > written) {
            WriteChars(o, lp.Padc, lp.Width - written);
        }
        return o;
    }

    template <typename T>
    struct THex {
        typedef typename TTypeTraits<T>::TNonQualified TR;
        STATIC_ASSERT(TTypeTraits<TR>::IsIntegral);

        typedef typename TTypeTraits<TR>::TUnsigned TUnsigned;
        TUnsigned Value;
        ui32 Flags;

        inline THex(const T& value, ui32 flags)
            : Value(value)
            , Flags(flags)
        {
        }
    };

    template <typename T>
    TOutputStream& operator<<(TOutputStream& o, const THex<T>& hex) {
        char buf[2 * sizeof(T)];
        TStringBuf str(buf, IntToString<16>(hex.Value, buf, sizeof(buf)));
        if (hex.Flags & HF_ADDX)
            o << "0x";
        if (hex.Flags & HF_FULL)
            WriteChars(o, '0', sizeof(buf) - str.size());
        o << str;
        return o;
    }

    template <typename T>
    struct TSHex {
        typedef typename TTypeTraits<T>::TNonQualified TdVal;
        STATIC_ASSERT(TTypeTraits<TdVal>::IsIntegral);

        TdVal Value;
        ui32 Flags;

        inline TSHex(const T& value, ui32 flags)
            : Value(value)
            , Flags(flags)
        {
        }
    };

    template <typename T>
    TOutputStream& operator<<(TOutputStream& o, const TSHex<T>& hex) {
        char buf[2 * sizeof(T) + 1]; // add 1 for the sign
        TStringBuf str(buf, IntToString<16>(hex.Value, buf, sizeof(buf)));
        if ('-' == str[0]) {
            o << '-';
            ++str;
        }
        if (hex.Flags & HF_ADDX)
            o << "0x";
        if (hex.Flags & HF_FULL)
            WriteChars(o, '0', sizeof(buf) - 1 - str.size());
        o << str;
        return o;
    }
}

template <typename T>
static inline ::NFormatPrivate::TLeftPad<T> LeftPad(const T& value, size_t width, char padc = ' ') {
    return ::NFormatPrivate::TLeftPad<T>(value, width, padc);
}

template<typename T, int N>
static inline ::NFormatPrivate::TLeftPad<const T*> LeftPad(const T (&value)[N], size_t width, char padc = ' ') {
    return ::NFormatPrivate::TLeftPad<const T*>(value, width, padc);
}

template <typename T>
static inline ::NFormatPrivate::TRightPad<T> RightPad(const T& value, size_t width, char padc = ' ') {
    return ::NFormatPrivate::TRightPad<T>(value, width, padc);
}

template<typename T, int N>
static inline ::NFormatPrivate::TRightPad<const T*> RightPad(const T (&value)[N], size_t width, char padc = ' ') {
    return ::NFormatPrivate::TRightPad<const T*>(value, width, padc);
}

template <typename T>
static inline ::NFormatPrivate::THex<T> Hex(const T& value, ui32 flags = HF_FULL | HF_ADDX) {
    return ::NFormatPrivate::THex<T>(value, flags);
}

template <typename T>
static inline ::NFormatPrivate::TSHex<T> SHex(
    const T& value, ui32 flags = HF_FULL | HF_ADDX)
{
    return ::NFormatPrivate::TSHex<T>(value, flags);
}

void Time(TOutputStream& l);
void TimeHumanReadable(TOutputStream& l);
