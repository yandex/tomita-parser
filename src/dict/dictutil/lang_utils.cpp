#include "dictutil.h"
#include "exceptions.h"
#include "lang_ids.h"
#include "lang_map.h"
#include <util/charset/codepage.h>

const char* LangCode(docLanguage langID, const char* unknownValue) {
    CHECK_ARG(langID != LANG_MAX, "langID");
    if (langID == LANG_UNK)
        return unknownValue;
    return IsoNameByLanguage(langID);
}

docLanguage LanguageByName(const WStroka& name) {
    Stroka nameA = WideToChar(CODES_ASCII, name);
    return LanguageByName(nameA);
}

#define CHARSET_(NAME) (1 + CODES_##NAME)

static const TLangMap<int>::TValue charsets[] = {
    { LANG_ALB, CHARSET_(ASCII)        },
    { LANG_ARA, CHARSET_(WINDOWS_1256) },
    { LANG_ARM, CHARSET_(ARMSCII)      },
    { LANG_AZE, CHARSET_(WINDOWS_1254) },
    { LANG_BLR, CHARSET_(YANDEX)       },
    { LANG_BOS, CHARSET_(WIN_EAST)     },
    { LANG_CAT, CHARSET_(ASCII)        },
    { LANG_CZE, CHARSET_(WIN_EAST)     },
    { LANG_DAN, CHARSET_(ASCII)        },
    { LANG_DUT, CHARSET_(ASCII)        },
    { LANG_ENG, CHARSET_(ASCII)        },
    { LANG_EST, CHARSET_(WINDOWS_1257) },
    { LANG_FIN, CHARSET_(ASCII)        },
    { LANG_FRN, CHARSET_(ASCII)        },
    { LANG_GEO, CHARSET_(GEO_ITA)      },
    { LANG_GER, CHARSET_(ASCII)        },
    { LANG_GRE, CHARSET_(WINDOWS_1253) },
    { LANG_HEB, CHARSET_(WINDOWS_1255) },
    { LANG_HRV, CHARSET_(WIN_EAST)     },
    { LANG_HUN, CHARSET_(WIN_EAST)     },
    { LANG_IND, CHARSET_(ASCII)        },
    { LANG_ISL, CHARSET_(ASCII)        },
    { LANG_ITA, CHARSET_(ASCII)        },
    { LANG_KAZ, CHARSET_(KAZWIN)       },
    { LANG_LAT, CHARSET_(ASCII)        },
    { LANG_LAV, CHARSET_(WINDOWS_1257) },
    { LANG_LIT, CHARSET_(WINDOWS_1257) },
    { LANG_MAC, CHARSET_(WIN)          },
    { LANG_MLT, CHARSET_(ISO_8859_3)   },
    { LANG_MSA, CHARSET_(ASCII)        },
    { LANG_NOR, CHARSET_(ASCII)        },
    { LANG_POL, CHARSET_(WIN_EAST)     },
    { LANG_POR, CHARSET_(ASCII)        },
    { LANG_RUM, CHARSET_(WIN_EAST)     },
    { LANG_RUS, CHARSET_(YANDEX)       },
    { LANG_SLO, CHARSET_(WIN_EAST)     },
    { LANG_SLV, CHARSET_(WIN_EAST)     },
    { LANG_SPA, CHARSET_(ASCII)        },
    { LANG_SRP, CHARSET_(WIN)          },
    { LANG_SWE, CHARSET_(ASCII)        },
    { LANG_TAT, CHARSET_(TATWIN)       },
    { LANG_TUR, CHARSET_(WINDOWS_1254) },
    { LANG_UKR, CHARSET_(YANDEX)       },
    { LANG_VIE, CHARSET_(CP1258)       },
};
static TLangMap<int> charsetsMap(charsets, ARRAY_SIZE(charsets));

ECharset CharsetForLang(docLanguage langID) {
    CHECK_ARG(langID != LANG_MAX, "langID");
    if (!charsetsMap.Ready)
        new (&charsetsMap) TLangMap<int>(charsets, ARRAY_SIZE(charsets));

    ECharset charset = (ECharset)(charsetsMap[langID] - 1);
    if (charset == CODES_UNKNOWN)
        charset = CODES_YANDEX;
    return charset;
}

const CodePage* CodePageForLang(docLanguage langID) {
    return CodePageByCharset(CharsetForLang(langID));
}

typedef TLangMap<const char*> TLangToStrMap;
static const TLangToStrMap::TValue ctryCodes[] = {
    { LANG_CZE, "CZ" },
    { LANG_ENG, "US" },
    { LANG_HUN, "HU" },
    { LANG_POL, "PL" },
    { LANG_RUS, "RU" },
    { LANG_TUR, "TR" },
    { LANG_UKR, "UA" },
};
static const TLangToStrMap ctryCodesMap(ctryCodes, ARRAY_SIZE(ctryCodes));

const char* CountryCode(docLanguage langID) {
    const char* ctry = ctryCodesMap[langID];
    if (!ctry)
        return "";
    return ctry;
}

static const TLangToStrMap::TValue yandexKbdCodes[] = {
    { LANG_BLR, "BY" },
    { LANG_CZE, "CZ" },
    { LANG_ENG, "EN" },
    { LANG_FRN, "FR" },
    { LANG_HUN, "HU" },
    { LANG_KAZ, "KZ" },
    { LANG_POL, "PL" },
    { LANG_RUS, "RU" },
    { LANG_TUR, "TR" },
    { LANG_UKR, "UA" },
};
static const TLangToStrMap yandexKbdCodesMap(yandexKbdCodes, ARRAY_SIZE(yandexKbdCodes));

const char* YandexKeyboardCode(docLanguage langID) {
    const char* code = yandexKbdCodesMap[langID];
    if (!code)
        return "";
    return code;
}

TScriptMask ScriptMaskForLangs(const TLangIDs& langIDs) {
    TScriptMask scriptMask(0);
    for (docLanguage lang = LANG_UNK; (lang = langIDs.FindNext(lang)) != LANG_MAX;)
        scriptMask |= ScriptMaskForLang(lang);
    return scriptMask;
}

// values calculated by test lang_utils_ut.cpp
static const TLangIDs arabicLangIDs(LANG_ARA, LANG_PER);
static const TLangIDs armenianLangIDs(LANG_ARM);
static const TLangIDs cyrLangIDs = TLangIDs::FromLong(0x20bc82110200722ULL);
static const TLangIDs georgianLangIDs(LANG_GEO);
static const TLangIDs greekLangIDs(LANG_GRE);
static const TLangIDs hanLangIDs(LANG_CHI);
static const TLangIDs hangulLangIDs(LANG_KOR);
static const TLangIDs hebrewLangIDs(LANG_HEB);
static const TLangIDs hiraganaLangIDs(LANG_JPN);
static const TLangIDs latLangIDs = TLangIDs::FromLong(0xb0343796a9dfb8dcULL);
static const TLangIDs thaiLangIDs = TLangIDs(LANG_THA);

const TLangIDs& LangIDsForScript(EScript script) {
    switch (script) {
        case SCRIPT_ARABIC: return arabicLangIDs;
        case SCRIPT_ARMENIAN: return armenianLangIDs;
        case SCRIPT_CYRILLIC: return cyrLangIDs;
        case SCRIPT_GEORGIAN: return georgianLangIDs;
        case SCRIPT_GREEK: return greekLangIDs;
        case SCRIPT_HAN: return hanLangIDs;
        case SCRIPT_HANGUL: return hangulLangIDs;
        case SCRIPT_HEBREW: return hebrewLangIDs;
        case SCRIPT_HIRAGANA: return hiraganaLangIDs;
        case SCRIPT_LATIN: return latLangIDs;
        case SCRIPT_THAI: return thaiLangIDs;
        default: return TLangIDs::Empty;
    }
}
