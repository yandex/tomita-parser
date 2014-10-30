#pragma once

#include "buf_ptr.h"
#include "defs.h"
#include "wcs.h"
#include <util/charset/unidata.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

class TCharSet;
typedef TWtringBuf TWStringBuf;
typedef yvector<Stroka> VectorStrok;
typedef yvector<WStroka> VectorWStrok;
typedef yvector<TStringBuf> TStrBufVector;
typedef yvector<TWStringBuf> TWStrBufVector;

// Returns timestamp formatted for log file.
enum ETimeFormat {
    TIME_NONE,
    TIME_ISO8601,
    TIME_UNIX,
    TIME_YANDEX
};

Stroka FormatTime(time_t time, ETimeFormat format = TIME_ISO8601);

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

struct WC {
    static const wchar16 Empty[];
    static const wchar16 CrLf[]; // "\r\n"
    static const wchar16 NewLine[]; // "\n"
    static const wchar16 Tab[];
    static const wchar16 Space[];
    static const wchar16 Dash[];
};

template<class TStroka>
struct TS {
    static const TStroka Empty;
    static const TStroka CrLf; // "\r\n"
    static const TStroka NewLine; // "\n"
    static const TStroka Tab;
    static const TStroka Space;
    static const TStroka Dash;
    static const TStroka True; // "true"
    static const TStroka False; // "false"
};

typedef TS<Stroka> S;
typedef TS<WStroka> WS;

////////////////////////////////////////////////////////////////////////////////
//
// String functions
//

template <typename TStroka>
typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString
Join(TStringBuf sep, const TStroka* text, int index, int count);

template <typename TStroka>
typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString
Join(TStringBuf sep, const TBufPtr<const TStroka>& buf);

template <typename TStroka>
typename TCharTraitsBase<typename TStroka::char_type>::TWhatString::TString
Join(TStringBuf sep, const yvector<TStroka>& text, int index = 0, int count = -1);

template<typename TStroka>
size_t Length(const yvector<TStroka>& text);

char* i2a(int value, char* str);
Stroka i2a(int i);
WStroka i2w(int i);
int a2i(const Stroka& str);
int w2i(const TWStringBuf& str);
double a2d(const TStringBuf& str);
Stroka d2a(double value);
WStroka d2w(double value);

extern i8 mapHex2i[128];
inline int hex2i(char ch) { return ui8(ch) < ARRAY_SIZE(mapHex2i) ? mapHex2i[ui8(ch)] : -1; }
inline int hex2i(wchar16 ch) { return ch < ARRAY_SIZE(mapHex2i) ? mapHex2i[ch] : -1; }
template<typename tchar_t> tchar_t* i2hex(int value, tchar_t* str, int len);
char* i2Hex(int value, char* str, int len);

template<typename TStroka>
TStroka& Append(TStroka& str, const char* add, size_t len = (size_t)-1);
WStroka& Append(WStroka& str, const Stroka& add);
void RemoveAll(Stroka& str, const char* remove, size_t startPos = 0);
void RemoveAll(WStroka& str, const wchar16* remove, size_t startPos = 0);
void RemoveAll(WStroka& str, const char* remove, size_t startPos = 0);
void RemoveAnyOf(Stroka& str, const char* remove);
void RemoveAnyOf(WStroka& str, const wchar16* remove);

template<typename TStroka, typename TChar, typename TChar2>
void ReplaceAll(TStroka& str, const TChar* oldStr, const TChar2* newStr);
template<typename TStroka, typename TChar, typename TStroka2>
void ReplaceAll(TStroka& str, const TChar* oldStr, const TStroka2& newStr);
void ReplaceAll(Stroka& str, char oldChr, char newChr);
void ReplaceAll(WStroka& str, wchar16 oldChr, wchar16 newChr);

void ReplaceAnyOf(Stroka& str, const char* oldChars, char newChar);
void ReplaceAnyOf(WStroka& str, const wchar16* oldChars, wchar16 newChar);
void ReplaceAnyOf(WStroka& str, const char* oldChars, char newChar);

/// Returns wide-string for given ASCII-string
/// (all non-ASCII characters from input string will be replaced by '?')
WStroka WStrokaFor(const char* str, size_t len = (size_t)-1);
WStroka WStrokaFor(TStringBuf str);

bool StartsWith(const WStroka& str, const char* prefix);
bool EndsWith(const WStroka& str, const char* suffix);

/** Returns length of common prefix for given strings */
template<typename TStroka>
size_t CommonPrefix(const TStroka& s1, const TStroka& s2);

/** Returns length of common suffix for given strings */
template<typename TStroka>
size_t CommonSuffix(const TStroka& s1, const TStroka& s2);

size_t Find(const WStroka& str, const char* what, size_t off = 0);
size_t FindAnyOf(const Stroka& str, const char* set, size_t off = 0);
size_t FindAnyOf(const WStroka& str, const char* set, size_t off = 0);
size_t FindAnyOf(const Stroka& str, const TCharSet& set, size_t off = 0);
size_t FindAnyOf(const WStroka& str, const TCharSet& set, size_t off = 0);

size_t FindLastOf(const WStroka& str, const TCharSet& set);

const int IGNORE_CASE = 0x0001;

bool Equals(const Stroka& str1, const char* str2, int flags = 0);
bool Equals(const WStroka& str, const char* asciiStr, int flags = 0);
bool Equals(const WStroka& str1, const WStroka& str2, int flags = 0);

bool IsNumber(const WStroka& s);

/// Returns true if given string is hex number ([0-9a-fA-F]+)
bool IsHex(const TStringBuf& s);
bool IsHex(const TWStringBuf& s);

inline bool IsAsciiDigit(char ch) { return ch >= '0' && ch <= '9'; }
inline bool IsAsciiDigit(wchar16 ch) { return ch >= '0' && ch <= '9'; }
inline bool IsAsciiLower(char ch) { return ch >= 'a' && ch <= 'z'; }
inline bool IsAsciiLower(wchar16 ch) { return ch >= 'a' && ch <= 'z'; }
inline bool IsAsciiAlpha(char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }
inline bool IsAsciiAlpha(wchar16 ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool IsAscii(const TStringBuf& s);
bool IsAscii(const TWStringBuf& s);

template<typename TStroka>
Stroka ToAscii(const TStroka& s);

/// Returns result of EscapeC converted to Stroka.
Stroka Escape(const WStroka& str);

void RemoveCrLf(char* s);
void RemoveCrLf(Stroka& s);

////////////////////////////////////////////////////////////////////////////////
//
// ISep
//

template<typename tchar_t>
class ISep {
public:
    virtual ~ISep() {}
    virtual const tchar_t* Find(
        const tchar_t* str, const tchar_t* end, size_t* cchSep) const = 0;
};

class TCharSep : public ISep<char>, public ISep<wchar16> {
// Constructor
public:
    // @param meta '+' for 'one-or-more' separator mode
    TCharSep(char ch, char meta = 0);
    TCharSep(wchar16 ch, char meta = 0);

// ISep methods
public:
    const char* Find(const char* s, const char* end, size_t* len) const;
    const wchar16* Find(const wchar16* s, const wchar16* end, size_t* len) const;

// Fields
private:
    wchar16 Char;
    bool Plus;
};

class TCharSetSep : public ISep<char>, public ISep<wchar16> {
    DECLARE_NOCOPY(TCharSetSep);

// Constructor
public:
    // @param meta '+' for 'one-or-more' separator mode
    TCharSetSep(const TCharSet& set, char meta = 0);

// ISep methods
public:
    const char* Find(const char* s, const char* end, size_t* len) const;
    const wchar16* Find(const wchar16* s, const wchar16* end, size_t* len) const;

// Fields
private:
    const TCharSet& Set;
    bool Plus;
};

////////////////////////////////////////////////////////////////////////////////
//
// Split functions
//

#define KEEP_EMPTY_TOKENS   0x01
#define KEEP_DELIMITERS     0x02
#define REMOVE_EMPTY_TOKENS 0x04

/** Splits an input string into an array of substrings
    @param str input string
    @sep separator
    @maxCount maximum number of substrings to return
    @flags zero or flags: KEEP_DELIMITERS, REMOVE_EMPTY_TOKENS
    @return array of substrings */
VectorStrok Split(const Stroka& str, const ISep<char>& sep, int maxCount = 0, int flags = 0);
VectorWStrok Split(const WStroka& str, const ISep<wchar16>& sep, int maxCount = 0, int flags = 0);

VectorStrok Split(const Stroka& str, char sep, int maxCount = 0, int flags = 0);
VectorWStrok Split(const WStroka& str, wchar16 sep, int maxCount = 0, int flags = 0);

VectorStrok Split(const Stroka& str, const TStringBuf& sep);
VectorWStrok Split(const WStroka& str, const TStringBuf& sep);

// Splits by [ \t]+ with REMOVE_EMPTY_TOKENS flags
VectorStrok Split(const Stroka& str);
VectorWStrok Split(const WStroka& str);
int Split(const TStringBuf& str, TStrBufVector* arr);

// Splits by [set]+ separator
VectorStrok SplitBySet(const Stroka& str, const char* set, int maxCount = 0, int flags = 0);
VectorWStrok SplitBySet(const WStroka& str, const char* set, int maxCount = 0, int flags = 0);

int Split(char* str, const char* sep, char* arr[], int maxCount);
int Split(const TStringBuf& str, const char* sep, TStringBuf arr[], int maxCount);
int Split(const TStringBuf& str, const TStringBuf& sep, TStrBufVector* arr);
int Split(const TWtringBuf& str, wchar16 sep, TWStrBufVector* arr);

// Splits "name SEPARATOR value" string into components.
// Returns separator position in the input string (or -1 if separator wasn't found)
// Remark: it's allowed when parameter 'name' or 'value' points to source string 'str'.
int SplitNameValue(const Stroka& str, char sep, Stroka* name, Stroka* value);
int SplitNameValue(const WStroka& str, wchar16 sep, WStroka* name, WStroka* value);

////////////////////////////////////////////////////////////////////////////////
//
// Variable arguments
//

#define VA_SPRINTF(str, fmt, args) \
    va_list args; \
    va_start(args, fmt); \
    vsprintf(str, fmt, args); \
    va_end(args)
