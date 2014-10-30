#pragma once

#include "codepage.h"

#include <util/generic/noncopyable.h>

#include <contrib/libs/libiconv/iconv.h>

// WARNING: Do not use this functions - use functions from wide.h or recyr.hh instead.

namespace NICONVPrivate {
    inline const char* CharsetName(ECharset code) {
        return NameByCharset(code);
    }
    inline const char* CharsetName(const char* code) {
        return code;
    }

    template<int size>
    inline const char* UnicodeNameBySize();

    template<>
    inline const char* UnicodeNameBySize<1>() {
        return "UTF-8";
    }
    template<>
    inline const char* UnicodeNameBySize<2>() {
        return "UTF-16LE";
    }
    template<>
    inline const char* UnicodeNameBySize<4>() {
        return "UCS-4LE";
    }

    template<class C>
    inline const char* UnicodeName() {
        return UnicodeNameBySize<sizeof(C)>();
    }

    class TDescriptor : NNonCopyable::TNonCopyable {
    private:
        iconv_t Descriptor;

    public:
        template <class TFrom, class TTo>
        inline TDescriptor(TFrom from, TTo to)
            // iconv_open(to, from) - be careful in parameters order!
            : Descriptor(iconv_open(CharsetName(to), CharsetName(from)))
        {
            if (!Invalid()) {
                int temp = 1;
                iconvctl(Descriptor, ICONV_SET_DISCARD_ILSEQ, &temp);
            }
        }

        inline ~TDescriptor() {
            if (!Invalid()) {
                iconv_close(Descriptor);
            }
        }

        inline iconv_t Get() const {
            return Descriptor;
        }

        bool Invalid() const {
            return Descriptor == (iconv_t)(-1);
        }
    };

    template <class TFrom, class TTo>
    inline bool CanConvert(TFrom from, TTo to) {
        TDescriptor descriptor(from, to);

        return !descriptor.Invalid();
    }

    inline size_t RecodeImpl(const TDescriptor& descriptor, const char* in, char* out, size_t inSize, size_t outSize, size_t& read, size_t& written) {
        YASSERT(!descriptor.Invalid());
        YASSERT(in);
        YASSERT(out);

        const char* inPtr = in;
        char* outPtr = out;
        size_t inSizeMod = inSize;
        size_t outSizeMod = outSize;
        size_t res = iconv(descriptor.Get(), &inPtr, &inSizeMod, &outPtr, &outSizeMod);

        read = inSize - inSizeMod;
        written = outSize - outSizeMod;

        return res;
    }

    template <class TFrom, class TTo>
    inline void Recode(TFrom from, TTo to, const char* in, char* out, size_t inSize, size_t outSize, size_t& read, size_t& written) {
        TDescriptor descriptor(from, to);

        if (descriptor.Invalid()) {
            ythrow yexception() << "Can not convert from " << CharsetName(from) << " to " << CharsetName(to);
        }

        size_t res = RecodeImpl(descriptor, in, out, inSize, outSize, read, written);

        if (res == static_cast<size_t>(-1)) {
            switch (errno) {
                case EILSEQ:
                    read = inSize;
                    break;
                case EINVAL:
                    read = inSize;
                    break;
                case E2BIG:
                    ythrow yexception() << "Iconv error: output buffer is too small";
                default:
                    ythrow yexception() << "Unknown iconv error";
            }
        }
    }

    template<class TCharType>
    inline void RecodeToUnicode(ECharset from, const char* in, TCharType* out, size_t inSize, size_t outSize, size_t &read, size_t &written) {
        const size_t charSize = sizeof(TCharType);

        Recode(from, UnicodeName<TCharType>(), in, reinterpret_cast<char*>(out), inSize, outSize * charSize, read, written);
        written /= charSize;
    }

    template<class TCharType>
    inline void RecodeFromUnicode(ECharset to, const TCharType* in, char* out, size_t inSize, size_t outSize, size_t &read, size_t &written) {
        const size_t charSize = sizeof(TCharType);

        Recode(UnicodeName<TCharType>(), to, reinterpret_cast<const char*>(in), out, inSize * charSize, outSize, read, written);
        read /= charSize;
    }

    template <class TFrom, class TTo>
    inline RECODE_RESULT RecodeNoThrow(TFrom from, TTo to, const char* in, char* out, size_t inSize, size_t outSize, size_t& read, size_t& written) {
        TDescriptor descriptor(from, to);
        if (descriptor.Invalid())
            return RECODE_ERROR;

        size_t res = RecodeImpl(descriptor, in, out, inSize, outSize, read, written);

        if (res == static_cast<size_t>(-1)) {
            switch (errno) {
                case EILSEQ:
                    read = inSize;
                    break;
                case EINVAL:
                    read = inSize;
                    break;
                case E2BIG:
                    return RECODE_EOOUTPUT;
                default:
                    return RECODE_ERROR;
            }
        }

        return RECODE_OK;
    }

    template<class TCharType>
    inline RECODE_RESULT RecodeToUnicodeNoThrow(ECharset from, const char* in, TCharType* out, size_t inSize, size_t outSize, size_t &read, size_t &written) {
        const size_t charSize = sizeof(TCharType);

        RECODE_RESULT res = RecodeNoThrow(from, UnicodeName<TCharType>(), in, reinterpret_cast<char*>(out), inSize, outSize * charSize, read, written);
        written /= charSize;

        return res;
    }

    template<class TCharType>
    inline RECODE_RESULT RecodeFromUnicodeNoThrow(ECharset to, const TCharType* in, char* out, size_t inSize, size_t outSize, size_t &read, size_t &written) {
        const size_t charSize = sizeof(TCharType);

        RECODE_RESULT res = RecodeNoThrow(UnicodeName<TCharType>(), to, reinterpret_cast<const char*>(in), out, inSize * charSize, outSize, read, written);
        read /= charSize;

        return res;
    }
} // namespace NIconvPrivate
