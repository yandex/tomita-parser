#pragma once

#include "defs.h"
#include "wcs.h"
#include <util/charset/doccodes.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

struct CodePage;
class TLangIDs;

typedef TWtringBuf TWStringBuf;
typedef yvector<Stroka> VectorStrok;
typedef yvector<WStroka> VectorWStrok;

////////////////////////////////////////////////////////////////////////////////
//
// Script Types
//

typedef ui64 TScriptMask;
#define SCRIPT_MASK_(NAME) (ui64(1) << SCRIPT_##NAME)
const TScriptMask SCRIPT_MASK_LATIN = SCRIPT_MASK_(LATIN);
const TScriptMask SCRIPT_MASK_CYRILLIC = SCRIPT_MASK_(CYRILLIC);

TScriptMask ClassifyScript(const wchar16* str, size_t len);
TScriptMask ClassifyScript(const WStroka& str);
TScriptMask ClassifyScript(const Stroka& str);

inline bool TestScriptMask(TScriptMask mask, EScript script) { return (mask >> script) & 1; }
inline TScriptMask ScriptMaskForScript(EScript script) { return 1ULL << script; }
inline TScriptMask ScriptMaskForLang(docLanguage lang) { return 1ULL << ScriptByLanguage(lang); }
TScriptMask ScriptMaskForLangs(const TLangIDs& langIDs);

const TLangIDs& LangIDsForScript(EScript script);

////////////////////////////////////////////////////////////////////////////////
//
// String functions
//

enum EPrefSuffMode {
    PS_NOT_GREEDY    = 0x00, // if prefix and suffix overlap -- shrink them
    PS_GREEDY_PREFIX = 0x01, // looks for the longest possible prefix
    PS_GREEDY_SUFFIX = 0x02, // looks for the longest possible suffix
    PS_GREEDY        = 0x03  // if prefix and suffix overlap -- don't shrink them
};

/// Returns common prefix/suffix length for given strings.
void PrefSuff(const wchar16* s, int lenS, const wchar16* t, int lenT,
    int* prefix, int* suffix, EPrefSuffMode mode = PS_GREEDY_PREFIX);
void PrefSuff(const char* s, int lenS, const char* t, int lenT,
    int* prefix, int* suffix, EPrefSuffMode mode = PS_GREEDY_PREFIX);

/// Maps one string to another, performing a specified locale-dependent transformation.
/*! @param langID language ID
    @param flags MAPSTR_XXX flags
    @param str source string
    @return transformed string */
Stroka MapString(docLanguage langID, int flags, const Stroka& str);
WStroka MapString(docLanguage langID, int flags, const WStroka& str);

const int MAPSTR_NONE       = 0x0000;
const int MAPSTR_LOWERCASE  = 0x0001;
const int MAPSTR_UPPERCASE  = 0x0002;
const int MAPSTR_DICT       = 0x0004;
const int MAPSTR_REPLACE_IO = MAPSTR_DICT; // replace RUS_IO -> RUS_IE
const int MAPSTR_REMOVE_DIACRITICS = 0x0008; // Remove diacritics for Russian and Turkish.
const int MAPSTR_RENYXA     = 0x0010; // converts Lat/Cyr equal letters (A, E, O, ...)
const int MAPSTR_SIMPLIFIED_CHINESE = 0x0020;

wchar16 ToSimplified(wchar16 ch);
WStroka ToSimplified(const WStroka& str);

/** Performs 'renyxa' conversion for given string
    @param script script of result string
    @param charset charset of input ANSI-string
    @param str input string
    @param flags RENYXA_XXX flags
    @return converted string or input string if conversion wasn't performed */
Stroka Renyxa(EScript script, ECharset charset, const Stroka& str, int flags = 0);
WStroka Renyxa(EScript script, const WStroka& str, int flags = 0);

const int RENYXA_MIXED_SOURCE = 0x0001; // Convert only strings with mixed alphabets
const int RENYXA_MIXED_RESULT = 0x0002; // Allow mixed alphabets in result string

////////////////////////////////////////////////////////////////////////////////
//
// Language functions
//

/** Returns ISO language name (2-letter code) for given langID.
    If langID = LANG_UNK -- returns 'unknownValue' (by default: empty string).
    If langID = LANG_MAX -- throws invalid_argument exception. */
const char* LangCode(docLanguage langID, const char* unknownValue = "");

/** Returns:
    0 if all characters are "neutral" (not letters),
    1 if all letters are from given language alphabet,
   -1 otherwise (at least one letter is not from alphabet) */
int CheckAbc(docLanguage langID, const wchar16* str, size_t len);
int CheckAbc(docLanguage langID, const WStroka& str);

/// Returns mask of docLanguage codes (1 << langID) for given character/string.
ui64 LangIDsFor(wchar16 ch);
ui64 LangIDsFor(const WStroka& str, ui64 langMask = 0);
ui64 LangIDsFor(const wchar16* str, const wchar16* end, ui64 langMask = 0);

/// Returns default codepage for given language
ECharset CharsetForLang(docLanguage langID);
const CodePage* CodePageForLang(docLanguage langID);

/// Returns langID for given name
docLanguage LanguageByName(const WStroka& name);

/// Returns country code for given langID (or "" for unknown lang)
const char* CountryCode(docLanguage langID);

/// Returns keyboard code for given langID (or "" for unknown lang)
/// Note: for Ukrainian returns "UA" (instead of "UK")
const char* YandexKeyboardCode(docLanguage langID);

////////////////////////////////////////////////////////////////////////////////
//
// Capitalization
//

/** Capitalization type */
enum ECapType {
    CAP_LOWER, /** lower-case */
    CAP_TITLE, /** title-case */
    CAP_UPPER, /** all upper-case */
    CAP_MIXED, /** mixed capitalization */
    CAP_NONE = -1 /** no capitalization (may be used as default value) */
};

struct TCapInfo {
    ECapType Type;
    ui32     Mask;

    TCapInfo()
      : Type(CAP_LOWER), Mask(0)
    {}

    TCapInfo(ECapType type)
      : Type(type), Mask(0)
    {}

    TCapInfo(ECapType type, ui32 mask)
      : Type(type), Mask(mask)
    {}

    void Clear() {
        Type = CAP_LOWER;
        Mask = 0;
    }

    bool operator==(const TCapInfo& other) const {
        return Type == other.Type &&
               (Type != CAP_MIXED || Mask == other.Mask);
    }
};

/**
Returns capitalization of given string.
@param str input string
@return capitalization of str
*/
TCapInfo GetCapInfo(const Stroka& str, const CodePage* cs = &csYandex);
TCapInfo GetCapInfo(const WStroka& str);
ECapType GetCapType(const Stroka& str, const CodePage* cs = &csYandex);
ECapType GetCapType(const WStroka& str);

/**
Capitalizes given string.
@param langID use capitalization rules for given language (treat Turkish i/I correctly)
@param str string to capitalize
@param capType required capitalization
@return capitalized word
@remarks if cap = CAP_MIXED or CAP_NONE -- input string will be returned.
@remarks functions with parameter langID treats Turkish i/I correctly;
         other functions use Unicode rules
*/

Stroka SetCap(docLanguage langID, const Stroka& str, ECapType capType);
Stroka SetCap(docLanguage langID, const Stroka& str, const TCapInfo& cap);
WStroka SetCap(docLanguage langID, const WStroka& str, ECapType capType);
WStroka SetCap(docLanguage langID, const WStroka& str, const TCapInfo& cap);
WStroka AddCap(docLanguage langID, const WStroka& str, ECapType capType);

Stroka SetCap(const Stroka& str, ECapType capType, const CodePage* cs = &csYandex);
Stroka SetCap(const Stroka& str, const TCapInfo& cap, const CodePage* cs = &csYandex);
WStroka SetCap(const WStroka& str, ECapType capType);
WStroka SetCap(const WStroka& str, const TCapInfo& cap);

////////////////////////////////////////////////////////////////////////////////
//
// String distance functions
//

/**
Returns Levenshtein distance for given strings (case sensitive).
*/
int strdist(const char* from, const char* to);
int strdist(const Stroka& from, const Stroka& to);
int strdist(const wchar16* from, const wchar16* to);
int strdist(const WStroka& from, const WStroka& to);

// diff mask
typedef ui64 correction_mask;

#define STRDIFF_IGNORE_CASE  MAPSTR_LOWERCASE
#define STRDIFF_DICT_RULES   MAPSTR_DICT // do not distinguish RUS_IE and RUS_IO
#define STRDIFF_DICT_RULES_I (STRDIFF_IGNORE_CASE | STRDIFF_DICT_RULES)

correction_mask diffmask(const char *left, size_t llen, const char *right, size_t rlen);
correction_mask diffmask(const Stroka &left, const Stroka &right);

/**
Markups differences between given strings.

@param source source string
@param target target string
@param open_tag tag used to markup begin of selection
@param close_tag tag used to markup end of selection
@flags STRDIFF_XXX flags

@return target string with marked differences.
*/
Stroka markup_diff(
    const Stroka& source, const Stroka& target,
    const Stroka& open_tag, const Stroka& close_tag,
    int flags = 0);

////////////////////////////////////////////////////////////////////////////////
//
// Parsing
//

enum EParseFlags {
    PARSE_NONE  = 0x0000,
    PARSE_FILE  = 0x0001,
    PARSE_URL   = 0x0002,
    PARSE_EMAIL = 0x0004
};

enum EParseOptions {
    PARSE_OPT_NONE = 0x0000,
    PARSE_OPT_ALLOW_CYR = 0x0001
};

enum EKnown {
    KNOWN_NONE = 0x0000,
    KNOWN_ROOT_DOMAIN = 0x0001,
    KNOWN_FILE_EXT = 0x0002
};

bool ParseString(const WStroka& str, int flags, int opt = 0);
bool IsKnown(const WStroka& str, int knownFlags, int opt = 0);
bool IsFileExt(const WStroka& str);
bool IsFileExt(const Stroka& str);

////////////////////////////////////////////////////////////////////////////////
//
// Encode/Decode
//

WStroka CharToWide(ECharset charset, const TStringBuf& str);
WStroka CharToWide(ECharset charset, const char* str, size_t cb = (size_t)-1);
void CharToWide(ECharset charset, const VectorStrok& str, VectorWStrok* result);

Stroka WideToChar(ECharset charset, const TWStringBuf& wstr);
Stroka WideToChar(ECharset charset, const wchar16* wstr, size_t cch = (size_t)-1);
void WideToChar(ECharset charset, const VectorWStrok& wstr, VectorStrok* result);

// Returns false if not all characters were converted successfully
bool WideToCharEx(ECharset charset, const WStroka& str, Stroka* result);

Stroka Convert(ECharset from, ECharset to, const Stroka& bytes);

////////////////////////////////////////////////////////////////////////////////
//
// I/O functions
//

int IsAtty(); // returns 1 if file is associated with a terminal
void SetBinaryMode(); // set binary mode for stdout
ECharset GetConsoleCharset();
void SetConsoleCharset(ECharset charset);

////////////////////////////////////////////////////////////////////////////////
//
// Path functions
//

// If "baseDir" or "path" is empty, method returns the other path.
// If "path" is absolute, method returns "path".
// Otherwise returns "path1" + "path2" (with correct '/' between)
Stroka CombinePath(const Stroka& baseDir, const Stroka& path);

// If "path1" or "path2" is empty, method returns the other path.
// Otherwise returns "path1" + "path2" (with correct '/' between)
Stroka JoinPath(const Stroka& path1, const Stroka& path2);

Stroka MakePath(const Stroka& path, const Stroka& name, const Stroka& ext);
void SplitPath(const Stroka& file, Stroka& path, Stroka& name);
void SplitPath(const Stroka& file, Stroka& path, Stroka& name, Stroka& ext);
Stroka ChangeFileExt(const Stroka& path, const Stroka& ext);
Stroka GetFileExt(const Stroka& path);
bool IsExist(const Stroka& path);

////////////////////////////////////////////////////////////////////////////////
//
// Buffer checks
//

// Check Byte Order Mark
ECharset CheckBom(const void* bom, size_t len, size_t* bomLen = 0);

struct TCheckBuf {
    ECharset Charset;
    size_t   BomLen;
    bool     IsBinary;

    TCheckBuf() : Charset(CODES_UNKNOWN), BomLen(0), IsBinary(false) {}

    void Clear() {
        Charset = CODES_UNKNOWN;
        BomLen = 0;
        IsBinary = false;
    }
};

void CheckBuf(const void* buf, size_t size, TCheckBuf* result);
bool CheckUtf8(const void* buf, size_t size, bool endOfInput = true);
bool CheckUtf8(const Stroka& buf, bool endOfInput = true);

////////////////////////////////////////////////////////////////////////////////
//
// Debug functions
//

void CheckMemoryLeaks();
void DebugPrint(const char* str);
void printf_format(1,2) DebugPrintf(const char* fmt, ...);

#ifdef NDEBUG
#define DEBUG_PRINTF(str, ...)
#else // DEBUG
#define DEBUG_PRINTF(str, ...)   DebugPrintf(str, __VA_ARGS__)
#endif // NDEBUG/DEBUG

////////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//

/** Compares three values and returns the smallest. */
template<typename T>
inline T min3(T x, T y, T z) {
    if (x < y)
        return (x < z ? x : z);
    else
        return (y < z ? y : z);
}

/** Compares three values and returns the largest. */
template<typename T>
inline T max3(T x, T y, T z) {
    if (x > y)
        return (x > z ? x : z);
    else
        return (y > z ? y : z);
}

/** Returns "next" bit in value.

@param value integer value
@param prev "previous" bit (0 for first call)

@return "next" bit or 0 if there are no 1-bits in "value".
*/
template<typename T>
T NextBit(const T value, const T prev) {
    T v = value;
    if (prev)
        v &= ~((prev << 1) - 1);
    return v & ~(v - 1);
}

#if defined(__GNUC__) && defined(_sse42_)
inline int CountBits32(ui32 value) { return __builtin_popcount(value); }
#else
inline int CountBits32(ui32 value) {
    extern const char CBITS[256];
    const ui8* p = (const ui8*)&value;
    return CBITS[p[0]] + CBITS[p[1]] + CBITS[p[2]] + CBITS[p[3]];
}
#endif

inline bool IsUtf16(ECharset charset) {
    return charset == CODES_UTF_16LE || charset == CODES_UTF_16BE;
}

inline bool IsUtf(ECharset charset) {
    return charset == CODES_UTF8 || IsUtf16(charset);
}

////////////////////////////////////////////////////////////////////////////////
//
// Math functions
//

/** Returns log2(value) rounded towards 0. Log2i(0) returns -1 */
int Log2i(ui32 value);

/** Returns log2(value) rounded towards +INF. Log2ii(0) returns -1 */
int Log2ii(ui32 value);

double Nan();
float Nanf();
