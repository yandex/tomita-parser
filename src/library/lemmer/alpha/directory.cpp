#include "abc.h"

#include <library/token/charfilter.h>

#include <util/generic/singleton.h>
#include <util/charset/doccodes.h>

namespace NLemmer {
namespace NDetail {
    const NLemmer::TAlphabetWordNormalizer& AlphaDefault();
    const NLemmer::TAlphabetWordNormalizer& AlphaArmenian();
    const NLemmer::TAlphabetWordNormalizer& AlphaBasicRussian();
    const NLemmer::TAlphabetWordNormalizer& AlphaBelarusian();
    const NLemmer::TAlphabetWordNormalizer& AlphaBulgarian();
    const NLemmer::TAlphabetWordNormalizer& AlphaChurchSlavonic();
    const NLemmer::TAlphabetWordNormalizer& AlphaChinese();
    const NLemmer::TAlphabetWordNormalizer& AlphaCzech();
    const NLemmer::TAlphabetWordNormalizer& AlphaEnglish();
    const NLemmer::TAlphabetWordNormalizer& AlphaEstonian();
    const NLemmer::TAlphabetWordNormalizer& AlphaBashkir();
    const NLemmer::TAlphabetWordNormalizer& AlphaFinnish();
    const NLemmer::TAlphabetWordNormalizer& AlphaFrench();
    const NLemmer::TAlphabetWordNormalizer& AlphaGeorgian();
    const NLemmer::TAlphabetWordNormalizer& AlphaGerman();
    const NLemmer::TAlphabetWordNormalizer& AlphaGreek();
    const NLemmer::TAlphabetWordNormalizer& AlphaItalian();
    const NLemmer::TAlphabetWordNormalizer& AlphaJapanese();
    const NLemmer::TAlphabetWordNormalizer& AlphaKazakh();
    const NLemmer::TAlphabetWordNormalizer& AlphaKorean();
    const NLemmer::TAlphabetWordNormalizer& AlphaLatvian();
    const NLemmer::TAlphabetWordNormalizer& AlphaLithuanian();
    const NLemmer::TAlphabetWordNormalizer& AlphaPolish();
    const NLemmer::TAlphabetWordNormalizer& AlphaPortuguese();
    const NLemmer::TAlphabetWordNormalizer& AlphaRomanian();
    const NLemmer::TAlphabetWordNormalizer& AlphaRussian();
    const NLemmer::TAlphabetWordNormalizer& AlphaSpanish();
    const NLemmer::TAlphabetWordNormalizer& AlphaTatar();
    const NLemmer::TAlphabetWordNormalizer& AlphaTurkish();
    const NLemmer::TAlphabetWordNormalizer& AlphaUkrainian();

}
}

namespace NLemmer {
// Auxiliary: table for language filtering by alphabet
namespace {
    class TGuessTable {
    private:
        TLangMask widechartable[0x10000];

    public:
        TGuessTable() {
        }

        void AddSymbol(const TLangMask& mask, wchar32 wch) {
            widechartable[(ui16) wch] |= mask;
        }

        TLangMask GetWideCharInfo(wchar16 wch) const {
            return widechartable[(ui16)wch];
        }
    };

////////////////////////////////////////////////////////////
/////// TAlphaDirectoryImpl
///////////////////////////////////////////////////////////
    class TAlphaDirectoryImpl {
    private:
        // Index of all languages by ID
        const TAlphabetWordNormalizer* IndexById[LANG_MAX];
        // Alphabet-matching table
        TGuessTable AbcGuessTable;
        TGuessTable AbcGuessNormalAlpha;
        TGuessTable AbcGuessStrictAlpha;

    public:
        TAlphaDirectoryImpl() {
            memset(IndexById, 0, sizeof(IndexById));
            using namespace NLemmer::NDetail;

            Register(&AlphaDefault(), LANG_UNK);
            Register(&AlphaArmenian(), LANG_ARM);
            Register(&AlphaBasicRussian(), LANG_BASIC_RUS);
            Register(&AlphaBelarusian(), LANG_BLR);
            Register(&AlphaBulgarian(), LANG_BUL);
            Register(&AlphaChurchSlavonic(), LANG_CHU);
            Register(&AlphaChinese(), LANG_CHI);
            Register(&AlphaCzech(), LANG_CZE);
            Register(&AlphaEnglish(), LANG_BASIC_ENG);
            Register(&AlphaEnglish(), LANG_ENG);
            Register(&AlphaEstonian(), LANG_EST);
            Register(&AlphaBashkir(), LANG_BAK);
            Register(&AlphaFinnish(), LANG_FIN);
            Register(&AlphaFrench(), LANG_FRN);
            Register(&AlphaGeorgian(), LANG_GEO);
            Register(&AlphaGerman(), LANG_GER);
            Register(&AlphaGreek(), LANG_GRE);
            Register(&AlphaItalian(), LANG_ITA);
            Register(&AlphaJapanese(), LANG_JPN);
            Register(&AlphaKazakh(), LANG_KAZ);
            Register(&AlphaKorean(), LANG_KOR);
            Register(&AlphaLatvian(), LANG_LAV);
            Register(&AlphaLithuanian(), LANG_LIT);
            Register(&AlphaPolish(), LANG_POL);
            Register(&AlphaPortuguese(), LANG_POR);
            Register(&AlphaRomanian(), LANG_RUM);
            Register(&AlphaRussian(), LANG_RUS);
            Register(&AlphaSpanish(), LANG_SPA);
            Register(&AlphaTatar(), LANG_TAT);
            Register(&AlphaTurkish(), LANG_TUR);
            Register(&AlphaUkrainian(), LANG_UKR);
        }

        const TAlphabetWordNormalizer* GetById(docLanguage id) const {
            YASSERT(id < LANG_MAX);
            return IndexById[id];
        }

        TLangMask GetWideCharInfo(wchar32 wch) const {
            return AbcGuessTable.GetWideCharInfo(static_cast<wchar16> (wch));
        }

        TLangMask GetWideCharInfoStrict(wchar32 wch) const {
            return AbcGuessStrictAlpha.GetWideCharInfo(static_cast<wchar16> (wch));
        }

        TLangMask GetWideCharInfoNormal(wchar32 wch) const {
            return AbcGuessNormalAlpha.GetWideCharInfo(static_cast<wchar16> (wch));
        }

    private:
        void Register(const TAlphabetWordNormalizer* alphabet, docLanguage id) {
            Register(alphabet, TLangMask(id));
        }

        void Register(const TAlphabetWordNormalizer* alphabet, const TLangMask& mask) {
            YASSERT(alphabet);

            if (!mask.any()) {
                IndexById[LANG_UNK] = alphabet;
                return;
            }

            for (docLanguage lg = mask.FindFirst(); mask.IsValid(lg); lg = mask.FindNext(lg))
                IndexById[lg] = alphabet;

            for (TChar alpha = 65535; alpha > 0; --alpha) {
                TAlphabet::TCharClass c = alphabet->GetAlphabet()->CharClass(alpha);
                if (c) {
                    AbcGuessTable.AddSymbol(mask, alpha);
                    if (c & (TAlphabet::CharAlphaNormal | TAlphabet::CharAlphaNormalDec)
                        && ((c & TAlphabet::CharDia) == 0))
                    {
                        AbcGuessNormalAlpha.AddSymbol(mask, alpha);
                        if (c & TAlphabet::CharAlphaNormal)
                            AbcGuessStrictAlpha.AddSymbol(mask, alpha);
                    }
                }
            }
        }
    };
}

static TAlphaDirectoryImpl* GetImpl() {
    return Singleton<TAlphaDirectoryImpl>();
}

const TAlphabetWordNormalizer* GetAlphaRules(docLanguage id) {
    return GetImpl()->GetById(id);
}

TLangMask ClassifyLanguageAlpha(const TChar* word, size_t len, bool ignoreDigit) {
    const TAlphaDirectoryImpl* impl = GetImpl();
    TLangMask lang = ~TLangMask();
    TLangMask langNormal = TLangMask();
    bool any = false;
    for (size_t i = 0; i < len; i++) {
        if (ignoreDigit && IsDigit(word[i]))
            continue;
        lang &= impl->GetWideCharInfo(word[i]);
        langNormal |= impl->GetWideCharInfoNormal(word[i]);
        any = true;
    }
    if (!any)
        return TLangMask();
    // Don't apply Russian, Ukrainian, or Kazakh to renyxa words
    lang &= langNormal;

    return lang;
}

TLangMask GetCharInfoAlpha(TChar ch) {
    return GetImpl()->GetWideCharInfoStrict(ch);
}

TCharCategory ClassifyCase(const TChar *word, size_t len) {
    TCharCategory flags = CC_WHOLEMASK & (~(CC_COMPOUND|CC_DIFFERENT_ALPHABET|CC_HAS_DIACRITIC));
    const TChar * end = word + len;

    EScript prevScript = SCRIPT_UNKNOWN;
    for (const TChar* p = word; p < end; p++) {
        if (*p < 128)  //-- ASCII?
            flags &= ~CC_NONASCII;
        else
            flags &= ~CC_ASCII;

        if (IsLower(*p)) {  //-- LOWER CASE?
            flags &= ~(CC_UPPERCASE | CC_NUMBER);
            if (p == word)
                flags &= ~(CC_TITLECASE | CC_NUTOKEN);
        } else if (IsUpper(*p)) {  //-- UPPER CASE?
            flags &= ~(CC_LOWERCASE | CC_NUMBER);
            if (p == word)
                flags &= ~CC_NUTOKEN;
            else
                flags &= ~CC_TITLECASE;
        } else if (IsDigit(*p)) {  //-- DIGIT /* TREATED_AS_UPPERCASE */
            flags &= ~(CC_ALPHA | CC_LOWERCASE);
            if (p == word)
                flags &= ~CC_NMTOKEN;
            else
                flags &= ~CC_TITLECASE;
        } else if (IsAlphabetic(*p)) { //-- ALPHABET SYMBOL WITHOUT CASE
            flags &= ~(CC_NUMBER | CC_LOWERCASE | CC_UPPERCASE | CC_MIXEDCASE);
            if (p == word)
                flags &= ~(CC_NUTOKEN | CC_TITLECASE);
        } else if (!IsCombining(*p)) { //-- NON ALPHANUMERIC /* TREATED_AS_LOWERCASE */
            flags &= ~(CC_ALPHA | CC_NUMBER | CC_NUTOKEN | CC_NMTOKEN | CC_LOWERCASE);
            if (p == word)
                flags &= ~CC_TITLECASE;
        }

        EScript script = ScriptByGlyph(*p);
        if (script != SCRIPT_UNKNOWN) {  // ignoring digits and other(?) strange symbols
            if (prevScript == SCRIPT_UNKNOWN)
                prevScript = script;
            else
                if (prevScript != script)
                    flags |= CC_DIFFERENT_ALPHABET;
        }

        const wchar32* d = LemmerDecomposition(*p, false, true);
        if (d)
            flags |= CC_HAS_DIACRITIC;
    }

    //-- Single uppercase characters are treated as title case
    //-- Commented out: complicates treatment of compound tokens
    // if ((flags & CC_UPPERCASE) && (flags & CC_TITLECASE))
    //     flags &= ~CC_UPPERCASE;

    // Any specific case pattern turns mixed-case bit off
    if (flags & (CC_UPPERCASE | CC_LOWERCASE | CC_TITLECASE))
        flags &= ~CC_MIXEDCASE;

    return flags;
}

} // NLemmer
