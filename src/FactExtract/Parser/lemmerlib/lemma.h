#pragma once

#include <util/system/defaults.h>
#include <util/charset/doccodes.h>
#include <util/system/dynlib.h>
#include <util/system/maxlen.h>
#include <library/token/token_flags.h>

#include <bindings/c/mystem/iface.h>

class TSimpleLemma;

const size_t MAX_GRAM_PER_WORD = 36;

namespace NLemmer {
    enum EAccept {
        AccFromEnglish = -2,
        AccTranslit    = -1,
        AccDictionary  = 0,
        AccSob,
        AccBastard,
        AccFoundling,
        AccInvalid
    };

    typedef TEnumBitSet<EAccept, AccFromEnglish, AccInvalid> TLanguageQualityAccept;

    struct TAnalyzeWordOpt {
        TAnalyzeWordOpt(const char* neededGramm = "") {
            UNUSED(neededGramm);
        }
    };
}

namespace NLemmerAux {
    inline NLemmer::TLanguageQualityAccept QualitySetByMaxLevel(NLemmer::EAccept accept) {
        using namespace NLemmer;
        TLanguageQualityAccept ret;
        switch (accept) {
        default: // AccTranslit etc
        case AccFoundling:
            ret.Set(AccFoundling);
        case AccBastard:
            ret.Set(AccBastard);
        case AccSob:
            ret.Set(AccSob);
        case AccDictionary:
            ret.Set(AccDictionary);
        }
        return ret;
    }
}

class TSimpleLemma;

class TSimpleWordform {
public:
    Wtroka w;
    const char* StemGram;
    yvector<const char*> FlexGram;

public:
    const Wtroka& GetText() const {
        return w;
    }

    const char* GetStemGram() const {
        return StemGram;
    }

    const char* const* GetFlexGram() const {
        return (FlexGram.size() > 0) ? &FlexGram[0] : NULL;
    }

    size_t FlexGramNum() const {
        return FlexGram.size();
    }
};

typedef yvector<TSimpleWordform> TWordformArray;

namespace NLemmer {
    class TSimpleLemmer;
    class TFormGenerator;
    class TGrammarFiltr;
}

namespace NLemmerAux {
    class TSimpleLemmaSetter;
}

class TSimpleLemma {
    TChar LemmaText[MAXWORD_BUF];               // текст леммы (для пользователя)
    size_t LemmaLen;                            // длина леммы (strlen(LemmaText))
    size_t SuffixLen;                           // длина суффикса ("+" в "европа+")
    ui32 RuleId;                                // внутренний идентификатор типа парадигмы (почти никому не нужно)
    ui32 Quality;                               // см. TQuality
    const char *StemGram;                       // указатель на грамматику при основе (не зависящую от формы)
    const char *FlexGram[MAX_GRAM_PER_WORD];    // грамматики формы
    size_t GramNum;                             // размер массива грамматик формы
    size_t FlexLen;                             // длина окончания
     TChar FormNormalized[MAXWORD_BUF];         // форма, приведённая к каноническому виду (диакритика, реникса...)
     size_t FormNormalizedLen;
     Wtroka SourceText;                         // исходная строка, по которой найдена лемма

public:
    friend class NLemmer::TSimpleLemmer;
    friend class NLemmerAux::TSimpleLemmaSetter;

    enum TQuality {
        QDictionary  = 0x00000000, // слово из словаря
        QBastard     = 0x00000001, // не словарное
        QSob         = 0x00000002, // из "быстрого словаря"
        QPrefixoid   = 0x00000004, // словарное + стандартный префикс (авто- мото- кино- фото-) всегда в компании с QBastard или QSob
        QFoundling   = 0x00000008, // непонятный набор букв, но проходящий в алфавит
        QBadRequest  = 0x00000010, // доп. флаг.: "плохая лемма" при наличии "хорошей" альтернативы ("махать" по форме "маша")
        QFromEnglish = 0x00010000, // переведено с английского
        QToEnglish   = 0x00020000, // переведено на английский
        QUntranslit  = 0x00040000, // "переведено" с транслита
        QOverrode    = 0x00100000, // текст леммы был перезаписан
        QFix         = 0x01000000, // слово из фикс-листа
    };
    static const ui32 QAnyBastard = QBastard | QSob | QFoundling;

    TSimpleLemma()
        : RuleId(0), FlexLen(0) {
        LemmaText[0] = 0;
        FormNormalized[0] = 0;
    }

    const TChar* GetText() const {
        return LemmaText;
    }

    size_t GetTextLength() const {
        return LemmaLen;
    }

    size_t FlexGramNum() const {
        return GramNum;
    }

    const char* const* GetFlexGram() const {
        return FlexGram;
    }

    ui32 GetRuleId() const {
        return RuleId;
    }

    size_t GetFlexLen() const {
        return FlexLen;
    }

    size_t GetSuffixLength() const {
        return 0;
    }

    size_t GetPrefLen() const {
        return 0;
    }

    const char* GetStemGram() const {
        return StemGram;
    }

    const TChar* GetForma() const {
        return FormNormalized;
    }

    size_t GetFormLength() const {
        return FormNormalizedLen;
    }

    const Wtroka& GetSourceText() const {
        return SourceText;
    }

    docLanguage GetLanguage() const {
        return LANG_RUS;
    }

    bool IsBastard() const {
        return (GetQuality() & QAnyBastard) != 0;
    }

    ui32 GetQuality() const {
        return Quality;
    }

    TAutoPtr<NLemmer::TFormGenerator> Generator(const NLemmer::TGrammarFiltr* filter = NULL) const;
};

typedef TSimpleLemma TYandexLemma;
typedef TSimpleWordform TYandexWordform;

namespace NLemmerAux {
class TSimpleLemmaSetter {
private:
    const TSimpleLemmaSetter& operator=(const TSimpleLemmaSetter&);
public:
    TSimpleLemmaSetter(TSimpleLemma& lemma)
        : Lemma (lemma) {
    }

    void SetLemma(const TChar* lemmaText, size_t lemmaLen, const TChar* formText, size_t formLen, const char* stemGram, const char* const* grs, size_t count, size_t flexLen, ui32 ruleId, int quality, const Wtroka& src);
    void SetFlexGrs(const char* const* grs, size_t count);
    void SetQuality(ui32 quality);
    void SetNormalizedForma(const TChar* form, size_t len);
    size_t AddFlexGr(const char* flexGr);

private:
    TSimpleLemma& Lemma;
};

typedef TSimpleLemmaSetter TYandexLemmaSetter;
}

typedef yvector<TSimpleLemma> TWLemmaArray;

class TLanguage {
    DECLARE_NOCOPY(TLanguage);
public:
    struct TRecognizeOpt {
        size_t MaxLemmasToReturn;
        NLemmer::TLanguageQualityAccept Accept;
        const char* RequiredGrammar;
        bool UseFixList;
        bool SkipNormalization;
        bool GenerateQuasiBastards;
        bool GenerateAllBastards;
        ui32 LastNonRareRuleId;

        TRecognizeOpt(NLemmer::EAccept accept = NLemmer::AccFoundling, const char* requiredGrammar = "", bool useFixList = true, bool generateQuasiBastards = false)
            : MaxLemmasToReturn(size_t(-1))
            , Accept(NLemmerAux::QualitySetByMaxLevel(accept))
            , RequiredGrammar(requiredGrammar)
            , UseFixList(useFixList)
            , SkipNormalization(false)
            , GenerateQuasiBastards(generateQuasiBastards)
            , GenerateAllBastards(false)
            , LastNonRareRuleId(0)
        {}
    };

    size_t Recognize(const TChar* word, size_t length, TWLemmaArray& out, const TRecognizeOpt& opt = TRecognizeOpt()) const {
        UNUSED(word);
        UNUSED(length);
        UNUSED(out);
        UNUSED(opt);
        return 0;
    }
};

namespace NLemmer {

size_t AnalyzeWord(const TChar* word, size_t len, TWLemmaArray& out, TLangMask langmask, const docLanguage* doclangs = NULL, const TAnalyzeWordOpt& opt = TAnalyzeWordOpt());
size_t Generate(const TSimpleLemma &lemma, TWordformArray& out, const char* needed_grammar = "");

const TLanguage* GetLanguageById(docLanguage id);

}
