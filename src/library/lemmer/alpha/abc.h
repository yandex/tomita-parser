#pragma once

#include "diacritics.h"
#include <util/generic/ptr.h>
#include <util/generic/stroka.h>
#include <library/token/enumbitset.h>
#include <library/token/token_flags.h>

namespace NLemmer {
namespace NDetail {
    class TTr;
}
}

namespace NLemmer {
    class TAlphabet;
    class TAlphabetWordNormalizer;

    const TAlphabetWordNormalizer* GetAlphaRules(docLanguage id);
    inline const TAlphabetWordNormalizer* GetAlphaRulesAnyway(docLanguage id) {
        const TAlphabetWordNormalizer* ret = GetAlphaRules(id);
        if (!ret)
            ret = GetAlphaRules(LANG_UNK);
        YASSERT(ret);
        return ret;
    }
    TLangMask ClassifyLanguageAlpha(const TChar* word, size_t len, bool ignoreDigit);
    TLangMask GetCharInfoAlpha(TChar ch);
    TCharCategory ClassifyCase(const TChar *word, size_t len);

    enum EConvert {
        CnvDecompose,
        CnvCompose,
        CnvLowCase,
        CnvDerenyx,
        CnvConvert,
        CnvAdvancedConvert,
        CnvCheckCharacters,
        CnvMax,
        CnvUpCase = CnvMax,
        CnvTitleCase,
        CnvSoftDecompose,
        CnvSoftCompose,
        CnvMaxChanged
    };
    typedef TEnumBitSet<EConvert, CnvDecompose, CnvMax> TConvertMode;

    struct TConvertRet {
        bool Valid;
        size_t Length;
        TEnumBitSet<EConvert, CnvDecompose, CnvMaxChanged> IsChanged;

        explicit TConvertRet(size_t len, bool valid = true);
    };

    class TAlphabetWordNormalizer {
    private:
        class THelper;
        friend class THelper;
    private:
        const TAlphabet* Alphabet;
        // composition of prelower, lower and derenyx
        const NLemmer::NDetail::TTr* PreConverter;
        const NLemmer::NDetail::TTr* Derenyxer;
        const NLemmer::NDetail::TTr* Converter;
        const NLemmer::NDetail::TTr* PreLowerCaser;
        const NLemmer::NDetail::TTr* PreUpperCaser;
        const NLemmer::NDetail::TTr* PreTitleCaser;
        const NLemmer::TDiacriticsMap* DiacriticsMap;

    public:
        TAlphabetWordNormalizer(
                const TAlphabet* alphabet,
                const NLemmer::NDetail::TTr* preConverter,
                const NLemmer::NDetail::TTr* derenyxer,
                const NLemmer::NDetail::TTr* converter,
                const NLemmer::NDetail::TTr* preLowerCaser,
                const NLemmer::NDetail::TTr* preUpperCaser,
                const NLemmer::NDetail::TTr* preTitleCaser,
                const NLemmer::TDiacriticsMap* diacriticsMap = NULL);

        TConvertRet Normalize(const TChar* word, size_t length, TChar* converted, size_t bufLen, TConvertMode mode = ~TConvertMode()) const;
        TConvertRet Normalize(Wtroka& s, TConvertMode mode = ~TConvertMode()) const;

        TConvertRet PreConvert(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;

        TConvertRet ToLower(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet ToLower(Wtroka& s) const;
        TChar ToLower(TChar a) const;

        TConvertRet ToUpper(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet ToUpper(Wtroka& s) const;
        TChar ToUpper(TChar a) const;

        TConvertRet ToTitle(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet ToTitle(Wtroka& s) const;
        TChar ToTitle(TChar a) const;

        TConvertRet Decompose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet Decompose(Wtroka& s) const;

        TConvertRet Compose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet Compose(Wtroka& s) const;

        TConvertRet SoftDecompose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet SoftDecompose(Wtroka& s) const;

        TConvertRet SoftCompose(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet SoftCompose(Wtroka& s) const;

        TConvertRet Derenyx(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet Derenyx(Wtroka& s) const;

        TConvertRet Convert(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet Convert(Wtroka& s) const;

        TConvertRet AdvancedConvert(const TChar* word, size_t length, TChar* converted, size_t bufLen) const;
        TConvertRet AdvancedConvert(Wtroka& s) const;

        const NLemmer::TDiacriticsMap * GetDiacriticsMap() const {
            return DiacriticsMap;
        }

        virtual bool IsNormalized(const TChar* word, size_t length) const;
        bool IsNormalized(const Wtroka& s) const {
            return IsNormalized(~s, +s);
        }

        const TAlphabet* GetAlphabet() const {
            return Alphabet;
        }

        static const TConvertMode& ModePreConvert() {
            static const TConvertMode mode(CnvCompose, CnvDerenyx);
            return mode;
        }

        static const TConvertMode& ModeConvertNormalized() {
            static const TConvertMode mode(CnvConvert, CnvAdvancedConvert, CnvCheckCharacters);
            return mode;
        }

        virtual ~TAlphabetWordNormalizer() {};
    protected:
        virtual bool AdvancedConvert(const TChar* word, size_t length, TChar* converted, size_t bufLen, TConvertRet& ret) const;
        virtual bool AdvancedConvert(Wtroka& s, TConvertRet& ret) const;
    };

    class TAlphabet {
    public:
        typedef unsigned char TCharClass;
        // + CharAlphaNormal - гласная (для бастардов)
        static const TCharClass CharAlphaRequired   = 1;
        // обычная буква
        static const TCharClass CharAlphaNormal     = 2;
        // буква получается в результате декомпозиции обычной буквы, но сама по себе в алфавит не входит
        static const TCharClass CharAlphaNormalDec  = 4;
        // то же самое для ренухи
        static const TCharClass CharAlphaAlien      = 8;
        static const TCharClass CharAlphaAlienDec   = 16;
        // диакритика, по умолчанию - несущественная (ударение в русском)
        static const TCharClass CharDia             = 32;
        // + CharDia диакритика существенная (умляут в немецком)
        static const TCharClass CharDiaSignificant  = 64;
        // дефисы, апострофы
        static const TCharClass CharSign            = 128;

    private:
        STATIC_ASSERT(sizeof(TChar) == 2);
        static const size_t MapSize = 65536;
        TCharClass Map[MapSize];
    public:
        class TComposer;
        THolder<TComposer> Composer;
    public:
        TAlphabet();
        TAlphabet(
            const TChar* alphaRequired,
            const TChar* alphaNormal,
            const TChar* alphaAlien,
            const TChar* diaAccidental,
            const TChar* signs
        );
        ~TAlphabet();

        TCharClass CharClass (TChar c) const {
            return Map[c];
        }

        bool IsSignificant(TChar c) const {
            return (CharClass(c) & (CharDia | CharDiaSignificant)) != CharDia;
        }

    protected:
        void Create(
            const TChar* alphaRequired,
            const TChar* alphaNormal,
            const TChar* alphaAlien,
            const TChar* diaAccidental,
            const TChar* signs);
    private:
        void AddAlphabet(const TChar* arr, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed);
        void AddDecomposedChain(const wchar32* chain, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed);
    };

namespace NDetail {
    struct TTransdRet {
        size_t Length;
        size_t Processed;
        bool Valid;
        bool Changed;

        TTransdRet()
        {}

        TTransdRet(size_t len, size_t proc, bool val, bool changed = true)
        : Length(len)
        , Processed(proc)
        , Valid(val)
        , Changed(changed)
        {}
    };

    TTransdRet ComposeWord(const TAlphabet& alphabet, const TChar* word, size_t length, TChar* buffer, size_t bufSize);
    TTransdRet ComposeAlpha(const TAlphabet& alphabet, const TChar* word, size_t length, TChar* buffer, size_t bufSize);

    class TTr {
        typedef wchar16 TChr;
        static const size_t MapSize = 65536;
        static const TChr NeedAdv = 0xFFFF;

        TChr Map[MapSize];
        struct TAdvancedMap;
        const size_t AdvancedMapSize;
        THolder<TAdvancedMap, TDeleteArray> AdvancedMap;

    public:
        TTr(const TChr *from, const TChr *to, const TChr *kill, const TChr* fromAdv = 0, const TChr** toAdv = 0);
        size_t operator() (const TChr *src, TChr *dst) const;
        TTransdRet operator() (const TChr *src, size_t len, TChr *dst, size_t bufLen) const;

        TTransdRet operator()(TChr *s, size_t l) const {
            return (*this)(s, l, s, l);
        }
        size_t operator () (TChr *s) const {
            return (*this)(s, s);
        }
        ~TTr();
    private:
        const TChr* GetAdvDec(TChr c) const;
    };
} // NDetail
} // NLemmer
