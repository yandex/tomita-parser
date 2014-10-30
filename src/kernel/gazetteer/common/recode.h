#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/charset/wide.h>
#include <util/charset/iconv.h>
#include <util/string/traits.h>

namespace NGzt {

    // Special half-static half-dynamic buffer used by TStaticCharCodec.
    template <typename TCharType, size_t StaticLen = 256>
    class TStaticCharBuffer
    {
    public:
        // @len here is measured in characters (not bytes)
        TCharType* Reserve(size_t len) {
            if (len < StaticLen)    // leave room for terminating null
                return StaticBuffer;
            DynamicBuffer.ReserveAndResize(len);
            return DynamicBuffer.begin();
        }

        void Truncate(size_t len) {
            // fix terminating null if necessary
            if (len < StaticLen)
                StaticBuffer[len] = 0;
            DynamicBuffer.remove(len);
        }

    private:
        TCharType StaticBuffer[StaticLen];
        typename TCharStringTraits<TCharType>::TString DynamicBuffer;
    };

} // namespace NGzt

namespace NDetail {

    template <typename TCharTo>
    struct TRecodeResultOps< NGzt::TStaticCharBuffer<TCharTo> >
    {
        static inline TCharTo* Reserve(NGzt::TStaticCharBuffer<TCharTo>& dst, size_t len) {
            return dst.Reserve(len);
        }

        static inline void Truncate(NGzt::TStaticCharBuffer<TCharTo>& dst, size_t len) {
            dst.Truncate(len);
        }
    };

}   // namespace NDetail


namespace NGzt {

// String encoding/decoding with minimal number of memory allocations.
// Careful, all returned TStringBuf or TWtringBuf's are valid only until
// next method of this class is called. This class could be useful for
// local conversions when you do not need to store the recoded string.
// All returned re-coded string buffers are always null-terminated.
template <size_t StaticLen = 256>
class TStaticCharCodec
{
public:

    inline TStaticCharCodec() {
    }

    // basic helpers
    inline TWtringBuf CharToWide(const TStringBuf& src, ECharset encoding) {
        return NDetail::Recode<char>(src, WideBuffer, encoding);
    }

    inline TStringBuf WideToChar(const TWtringBuf& src, ECharset encoding) {
        return NDetail::Recode<wchar16>(src, CharBuffer, encoding);
    }

    // utf-8 helpers
    inline TWtringBuf Utf8ToWide(const TStringBuf& src) {
        return CharToWide(src, CODES_UTF8);
    }

    inline TStringBuf WideToUtf8(const TWtringBuf& src) {
        return WideToChar(src, CODES_UTF8);
    }

    // mix helpers
    inline TStringBuf CharToUtf8(const TStringBuf& src, ECharset encoding) {
        if (encoding == CODES_UTF8 && IsNullTerminated(src))
            return src;     // careful here!
        else
            return WideToUtf8(CharToWide(src, encoding));
    }

    inline TStringBuf Utf8ToChar(const TStringBuf& src, ECharset encoding) {
        if (encoding == CODES_UTF8 && IsNullTerminated(src))
            return src;     // careful here!
        else
            return WideToChar(Utf8ToWide(src), encoding);
    }

    // useful overloads
    inline TStringBuf ToUtf8(const TStringBuf& src, ECharset encoding) {
        return CharToUtf8(src, encoding);
    }

    inline TStringBuf ToUtf8(const TWtringBuf& src, ECharset /*encoding*/) {
        return WideToUtf8(src);
    }

private:
    bool IsNullTerminated(const TStringBuf& str) const {
        return str.IsInited() && str[str.size()] == 0;
    }

private:
    TStaticCharBuffer<char, StaticLen> CharBuffer;
    TStaticCharBuffer<wchar16, StaticLen> WideBuffer;

    DECLARE_NOCOPY(TStaticCharCodec);
};

}  // namespace NGzt
