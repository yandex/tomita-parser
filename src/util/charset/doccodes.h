#pragma once

#include <util/system/defaults.h>
#include <util/generic/strbuf.h>

#include <cstring>

enum ECharset{
    CODES_UNSUPPORTED = -2, // valid but unsupported encoding
    CODES_UNKNOWN = -1,  // invalid or unspecified encoding
    CODES_WIN,           // [ 0] WINDOWS_1251     Windows
    CODES_KOI8,          // [ 1] KOI8_U           Koi8-u
    CODES_ALT,           // [ 2] IBM_866          MS DOS, alternative
    CODES_MAC,           // [ 3] MAC_CYRILLIC     Macintosh
    CODES_MAIN,          // [ 4] ISO_LATIN_CYRILLIC Main
    CODES_ASCII,         // [ 5] WINDOWS_1252     Latin 1
    CODES_RESERVED_3, // reserved code: use it for new encodings before adding them to the end of the list
    CODES_WIN_EAST,      // [ 7] WINDOWS_1250     WIN PL
    CODES_ISO_EAST,      // [ 8] ISO_8859_2       ISO PL
    // our superset of subset of windows-1251
    CODES_YANDEX,        // [ 9] YANDEX
    CODES_UTF_16BE,      // [10] UTF_16BE
    CODES_UTF_16LE,      // [11] UTF_16LE
    // missing standard codepages
    CODES_IBM855,        // [12] IBM_855
    CODES_UTF8,          // [13] UTF8
    CODES_UNKNOWNPLANE,  // [14] Unrecognized characters are mapped into the PUA: U+F000..U+F0FF

    CODES_KAZWIN,        // [15] WINDOWS_1251_K   Kazakh version of Windows-1251
    CODES_TATWIN,        // [16] WINDOWS_1251_T   Tatarian version of Windows-1251
    CODES_ARMSCII,       // [17] Armenian ASCII
    CODES_GEO_ITA,       // [18] Academy of Sciences Georgian
    CODES_GEO_PS,        // [19] Georgian Parliament
    CODES_ISO_8859_3,    // [20] Latin-3: Turkish, Maltese and Esperanto
    CODES_ISO_8859_4,    // [21] Latin-4: Estonian, Latvian, Lithuanian, Greenlandic, Sami
    CODES_ISO_8859_6,    // [22] Latin/Arabic: Arabic
    CODES_ISO_8859_7,    // [23] Latin/Greek: Greek
    CODES_ISO_8859_8,    // [24] Latin/Hebrew: Hebrew
    CODES_ISO_8859_9,    // [25] Latin-5 or Turkish: Turkish
    CODES_ISO_8859_13,   // [26] Latin-7 or Baltic Rim: Baltic languages
    CODES_ISO_8859_15,   // [27] Latin-9: Western European languages
    CODES_ISO_8859_16,   // [28] Latin-10: South-Eastern European languages
    CODES_WINDOWS_1253,  // [29] for Greek
    CODES_WINDOWS_1254,  // [30] for Turkish
    CODES_WINDOWS_1255,  // [31] for Hebrew
    CODES_WINDOWS_1256,  // [32] for Arabic
    CODES_WINDOWS_1257,  // [33] for Estonian, Latvian and Lithuanian

    // these codes are all the other 8bit codes known by libiconv
    // they follow in alphanumeric order
    CODES_CP1046,
    CODES_CP1124,
    CODES_CP1125,
    CODES_CP1129,
    CODES_CP1131,
    CODES_CP1133,
    CODES_CP1161, // [40]
    CODES_CP1162,
    CODES_CP1163,
    CODES_CP1258,
    CODES_CP437,
    CODES_CP737,
    CODES_CP775,
    CODES_CP850,
    CODES_CP852,
    CODES_CP853,
    CODES_CP856, // [50]
    CODES_CP857,
    CODES_CP858,
    CODES_CP860,
    CODES_CP861,
    CODES_CP862,
    CODES_CP863,
    CODES_CP864,
    CODES_CP865,
    CODES_CP869,
    CODES_CP874, // [60]
    CODES_CP922,
    CODES_HP_ROMAN8,
    CODES_ISO646_CN,
    CODES_ISO646_JP,
    CODES_ISO8859_10,
    CODES_ISO8859_11,
    CODES_ISO8859_14,
    CODES_JISX0201,
    CODES_KOI8_T,
    CODES_MAC_ARABIC, // [70]
    CODES_MAC_CENTRALEUROPE,
    CODES_MAC_CROATIAN,
    CODES_MAC_GREEK,
    CODES_MAC_HEBREW,
    CODES_MAC_ICELAND,
    CODES_MAC_ROMANIA,
    CODES_MAC_ROMAN,
    CODES_MAC_THAI,
    CODES_MAC_TURKISH,
    CODES_RESERVED_2, // [80] reserved code: use it for new encodings before adding them to the end of the list
    CODES_MULELAO,
    CODES_NEXTSTEP,
    CODES_PT154,
    CODES_RISCOS_LATIN1,
    CODES_RK1048,
    CODES_TCVN,
    CODES_TDS565,
    CODES_TIS620,
    CODES_VISCII,

    // libiconv multibyte codepages
    CODES_BIG5, // [90]
    CODES_BIG5_HKSCS,
    CODES_BIG5_HKSCS_1999,
    CODES_BIG5_HKSCS_2001,
    CODES_CP932,
    CODES_CP936,
    CODES_CP949,
    CODES_CP950,
    CODES_EUC_CN,
    CODES_EUC_JP,
    CODES_EUC_KR, // [100]
    CODES_EUC_TW,
    CODES_GB18030,
    CODES_GBK,
    CODES_HZ,
    CODES_ISO_2022_CN,
    CODES_ISO_2022_CN_EXT,
    CODES_ISO_2022_JP,
    CODES_ISO_2022_JP_1,
    CODES_ISO_2022_JP_2,
    CODES_ISO_2022_KR, // [110]
    CODES_JOHAB,
    CODES_SHIFT_JIS,

    CODES_MAX
};

// Language names are given according to ISO 639-1
// http://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
enum docLanguage {
    LANG_UNK = 0, // Unknown
    LANG_RUS = 1, // Russian
    LANG_ENG = 2, // English
    LANG_POL = 3, // Polish
    LANG_HUN = 4, // Hungarian
    LANG_UKR = 5, // Ukrainian
    LANG_GER = 6, // German
    LANG_FRN = 7, // French
    LANG_TAT = 8, // Tatar
    LANG_BLR = 9, // Belorussian
    LANG_KAZ = 10, // Kazakh
    LANG_ALB = 11, // Albanian
    LANG_SPA = 12, // Spanish
    LANG_ITA = 13, // Italian
    LANG_ARM = 14, // Armenian
    LANG_DAN = 15, // Danish
    LANG_POR = 16, // Portuguese
    LANG_ISL = 17, // Icelandic
    LANG_SLO = 18, // Slovak
    LANG_SLV = 19, // Slovene
    LANG_DUT = 20, // Dutch (Netherlandish language)
    LANG_BUL = 21, // Bulgarian
    LANG_CAT = 22, // Catalan
    LANG_HRV = 23, // Croatian
    LANG_CZE = 24, // Czech
    LANG_GRE = 25, // Greek
    LANG_HEB = 26, // Hebrew
    LANG_NOR = 27, // Norwegian
    LANG_MAC = 28, // Macedonian
    LANG_SWE = 29, // Swedish
    LANG_KOR = 30, // Korean
    LANG_LAT = 31, // Latin
    LANG_BASIC_RUS = 32, // Simplified version of Russian (used at lemmer only)
    LANG_BOS = 33, // Bosnian
    LANG_MLT = 34, // Maltese
    LANG_EMPTY = 35, // Indicate what document is empty
    LANG_UNK_LAT = 36, // Any unrecognized latin language
    LANG_UNK_CYR = 37, // Any unrecognized cyrillic language
    LANG_UNK_ALPHA = 38, // Any unrecognized alphabetic language not fit into previous categories
    LANG_FIN = 39, // Finnish
    LANG_EST = 40, // Estonian
    LANG_LAV = 41, // Latvian
    LANG_LIT = 42, // Lithuanian
    LANG_BAK = 43, // Bashkir
    LANG_TUR = 44, // Turkish
    LANG_RUM = 45, // Romanian (also Moldavian)
    LANG_MON = 46, // Mongolian
    LANG_UZB = 47, // Uzbek
    LANG_KIR = 48, // Kirghiz
    LANG_TGK = 49, // Tajik
    LANG_TUK = 50, // Turkmen
    LANG_SRP = 51, // Serbian
    LANG_AZE = 52, // Azerbaijani
    LANG_BASIC_ENG = 53, // Simplified version of English (used at lemmer only)
    LANG_GEO = 54, // Georgian
    LANG_ARA = 55, // Arabic
    LANG_PER = 56, // Persian
    LANG_CHU = 57, // Church Slavonic
    LANG_CHI = 58, // Chinese
    LANG_JPN = 59, // Japanese
    LANG_IND = 60, // Indonesian
    LANG_MSA = 61, // Malay
    LANG_THA = 62, // Thai
    LANG_VIE = 63, // Vietnamese
    LANG_MAX
};

docLanguage LanguageByName(const TStringBuf &name);
docLanguage LanguageByNameStrict(const TStringBuf &name);
const char* NameByLanguage(docLanguage language);
const char* IsoNameByLanguage(docLanguage language);
const char* FullNameByLanguage(docLanguage language);

// Same as LanguageByName, but throws yexception() if name is invalid
docLanguage LanguageByNameOrDie(const TStringBuf &name);

inline bool UnknownLanguage(docLanguage language) {
    return language == LANG_UNK || language == LANG_UNK_LAT || language == LANG_UNK_CYR || language == LANG_UNK_ALPHA || language == LANG_EMPTY;
}

class TDocLanguage {
    docLanguage ID_;

public:
    TDocLanguage(docLanguage id)
        : ID_(id)
    {}
    TDocLanguage(const TStringBuf &str)
        : ID_(Parse(str))
    {}
    template <typename T, typename TpTraits>
    TDocLanguage(const TStringBase<T, char, TpTraits> &str)
        : ID_(Parse(str))
    {}

public:
    docLanguage ID() const {
        return ID_;
    }
    bool IsUnknown() const {
        return UnknownLanguage(ID());
    }
    const char* Name() const {
        return NameByLanguage(ID());
    }
    const char* IsoName() const {
        return IsoNameByLanguage(ID());
    }
    const char* FullName() const {
        return FullNameByLanguage(ID());
    }

public:
    static docLanguage Parse(const TStringBuf &str) {
        return LanguageByName(str);
    }
};

// Writing systems, a.k.a. scripts

enum EScript {
    SCRIPT_UNKNOWN = 0,
    SCRIPT_LATIN,
    SCRIPT_CYRILLIC,

    SCRIPT_GREEK,
    SCRIPT_ARABIC,
    SCRIPT_HEBREW,
    SCRIPT_ARMENIAN,
    SCRIPT_GEORGIAN,

    SCRIPT_HAN,
    SCRIPT_KATAKANA,
    SCRIPT_HIRAGANA,
    SCRIPT_HANGUL,

    SCRIPT_DEVANAGARI,
    SCRIPT_BENGALI,
    SCRIPT_GUJARATI,
    SCRIPT_GURMUKHI,
    SCRIPT_KANNADA,
    SCRIPT_MALAYALAM,
    SCRIPT_ORIYA,
    SCRIPT_TAMIL,
    SCRIPT_TELUGU,
    SCRIPT_THAANA,
    SCRIPT_SINHALA,

    SCRIPT_MYANMAR,
    SCRIPT_THAI,
    SCRIPT_LAO,
    SCRIPT_KHMER,
    SCRIPT_TIBETAN,
    SCRIPT_MONGOLIAN,

    SCRIPT_ETHIOPIC,
    SCRIPT_RUNIC,
    SCRIPT_COPTIC,
    SCRIPT_SYRIAC,

    SCRIPT_OTHER,
    SCRIPT_MAX
};

EScript ScriptByLanguage(docLanguage language);
EScript ScriptByGlyph(wchar32 glyph);

namespace NCharsetInternal {
    void InitScriptData(ui8 data[], size_t len);
}

inline bool LatinScript(docLanguage language) {
    return ScriptByLanguage(language) == SCRIPT_LATIN;
}

inline bool CyrillicScript(docLanguage language) {
    return ScriptByLanguage(language) == SCRIPT_CYRILLIC;
}

enum MimeTypes {
    MIME_UNKNOWN    = 0,
    MIME_TEXT       = 1,
    MIME_HTML       = 2,  MIME_XHTMLXML = MIME_HTML,
    MIME_PDF        = 3,
    MIME_RTF        = 4,
    MIME_DOC        = 5,  MIME_MSWORD = MIME_DOC,
    MIME_MPEG       = 6,
    MIME_XML        = 7,  MIME_RSS = MIME_XML,
    MIME_WML        = 8,
    MIME_SWF        = 9,  MIME_FLASH = MIME_SWF,
    MIME_XLS        = 10, MIME_EXCEL = MIME_XLS,
    MIME_PPT        = 11,
    MIME_IMAGE_JPG  = 12,
    MIME_IMAGE_PJPG = 13,
    MIME_IMAGE_PNG  = 14,
    MIME_IMAGE_GIF  = 15,
    MIME_DOCX       = 16,
    MIME_ODT        = 17,
    MIME_ODP        = 18,
    MIME_ODS        = 19,
    //MIME_XHTMLXML   = 20,
    MIME_IMAGE_BMP  = 21,
    MIME_WAV        = 22,
    MIME_ARCHIVE    = 23,
    MIME_EXE        = 24,
    MIME_ODG        = 25,
    MIME_GZIP       = 26,
    MIME_XLSX       = 27,
    MIME_PPTX       = 28,
    MIME_JAVASCRIPT = 29,
    MIME_EPUB       = 30,
    MIME_TEX        = 31,
    MIME_MAX
};

extern const char *MimeNames[MIME_MAX];

const char* mimetypeByExt(const char *fname, const char* check_ext=0);
MimeTypes mimeByStr(const char *mimeStr);
const char* strByMime(MimeTypes mime);
