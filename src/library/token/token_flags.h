#pragma once

#include <util/charset/doccodes.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>

#include "enumbitset.h"

typedef TSfEnumBitSet<docLanguage, static_cast<docLanguage>(LANG_UNK + 1), LANG_MAX> TLangMask;

// Useful language sets
namespace NLanguageMasks {
    namespace NPrivate {
        class TScriptMap: public yhash<EScript, TLangMask> {
        public:
            TScriptMap() {
                for (size_t i = 0; i != LANG_MAX; ++i) {
                    docLanguage language = static_cast<docLanguage>(i);
                    if (!UnknownLanguage(language))
                        (*this)[ScriptByLanguage(language)].SafeSet(language);
                }
            }
        };
    } // namespace NPrivate

    typedef yhash<EScript, TLangMask> TScriptMap;
    inline const TScriptMap& ScriptMap() {
        return *Singleton<NPrivate::TScriptMap>();
    }

    inline const TLangMask& BasicLanguages() {
        const static TLangMask ret(LANG_ENG, LANG_RUS, LANG_UKR);
        return ret;
    }
    inline const TLangMask& DefaultRequestLanguages() {
        const static TLangMask ret = BasicLanguages() | TLangMask(LANG_KAZ, LANG_BLR, LANG_TAT);
        return ret;
    }
    inline const TLangMask& AllLanguages() {
        const static TLangMask ret = ~TLangMask() & ~TLangMask(LANG_BASIC_ENG, LANG_BASIC_RUS);
        return ret;
    }
    inline const TLangMask& CyrillicLanguages() {
        const static TLangMask ret = TLangMask(LANG_RUS, LANG_UKR, LANG_BLR);
        return ret;
    }
    inline const TLangMask& CyrillicLanguagesExt() {
        return ScriptMap().find(SCRIPT_CYRILLIC)->second;
    }
    inline const TLangMask& LatinLanguages() {
        return ScriptMap().find(SCRIPT_LATIN)->second;
    }
    inline const TLangMask& LemmasInIndex() {
        const static TLangMask ret = TLangMask(LANG_RUS, LANG_ENG, LANG_UKR, LANG_TUR) |
            TLangMask(LANG_BASIC_RUS, LANG_BASIC_ENG);
        return ret;
    }
    inline const TLangMask& NoBastardsInSearch() {
        const static TLangMask ret = ~LemmasInIndex();
        return ret;
    }

    TLangMask SameScriptLanguages(TLangMask mask);

    inline TLangMask RestrictLangMaskWithSameScripts(const TLangMask& mask, const TLangMask& by) {
        return mask & ~SameScriptLanguages(by);
    }

    inline const TLangMask& SameScriptLanguages(EScript scr) {
        static const TLangMask empty;
        TScriptMap::const_iterator it = ScriptMap().find(scr);
        return ScriptMap().end() == it ? empty : it->second;
    }

    inline TLangMask OtherSameScriptLanguages(const TLangMask& mask) {
        return ~mask & SameScriptLanguages(mask);
    }

    //List is string with list of languages names splinted by ','.
    TLangMask CreateFromList(const Stroka& list); // throws exception on unknown name
    TLangMask SafeCreateFromList(const Stroka& list); // ignore unknown names

    Stroka ToString(const TLangMask& langMask);

} // NLanguageMasks

#define LI_BASIC_LANGUAGES              NLanguageMasks::BasicLanguages()
#define LI_DEFAULT_REQUEST_LANGUAGES    NLanguageMasks::DefaultRequestLanguages()
#define LI_ALL_LANGUAGES                NLanguageMasks::AllLanguages()
#define LI_CYRILLIC_LANGUAGES           NLanguageMasks::CyrillicLanguages()
#define LI_CYRILLIC_LANGUAGES_EXT       NLanguageMasks::CyrillicLanguagesExt()
#define LI_LATIN_LANGUAGES              NLanguageMasks::LatinLanguages()

// Casing and composition of a word. Used in bitwise unions.
typedef long TCharCategory;
const TCharCategory CC_EMPTY       = 0x0000;
const TCharCategory CC_ALPHA       = 0x0001;
const TCharCategory CC_NMTOKEN     = 0x0002;
const TCharCategory CC_NUMBER      = 0x0004;
const TCharCategory CC_NUTOKEN     = 0x0008;
// Beware: CC_ASCII .. CC_TITLECASE shall occupy bits 4 to 6. Don't move them.
const TCharCategory CC_ASCII       = 0x0010;
const TCharCategory CC_NONASCII    = 0x0020;
const TCharCategory CC_TITLECASE   = 0x0040;
const TCharCategory CC_UPPERCASE   = 0x0080;
const TCharCategory CC_LOWERCASE   = 0x0100;
const TCharCategory CC_MIXEDCASE   = 0x0200;
const TCharCategory CC_COMPOUND    = 0x0400;
const TCharCategory CC_HAS_DIACRITIC    = 0x0800;
const TCharCategory CC_DIFFERENT_ALPHABET    = 0x1000;

const TCharCategory CC_WHOLEMASK   = 0x1FFF;

struct TOldLanguageEncoder {
    typedef long TLanguageId;

private:
    typedef yvector<TLanguageId> TD2LBase;

    class TD2L: public TD2LBase {
    public:
        TD2L()
            : TD2LBase(LANG_MAX, 0)
        {
            static const TLanguageId LI_UNKNOWN            = 0x00000000;  // special code - shall be zero
            static const TLanguageId LI_ENGLISH            = 0x00000001;
            static const TLanguageId LI_RUSSIAN            = 0x00000002;
            static const TLanguageId LI_POLISH             = 0x00000004;
            static const TLanguageId LI_UKRAINIAN          = 0x00000008;
            static const TLanguageId LI_GERMAN             = 0x00000010;
            static const TLanguageId LI_FRENCH             = 0x00000020;
            // Beware: a hole should be left at 0x40 - 0x80,
            // to prevent overlap with CC_UPPERCASE / CC_TITLECASE
            static const TLanguageId LI_HUNGARIAN          = 0x00000100;
//            static const TLanguageId LI_UKRAINIAN_ABBYY    = 0x00000200;
            static const TLanguageId LI_ITALIAN            = 0x00000400;
            static const TLanguageId LI_BELORUSSIAN        = 0x00000800;
            static const TLanguageId LI_KAZAKH             = 0x00008000;

            (*this)[LANG_UNK] = LI_UNKNOWN;
            (*this)[LANG_ENG] = LI_ENGLISH;
            (*this)[LANG_RUS] = LI_RUSSIAN;
            (*this)[LANG_POL] = LI_POLISH;
            (*this)[LANG_UKR] = LI_UKRAINIAN;
            (*this)[LANG_GER] = LI_GERMAN;
            (*this)[LANG_FRN] = LI_FRENCH;
            (*this)[LANG_HUN] = LI_HUNGARIAN;
//            (*this)[] = LI_UKRAINIAN_ABBYY;
            (*this)[LANG_ITA] = LI_ITALIAN;
            (*this)[LANG_BLR] = LI_BELORUSSIAN;
            (*this)[LANG_KAZ] = LI_KAZAKH;
        }
    };

    class TL2D: public ymap<TLanguageId, docLanguage> {
    public:
        TL2D() {
            const TD2L& v = GetD2L();
            for (size_t i = v.size(); i > 0; --i)
                (*this)[v[i - 1]] = static_cast<docLanguage>(i - 1);
            YASSERT(this->find(0)->second == 0);
        }
    };

private:
    static const TD2L& GetD2L() {
        return *Singleton<TD2L>();
    }

    static const TL2D& GetL2D() {
        return *Singleton<TL2D>();
    }

public:
    static TLanguageId ToOld(docLanguage l) {
        if (size_t(l) >= GetD2L().size())
            l = LANG_UNK;
        return GetD2L()[l];
    }

    static docLanguage FromOld1(TLanguageId l) {
        TL2D::const_iterator it = GetL2D().find(l);
        if (it == GetL2D().end())
            return LANG_UNK;
        return it->second;
    }

    static TLanguageId ToOld(const TLangMask& lm) {
        TLanguageId ret = 0;
        for (size_t i = TLangMask::Begin; i < TLangMask::End; ++i) {
            if (lm.Test(static_cast<docLanguage> (i))) {
                TLanguageId id = ToOld(static_cast<docLanguage> (i));
                ret |= id;
            }
        }
        return ret;
    }

    static TLangMask FromOld(TLanguageId lm) {
        static const TLanguageId allLangMask = TLanguageId(-1) & ~(0x40 | 0x80);
        static const size_t numBits = sizeof(TLanguageId) * CHAR_BIT;
        TLangMask ret;
        lm &= allLangMask;
        for (size_t i = 1; i < numBits; ++i) {
            TLanguageId id = TLanguageId(1) << (i - 1);
            if (lm & id)
                ret.SafeSet(FromOld1(id));
        }
        return ret;
    }
};
