#include "charset.h"
#include "chars.h"
#include "defs.h"
#include "exceptions.h"
#include <contrib/libs/libiconv/iconv.h>
#include <util/charset/codepage.h>
#include <util/system/mutex.h>

using std::swap;

extern const Encoder asciiEncoder;
extern const CodePage asciiCodePage;

////////////////////////////////////////////////////////////////////////////////
//
// SingleByteCharset
//

class TSingleByteCharset : public TCharset {
    DECLARE_NOCOPY(TSingleByteCharset);

// Constructor
public:
    TSingleByteCharset(ECharset code)
      : TCharset(code)
      , Enc(code == CODES_UNKNOWN ? &asciiEncoder : &EncoderByCharset(code))
      , Unicode(code == CODES_UNKNOWN ? asciiCodePage.unicode : CodePageByCharset(code)->unicode)
    {}

// Charset methods
public:
    void Encode(
        const wchar16 chars[], size_t charCount,
        char bytes[], size_t byteCount,
        size_t* charsUsed, size_t* bytesUsed) const
    {
        size_t count = Min(charCount, byteCount);
        for (size_t i = 0; i != count; ++i)
            bytes[i] = Enc->Tr(chars[i]);

        *charsUsed = *bytesUsed = count;
    }

    void Decode(
        const char bytes[], size_t byteCount, bool /* endOfInput */,
        wchar16 chars[], size_t charCount,
        size_t* bytesUsed, size_t* charsUsed) const
    {
        size_t count = Min(byteCount, charCount);
        for (size_t i = 0; i != count; ++i)
            chars[i] = (wchar16)Unicode[(ui8)bytes[i]];

        *bytesUsed = *charsUsed = count;
    }

// Fields
private:
    const Encoder* const Enc;
    const wchar32* const Unicode;
};

////////////////////////////////////////////////////////////////////////////////
//
// IconvCharset
//

class TIconvCharset : public TCharset {
    DECLARE_NOCOPY(TIconvCharset);

// Constructor/Destructor
public:
    TIconvCharset(ECharset code);
    ~TIconvCharset();

// Charset methods
public:
    void Encode(
        const wchar16 chars[], size_t charCount,
        char bytes[], size_t byteCount,
        size_t* charsUsed, size_t* bytesUsed) const;
    void Decode(
        const char bytes[], size_t byteCount, bool /* endOfInput */,
        wchar16 chars[], size_t charCount,
        size_t* bytesUsed, size_t* charsUsed) const;

// Helper methods
private:
    void Init();
    void Free();

// Fields
private:
    static const char ClassName[];
    static const size_t ERR = (size_t)-1;
    static const iconv_t CD_INVALID;
    iconv_t CdDecode;
    iconv_t CdEncode;
    TMutex MutexDecode;
    TMutex MutexEncode;
};

const char TIconvCharset::ClassName[] = "IconvCharset";
const iconv_t TIconvCharset::CD_INVALID = (iconv_t)-1;

TIconvCharset::TIconvCharset(ECharset code)
  : TCharset(code)
  , CdDecode(CD_INVALID), CdEncode(CD_INVALID)
  , MutexDecode(), MutexEncode()
{
    try {
        Init();
    }
    catch (const std::exception&) {
        Free();
        throw;
    }
}

TIconvCharset::~TIconvCharset() {
    Free();
}

void TIconvCharset::Init() {
    const char* name = NameByCharset(Code);
    CdDecode = iconv_open("UTF-16LE", name);
    CdEncode = iconv_open(name, "UTF-16LE");
    if (CdDecode == CD_INVALID || CdEncode == CD_INVALID)
        throw TException(EXSRC_CLASS, "Charset '%s' not supported", name);
}

void TIconvCharset::Free() {
    if (CdDecode != CD_INVALID) {
        iconv_close(CdDecode);
        CdDecode = CD_INVALID;
    }
    if (CdEncode != CD_INVALID) {
        iconv_close(CdEncode);
        CdEncode = CD_INVALID;
    }
}

void TIconvCharset::Encode(
    const wchar16 chars[], size_t charCount,
    char bytes[], size_t byteCount,
    size_t* charsUsed, size_t* bytesUsed) const
{
    TGuard<TMutex> lock(MutexEncode);
    char* bytesEnd = bytes;
    size_t charsLeft = charCount * sizeof(wchar16);
    size_t bytesLeft = byteCount;

    const char* inbuf = (const char*)chars;
    size_t ret = iconv(CdEncode, &inbuf, &charsLeft, &bytesEnd, &bytesLeft);
    charsLeft /= sizeof(wchar16);

    if (ret == ERR) {
        switch (errno) {
        case E2BIG:
            break;
        case EINVAL:
        case EILSEQ:
        default:
            if (charsLeft && bytesLeft) {
                --charsLeft;
                --bytesLeft;
                *bytesEnd++ = '?';
            }
            break;
        }
    }

    *charsUsed = charCount - charsLeft;
    *bytesUsed = byteCount - bytesLeft;
}

void TIconvCharset::Decode(
    const char bytes[], size_t byteCount, bool endOfInput,
    wchar16 chars[], size_t charCount,
    size_t* bytesUsed, size_t* charsUsed) const
{
    TGuard<TMutex> lock(MutexDecode);
    const char* bytesEnd = bytes;
    size_t bytesLeft = byteCount;
    size_t charsLeft = charCount * sizeof(wchar16);

    char* outbuf = (char*)chars;
    size_t ret = iconv(CdDecode, &bytesEnd, &bytesLeft, &outbuf, &charsLeft);
    wchar16* charsEnd = (wchar16*)outbuf;
    charsLeft /= sizeof(wchar16);

    if (ret == ERR) {
        int err = errno;
        switch (err) {
        case E2BIG:
            break;
        case EINVAL:
            if (!endOfInput && bytesLeft < 4)
                break;
            /* no break */
        case EILSEQ:
        default:
            if (bytesLeft && charsLeft) {
                --bytesLeft;
                if (err == EINVAL && endOfInput)
                    bytesLeft = 0;
                --charsLeft;
                *charsEnd++ = wINVALID;
            }
            break;
        }
    }

    *bytesUsed = byteCount - bytesLeft;
    *charsUsed = charCount - charsLeft;
}

////////////////////////////////////////////////////////////////////////////////
//
// CharsetUtf8
//

class TCharsetUtf8 : public TCharset {
    DECLARE_NOCOPY(TCharsetUtf8);

// Constructor
public:
    TCharsetUtf8() : TCharset(CODES_UTF8) {}

// TCharset methods
public:
    void Encode(
        const wchar16 chars[], size_t charCount,
        char bytes[], size_t byteCount,
        size_t* charsUsed, size_t* bytesUsed) const
    {
        const wchar16* charPtr = chars;
        const wchar16* charEnd = chars + charCount;
        ui8* start = (ui8*)bytes;
        ui8* ptr = start;
        ui8* end = start + byteCount;

        while (charPtr < charEnd && ptr < end) {
            size_t len = 0;
            wchar16 rune = *charPtr;
            if (IsSurrogate(rune))
                rune = wINVALID;
            RECODE_RESULT ret = utf8_put_rune(rune, len, ptr, end);
            if (ret == RECODE_EOOUTPUT)
                break;
            ptr += len;
            ++charPtr;
        }

        *charsUsed = (charPtr - chars);
        *bytesUsed = (ptr - start);
    }

    void Decode(
        const char bytes[], size_t byteCount, bool endOfInput,
        wchar16 chars[], size_t charCount,
        size_t* bytesUsed, size_t* charsUsed) const
    {
        const ui8* ptr = (const ui8*)bytes;
        const ui8* end = ptr + byteCount;
        wchar16* charPtr = chars;
        wchar16* charEnd = chars + charCount;
        while (ptr < end && charPtr < charEnd) {
            wchar32 rune;
            size_t len;
            RECODE_RESULT ret = utf8_read_rune(rune, len, ptr, end);
            if (ret != RECODE_OK) {
                rune = wINVALID;
                len = 1;
                if (ret == RECODE_EOINPUT) {
                    if (!endOfInput)
                        break;
                    len = (end - ptr);
                }
            }
            if (rune > 0xffff)
                rune = wINVALID;
            ptr += len;
            *charPtr++ = (wchar16)rune;
        }

        *bytesUsed = (ptr - (const ui8*)bytes);
        *charsUsed = (charPtr - chars);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CharsetUtf16
//

class TCharsetUtf16 : public TCharset {
    DECLARE_NOCOPY(TCharsetUtf16);

// Constructor
public:
    TCharsetUtf16(ECharset code) : TCharset(code) {}

// TCharset methods
public:
    void Encode(
        const wchar16 chars[], size_t charCount,
        char bytes[], size_t byteCount,
        size_t* charsUsed, size_t* bytesUsed) const
    {
        const wchar16* charPtr = chars;
        const wchar16* charEnd = chars + charCount;
        ui8* start = (ui8*)bytes;
        ui8* ptr = start;
        ui8* end = start + byteCount;

        while (charPtr < charEnd && ptr + 2 <= end) {
            wchar16 ch = *charPtr++;
            if (IsSurrogate(ch))
                ch = wINVALID;
            *ptr++ = ui8(ch);
            *ptr++ = ui8(ch >> 8);
        }

        if (Code == CODES_UTF_16BE) {
            for (ui8* p = start; p < ptr; p += 2)
                swap(p[0], p[1]);
        }

        *charsUsed = (charPtr - chars);
        *bytesUsed = (ptr - start);
    }

    void Decode(
        const char bytes[], size_t byteCount, bool endOfInput,
        wchar16 chars[], size_t charCount,
        size_t* bytesUsed, size_t* charsUsed) const
    {
        const ui8* start = (const ui8*)bytes;
        const ui8* ptr = start;
        const ui8* end = ptr + byteCount;
        wchar16* charPtr = chars;
        wchar16* charEnd = chars + charCount;

        for (; ptr + 2 <= end && charPtr < charEnd; ptr += 2)
            *charPtr++ = wchar16(ptr[0] | (ptr[1] << 8));

        if (Code == CODES_UTF_16BE) {
            for (ui8 *p = (ui8*)chars, *e = (ui8*)charPtr; p < e; p += 2)
                swap(p[0], p[1]);
        }

        *bytesUsed = (ptr - start);
        *charsUsed = (charPtr - chars);
        if (endOfInput && ptr != end && charPtr != chars + charCount) {
            *charPtr++ = wINVALID;
            ++*charsUsed;
            *bytesUsed = byteCount;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// Charset
//

DEFINE_STATIC_CTOR(TCharset);
const char TCharset::ClassName[] = "Charset";
const TCharset* TCharset::Charsets[CODES_MAX + 2]; // 2 == -CODES_UNSUPPORTED
bool TCharset::Ready = false;
TMutex TCharset::Mutex;

void TCharset::StaticCtor() {
    Ready = true;
}

void TCharset::StaticDtor() {
    for (int i = 0; i < ARRAY_SSIZE(Charsets); ++i)
        delete Charsets[i];
}

const TCharset* TCharset::ForCode(ECharset code) {
    CHECK_ARG(code != CODES_MAX, "code");

    const int index = code - CODES_UNSUPPORTED;
    const TCharset* charset = Charsets[index];
    if (charset == 0) {
        if (!Ready) {
            Charsets[index] = CreateCharset(code);
        }
        else {
            TGuard<TMutex> lock(Mutex);
            if (Charsets[index] == 0)
                Charsets[index] = CreateCharset(code);
        }
        charset = Charsets[index];
    }
    return charset;
}

const TCharset* TCharset::CreateCharset(ECharset code) {
    CHECK_ARG(code != CODES_MAX, "code");

    switch (code) {
        case CODES_UTF8:
            return new TCharsetUtf8();
        case CODES_UTF_16LE:
        case CODES_UTF_16BE:
            return new TCharsetUtf16(code);
        case CODES_UNSUPPORTED:
            throw TException(EXSRC_METHOD("CreateCharset"), "Unsupported charset");
        default:
            if (code == CODES_UNKNOWN || SingleByteCodepage(code))
                return new TSingleByteCharset(code);
            return new TIconvCharset(code);
    }
}
