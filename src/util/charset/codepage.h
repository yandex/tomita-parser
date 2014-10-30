#pragma once

#include "doccodes.h"

#include "unidata.h"   // all wchar32 functions

#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/generic/ylimits.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

#include <cctype>

struct CodePage;
struct Recoder;
struct Encoder;

/*****************************************************************\
*                    struct CodePage                              *
\*****************************************************************/
struct CodePage {
    ECharset    CPEnum;       // int MIBEnum;
    const char* Names[30];    // name[0] -- preferred mime-name
    wchar32     unicode[256];
    const char* DefaultChar;  //[CCL_NUM]

    bool IsLower(unsigned char ch) const {
        return ::IsLower(unicode[ch]);
    }
    bool IsUpper(unsigned char ch) const {
        return ::IsUpper(unicode[ch]);
    }
    bool IsAlpha(unsigned char ch) const {
        return ::IsAlpha(unicode[ch]);
    }
    bool IsDigit(unsigned char ch) const {
        return ::IsDigit(unicode[ch]);
    }
    bool IsXdigit(unsigned char ch)const {
        return ::IsXdigit(unicode[ch]);
    }
    bool IsAlnum(unsigned char ch) const {
        return ::IsAlnum(unicode[ch]);
    }
    bool IsSpace(unsigned char ch) const {
        return ::IsSpace(unicode[ch]);
    }
    bool IsPunct(unsigned char ch) const {
        return ::IsPunct(unicode[ch]);
    }
    bool IsCntrl(unsigned char ch) const {
        return ::IsCntrl(unicode[ch]);
    }
    bool IsGraph(unsigned char ch) const {
        return ::IsGraph(unicode[ch]);
    }
    bool IsPrint(unsigned char ch) const {
        return ::IsPrint(unicode[ch]);
    }
    bool IsComposed(unsigned char ch) const {
        return ::IsComposed(unicode[ch]);
    }

    // return pointer to char after the last char
    char* ToLower(const char* begin, const char* end, char* to) const;
    char* ToLower(const char* begin, char* to) const;

     // return pointer to char after the last char
    char* ToUpper(const char* begin, const char* end, char* to) const;
    char* ToUpper(const char* begin, char* to) const;

    int   stricmp(const char* s1, const char* s2) const;
    int   strnicmp(const char* s1, const char* s2, size_t len) const;

    inline unsigned char ToUpper(unsigned char ch) const;
    inline unsigned char ToLower(unsigned char ch) const;
    inline unsigned char ToTitle(unsigned char ch) const;

    inline int ToDigit(unsigned char ch) const {
        return ::ToDigit(unicode[ch]);
    }

    static void Initialize();

    inline bool SingleByteCodepage() const {
        return DefaultChar != NULL;
    }
    inline bool NativeCodepage() const {
        return SingleByteCodepage() || CPEnum == CODES_UTF8;
    }
};

class TCodePageHash;

namespace NCodepagePrivate {
    class TCodepagesMap {
    private:
        static const int DataShift = 2;
        static const int DataSize = CODES_MAX + DataShift;
        const CodePage* Data[DataSize];

    private:
        inline const CodePage* GetPrivate(ECharset e) const {
            YASSERT(e + DataShift >= 0 && e + DataShift < DataSize);
            return Data[e + DataShift];
        }

        void SetData(const CodePage* cp);

    public:
        TCodepagesMap();

        inline const CodePage* Get(ECharset e) const {
            const CodePage* res = GetPrivate(e);
            if (!res->SingleByteCodepage()) {
                ythrow yexception() << "CodePage structure can only be used for single byte encodings";
            }

            return res;
        }

        inline bool SingleByteCodepage(ECharset e) const {
            return GetPrivate(e)->SingleByteCodepage();
        }
        inline bool NativeCodepage(ECharset e) const {
            return GetPrivate(e)->NativeCodepage();
        }
        inline const char* NameByCharset(ECharset e) const {
            return GetPrivate(e)->Names[0];
        }

        friend class ::TCodePageHash;
    };

    inline bool NativeCodepage(ECharset e) {
        return Singleton<NCodepagePrivate::TCodepagesMap>()->NativeCodepage(e);
    }
}

inline bool SingleByteCodepage(ECharset e) {
    return Singleton<NCodepagePrivate::TCodepagesMap>()->SingleByteCodepage(e);
}

inline bool ValidCodepage(ECharset e) {
    return e >= 0 && e < CODES_MAX;
}

inline const CodePage* CodePageByCharset(ECharset e) {
    return Singleton<NCodepagePrivate::TCodepagesMap>()->Get(e);
}

ECharset CharsetByName(const char* name);

inline ECharset CharsetByName(const Stroka& name) {
    return CharsetByName(name.c_str());
}

// Same as CharsetByName, but throws yexception() if name is invalid
ECharset CharsetByNameOrDie(const char* name);

inline ECharset CharsetByCodePage(const CodePage* CP) {
    return CP->CPEnum;
}

inline const char* NameByCharset(ECharset e) {
    return Singleton<NCodepagePrivate::TCodepagesMap>()->NameByCharset(e);
}

inline const char* NameByCharsetSafe(ECharset e) {
    if (CODES_UNKNOWN < e && e < CODES_MAX)
        return Singleton<NCodepagePrivate::TCodepagesMap>()->NameByCharset(e);
    else
        throw yexception() << "unknown encoding: " << (int)e;
}

inline const char* NameByCodePage(const CodePage* CP) {
    return CP->Names[0];
}

inline const CodePage* CodePageByName(const char* name) {
    ECharset code = CharsetByName(name);
    if (code == CODES_UNKNOWN)
        return NULL;

    return CodePageByCharset(code);
}

ECharset EncodingHintByName(const char* name);

/*****************************************************************\
*                    struct Encoder                               *
\*****************************************************************/
enum RECODE_RESULT{
    RECODE_OK,
    RECODE_EOINPUT,
    RECODE_EOOUTPUT,
    RECODE_BROKENSYMBOL,
    RECODE_ERROR,
};

struct Encoder {
    char* Table[256];
    const char* DefaultChar;

    inline char Code(wchar32 ch) const
    {
        if (ch > 0xFFFF)
            return 0;
        return (unsigned char)Table[(ch>>8)&255][ch&255];
    }

    inline char Tr(wchar32 ch) const
    {
        char code = Code(ch);
        if (code == 0 && ch != 0)
            code =  DefaultChar[NUnicode::CharType(ch)];
        YASSERT(code != 0 || ch == 0);
        return code;
    }

    inline unsigned char operator [](wchar32 ch) const
    {
        return Tr(ch);
    }

    void Tr(const wchar32* in, char* out, size_t len) const;
    void Tr(const wchar32* in, char* out) const;
    char* DefaultPlane;
};

/*****************************************************************\
*                    struct Recoder                               *
\*****************************************************************/
struct Recoder {
    unsigned char Table[257];

    void Create(const CodePage &source, const CodePage &target);
    void Create(const CodePage &source, const Encoder* wideTarget);

    void Create(const CodePage &page, wchar32 (*mapper)(wchar32));
    void Create(const CodePage &page, const Encoder* widePage, wchar32 (*mapper)(wchar32));

    inline unsigned char Tr(unsigned char c) const
    {
        return Table[c];
    }
    inline unsigned char operator [](unsigned char c) const
    {
        return Table[c];
    }
    void  Tr(const char* in, char* out, size_t len) const;
    void  Tr(const char* in, char* out) const;
    void  Tr(char* in_out, size_t len) const;
    void  Tr(char* in_out) const;
};

extern const struct Encoder &WideCharToYandex;

const Encoder& EncoderByCharset(ECharset enc);

namespace NCodepagePrivate {
    class TCodePageData {
    private:
        static const CodePage* AllCodePages[];

        static const Recoder rcdr_to_yandex[];
        static const Recoder rcdr_from_yandex[];
        static const Recoder rcdr_to_lower[];
        static const Recoder rcdr_to_upper[];
        static const Recoder rcdr_to_title[];

        static const Encoder* EncodeTo[];

        friend struct ::CodePage;
        friend class TCodepagesMap;
        friend RECODE_RESULT _recodeToYandex(ECharset, const char*, char*, size_t, size_t, size_t&, size_t&);
        friend RECODE_RESULT _recodeFromYandex(ECharset, const char*, char*, size_t, size_t, size_t&, size_t&);
        friend const Encoder& ::EncoderByCharset(ECharset enc);
    };
};

inline const Encoder& EncoderByCharset(ECharset enc) {
    if (!SingleByteCodepage(enc)) {
        ythrow yexception() << "Encoder structure can only be used for single byte encodings";
    }

    return *NCodepagePrivate::TCodePageData::EncodeTo[enc];
}

inline unsigned char CodePage::ToUpper(unsigned char ch) const {
    return NCodepagePrivate::TCodePageData::rcdr_to_upper[CPEnum].Table[ch];
}
inline unsigned char CodePage::ToLower(unsigned char ch) const {
    return NCodepagePrivate::TCodePageData::rcdr_to_lower[CPEnum].Table[ch];
}
inline unsigned char CodePage::ToTitle(unsigned char ch) const {
    return NCodepagePrivate::TCodePageData::rcdr_to_title[CPEnum].Table[ch];
}

extern const CodePage& csYandex;

inline unsigned char utf8_leadbyte_mask(size_t len) {
    // YASSERT (len <= 4);
    return "\0\0\037\017\007"[len];
}

inline size_t utf8_rune_len(const unsigned char p)
{
    return "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\2\2\2\2\3\3\4\0"[p>>3];
}

inline size_t utf8_rune_len_by_ucs(wchar32 rune)
{
    if (rune < 0x80)
        return 1U;
    else if (rune < 0x800)
        return 2U;
    else if (rune < 0x10000)
        return 3U;
    else if (rune < 0x200000)
        return 4U;
    else if (rune < 0x4000000)
        return 5U;
    else
        return 6U;
}

const size_t UTF_MAX = 4;

extern const wchar32 BROKEN_RUNE;

inline RECODE_RESULT utf8_read_rune(wchar32 &rune, size_t &rune_len, const unsigned char* s, const unsigned char* end)
{
    rune = BROKEN_RUNE;
    rune_len = 0;
    wchar32 _rune;

    size_t _len = utf8_rune_len(*s);
    if (s + _len > end) return RECODE_EOINPUT;  //[EOINPUT]
    if (_len==0) return RECODE_BROKENSYMBOL;    //[BROKENSYMBOL] in first byte
    _rune = *s++;                               //[00000000 0XXXXXXX]

    if (_len > 1) {
        _rune &= utf8_leadbyte_mask(_len);
        unsigned char ch = *s++;
        if ((ch & 0xC0) != 0x80)
            return RECODE_BROKENSYMBOL;         //[BROKENSYMBOL] in second byte
        _rune <<= 6;
        _rune |= ch & 0x3F;                     //[00000XXX XXYYYYYY]
        if (_len > 2) {
            ch = *s++;
            if ((ch & 0xC0) != 0x80)
                return RECODE_BROKENSYMBOL;     //[BROKENSYMBOL] in third byte
            _rune <<= 6;
            _rune |= ch & 0x3F;                 //[XXXXYYYY YYZZZZZZ]
            if (_len > 3) {
                ch = *s;
                if ((ch & 0xC0) != 0x80)
                    return RECODE_BROKENSYMBOL; //[BROKENSYMBOL] in fourth byte
                _rune <<= 6;
                _rune |= ch & 0x3F;             //[XXXYY YYYYZZZZ ZZQQQQQQ]
                if (_rune >= 0x200000)
                    return RECODE_BROKENSYMBOL;
                if (_rune < 0x10000)
                    return RECODE_BROKENSYMBOL;
            } else {
                if (_rune < 0x800)
                    return RECODE_BROKENSYMBOL;
            }
        } else {
            if (_rune < 0x80)
                return RECODE_BROKENSYMBOL;
        }
    }
    rune_len = _len;
    rune = _rune;
    return RECODE_OK;
}

inline RECODE_RESULT utf8_put_rune(wchar32 rune, size_t &rune_len, unsigned char* s, size_t tail) {
    rune_len = 0;
    if (rune < 0x80){
        if (tail <= 0) return RECODE_EOOUTPUT;
        *s = (unsigned char)rune;
        rune_len = 1;
        return RECODE_OK;
    }
    if (rune < 0x800){
        if (tail <= 1) return RECODE_EOOUTPUT;
        *s++ = (unsigned char)(0xC0 | (rune >> 6));
        *s   = (unsigned char)(0x80 | (rune & 0x3F));
        rune_len = 2;
        return RECODE_OK;
    }
    if (rune < 0x10000) {
        if (tail <= 2) return RECODE_EOOUTPUT;
        *s++ = (unsigned char)(0xE0 | (rune >> 12));
        *s++ = (unsigned char)(0x80 | ((rune >> 6) & 0x3F));
        *s   = (unsigned char)(0x80 | (rune & 0x3F));
        rune_len = 3;
        return RECODE_OK;
    }
    /*if (rune < 0x200000)*/ {
        if (tail <= 3) return RECODE_EOOUTPUT;
        *s++ = (unsigned char)(0xF0 | ((rune >> 18) & 0x07));
        *s++ = (unsigned char)(0x80 | ((rune >> 12) & 0x3F));
        *s++ = (unsigned char)(0x80 | ((rune >> 6) & 0x3F));
        *s   = (unsigned char)(0x80 | (rune & 0x3F));
        rune_len = 4;
        return RECODE_OK;
    }
}

inline RECODE_RESULT utf8_put_rune(wchar32 rune, size_t &rune_len, unsigned char* s, const unsigned char* end) {
    return utf8_put_rune(rune, rune_len, s, end - s);
}

/// these functions change (lowers) [end] position in case of utf-8
/// null character is NOT assumed or written at [*end]
void DecodeUnknownPlane(wchar32* start, wchar32*& end, const ECharset enc4unk);
void DecodeUnknownPlane(TChar* start, TChar*& end, const ECharset enc4unk);

inline void ToLower(char* s, size_t n, const CodePage& cp = csYandex) {
    char* const e = s + n;
    for (; s != e; ++s)
        *s = cp.ToLower(*s);
}

inline void ToUpper(char* s, size_t n, const CodePage& cp = csYandex) {
    char* const e = s + n;
    for (; s != e; ++s)
        *s = cp.ToUpper(*s);
}
