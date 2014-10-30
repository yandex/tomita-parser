#include "abc.h"
#include "diacritics.h"
#include <library/token/charfilter.h>
#include <util/charset/wide.h>
#include <util/generic/singleton.h>


namespace {
    // Most of the constants and several classes used in current file
    // are declared in file 'abc_data.h'.  The file
    // 'abc_data.h' is generated using file 'build_abc_data.py'
    #include "generated/abc_data.h"

    using NLemmer::TAlphabet;

    void PushRange(TChar begin, TChar end, yvector<TChar>& destination) {
        for (TChar current = begin; current != end; ++current) {
            destination.push_back(current);
        }
    }

    // alphabets definitions (in alphabetical order)

    struct TAlphaArmenian: public TAlphabet {
        TAlphaArmenian()
            : TAlphabet(ARM_REQUIRED_ALPHA, ARM_NORMAL_ALPHA, 0, 0, ARM_SIGNS)
        {
        }
    };

    struct TAlphaBashkir: public TAlphabet {
        TAlphaBashkir()
            : TAlphabet(BAK_REQUIRED_ALPHA, BAK_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaBasicRussian: public TAlphabet {
        TAlphaBasicRussian()
            : TAlphabet(RUS_REQUIRED_ALPHA, RUS_BASIC_ALPHA, EASTERN_SLAVIC_RENYXA, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaBelarusian: public TAlphabet {
        TAlphaBelarusian()
            : TAlphabet(BEL_REQUIRED_ALPHA, BEL_NORMAL_ALPHA, EASTERN_SLAVIC_RENYXA, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaBulgarian: public TAlphabet {
        TAlphaBulgarian()
            : TAlphabet(BUL_REQUIRED_ALPHA, BUL_NORMAL_ALPHA, BUL_ALIEN_ALPHA, STRESS_BULGARIAN, APOST_AND_HYPH)
        {
        }
    };

    yvector<TChar> ChineseRequiredAlpha() {
        yvector<TChar> result;
        result.reserve(CHINESE_REQUIRED_ALPHA_SIZE);

        PushRange(CJK_UNIFIED_BEGIN, CJK_UNIFIED_END, result);
        PushRange(CJK_UNIFIED_EXT_A_BEGIN, CJK_UNIFIED_EXT_A_END, result);
        PushRange(CJK_COMPATIBILITY_BEGIN, CJK_COMPATIBILITY_END, result);
        PushRange(CJK_DESCRIPTION_BEGIN, CJK_DESCRIPTION_END, result);
        PushRange(BOPOMOFO_BEGIN, BOPOMOFO_END, result);

        result.push_back(0);

        return result;
    }

    struct TAlphaChinese: public TAlphabet {
        TAlphaChinese()
            : TAlphabet()
        {
            yvector<TChar> requiredAlpha(ChineseRequiredAlpha());
            Create(~requiredAlpha, 0, 0, 0, CJK_APOST_AND_HYPH);
        }
    };

    struct TAlphaChurchSlavonic: public TAlphabet {
        TAlphaChurchSlavonic()
            : TAlphabet(CHU_REQUIRED_ALPHA, CHU_NORMAL_ALPHA, 0, CHU_STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaCzech: public TAlphabet {
        TAlphaCzech()
            : TAlphabet(CZE_REQUIRED_ALPHA, CZE_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaEnglish: public TAlphabet {
        TAlphaEnglish()
            : TAlphabet(ENG_REQUIRED_ALPHA, ENG_NORMAL_ALPHA, 0, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaEstonian: public TAlphabet {
        TAlphaEstonian()
            : TAlphabet(EST_REQUIRED_ALPHA, EST_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaFinnish: public TAlphabet {
        TAlphaFinnish()
            : TAlphabet(FIN_REQUIRED_ALPHA, FIN_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaFrench: public TAlphabet {
        TAlphaFrench()
            : TAlphabet(FRA_REQUIRED_ALPHA, FRA_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaGeorgian: public TAlphabet {
        TAlphaGeorgian()
            : TAlphabet(GEO_REQUIRED_ALPHA, GEO_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaGerman: public TAlphabet {
        TAlphaGerman()
            : TAlphabet(GER_REQUIRED_ALPHA, GER_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaGreek: public TAlphabet {
        TAlphaGreek()
            : TAlphabet(GRE_REQUIRED_ALPHA, GRE_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaItalian: public TAlphabet {
        TAlphaItalian()
            : TAlphabet(ITA_REQUIRED_ALPHA, ITA_NORMAL_ALPHA, 0, CIRCUMFLEX, APOST_AND_HYPH)
        {
        }
    };

    yvector<TChar> JapaneseRequiredAlpha() {
        yvector<TChar> result;
        result.reserve(JAPANESE_REQUIRED_ALPHA_SIZE);

        PushRange(CJK_UNIFIED_BEGIN, CJK_UNIFIED_END, result);
        PushRange(CJK_UNIFIED_EXT_A_BEGIN, CJK_UNIFIED_EXT_A_END, result);
        PushRange(CJK_COMPATIBILITY_BEGIN, CJK_COMPATIBILITY_END, result);
        PushRange(CJK_DESCRIPTION_BEGIN, CJK_DESCRIPTION_END, result);
        PushRange(HIRAGANA_KATAKANA_BEGIN, HIRAGANA_KATAKANA_END, result);
        PushRange(HALFWIDTH_KATAKANA_BEGIN, HALFWIDTH_KATAKANA_END, result);

        result.push_back(0);

        return result;
    }

    struct TAlphaJapanese: public TAlphabet {
        TAlphaJapanese()
            : TAlphabet()
        {
            yvector<TChar> requiredAlpha(JapaneseRequiredAlpha());
            Create(~requiredAlpha, 0, 0, 0, CJK_APOST_AND_HYPH);
        }
    };

    struct TAlphaKazakh: public TAlphabet {
        TAlphaKazakh()
            : TAlphabet(KAZ_REQUIRED_ALPHA, KAZ_NORMAL_ALPHA, EASTERN_SLAVIC_RENYXA, STRESS, APOST_AND_HYPH)
        {
        }
    };

    yvector<TChar> KoreanRequiredAlpha() {
        yvector<TChar> result;
        result.reserve(KOREAN_REQUIRED_ALPHA_SIZE);

        PushRange(CJK_UNIFIED_BEGIN, CJK_UNIFIED_END, result);
        PushRange(CJK_UNIFIED_EXT_A_BEGIN, CJK_UNIFIED_EXT_A_END, result);
        PushRange(CJK_COMPATIBILITY_BEGIN, CJK_COMPATIBILITY_END, result);
        PushRange(CJK_DESCRIPTION_BEGIN, CJK_DESCRIPTION_END, result);
        PushRange(HANGUL_BEGIN, HANGUL_END, result);
        PushRange(HANGUL_EXT_A_BEGIN, HANGUL_EXT_A_END, result);
        PushRange(HANGUL_EXT_B_BEGIN, HANGUL_EXT_B_END, result);
        PushRange(HANGUL_COMPATIBILITY_BEGIN,
                  HANGUL_COMPATIBILITY_END,
                  result);
        PushRange(HANGUL_SYLLABLES_BEGIN, HANGUL_SYLLABLES_END, result);
        PushRange(HALFWIDTH_HANGUL_BEGIN, HALFWIDTH_HANGUL_END, result);

        result.push_back(0);

        return result;
    }

    struct TAlphaKorean : public TAlphabet {
        TAlphaKorean()
            : TAlphabet()
        {
            yvector<TChar> requiredAlpha(KoreanRequiredAlpha());
            Create(~requiredAlpha, 0, 0, 0, CJK_APOST_AND_HYPH);
        }
    };

    struct TAlphaLatvian: public TAlphabet {
        TAlphaLatvian()
            : TAlphabet(LAV_REQUIRED_ALPHA, LAV_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaLithuanian: public TAlphabet {
        TAlphaLithuanian()
            : TAlphabet(LIT_REQUIRED_ALPHA, LIT_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaPolish: public TAlphabet {
        TAlphaPolish()
            : TAlphabet(POL_REQUIRED_ALPHA, POL_NORMAL_ALPHA, 0, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaPortuguese: public TAlphabet {
        TAlphaPortuguese()
            : TAlphabet(POR_REQUIRED_ALPHA, POR_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaRomanian: public TAlphabet {
        TAlphaRomanian()
            : TAlphabet(RUM_REQUIRED_ALPHA, RUM_NORMAL_ALPHA, 0, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaRussian: public TAlphabet {
        TAlphaRussian()
            : TAlphabet(RUS_REQUIRED_ALPHA, RUS_NORMAL_ALPHA, EASTERN_SLAVIC_RENYXA, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaSpanish: public TAlphabet {
        TAlphaSpanish()
            : TAlphabet(SPA_REQUIRED_ALPHA, SPA_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaTatar: public TAlphabet {
        TAlphaTatar()
            : TAlphabet(TAT_REQUIRED_ALPHA, TAT_NORMAL_ALPHA, EASTERN_SLAVIC_RENYXA, STRESS, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaTurkish: public TAlphabet {
        TAlphaTurkish()
            : TAlphabet(TUR_REQUIRED_ALPHA, TUR_NORMAL_ALPHA, 0, 0, APOST_AND_HYPH)
        {
        }
    };

    struct TAlphaUkrainian: public TAlphabet {
        TAlphaUkrainian()
            : TAlphabet(UKR_REQUIRED_ALPHA, UKR_NORMAL_ALPHA, UKR_ALIEN_ALPHA, STRESS, APOST_AND_HYPH)
        {
        }
    };

} // namespace

namespace NLemmer {
    namespace NDetail {

        const TChar EMPTY_LIST[] = {0};

        struct TEmptyConverter: TTr {
            TEmptyConverter()
                : TTr(EMPTY_LIST, EMPTY_LIST, EMPTY_LIST)
            {}
        };
    }
}

/// Redefine the singleton values for specific classes
/** This is necessary for TAlphaStructConstructer class, so that no-effect
  * classes return NULL instead of a class instance.
  */
template<class T>
const T* SingletonDefaulter() {
    return Singleton<T>();
}

template<>
const NLemmer::NDetail::TEmptyConverter* SingletonDefaulter<NLemmer::NDetail::TEmptyConverter>() {
    return NULL;
}

namespace {
class TEmptyDiacriticsMap: public NLemmer::TDiacriticsMap {};
}

template<>
const TEmptyDiacriticsMap* SingletonDefaulter<TEmptyDiacriticsMap>() {
    return NULL;
}

namespace NLemmer {
namespace NDetail {
    template <class TAlpha,
              class TPreConverter,
              class TDerenyxer,
              class TConverter,
              class TPreLower = TEmptyConverter,
              class TPreUpper = TEmptyConverter,
              class TPreTitle = TEmptyConverter,
              class TDiaMap = TEmptyDiacriticsMap>
    struct TAlphaStructConstructer: public NLemmer::TAlphabetWordNormalizer {
        TAlphaStructConstructer()
            : TAlphabetWordNormalizer(
                SingletonDefaulter<TAlpha>(),
                SingletonDefaulter<TPreConverter>(),
                SingletonDefaulter<TDerenyxer>(),
                SingletonDefaulter<TConverter>(),
                SingletonDefaulter<TPreLower>(),
                SingletonDefaulter<TPreUpper>(),
                SingletonDefaulter<TPreTitle>(),
                SingletonDefaulter<TDiaMap>())
        {}
    };

    namespace {
        struct TDefaultConverter: public NLemmer::TAlphabetWordNormalizer {
            TDefaultConverter()
                : TAlphabetWordNormalizer(
                    NULL,
                    SingletonDefaulter<TDefaultPreConverter>(),
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL)
            {}
            bool AdvancedConvert(const TChar* word,
                                 size_t length,
                                 TChar* converted,
                                 size_t bufLen,
                                 NLemmer::TConvertRet& ret) const
            {
                ret.Length = NormalizeUnicode(word, length, converted, bufLen);
                return true;
            }
            bool AdvancedConvert(Wtroka& s, NLemmer::TConvertRet& ret) const {
                s = NormalizeUnicode(s);
                ret.Length = s.length();
                return true;
            }
        };
    }

    // alphabet normalizers definitions (in alphabetical order)

    const NLemmer::TAlphabetWordNormalizer& AlphaDefault() {
        return *Singleton<TDefaultConverter>();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaArmenian() {
        return *Singleton<TAlphaStructConstructer<TAlphaArmenian, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaBashkir() {
        return *Singleton<TAlphaStructConstructer<TAlphaBashkir, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaBasicRussian() {
        return *Singleton<TAlphaStructConstructer<TAlphaBasicRussian, TEasternSlavicPreConverter, TEasternSlavicDerenyxer, TGeneralCyrilicConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaBelarusian() {
        return *Singleton<TAlphaStructConstructer<TAlphaBelarusian, TEasternSlavicPreConverter, TEasternSlavicDerenyxer, TBelarusianConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaBulgarian() {
        return *Singleton<TAlphaStructConstructer<TAlphaBulgarian, TEasternSlavicPreConverter, TEasternSlavicDerenyxer, TGeneralCyrilicConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaChinese() {
        return *Singleton<TAlphaStructConstructer<TAlphaChinese, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaChurchSlavonic() {
        return *Singleton<TAlphaStructConstructer<TAlphaChurchSlavonic, TDefaultPreConverter, TEmptyConverter, TGeneralCyrilicConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaCzech() {
        return *Singleton<TAlphaStructConstructer<TAlphaCzech, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaEnglish() {
        return *Singleton<TAlphaStructConstructer<TAlphaEnglish, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaEstonian() {
        return *Singleton<TAlphaStructConstructer<TAlphaEstonian, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaFinnish() {
        return *Singleton<TAlphaStructConstructer<TAlphaFinnish, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaFrench() {
        return *Singleton<TAlphaStructConstructer<TAlphaFrench, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaGeorgian() {
        return *Singleton<TAlphaStructConstructer<TAlphaGeorgian, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaGerman() {
        return *Singleton<TAlphaStructConstructer<TAlphaGerman, TDefaultPreConverter, TEmptyConverter, TGermanConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaGreek() {
        return *Singleton<TAlphaStructConstructer<TAlphaGreek, TDefaultPreConverter, TEmptyConverter, TGermanConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaItalian() {
        return *Singleton<TAlphaStructConstructer<TAlphaItalian, TDefaultPreConverter, TEmptyConverter, TItalianConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaJapanese() {
        return *Singleton<TAlphaStructConstructer<TAlphaJapanese, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaKazakh() {
        return *Singleton<TAlphaStructConstructer<TAlphaKazakh, TEasternSlavicPreConverter, TEasternSlavicDerenyxer, TGeneralCyrilicConverter,
            TEmptyConverter, TEmptyConverter, TEmptyConverter, TKazakhDiacriticsMap> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaKorean() {
        return *Singleton<TAlphaStructConstructer<TAlphaKorean, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaLatvian() {
        return *Singleton<TAlphaStructConstructer<TAlphaLatvian, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaLithuanian() {
        return *Singleton<TAlphaStructConstructer<TAlphaLithuanian, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaPolish() {
        return *Singleton<TAlphaStructConstructer<TAlphaPolish, TDefaultPreConverter, TEmptyConverter, TApostropheConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaPortuguese() {
        return *Singleton<TAlphaStructConstructer<TAlphaPortuguese, TDefaultPreConverter, TEmptyConverter, TPortugueseConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaRomanian() {
        return *Singleton<TAlphaStructConstructer<TAlphaRomanian, TRomanianPreConverter, TRomanianDerenyxer, TApostropheConverter> >();
    }

    namespace {
        class TRussianWordNormalizer : public TAlphaStructConstructer<
            TAlphaRussian,
            TEasternSlavicPreConverter,
            TEasternSlavicDerenyxer,
            TGeneralCyrilicConverter,
            TEmptyConverter,
            TEmptyConverter,
            TEmptyConverter,
            TEmptyDiacriticsMap>
        {
        public:
            TRussianWordNormalizer()
                : TAlphaStructConstructer<TAlphaRussian,
                                          TEasternSlavicPreConverter,
                                          TEasternSlavicDerenyxer,
                                          TGeneralCyrilicConverter,
                                          TEmptyConverter,
                                          TEmptyConverter,
                                          TEmptyConverter,
                                          TEmptyDiacriticsMap>()
            { }

        private:
            template<class T>
            void AdvancedConvert_(T& word, NLemmer::TConvertRet& ret) const;
            bool AdvancedConvert(const TChar* word, size_t length, TChar* converted, size_t bufLen, NLemmer::TConvertRet& ret) const {
                if (length > bufLen) {
                    ret.Valid = false;
                    length = bufLen;
                }
                memcpy(converted, word, length * sizeof(TChar));
                ret.Length = length;
                AdvancedConvert_(converted, ret);
                return true;
            }
            bool AdvancedConvert(Wtroka& s, NLemmer::TConvertRet& ret) const {
                AdvancedConvert_(s, ret);
                if (s.length() > ret.Length)
                    s.resize(ret.Length);
                YASSERT(s.length() == ret.Length);
                return true;
            }
        };
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaRussian() {
        return *Singleton<TRussianWordNormalizer>();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaSpanish() {
        return *Singleton<TAlphaStructConstructer<TAlphaSpanish, TDefaultPreConverter, TEmptyConverter, TSpanishConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaTatar() {
        return *Singleton<TAlphaStructConstructer<TAlphaTatar, TEasternSlavicPreConverter, TEasternSlavicDerenyxer, TGeneralCyrilicConverter> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaTurkish() {
        return *Singleton<TAlphaStructConstructer<TAlphaTurkish, TTurkishPreConverter, TEmptyConverter, TTurkishConverter, TTurkishPreLower, TTurkishPreUpper, TTurkishPreUpper, TTurkishDiacriticsMap> >();
    }

    const NLemmer::TAlphabetWordNormalizer& AlphaUkrainian() {
        return *Singleton<TAlphaStructConstructer<TAlphaUkrainian, TEasternSlavicPreConverter, TEasternSlavicDerenyxer, TGeneralCyrilicConverter> >();
    }

    namespace {
    struct TRussianConvertConsts {
        const TChar letYat;
        const TChar letDecimalI;
        const TChar letHardSign;
        const TChar letI;
        const TChar letE;
        const TChar letM;
        const TChar letZ;
        const TChar letS;
        const Wtroka strOne;
        const Wtroka strOdne;
        const Wtroka strOdneh;
        const Wtroka strVozs;
        const Wtroka strNizs;
        const Wtroka strRazs;
        const Wtroka strRozs;
        const Wtroka strBe;
        const Wtroka strIz;
        const Wtroka strChrez;
        const Wtroka strCherezs;

        const Wtroka setVocals;
        const Wtroka setConsonants;
        const Wtroka setBreathConsonants;
        const Wtroka setPp;
        TRussianConvertConsts()
            : letYat(0x0463)   // e
            , letDecimalI(0x0456)   // i
            , letHardSign(0x044A)
            , letI(0x0438)
            , letE(0x0435)
            , letM(0x043C)
            , letZ(0x0437)
            , letS(0x0441)
            , strOne(CharToWide("он", CODES_UTF8) + CharToWide("\xB9")) // онѣ
            , strOdne(CharToWide("одн", CODES_UTF8) + CharToWide("\xB9")) // однѣ
            , strOdneh(CharToWide("одн", CODES_UTF8) + CharToWide("\xB9") + CharToWide("хъ", CODES_UTF8)) //однѣхъ
            , strVozs(CharToWide("возс", CODES_UTF8))
            , strNizs(CharToWide("низс", CODES_UTF8))
            , strRazs(CharToWide("разс", CODES_UTF8))
            , strRozs(CharToWide("розс", CODES_UTF8))
            , strBe(CharToWide("бе", CODES_UTF8))
            , strIz(CharToWide("из", CODES_UTF8))
            , strChrez(CharToWide("чрез", CODES_UTF8))
            , strCherezs(CharToWide("черезс", CODES_UTF8))

            , setVocals(CharToWide("аaеeиоoуyыэюяйё", CODES_UTF8)) //und semivocals
            , setConsonants(CharToWide("бвгджзклмнпрстфхцчшщ", CODES_UTF8))
            , setBreathConsonants(CharToWide("пткцчфсшщх", CODES_UTF8))
            , setPp(CharToWide("рp", CODES_UTF8))
        {};
    };
    }

    static bool isPrefix(const TChar* word, const Wtroka& pref) {
        return !TCharTraits<TChar>::Compare(word, pref.c_str(), pref.length());
    }

    static bool isPrefix(const Wtroka& word, const Wtroka& pref) {
        return !TCharTraits<TChar>::Compare(~word, pref.c_str(), pref.length());
    }

    static bool Eq(const Wtroka& pref, const TChar* word, size_t length) {
        return  pref.length() == length && !TCharTraits<TChar>::Compare(~pref, word, length);
    }

    static bool Eq(const Wtroka& s, const Wtroka& word, size_t) {
        return s == word;
    }

    static bool Contains(const Wtroka& s, TChar c, size_t = 0) {
        return s.find(c) < s.length();
    }

    static bool Contains(const TChar* s, TChar c, size_t length) {
        return TCharTraits<TChar>::Find(s, c, length);
    }

    void Replace(TChar* word, size_t pos, TChar c) {
        word[pos] = c;
    }

    void Replace(Wtroka& word, size_t pos, TChar c) {
        word.begin()[pos] = c;
    }

    template <typename T>
    void Replace(T& word, size_t pos, TChar c, NLemmer::TConvertRet& ret) {
        Replace(word, pos, c);
        ret.IsChanged.Set(NLemmer::CnvAdvancedConvert);
    }

    template<class T>
    void TRussianWordNormalizer::AdvancedConvert_(T& word, NLemmer::TConvertRet& ret) const {
        const TRussianConvertConsts* consts = Singleton<TRussianConvertConsts>();

        // fita-f, final er, etc.
        size_t len = ret.Length;

        for (size_t i = 0; i < len; ++i) {
            if (word[i] == consts->letYat) {                        // one,odne,odneh
                if (Eq(consts->strOne, word, len) || Eq(consts->strOdne, word, len) || Eq(consts->strOdneh, word, len)) {
                    Replace(word, i, consts->letI, ret);
                    break;
                } else {
                    Replace(word, i, consts->letE, ret);
                }
            }
            else if (word[i] == consts->letDecimalI && i < len - 1 &&       // Ucrainian single-dot
                        ((i > 0 && word[i - 1] == consts->letM && Contains(consts->setPp, word[i + 1])) ||  // мiръ
                         Contains(consts->setVocals, word[i + 1])))  // before vocal or semivocal
            {
                Replace(word, i, consts->letI, ret);
            }
        }
        //final (but not lonely) er
        if (len > 1 && word[len - 1] == consts->letHardSign && Contains(consts->setConsonants, word[len - 2]))
        {
            --len;
             ret.IsChanged.Set(NLemmer::CnvAdvancedConvert);
        }

        ret.Length = len;
        ret.Valid = ret.Valid && !Contains(word, consts->letDecimalI, ret.Length);

        if (ret.Length < 5)
            return;

        //prefixes - bes/bez, is/iz, etc.
        if (word[2] == consts->letZ) {
            if (isPrefix(word, consts->strVozs)    || isPrefix(word, consts->strNizs)
                || isPrefix(word, consts->strRazs) || isPrefix(word, consts->strRozs)
                || isPrefix(word, consts->strBe) && Contains(consts->setBreathConsonants, word[3]))
            {
                Replace(word, 2, consts->letS, ret);
            }
        } else if (word[2] == consts->letS) {
            if (isPrefix(word, consts->strIz)) {
                Replace(word, 1, consts->letS, ret);
            }
        } else if (ret.Length > 6){
            if (word[4] == consts->letZ){
                if (isPrefix(word, consts->strCherezs)) {
                    Replace(word, 4, consts->letS, ret);
                }
            } else if (word[4] == consts->letS) {
                if (isPrefix(word, consts->strChrez)) {
                    Replace(word, 3, consts->letS, ret);
                }
            }
        }
    }
} // NLemmer
} // NDetail
