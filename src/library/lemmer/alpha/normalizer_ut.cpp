#include "abc.h"

#include <library/unittest/registar.h>
#include <util/charset/wide.h>

namespace NNormalizerTest {
static const size_t BufferSize = 56;
using NLemmer::TAlphabetWordNormalizer;
using NLemmer::TConvertMode;

static const char* const RussianText[] = {
    "мiрНЫЙ",
    "ТРуЪ",
    "áдмIнъ",
    "3аЛеЗ",
    "на",
    "Ёлку",
    "c",
//    "ëжикомъ",  // needs derenyx 0x0BE => 0x451
    0
};
static const char* const RussianLowCase[] = {
    "мiрный, true",
    "труъ, true",
    "áдмiнъ, true",
    "3алез, true",
    "на, true",
    "ёлку, true",
    "c, true",
    "ëжикомъ, true"
};

static const char* const RussianUpCase[] = {
    "МIРНЫЙ, true",
    "ТРУЪ, true",
    "ÁДМIНЪ, true",
    "3АЛЕЗ, true",
    "НА, true",
    "ЁЛКУ, true",
    "C, true",
    "ËЖИКОМЪ, true"
};

static const char* const RussianTitleCase[] = {
    "Мiрный, true",
    "Труъ, true",
    "Áдмiнъ, true",
    "3алез, true",
    "На, true",
    "Ёлку, true",
    "C, true",
    "Ëжикомъ, true"
};

static const char* const RussianPreConvert[] = {
    "мірный, true",
    "труъ, true",
    "адмінъ, true",
    "3алез, true",
    "на, true",
    "ёлку, true",
    "с, true",
    "ёжикомъ, true"
};

static const char* const RussianNormal[] = {
    "мирный, true",
    "труъ, true",
    "адмін, false", // because "i" can't be here
    "3алез, false", // because of numeral "3"
    "на, true",
    "елку, true",
    "с, true",
    "ежиком, true"
};

static const TChar wtrInval[] = {'i', 0xFFFF, 's', 't', 0};
static const Stroka strInval =~ WideToChar(TWtringBuf(wtrInval), CODES_UTF8);

static const char* const TurkishText[] = {
    "Istânbul'da",
    "ıstanbul'da",
    "İslâm",
    "islam",
    ~strInval,
    0
};

static const char* const TurkishLowCase[] = {
    "ıstânbul'da, true",
    "ıstanbul'da, true",
    "islâm, true",
    "islam, true",
    "ist, true"
};

static const char* const TurkishUpCase[] = {
    "ISTÂNBUL'DA, true",
    "ISTANBUL'DA, true",
    "İSLÂM, true",
    "İSLAM, true",
    "İST, true"
};

static const char* const TurkishTitleCase[] = {
    "Istânbul'da, true",
    "Istanbul'da, true",
    "İslâm, true",
    "İslam, true",
    "İst, true"
};

static const char* const TurkishPreConvert[] = {
    "ıstânbul'da, true",
    "ıstanbul'da, true",
    "islâm, true",
    "islam, true",
    "ist, true"
};

static const char* const TurkishNormal[] = {
    "ıstanbulda, true",
    "ıstanbulda, true",
    "islam, true",
    "islam, true",
    "ist, true"
};

class TAlphaRulesTest : public TTestBase
{
    UNIT_TEST_SUITE(TAlphaRulesTest);
        UNIT_TEST(TestRussian);
        UNIT_TEST(TestTurkish);
    UNIT_TEST_SUITE_END();
private:
    void TestRussian()    {
        CheckNormalization (LANG_RUS,
                    RussianText,
                    RussianLowCase,
                    RussianUpCase,
                    RussianTitleCase,
                    RussianPreConvert,
                    RussianNormal);
    }
    void TestTurkish()    {
        CheckNormalization (LANG_TUR,
                    TurkishText,
                    TurkishLowCase,
                    TurkishUpCase,
                    TurkishTitleCase,
                    TurkishPreConvert,
                    TurkishNormal);
    }
    void CheckNormalization(docLanguage lang, const char* const text[], const char* const low[], const char* const up[], const char* const title[], const char* const pre[], const char* const norm[]) const;
};

namespace {
#define TTCaser(Name)                                               \
   struct T##Name {                                                 \
        docLanguage Id;                                             \
        const TAlphabetWordNormalizer* Nrm;                         \
        T##Name(const TAlphabetWordNormalizer* nrm, docLanguage id) \
            : Id(id)                                                \
            , Nrm(nrm)                                              \
        {                                                           \
        }                                                           \
        NLemmer::TConvertRet operator ()(const TChar* word, size_t length, TChar* converted, size_t bufLen) const { \
            return Nrm->Name(word, length, converted, bufLen);                                                      \
        }                                                           \
        NLemmer::TConvertRet operator ()(Wtroka& s) const {         \
            return Nrm->Name(s);                                    \
        }                                                           \
    }

    TTCaser(ToLower);
    TTCaser(ToUpper);
    TTCaser(ToTitle);

#undef TTCaser

    struct TNormalize {
        docLanguage Id;
        const TAlphabetWordNormalizer* Nrm;
        const TConvertMode Mode;
        TNormalize(const TAlphabetWordNormalizer* nrm, const TConvertMode& mode, docLanguage id)
            : Id(id)
            , Nrm(nrm)
            , Mode(mode)
        {
        }
        NLemmer::TConvertRet operator ()(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
            return Nrm->Normalize(word, length, converted, bufLen, Mode);
        }
        NLemmer::TConvertRet operator ()(Wtroka& s) const {
            return Nrm->Normalize(s, Mode);
        }
    };

    struct TNormalize2 {
        docLanguage Id;
        const TAlphabetWordNormalizer* Nrm;
        const TConvertMode Mode1;
        const TConvertMode Mode2;
        TNormalize2(const TAlphabetWordNormalizer* nrm, docLanguage id)
            : Id(id)
            , Nrm(nrm)
            , Mode1(TAlphabetWordNormalizer::ModePreConvert() | TConvertMode(NLemmer::CnvDecompose, NLemmer::CnvLowCase))
            , Mode2(TAlphabetWordNormalizer::ModeConvertNormalized())
        {
        }
        NLemmer::TConvertRet operator ()(const TChar* word, size_t length, TChar* converted, size_t bufLen) const {
            size_t bufLen2 = length * 4 + 1;
            TChar* buf = (TChar*)alloca(bufLen2 * sizeof(TChar));
            NLemmer::TConvertRet ret1 = Nrm->Normalize(word, length, buf, bufLen2, Mode1);
            if (!ret1.Valid) {
                ret1.Length = Min(ret1.Length, bufLen);
                memcpy(converted, buf, ret1.Length);
                return ret1;
            }
            NLemmer::TConvertRet ret2 = Nrm->Normalize(buf, ret1.Length, converted, bufLen, Mode2);
            ret2.IsChanged |= ret1.IsChanged;
            return ret2;
        }

        NLemmer::TConvertRet operator ()(Wtroka& s) const {
            NLemmer::TConvertRet ret1 = Nrm->Normalize(s, Mode1);
            if (!ret1.Valid)
                return ret1;
            NLemmer::TConvertRet ret2 = Nrm->Normalize(s, Mode2);
            ret2.IsChanged |= ret1.IsChanged;
            return ret2;
        }
    };
}

Stroka PrintRes(const TChar* buffer, const NLemmer::TConvertRet& ret);
Stroka PrintRes(const Wtroka& buffer, const NLemmer::TConvertRet& ret);

Stroka check(const char* to, const TChar* ws1, const NLemmer::TConvertRet& r1, const Wtroka& ws2, const NLemmer::TConvertRet& r2) {
    Stroka outStr;
    TStringOutput out(outStr);

    if (r1.Length != r2.Length)
        out << "different length for buffer and Wtroka result: " << r1.Length << " " << r2.Length << Endl;
    if (r1.Valid != r2.Valid)
        out << "different validity for buffer and Wtroka result: " << r1.Valid << " " << r2.Valid << Endl;
//    if (r1.IsChanged != r2.IsChanged)
//        out << "different IsChanged for buffer and Wtroka result" << Endl;

    Stroka s1 = PrintRes(ws1, r1);
    Stroka s2 = PrintRes(ws2, r2);

    if (s1 != s2)
        out << "different buffer and Wtroka result: \"" << s1 << "\" \"" << s2 << "\"" << Endl;

    if (s1 != to)
        out << "\"" << s1 << "\" doesn't match to \"" << to << "\"" << Endl;

    return outStr;
}

template<class TConv>
bool Checker(const Wtroka& ws, const char* to, const TConv& cnv) {
    TChar buffer[BufferSize];
    Wtroka sbuf = ws;

    NLemmer::TConvertRet ret1 = cnv(~ws, +ws, buffer, BufferSize);
    NLemmer::TConvertRet ret2 = cnv(sbuf);

    Stroka serr = check(to, buffer, ret1, sbuf, ret2);
    if (!!serr) {
        Cerr << "While processing string \"" << WideToChar(ws, CODES_UTF8) << "\" "
            << "with \"" << NameByLanguage(cnv.Id) << "\" language:\n\t"
            << serr;
        return false;
    }
    return true;
}

void TAlphaRulesTest::CheckNormalization(docLanguage lang, const char* const text[], const char* const low[], const char* const up[], const char* const title[], const char* const pre[], const char* const norm[]) const
{
    const TAlphabetWordNormalizer* nrm = NLemmer::GetAlphaRules(lang);
    UNIT_ASSERT(nrm);

    for (size_t i = 0; text[i]; ++i) {
        Wtroka s = CharToWide(text[i], CODES_UTF8);
        UNIT_ASSERT(Checker(s, low[i], TToLower(nrm, lang)));
        UNIT_ASSERT(Checker(s, up[i], TToUpper(nrm, lang)));
        UNIT_ASSERT(Checker(s, title[i], TToTitle(nrm, lang)));
        UNIT_ASSERT(Checker(s, pre[i], TNormalize(nrm, TAlphabetWordNormalizer::ModePreConvert() | TConvertMode(NLemmer::CnvDecompose, NLemmer::CnvLowCase), lang)));
        UNIT_ASSERT(Checker(s, norm[i], TNormalize(nrm, ~TConvertMode(), lang)));
        UNIT_ASSERT(Checker(s, norm[i], TNormalize2(nrm, lang)));
    }
}

Stroka PrintRes_(Stroka s, bool t) {
    s += ", ";
    s += t ? "true" : "false";
    return s;
}

Stroka PrintRes(const TChar* buffer, const NLemmer::TConvertRet& ret) {
    Stroka s = WideToChar(buffer, ret.Length, CODES_UTF8);
    return PrintRes_(s, ret.Valid);
}

Stroka PrintRes(const Wtroka& buffer, const NLemmer::TConvertRet& ret) {
    Stroka s = WideToChar(buffer, CODES_UTF8);
    return PrintRes_(s, ret.Valid);
}

UNIT_TEST_SUITE_REGISTRATION(TAlphaRulesTest);
}
