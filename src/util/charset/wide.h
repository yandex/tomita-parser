#pragma once

#include "iconv.h"
#include "unidata.h"

#include <util/system/yassert.h>
#include <util/memory/tempbuf.h>
#include <util/generic/stroka.h>
#include <util/charset/codepage.h>
#include <util/generic/yexception.h>
#include <util/generic/typehelpers.h>
#include <util/generic/static_assert.h>

template <class T> class TTempArray;
typedef TTempArray<TChar> TCharTemp;

namespace NDetail {
    inline void PutUTF8LeadBits(wchar32& rune, unsigned char c, size_t len) {
        rune = c;
        rune &= utf8_leadbyte_mask(len);
    }

    inline void PutUTF8SixBits(wchar32& rune, unsigned char c) {
        rune <<= 6;
        rune |= c & 0x3F;
    }

    inline bool IsUTF8ContinuationByte(unsigned char c) {
       return (c & (unsigned char)0xC0) == (unsigned char)0x80;
    }

    //! reads one unicode symbol from a character sequence encoded UTF8 and moves pointer to the next character
    //! @param c    value of the current character
    //! @param p    pointer to the current character, it will be changed in case of valid UTF8 byte sequence
    //! @param e    the end of the character sequence
    inline RECODE_RESULT ReadUTF8Char(wchar32& c, const unsigned char*& p, const unsigned char* e) {
        YASSERT(p < e); // since p < e then we will check RECODE_EOINPUT only for n > 1 (see calls of this functions)
        switch (utf8_rune_len(*p)) {
        case 0:
            c = BROKEN_RUNE;
            return RECODE_BROKENSYMBOL;     //[BROKENSYMBOL] in first byte

        case 1:
            c = *p;                         //[00000000 0XXXXXXX]
            ++p;
            return RECODE_OK;

        case 2:
            if (p + 2 > e) {
                return RECODE_EOINPUT;
            } else if (!IsUTF8ContinuationByte(p[1])) {
                c = BROKEN_RUNE;
                return RECODE_BROKENSYMBOL;
            } else {
                PutUTF8LeadBits(c, *p++, 2); //[00000000 000XXXXX]
                PutUTF8SixBits(c, *p++);     //[00000XXX XXYYYYYY]
                return RECODE_OK;
            }
        case 3:
            if (p + 3 > e) {
                return RECODE_EOINPUT;
            } else if (!IsUTF8ContinuationByte(p[1]) ||
                       !IsUTF8ContinuationByte(p[2])) {
                c = BROKEN_RUNE;
                return RECODE_BROKENSYMBOL;
            } else {
                PutUTF8LeadBits(c, *p++, 3); //[00000000 0000XXXX]
                PutUTF8SixBits(c, *p++);     //[000000XX XXYYYYYY]
                PutUTF8SixBits(c, *p++);     //[XXXXYYYY YYZZZZZZ]
                return RECODE_OK;
            }
        default: // actually 4
            if (p + 4 > e) {
                return RECODE_EOINPUT;
            } else if (!IsUTF8ContinuationByte(p[1]) ||
                       !IsUTF8ContinuationByte(p[2]) ||
                       !IsUTF8ContinuationByte(p[3])) {
                c = BROKEN_RUNE;
                return RECODE_BROKENSYMBOL;
            } else {
                PutUTF8LeadBits(c, *p++, 4); //[00000000 00000000 00000XXX]
                PutUTF8SixBits(c, *p++);     //[00000000 0000000X XXYYYYYY]
                PutUTF8SixBits(c, *p++);     //[00000000 0XXXYYYY YYZZZZZZ]
                PutUTF8SixBits(c, *p++);     //[000XXXYY YYYYZZZZ ZZQQQQQQ]
                return RECODE_OK;
            }
        }
    }

    //! returns length of the current UTF8 character
    //! @param n    length of the current character, it is assigned in case of valid UTF8 byte sequence
    //! @param p    pointer to the current character
    //! @param e    end of the character sequence
    inline RECODE_RESULT GetUTF8CharLen(size_t& n, const unsigned char* p, const unsigned char* e) {
        YASSERT(p < e); // since p < e then we will check RECODE_EOINPUT only for n > 1 (see calls of this functions)
        switch (utf8_rune_len(*p)) {
        case 0:
            return RECODE_BROKENSYMBOL;     //[BROKENSYMBOL] in first byte

        case 1:
            n = 1;
            return RECODE_OK;

        case 2:
            if (p + 2 > e) {
                return RECODE_EOINPUT;
            } else if (!IsUTF8ContinuationByte(p[1])) {
                return RECODE_BROKENSYMBOL;
            } else {
                n = 2;
                return RECODE_OK;
            }
        case 3:
            if (p + 3 > e) {
                return RECODE_EOINPUT;
            } else if (!IsUTF8ContinuationByte(p[1]) ||
                       !IsUTF8ContinuationByte(p[2])) {
                return RECODE_BROKENSYMBOL;
            } else {
                n = 3;
                return RECODE_OK;
            }
        default: // actually 4
            if (p + 4 > e) {
                return RECODE_EOINPUT;
            } else if (!IsUTF8ContinuationByte(p[1]) ||
                       !IsUTF8ContinuationByte(p[2]) ||
                       !IsUTF8ContinuationByte(p[3])) {
                return RECODE_BROKENSYMBOL;
            } else {
                n = 4;
                return RECODE_OK;
            }
        }
    }

    //! writes one unicode symbol into a character sequence encoded UTF8
    //! @attention this function works as @c utf8_put_rune but it does not check
    //!            size of the output buffer, it supposes that buffer is long enough
    inline void WriteUTF8Char(wchar32 c, size_t& n, unsigned char* s) {
        if (c < 0x80){
            *s = static_cast<unsigned char>(c);
            n = 1;
            return;
        }
        if (c < 0x800){
            *s++ = static_cast<unsigned char>(0xC0 | (c >> 6));
            *s   = static_cast<unsigned char>(0x80 | (c & 0x3F));
            n = 2;
            return;
        }
        if (c < 0x10000) {
            *s++ = static_cast<unsigned char>(0xE0 | (c >> 12));
            *s++ = static_cast<unsigned char>(0x80 | ((c >> 6) & 0x3F));
            *s   = static_cast<unsigned char>(0x80 | (c & 0x3F));
            n = 3;
            return;
        }
        /*if (c < 0x200000)*/ {
            *s++ = static_cast<unsigned char>(0xF0 | ((c >> 18) & 0x07));
            *s++ = static_cast<unsigned char>(0x80 | ((c >> 12) & 0x3F));
            *s++ = static_cast<unsigned char>(0x80 | ((c >> 6) & 0x3F));
            *s   = static_cast<unsigned char>(0x80 | (c & 0x3F));
            n = 4;
        }
    }

    inline Stroka InStringMsg(const char* s, size_t len) {
        return (len <= 50) ? " in string " + Stroka(s,len).Quote() : Stroka();
    }

    inline bool IsSurrogateLead(wchar16 c) {
        return 0xD800 <= c && c <= 0xDBFF;
    }

    inline bool IsSurrogateTail(wchar16 c) {
        return 0xDC00 <= c && c <= 0xDFFF;
    }

    template<bool isPointer>
    struct TSelector;

    template<>
    struct TSelector<false> {
        template <class T>
        static inline void WriteSymbol(wchar16 s, T& dest) {
            dest.push_back(s);
        }
    };
    template<>
    struct TSelector<true> {
        template <class T>
        static inline void WriteSymbol(wchar16 s, T& dest) {
            *(dest++) = s;
        }
    };

    inline wchar32 ReadSurroagatePair(const wchar16* chars) {
        const wchar32 SURROGATE_OFFSET = static_cast<wchar32>(0x10000 - (0xD800 << 10) - 0xDC00);
        wchar16 lead = chars[0];
        wchar16 tail = chars[1];

        YASSERT(IsSurrogateLead(lead));
        YASSERT(IsSurrogateTail(tail));

        return (static_cast<wchar32>(lead) << 10) + tail + SURROGATE_OFFSET;
    }

    template <class T>
    inline void WriteSurroagatePair(wchar32 s, T& dest);

    inline size_t SymbolSize(const wchar16* begin, const wchar16* end) {
        YASSERT(begin < end);

        if ((begin + 1 != end) && NDetail::IsSurrogateLead(*begin) && NDetail::IsSurrogateTail(*(begin + 1))) {
            return 2;
        }

        return 1;
    }
} // NDetail

inline wchar16* SkipSymbol(wchar16* begin, const wchar16* end) {
    return begin + NDetail::SymbolSize(begin, end);
}
inline const wchar16* SkipSymbol(const wchar16* begin, const wchar16* end) {
    return begin + NDetail::SymbolSize(begin, end);
}

inline wchar32 ReadSymbol(const wchar16* begin, const wchar16* end) {
    YASSERT(begin < end);
    if (NDetail::IsSurrogateLead(*begin)) {
        if (begin + 1 != end && NDetail::IsSurrogateTail(*(begin + 1)))
            return NDetail::ReadSurroagatePair(begin);

        return BROKEN_RUNE;
    }

    return *begin;
}

//! presuming input data is either big enought of null terminated
inline wchar32 ReadSymbolAndAdvance(const wchar16*& begin) {
    YASSERT(*begin);
    if (NDetail::IsSurrogateLead(begin[0])) {
        if (NDetail::IsSurrogateTail(begin[1])) {
            YASSERT(begin[1] != 0);
            const wchar32 c = NDetail::ReadSurroagatePair(begin);
            begin += 2;
            return c;
        }
        ++begin;
        return BROKEN_RUNE;
    }
    return *(begin++);
}

inline wchar32 ReadSymbolAndAdvance(const wchar16*& begin, const wchar16* end) {
    YASSERT(begin < end);
    if (NDetail::IsSurrogateLead(begin[0])) {
        if (begin + 1 != end && NDetail::IsSurrogateTail(begin[1])) {
            const wchar32 c = NDetail::ReadSurroagatePair(begin);
            begin += 2;
            return c;
        }
        ++begin;
        return BROKEN_RUNE;
    }
    return *(begin++);
}

inline wchar32 ReadSymbolAndAdvance(const wchar32*& begin, const wchar32* end) {
    YASSERT(begin < end);
    return *(begin++);
}

template <class T>
inline size_t WriteSymbol(wchar16 s, T& dest) {
    NDetail::TSelector<TTypeTraits<T>::IsPointer>::WriteSymbol(s, dest);
    return 1;
}

template <class T>
inline size_t WriteSymbol(wchar32 s, T& dest) {
    if (s > 0xFFFF) {
        if (s >= ::NUnicode::UnicodeInstancesLimit()) {
            return WriteSymbol(static_cast<wchar16>(BROKEN_RUNE), dest);
        }

        NDetail::WriteSurroagatePair(s, dest);
        return 2;
    }

    return WriteSymbol(static_cast<wchar16>(s), dest);
}

inline bool WriteSymbol(wchar32 s, wchar16*& dest, const wchar16* destEnd) {
    YASSERT(dest < destEnd);

    if (s > 0xFFFF) {
        if (s >= NUnicode::UnicodeInstancesLimit()) {
            *(dest++) = static_cast<wchar16>(BROKEN_RUNE);
            return true;
        }

        if (dest + 2 > destEnd)
            return false;

        NDetail::WriteSurroagatePair(s, dest);
    } else {
        *(dest++) = static_cast<wchar16>(s);
    }

    return true;
}

inline size_t WriteSymbol(wchar32 s, wchar32*& dest) {
    *(dest++) = s;
    return 1;
}

inline bool WriteSymbol(wchar32 s, wchar32*& dest, const wchar32* destEnd) {
    YASSERT(dest < destEnd);

    *(dest++) = s;

    return true;
}

template <class T>
inline void NDetail::WriteSurroagatePair(wchar32 s, T& dest) {
    const wchar32 LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
    YASSERT(s > 0xFFFF && s < ::NUnicode::UnicodeInstancesLimit());

    wchar16 lead = LEAD_OFFSET + (static_cast<wchar16>(s >> 10));
    wchar16 tail = 0xDC00 + static_cast<wchar16>(s & 0x3FF);
    YASSERT(IsSurrogateLead(lead));
    YASSERT(IsSurrogateTail(tail));

    WriteSymbol(lead, dest);
    WriteSymbol(tail, dest);
}

class TCharIterator {
private:
    const wchar16* Begin;
    const wchar16* End;

public:
    inline explicit TCharIterator(const wchar16* end)
        : Begin(end)
        , End(end)
    {}

    inline TCharIterator(const wchar16* begin, const wchar16* end)
        : Begin(begin)
        , End(end)
    {}

    inline TCharIterator& operator ++() {
        Begin = SkipSymbol(Begin, End);

        return *this;
    }

    inline bool operator == (const wchar16* other) const {
        return Begin == other;
    }
    inline bool operator != (const wchar16* other) const {
        return !(*this == other);
    }

    inline bool operator == (const TCharIterator& other) const {
        return *this == other.Begin;
    }
    inline bool operator != (const TCharIterator& other) const {
        return *this != other.Begin;
    }

    inline wchar32 operator *() const {
        return ReadSymbol(Begin, End);
    }

    inline const wchar16* Get() const {
        return Begin;
    }
};

//! converts text from unicode to yandex codepage
//! @attention destination buffer must be long enough to fit all characters of the text
//! @note @c dest buffer must fit at least @c len number of characters
template <typename TCharType>
inline size_t WideToChar(const TCharType* text, size_t len, char* dest, ECharset enc = CODES_YANDEX) {
    YASSERT(SingleByteCodepage(enc));

    const char* start = dest;

    const Encoder* const encoder = &EncoderByCharset(enc);
    const TCharType* const last = text + len;
    for (const TCharType* cur = text; cur != last; ++dest) {
        *dest = encoder->Tr(ReadSymbolAndAdvance(cur, last));
    }

    return dest - start;
}

//! converts text to unicode using a codepage object
//! @attention destination buffer must be long enough to fit all characters of the text
//! @note @c dest buffer must fit at least @c len number of characters;
//!       if you need convert zero terminated string you should determine length of the
//!       string using the @c strlen function and pass as the @c len parameter;
//!       it does not make sense to create an additional version of this function because
//!       it will call to @c strlen anyway in order to allocate destination buffer
template <typename TCharType>
inline void CharToWide(const char* text, size_t len, TCharType* dest, const CodePage& cp = csYandex) {
    const unsigned char* cur = reinterpret_cast<const unsigned char*>(text);
    const unsigned char* const last = cur + len;
    for (; cur != last; ++cur, ++dest) {
        *dest = static_cast<TCharType>(cp.unicode[*cur]); // static_cast is safe as no 1char codepage contains non-BMP symbols
    }
}

//! converts text from unicode to UTF8
//! @attention destination buffer must be long enough to fit all characters of the text,
//!            @c WriteUTF8Char converts @c wchar32 into maximum 4 bytes of UTF8 so
//!            destination buffer must have length equal to <tt> len * 4 </tt>
template <typename TCharType>
inline void WideToUTF8(const TCharType* text, size_t len, char* dest, size_t& written) {
    const TCharType* const last = text + len;
    unsigned char* p = reinterpret_cast<unsigned char*>(dest);
    size_t runeLen;
    for (const TCharType* cur = text; cur != last;) {
        NDetail::WriteUTF8Char(ReadSymbolAndAdvance(cur, last), runeLen, p);
        YASSERT(runeLen <= 4);
        p += runeLen;
    }
    written = p - reinterpret_cast<unsigned char*>(dest);
}

//! @return len if robust and position where encoding stopped if not
template <bool robust, typename TCharType>
inline size_t UTF8ToWideImpl(const char* text, size_t len, TCharType* dest, size_t& written) {
    const unsigned char* cur = reinterpret_cast<const unsigned char*>(text);
    const unsigned char* const last = cur + len;
    TCharType* p = dest;
    wchar32 rune = BROKEN_RUNE;

    while (cur != last) {
        if (NDetail::ReadUTF8Char(rune, cur, last) != RECODE_OK) {
            if (robust) {
                rune = BROKEN_RUNE;
                ++cur;
            } else
                break;
        }

        YASSERT(cur <= last);
        WriteSymbol(rune, p);
    }

    written = p - dest;
    return cur - reinterpret_cast<const unsigned char*>(text);
}

template <typename TCharType>
inline size_t UTF8ToWideImpl(const char* text, size_t len, TCharType* dest, size_t& written) {
    return UTF8ToWideImpl<false>(text, len, dest, written);
}

template <bool robust, typename TCharType>
inline bool UTF8ToWide(const char* text, size_t len, TCharType* dest, size_t& written) {
    return UTF8ToWideImpl<robust>(text, len, dest, written) == len;
}

//! converts text from UTF8 to unicode, stops immediately it UTF8 byte sequence is wrong
//! @attention destination buffer must be long enough to fit all characters of the text,
//!            conversion stops if a broken symbol is met
//! @return @c true if all the text converted successfully, @c false - a broken symbol was found
template <typename TCharType>
inline bool UTF8ToWide(const char* text, size_t len, TCharType* dest, size_t& written) {
    return UTF8ToWide<false>(text, len, dest, written);
}

//! returns number of characters in UTF8 encoded text, stops immediately if UTF8 byte sequence is wrong
//! @param text     UTF8 encoded text
//! @param len      the length of the text in bytes
//! @param number   number of encoded symbols in the text
inline bool GetNumberOfUTF8Chars(const char* text, size_t len, size_t& number) {
    const unsigned char* cur = reinterpret_cast<const unsigned char*>(text);
    const unsigned char* const last = cur + len;
    number = 0;
    size_t runeLen;
    bool res = true;
    while (cur != last) {
        if (NDetail::GetUTF8CharLen(runeLen, cur, last) != RECODE_OK) { // actually it could be RECODE_BROKENSYMBOL only
            res = false;
            break;
        }
        cur += runeLen;
        YASSERT(cur <= last);
        ++number;
    }
    return res;
}

TStringBuf SubstrUTF8(const TStringBuf& str, size_t pos, size_t len);

namespace NDetail {

    namespace NBaseOps {

        // Template interface base recoding drivers, do not perform any memory management,
        // do not care about buffer size, so supplied @dst
        // should have enough room for the result (with proper reserve for the worst case)

        // Depending on template params, perform conversion of single-byte/multi-byte/utf8 string to/from wide string.

        template <typename TCharType>
        inline TStringBufImpl<TCharType> RecodeSingleByteChar(const TStringBuf& src, TCharType* dst, const CodePage& cp) {
            YASSERT(cp.SingleByteCodepage());
            ::CharToWide(~src, +src, dst, cp);
            return TStringBufImpl<TCharType>(dst, +src);
        }

        template <typename TCharType>
        inline TStringBuf RecodeSingleByteChar(const TStringBufImpl<TCharType>& src, char* dst, const CodePage& cp) {
            YASSERT(cp.SingleByteCodepage());
            ::WideToChar(~src, +src, dst, cp.CPEnum);
            return TStringBuf(dst, +src);
        }

        template <typename TCharType>
        inline TStringBufImpl<TCharType> RecodeMultiByteChar(const TStringBuf& src, TCharType* dst, ECharset encoding) {
            YASSERT(!NCodepagePrivate::NativeCodepage(encoding));
            size_t read = 0;
            size_t written = 0;
            ::NICONVPrivate::RecodeToUnicode(encoding, ~src, dst, +src, +src, read, written);
            return TStringBufImpl<TCharType>(dst, written);
        }

        template <typename TCharType>
        inline TStringBuf RecodeMultiByteChar(const TStringBufImpl<TCharType>& src, char* dst, ECharset encoding) {
            YASSERT(!NCodepagePrivate::NativeCodepage(encoding));
            size_t read = 0;
            size_t written = 0;
            ::NICONVPrivate::RecodeFromUnicode(encoding, ~src, dst, +src, +src * 3, read, written);
            return TStringBuf(dst, written);
        }

        template <typename TCharType>
        inline TStringBufImpl<TCharType> RecodeUtf8(const TStringBuf& src, TCharType* dst) {
            size_t len = 0;
            if (!::UTF8ToWide(~src, +src, dst, len))
                ythrow yexception() << "Invalid UTF8: \"" << src.SubStr(0, 50) << (src.size() > 50 ? "...\"" : "\"");
            return TStringBufImpl<TCharType>(dst, len);
        }

        template <typename TCharType>
        inline TStringBuf RecodeUtf8(const TStringBufImpl<TCharType>& src, char* dst) {
            size_t len = 0;
            ::WideToUTF8(~src, +src, dst, len);
            return TStringBuf(dst, len);
        }

        // Select one of re-coding methods from above, based on provided @encoding

        template <typename TCharFrom, typename TCharTo>
        TStringBufImpl<TCharTo> Recode(const TStringBufImpl<TCharFrom>& src, TCharTo* dst, ECharset encoding) {
            if (encoding == CODES_UTF8)
                return RecodeUtf8(src, dst);
            else if (SingleByteCodepage(encoding))
                return RecodeSingleByteChar(src, dst, *CodePageByCharset(encoding));
            else
                return RecodeMultiByteChar(src, dst, encoding);
        }

    }   // namespace NBaseOps

    template <typename TCharFrom>
    struct TRecodeTraits;

    template <>
    struct TRecodeTraits<char> {
        typedef wchar16 TCharTo;
        typedef TWtringBuf TStringBufTo;
        typedef Wtroka TStringTo;
        enum { ReserveSize = 4 };  // How many TCharFrom characters we should reserve for one TCharTo character in worst case
                                   // Here an unicode character can be converted up to 4 bytes of UTF8
    };

    template <>
    struct TRecodeTraits<wchar16> {
        typedef char TCharTo;
        typedef TStringBuf TStringBufTo;
        typedef Stroka TStringTo;
        enum { ReserveSize = 2 };   // possible surrogate pairs ?
    };

    // Operations with destination buffer where recoded string will be written
    template <typename TResult>
    struct TRecodeResultOps {
        // default implementation will work with Stroka and Wtroka - 99% of usage
        typedef typename TResult::char_type TResultChar;

        static inline size_t Size(const TResult& dst) {
            return dst.size();
        }

        static inline TResultChar* Reserve(TResult& dst, size_t len) {
            dst.ReserveAndResize(len);
            return dst.begin();
        }

        static inline void Truncate(TResult& dst, size_t len) {
            dst.resize(len);
        }
    };

    // Main template interface for recoding in both directions

    template <typename TCharFrom, typename TResult>
    typename TRecodeTraits<TCharFrom>::TStringBufTo Recode(const TStringBufImpl<TCharFrom>& src, TResult& dst, ECharset encoding) {
        typedef typename TRecodeTraits<TCharFrom>::TCharTo TCharTo;
        // make enough room for re-coded string
        TCharTo* dstbuf = TRecodeResultOps<TResult>::Reserve(dst, src.size() * TRecodeTraits<TCharTo>::ReserveSize);
        // do re-coding
        TStringBufImpl<TCharTo> res = NBaseOps::Recode(src, dstbuf, encoding);
        // truncate result back to proper size
        TRecodeResultOps<TResult>::Truncate(dst, res.size());
        return res;
    }

    // appending version of Recode()
    template <typename TCharFrom, typename TResult>
    typename TRecodeTraits<TCharFrom>::TStringBufTo RecodeAppend(const TStringBufImpl<TCharFrom>& src, TResult& dst, ECharset encoding) {
        typedef typename TRecodeTraits<TCharFrom>::TCharTo TCharTo;
        size_t dstOrigSize = TRecodeResultOps<TResult>::Size(dst);
        TCharTo* dstbuf = TRecodeResultOps<TResult>::Reserve(dst, dstOrigSize + src.size() * TRecodeTraits<TCharTo>::ReserveSize);
        TStringBufImpl<TCharTo> appended = NBaseOps::Recode(src, dstbuf + dstOrigSize, encoding);
        size_t dstFinalSize = dstOrigSize + appended.size();
        TRecodeResultOps<TResult>::Truncate(dst, dstFinalSize);
        return TStringBufImpl<TCharTo>(dstbuf, dstFinalSize);
    }

    // special implementation for robust utf8 functions
    template <typename TResult>
    TWtringBuf RecodeUTF8Robust(const TStringBuf& src, TResult& dst) {
        // make enough room for re-coded string
        TChar* dstbuf = TRecodeResultOps<TResult>::Reserve(dst, src.size() * TRecodeTraits<TChar>::ReserveSize);

        // do re-coding
        size_t written = 0;
        UTF8ToWide<true>(~src, +src, dstbuf, written);

        // truncate result back to proper size
        TRecodeResultOps<TResult>::Truncate(dst, written);
        return TWtringBuf(dstbuf, written);
    }

    template <typename TCharFrom>
    inline typename TRecodeTraits<TCharFrom>::TStringTo Recode(const TStringBufImpl<TCharFrom>& src, ECharset encoding) {
        typename TRecodeTraits<TCharFrom>::TStringTo res;
        Recode<TCharFrom>(src, res, encoding);
        return res;
    }
}   // NDetail

// Write result into @dst. Return string-buffer pointing to re-coded content of @dst.

template <bool robust>
inline TWtringBuf CharToWide(const TStringBuf& src, Wtroka& dst, ECharset encoding) {
    if (robust && CODES_UTF8 == encoding)
        return NDetail::RecodeUTF8Robust(src, dst);
    return NDetail::Recode<char>(src, dst, encoding);
}

inline TWtringBuf CharToWide(const TStringBuf& src, Wtroka& dst, ECharset encoding) {
    return NDetail::Recode<char>(src, dst, encoding);
}

inline TStringBuf WideToChar(const TWtringBuf& src, Stroka& dst, ECharset encoding) {
    return NDetail::Recode<wchar16>(src, dst, encoding);
}

template <bool robust>
inline TWtringBuf UTF8ToWide(const TStringBuf& src, Wtroka& dst) {
    return CharToWide<robust>(src, dst, CODES_UTF8);
}

inline TWtringBuf UTF8ToWide(const TStringBuf& src, Wtroka& dst) {
    return NDetail::Recode<char>(src, dst, CODES_UTF8);
}

inline TStringBuf WideToUTF8(const TWtringBuf& src, Stroka& dst) {
    return NDetail::Recode<wchar16>(src, dst, CODES_UTF8);
}

inline Stroka WideToUTF8(const wchar16* text, size_t len) {
    Stroka s(static_cast<int>(len * 4)); // * 4 because the conversion functions can convert unicode character into maximum 4 bytes of UTF8
    size_t written = 0;
    WideToUTF8(text, len, s.begin(), written);
    YASSERT(s.size() >= written);
    s.remove(written);
    return s;
}

template <bool robust>
inline Wtroka UTF8ToWide(const char* text, size_t len) {
    Wtroka w(static_cast<int>(len));
    size_t written;
    size_t pos = UTF8ToWideImpl<robust>(text, len, w.begin(), written);
    if (pos != len)
        ythrow yexception() << "failed to decode UTF-8 string at pos " << pos << NDetail::InStringMsg(text, len);
    YASSERT(w.size() >= written);
    w.remove(written);
    return w;
}

inline Wtroka UTF8ToWide(const char* text, size_t len) {
    return UTF8ToWide<false>(text, len);
}

//! calls either to @c WideToUTF8 or @c WideToChar depending on the encoding type
inline Stroka WideToChar(const wchar16* text, size_t len, ECharset enc = CODES_YANDEX) {
    if (NCodepagePrivate::NativeCodepage(enc)) {
        if (enc == CODES_UTF8)
            return WideToUTF8(text, len);

        Stroka s(static_cast<int>(len));
        s.remove(WideToChar(text, len, s.begin(), enc));

        return s;
    }

    Stroka s(static_cast<int>(len) * 3);

    size_t read = 0;
    size_t written = 0;
    NICONVPrivate::RecodeFromUnicode(enc, text, s.begin(), len, s.size(), read, written);
    s.remove(written);

    return s;
}

inline Wtroka CharToWide(const char* text, size_t len, const CodePage& cp) {
    Wtroka w(static_cast<int>(len));
    CharToWide(text, len, w.begin(), cp);
    return w;
}

//! calls either to @c UTF8ToWide or @c CharToWide depending on the encoding type
template <bool robust>
inline Wtroka CharToWide(const char* text, size_t len, ECharset enc = CODES_YANDEX) {
    if (NCodepagePrivate::NativeCodepage(enc)) {
        if (enc == CODES_UTF8)
            return UTF8ToWide<robust>(text, len);

        return CharToWide(text, len, *CodePageByCharset(enc));
    }

    Wtroka w(static_cast<int>(len) * 2);

    size_t read = 0;
    size_t written = 0;
    NICONVPrivate::RecodeToUnicode(enc, text, w.begin(), len, len, read, written);
    w.remove(written);

    return w;
}

inline Stroka WideToChar(const TWtringBuf& w, ECharset enc = CODES_YANDEX) {
    return WideToChar(w.c_str(), w.size(), enc);
}

inline Stroka WideToUTF8(const TWtringBuf& w) {
    return WideToUTF8(w.c_str(), w.size());
}

inline Wtroka CharToWide(const TStringBuf& s, ECharset enc = CODES_YANDEX) {
    return CharToWide<false>(s.c_str(), s.size(), enc);
}

template <bool robust>
inline Wtroka CharToWide(const TStringBuf& s, ECharset enc = CODES_YANDEX) {
    return CharToWide<robust>(s.c_str(), s.size(), enc);
}

inline Wtroka CharToWide(const TStringBuf& s, const CodePage& cp) {
    return CharToWide(s.c_str(), s.size(), cp);
}

template <bool robust>
inline Wtroka UTF8ToWide(const TStringBuf& s) {
    return UTF8ToWide<robust>(s.c_str(), s.size());
}

inline Wtroka UTF8ToWide(const TStringBuf& s) {
    return UTF8ToWide<false>(s.c_str(), s.size());
}

//! converts text from UTF8 to unicode, if conversion fails it uses codepage to convert the text
//! @param text     text to be converted
//! @param len      length of the text in characters
//! @param cp       a codepage that is used in case of failed conversion from UTF8
inline Wtroka UTF8ToWide(const char* text, size_t len, const CodePage& cp) {
    Wtroka w(static_cast<int>(len));
    size_t written = 0;
    if (UTF8ToWide(text, len, w.begin(), written))
        w.remove(written);
    else
        CharToWide(text, len, w.begin(), cp);
    return w;
}

inline Wtroka WideToWide(const wchar32* begin, size_t len) {
    Wtroka res;
    res.reserve(len);

    const wchar32* end = begin + len;
    for (const wchar32* i = begin; i != end; ++i) {
        WriteSymbol(*i, res);
    }

    return res;
}

//! returns @c true if character sequence has no symbols with value greater than 0x7F
template <typename TChar>
inline bool IsStringASCII(const TChar* first, const TChar* last) {
    typedef typename TTypeTraits<TChar>::TUnsigned TUnsignedChar;
    YASSERT(first <= last);
    for (; first != last; ++first) {
        if (static_cast<TUnsignedChar>(*first) > 0x7F)
            return false;
    }
    return true;
}

//! copies elements from one character sequence to another using memcpy
//! for compatibility only
template <typename TChar>
inline void Copy(const TChar* first, size_t len, TChar* result) {
    memcpy(result, first, len * sizeof(TChar));
}

template <typename TChar>
inline void Copy(const TChar* first, const TChar* last, TChar* result) {
    YASSERT(first <= last);
    Copy(first, last - first, result);
}

//! copies elements from one character sequence to another using @c static_cast for assignment
template <typename TChar1, typename TChar2>
inline void Copy(const TChar1* first, const TChar1* last, TChar2* result) {
    STATIC_ASSERT(sizeof(TChar1) != sizeof(TChar2)); // use memcpy instead of this function
    YASSERT(first <= last);
    for (; first != last; ++first, ++result) {
        *result = static_cast<TChar2>(*first);
    }
}

template <typename TChar1, typename TChar2>
inline void Copy(const TChar1* first, size_t len, TChar2* result) {
    Copy(first, first + len, result);
}

//! copies symbols from one character sequence to another without any conversion
//! @note this function can be used instead of the template constructor of @c std::basic_string:
//!       template <typename InputIterator>
//!       basic_string(InputIterator begin, InputIterator end, const Allocator& a = Allocator());
//!       and the family of template member functions: append, assign, insert, replace.
template <typename TString, typename TChar>
inline TString CopyTo(const TChar* first, const TChar* last) {
    YASSERT(first <= last);
    TString str(static_cast<int>(last - first));
    Copy(first, last, str.begin());
    return str;
}

template <typename TString, typename TChar>
inline TString CopyTo(const TChar* s, size_t n) {
    TString str(static_cast<int>(n));
    Copy(s, n, str.begin());
    return str;
}

inline Stroka WideToASCII(const TWtringBuf& w) {
    YASSERT(IsStringASCII(w.begin(), w.end()));
    return CopyTo<Stroka>(w.begin(), w.end());
}

inline Wtroka ASCIIToWide(const TStringBuf& s) {
    YASSERT(IsStringASCII(s.begin(), s.end()));
    return CopyTo<Wtroka>(s.begin(), s.end());
}

//! returns @c true if string contains whitespace characters only
inline bool IsSpace(const wchar16* s, size_t n) {
    if (n == 0)
        return false;

    YASSERT(s);

    const wchar16* const e = s + n;
    for (const wchar16* p = s; p != e; ++p) {
        if (!IsWhitespace(*p))
            return false;
    }
    return true;
}

//! returns @c true if string contains whitespace characters only
inline bool IsSpace(const TWtringBuf& s) {
    return IsSpace(s.c_str(), s.length());
}

// true if @text can be fully encoded to specified @encoding,
// with possibility to recover exact original text after decoding
bool CanBeEncoded(TWtringBuf text, ECharset encoding);

//! replaces multiple sequential whitespace characters with a single space character
void Collapse(Wtroka& w);

//! @return new length
size_t Collapse(wchar16* s, size_t n);

inline void ToLower(const wchar16* from, size_t len, wchar16* to) {
    const wchar16* const end = from + len;

    for (const wchar16* i = from; i != end;) {
        wchar32 c = ToLower(ReadSymbolAndAdvance(i, end));

        WriteSymbol(c, to);
    }
}

inline void ToLower(wchar16* s, size_t n) {
    ToLower(s, n, s);
}

inline void ToUpper(const wchar16* from, size_t len, wchar16* to) {
    const wchar16* const end = from + len;

    for (const wchar16* i = from; i != end;) {
        wchar32 c = ToUpper(ReadSymbolAndAdvance(i, end));

        WriteSymbol(c, to);
    }
}

inline void ToUpper(wchar16* s, size_t n) {
    ToUpper(s, n, s);
}

inline void ToTitle(const wchar16* from, size_t len, wchar16* to) {
    if (len == 0)
        return;

    const wchar16* const end = from + len;
    const wchar16* i = from;

    wchar32 c = ToTitle(ReadSymbolAndAdvance(i, end));
    WriteSymbol(c, to);
    YASSERT(i <= end);

    while (i != end) {
        wchar32 c = ToLower(ReadSymbolAndAdvance(i, end));

        WriteSymbol(c, to);
    }
}

inline void ToTitle(wchar16* s, size_t n) {
    ToTitle(s, n, s);
}

inline void Reverse(const wchar16* from, size_t len, wchar16* to) {
    if (len == 0)
        return;

    const wchar16* const end = from + len;
    wchar16* reversed = to + len;

    for (const wchar16* i = from; i != end;) {
        size_t size = NDetail::SymbolSize(i, end);

        reversed -= size;
        Copy(i, size, reversed);

        i += size;
    }
}

//! removes leading and trailing whitespace characters
void Strip(Wtroka& w);

//! replaces the '<', '>' and '&' characters in string with '&lt;', '&gt;' and '&amp;' respectively
// insertBr=true - replace '\r' and '\n' with "<BR>"
template<bool insertBr>
void EscapeHtmlChars(Wtroka& str);
