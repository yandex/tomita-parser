#include "dictutil.h"
#include "chars.h"
#include "char_map.h"
#include "lang_ids.h"
#include "static_ctor.h"
#include "str.h"
#include <library/unicode_normalization/normalization.h>
#include <util/string/util.h>

using namespace NUnicode;

////////////////////////////////////////////////////////////////////////////////
//
// Renyxer
//

class TRenyxer {
    DECLARE_NOCOPY(TRenyxer);

// Constructor
public:
    TRenyxer()
      : MapY(), LatY(), CyrY()
      , Map(), Lat(), Cyr()
    {
        Init();
    }

// Methods
public:
    Stroka Convert(EScript script, ECharset charset, const Stroka& str, int flags) const;
    WStroka Convert(EScript script, const WStroka& str, int flags) const;

// Helper methods
private:
    void Init();

// Fields
private:
    ui8 MapY[256];
    Stroka LatY, CyrY;
    TWCharMap<ui8> Map;
    WStroka Lat, Cyr;
};

static const TRenyxer theRenyxer;

void TRenyxer::Init() {
    // Correction: "г->r" is useful for URLs (".гu -> .ru")
    static const char* LAT = ".ABCEHKMOPTXY"  "acegkmnopruxy"  "Ii";
    static const char* CYR = ".АВСЕНКМОРТХУ"  "аседкмпоргиху"  "Іі";

    LatY = LAT;
    CyrY = ::Convert(CODES_UTF8, CODES_YANDEX, CYR);
    Lat = CharToWide(CODES_UTF8, LAT);
    Cyr = CharToWide(CODES_UTF8, CYR);

    for (ui8 i = 1, n = (ui8)LatY.length(); i < n; ++i) {
        MapY[(ui8)LatY[i]] = i;
        MapY[(ui8)CyrY[i]] = i;
        Map.at(Lat[i]) = i;
        Map.at(Cyr[i]) = i;
    }
}

Stroka TRenyxer::Convert(EScript script, ECharset charset, const Stroka& str, int flags) const {
    static const int cyrCharsets
        = (1 << CODES_YANDEX) | (1 << CODES_WIN) | (1 << CODES_KAZWIN) | (1 << CODES_TATWIN);
    if (!(cyrCharsets & (1 << charset)) || charset > CODES_TATWIN)
        return str;

    const TScriptMask scriptMask = ScriptMaskForScript(script);
    if (flags & RENYXA_MIXED_SOURCE) {
        if ((ClassifyScript(str) & scriptMask) == 0)
            return str;
    }

    const char* target = 0;
    switch (script) {
        case SCRIPT_LATIN: target = LatY.c_str(); break;
        case SCRIPT_CYRILLIC: target = CyrY.c_str(); break;
        default: return str;
    }

    Stroka result(str);
    for (int i = 0, n = (int)str.length(); i < n; ++i) {
        ui8 index = MapY[(ui8)str[i]];
        if (index && str[i] != target[index])
            result.replace(i, 1, 1, target[index]);
    }

    if ((flags & RENYXA_MIXED_RESULT) == 0) {
        if (ClassifyScript(result) != scriptMask)
            return str;
    }
    return result;
}

WStroka TRenyxer::Convert(EScript script, const WStroka& str, int flags) const {
    const TScriptMask scriptMask = 1ULL << script;
    if (flags & RENYXA_MIXED_SOURCE) {
        if ((ClassifyScript(str) & scriptMask) == 0)
            return str;
    }

    const wchar16* target = 0;
    switch (script) {
        case SCRIPT_LATIN: target = Lat.c_str(); break;
        case SCRIPT_CYRILLIC: target = Cyr.c_str(); break;
        default: return str;
    }

    WStroka result(str);
    for (int i = 0, n = (int)str.length(); i < n; ++i) {
        ui8 index = Map[str[i]];
        if (index && str[i] != target[index])
            result.replace(i, 1, 1, target[index]);
    }

    if ((flags & RENYXA_MIXED_RESULT) == 0) {
        if (ClassifyScript(result) != scriptMask)
            return str;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// Simplified Chinese
//

inline wchar16 ToSimplified(wchar16 ch) {
    extern const wchar16* scTable[];
    if (const wchar16* block = scTable[ch >> 8])
        return block[ch & 0xff];
    return 0;
}

WStroka ToSimplified(const WStroka& str) {
    const int n = int(str.length());
    WStroka result;
    result.reserve(n);
    for (int i = 0; i < n; ++i) {
        wchar16 ch = str[i];
        if (wchar16 simple = ToSimplified(ch))
            ch = simple;
        result.push_back(ch);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// Mapper
//

class TMapper {
    DECLARE_NOCOPY(TMapper);
    DECLARE_STATIC_CTOR_AND_DTOR;

// Constructor/Destructor
public:
    TMapper();
    ~TMapper();

// Methods
public:
    void RemoveDiacritics(docLanguage langID, WStroka& str) const;
    void RemoveDiacritics(docLanguage langID, Stroka& str) const;
    void ReplaceYo(Stroka& str) const { TrYo.Do(str); }

// Helper methods
private:
    void Init();
    void InitWide();
    void InitChar();

// Instance
public:
    static const TMapper* Instance;

// Fields
private:
    static const char YO[];
    static const char YE[];
    TLangIDs LangsWithDiacr;
    TWCharMap<wchar16> NormW;
    std::vector<const char*> NormA;
    Tr TrYo; // conversion: YO -> YE
};

const TMapper* TMapper::Instance = 0;
const char TMapper::YO[] = { yaCYR_IO, yaCYR_io, 0 };
const char TMapper::YE[] = { yaCYR_IE, yaCYR_ie, 0 };

DEFINE_STATIC_CTOR(TMapper);

void TMapper::StaticCtor() {
    if (!Instance)
        Instance = new TMapper();
}

void TMapper::StaticDtor() {
    delete Instance;
}

TMapper::TMapper()
  : LangsWithDiacr()
  , NormW(), NormA()
  , TrYo(YO, YE)
{
    Init();
}

TMapper::~TMapper() {
    for (int i = 0; i < (int)NormA.size(); ++i)
        delete [] NormA[i];
    NormA.clear();
}

void TMapper::Init() {
    static const docLanguage langsWithDiacr[] = {
        LANG_UNK, LANG_ALB, LANG_AZE, LANG_BOS, LANG_DAN, LANG_DUT, LANG_CAT,
        LANG_CZE, LANG_ENG, LANG_EST, LANG_FIN, LANG_FRN, LANG_GER, LANG_GRE,
        LANG_HRV, LANG_HUN, LANG_ISL, LANG_ITA, LANG_LAV, LANG_LIT, LANG_MAC,
        LANG_MLT, LANG_NOR, LANG_POL, LANG_POR, LANG_RUM, LANG_RUS, LANG_SLO,
        LANG_SLV, LANG_SPA, LANG_SWE, LANG_TUR, LANG_VIE
    };
    LangsWithDiacr.Set(langsWithDiacr, ARRAY_SSIZE(langsWithDiacr));

    InitWide();
    InitChar();
}

void TMapper::InitWide() {
    static const struct TRange {
        wchar16 Start;
        ui16    Len;
    }
    ranges[] = {
        { 0x080, 128 },
        { 0x100, 180 }, // Latin Extended-A, Latin Extended-B
        { 0x218,   4 }, // ȘȚ (with comma below)
        { 0x380, 128 }, // Greek
        { 0x400, 256 }, // Cyrillic
    };

    TNormalizer<NFD> nfd;
    wchar16 normCh[4];
    for (const TRange* range = ranges; range != ARRAY_END(ranges); ++range) {
        for (wchar16 ch = range->Start, end = wchar16(ch + range->Len); ch != end; ++ch) {
            if (ch == wCYR_SHORT_I || ch == wCYR_short_i)
                continue; // prevent conversion: Й -> И
            if (ch == wCYR_YI || ch == wCYR_yi)
                continue; // prevent conversion: Ї -> i

            wchar16* p = normCh;
            nfd.Normalize(&ch, 1, p);
            assert(p <= ARRAY_END(normCh));
            if (normCh[0] != ch)
                NormW.at(ch) = normCh[0];
        }
    }

    NormW.at(wDotless_i) = 'i';
}

void TMapper::InitChar() {
    static const ECharset charsets[] = {
        CODES_ASCII, CODES_ISO_8859_3, CODES_KAZWIN, CODES_WIN,
        CODES_WINDOWS_1253, CODES_WINDOWS_1254, CODES_WINDOWS_1257,
        CODES_CP1258, CODES_WIN_EAST, CODES_YANDEX
    };
    static const int BLOCK_SIZE = 128;

    char str[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; ++i)
        str[i] = char(0x80 + i);

    for (int j = 0; j < ARRAY_SSIZE(charsets); ++j) {
        ECharset charset = charsets[j];
        if (charset >= (int)NormA.size())
            NormA.resize(charset + 1);

        const WStroka wide = CharToWide(charset, str, BLOCK_SIZE);
        WStroka normW(wide);
        RemoveDiacritics(LANG_UNK, normW);
        if (normW == wide) {
            assert(false);
            continue;
        }
        Stroka normA = WideToChar(charset, normW);
        assert((int)normA.length() == BLOCK_SIZE);

        char* chars = new char[BLOCK_SIZE];
        NormA[charset] = chars;
        memset(chars, 0, 128);
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            if (normA[i] != str[i])
                chars[i] = normA[i];
        }
    }
}

void TMapper::RemoveDiacritics(docLanguage langID, WStroka& str) const {
    if (!LangsWithDiacr.Test(langID))
        return;

    WStroka::const_iterator cit = str.begin();
    for (; cit != str.end(); ++cit) {
        if (NormW[*cit])
            break;
    }
    if (cit == str.end())
        return;

    WStroka::iterator it = str.begin();
    for (it += (cit - it); it != str.end(); ++it) {
        wchar16 ch = NormW[*it];
        if (ch)
            *it = ch;
    }
}

void TMapper::RemoveDiacritics(docLanguage langID, Stroka& str) const {
    if (!LangsWithDiacr.Test(langID))
        return;

    const char* noDiacr = 0;
    const ECharset charset = CharsetForLang(langID);
    if (charset >= 0 && charset < (int)NormA.size())
        noDiacr = NormA[charset];
    if (!noDiacr)
        return;

    Stroka::const_iterator cit = str.begin();
    for (; cit != str.end(); ++cit) {
        if ((*cit & 0x80) && noDiacr[*cit & 0x7f])
            break;
    }
    if (cit == str.end())
        return;

    Stroka::iterator it = str.begin();
    for (it += (cit - it); it != str.end(); ++it) {
        if (*it & 0x80) {
            char ch = noDiacr[*it & 0x7f];
            if (ch)
                *it = ch;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// TTrConverter
//

namespace {

// It is Tr because it uses struct Tr, not because it was used for Turkish.
class TTrConverter {
    DECLARE_NOCOPY(TTrConverter);
public:
    explicit TTrConverter(docLanguage lang);
    Stroka Convert(const Stroka& input, int flags) const;

private:
    Stroka SymbolList;
    Tr Uppercase;
    Tr Lowercase;
    Tr Dict;
    Tr RemoveDiacritics;

private:
    static Stroka CreateSymbolList();
    static Stroka SlowConvert(const Stroka& input, int flags, docLanguage lang);
};

Stroka TTrConverter::CreateSymbolList() {
    Stroka result;
    result.reserve(256);
    for (int i = 32; i <= 255; i++) // Omit control characters.
        result.push_back(static_cast<char>(i));
    return result;
}

Stroka TTrConverter::SlowConvert(const Stroka& input, int flags, docLanguage lang) {
    const ECharset charset = CharsetForLang(lang);
    const WStroka wInput = CharToWide(charset, input);
    const WStroka wResult = MapString(lang, flags, wInput);
    return WideToChar(charset, wResult);
}

TTrConverter::TTrConverter(docLanguage lang)
    : SymbolList(CreateSymbolList())
    , Uppercase(~SymbolList, ~SlowConvert(SymbolList, MAPSTR_UPPERCASE, lang))
    , Lowercase(~SymbolList, ~SlowConvert(SymbolList, MAPSTR_LOWERCASE, lang))
    , Dict(~SymbolList, ~SlowConvert(SymbolList, MAPSTR_DICT, lang))
    , RemoveDiacritics(~SymbolList, ~SlowConvert(SymbolList, MAPSTR_REMOVE_DIACRITICS, lang))
{}

Stroka TTrConverter::Convert(const Stroka& input, int flags) const {
    Stroka result(input);
    if (flags & MAPSTR_LOWERCASE)
        Lowercase.Do(result);
    if (flags & MAPSTR_UPPERCASE)
        Uppercase.Do(result);
    if (flags & MAPSTR_REMOVE_DIACRITICS)
        RemoveDiacritics.Do(result);
    else if (flags & MAPSTR_DICT)
        Dict.Do(result);
    return result;
}

static TTrConverter turConverter(LANG_TUR);

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
//
// MapString
//

Stroka MapString(docLanguage langID, int flags, const Stroka& str) {
    if (!TMapper::Instance)
        TMapper::Instance = new TMapper();

    if (langID == LANG_TUR || langID == LANG_AZE)
        return turConverter.Convert(str, flags);

    const CodePage* cp = CodePageForLang(langID);
    Stroka result(str);
    if (flags & MAPSTR_LOWERCASE)
        result.to_lower(0, Stroka::npos, *cp);
    if (flags & MAPSTR_UPPERCASE)
        result.to_upper(0, Stroka::npos, *cp);
    if (flags & MAPSTR_REMOVE_DIACRITICS) {
        TMapper::Instance->RemoveDiacritics(langID, result);
    }
    else if (flags & MAPSTR_DICT) {
        if (langID == LANG_RUS)
            TMapper::Instance->ReplaceYo(result);
    }
    if (EXPECT_FALSE(flags & MAPSTR_RENYXA))
        result = theRenyxer.Convert(ScriptByLanguage(langID), CharsetForLang(langID), result, 0);
    return result;
}

WStroka MapString(docLanguage langID, int flags, const WStroka& str) {
    if (!TMapper::Instance)
        TMapper::Instance = new TMapper();

    WStroka result(str);
    if (flags & MAPSTR_LOWERCASE) {
        if (langID == LANG_TUR || langID == LANG_AZE)
            ReplaceAll(result, 'I', wDotless_i);
        result.to_lower();
    }
    if (flags & MAPSTR_UPPERCASE) {
        if (langID == LANG_TUR || langID == LANG_AZE)
            ReplaceAll(result, 'i', wDotted_I);
        result.to_upper();
    }
    if (flags & MAPSTR_REMOVE_DIACRITICS) {
        TMapper::Instance->RemoveDiacritics(langID, result);
    }
    else if (flags & MAPSTR_DICT) {
        if (langID == LANG_RUS) {
            ReplaceAll(result, wCYR_io, wCYR_ie);
            ReplaceAll(result, wCYR_IO, wCYR_IE);
        }
        else if (langID == LANG_TUR || langID == LANG_AZE) {
            ReplaceAll(result, wCircumflex_A, 'A');
            ReplaceAll(result, wCircumflex_a, 'a');
            ReplaceAll(result, wCircumflex_I, wDotted_I);
            ReplaceAll(result, wCircumflex_i, 'i');
            ReplaceAll(result, wCircumflex_U, 'U');
            ReplaceAll(result, wCircumflex_u, 'u');
        }
    }
    if (EXPECT_FALSE(flags & MAPSTR_RENYXA))
        result = theRenyxer.Convert(ScriptByLanguage(langID), result, 0);
    if (EXPECT_FALSE(flags & MAPSTR_SIMPLIFIED_CHINESE))
        result = ToSimplified(result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// Renyxa
//

Stroka Renyxa(EScript script, ECharset charset, const Stroka& str, int flags) {
    return theRenyxer.Convert(script, charset, str, flags);
}

WStroka Renyxa(EScript script, const WStroka& str, int flags) {
    return theRenyxer.Convert(script, str, flags);
}
