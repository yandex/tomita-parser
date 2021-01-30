#include "numeral.h"
#include "gramfeatures.h"
#include "inflector.h"
#include "ylemma.h"

#include <util/charset/wide.h>
#include <util/generic/singleton.h>

namespace NInfl {


using NSpike::TGrammarBunch;


struct TNumFlexForm {
    const Wtroka* Flexion;
    TGramBitSet Grammems;

    TNumFlexForm()
        : Flexion(NULL)
        , Grammems()
    {}

    TNumFlexForm(const TGramBitSet& gram)
        : Flexion(NULL)
        , Grammems(gram)
    {}

    TNumFlexForm(const Wtroka& flex, const TGramBitSet& gram)
        : Flexion(&flex)
        , Grammems(gram)
    {}
};

// TFormOps<TNumFlexForm> specialization
template <>
struct TFormOps<TNumFlexForm> {

    static bool IsValid(const TNumFlexForm& form) {
        return form.Flexion != NULL;
    }

    static inline TGramBitSet ExtractGrammems(const TNumFlexForm& form) {
        return form.Grammems;
    }

    static inline size_t CommonPrefixSize(const TNumFlexForm& /*form1*/, const TNumFlexForm& /*form2*/) {
        return 0;   // not necessary
    }

    static Stroka DebugString(const TNumFlexForm&) {
        return Stroka();    // TODO
    }
};

class TNumFlexParadigm {
public:
    TNumFlexParadigm(size_t maxFlexLen, size_t minFlexLen = 1)
        : MaxFlexLen(maxFlexLen), MinFlexLen(minFlexLen)
    {
    }

    virtual ~TNumFlexParadigm() {
    }

    void AddForm(const TWtringBuf& text, const TGramBitSet& grammems) {
        for (size_t len = MinFlexLen; len <= MaxFlexLen && len <= text.size(); ++len) {
            Wtroka flex = ::ToWtring(text.Last(len));
            Forms[flex].insert(grammems);
        }
        Wtroka norm = Normalize(text);
        if (!norm.empty())
            NormForms[norm].insert(grammems);
    }

    TWtringBuf DefaultNormalize(const TWtringBuf& flex) const {
        // default normalization: take last 2 characters
        return flex.Last(2);
    }

    virtual Wtroka Normalize(const TWtringBuf& flex) const {
        return ::ToWtring(DefaultNormalize(flex));
    }

    const TGrammarBunch* Recognize(const TWtringBuf& flexion) const {
        TFlexMap::const_iterator form = Forms.find(flexion.Last(MaxFlexLen));
        return form != Forms.end() ? &(form->second) : NULL;
    }

    bool Inflect(const TWtringBuf& flexion, const TGramBitSet& requested,
                 Wtroka& restext, TGramBitSet& resgram) const;

private:
    inline bool InflectForm(TNumFlexForm& form, const TGramBitSet& requested) const;

private:
    size_t MaxFlexLen, MinFlexLen;

    typedef ymap<Wtroka, TGrammarBunch, TLess<TWtringBuf> > TFlexMap;
    TFlexMap Forms;       // all flexions (of size 1 to MaxFlexLen)
    TFlexMap NormForms;    // only normalized flexions (e.g. of size 2)

    friend class TNumFlexIterator;
};


class TNumFlexIterator {
public:
    TNumFlexIterator(const TNumFlexParadigm& numflex)
        : NumFlex(&numflex)
    {
        Restart();
    }

    bool Restart() {
        CurFlex = NumFlex->NormForms.begin();
        YASSERT(!CurFlex->second.empty());
        CurGram = CurFlex->second.begin();
        return Ok();
    }

    bool Ok() const {
        return CurFlex != NumFlex->NormForms.end() &&
               CurGram != CurFlex->second.end();
    }

    void operator++() {
        ++CurGram;
        if (CurGram == CurFlex->second.end()) {
            ++CurFlex;
            if (CurFlex != NumFlex->NormForms.end()) {
                YASSERT(!CurFlex->second.empty());
                CurGram = CurFlex->second.begin();
            }
        }
    }

    TNumFlexForm operator* () const {
        return TNumFlexForm(CurFlex->first, *CurGram);
    }

private:
    const TNumFlexParadigm* NumFlex;
    TNumFlexParadigm::TFlexMap::const_iterator CurFlex;
    TGrammarBunch::const_iterator CurGram;
};


inline bool TNumFlexParadigm::InflectForm(TNumFlexForm& form, const TGramBitSet& requested) const {
    TInflector<TNumFlexForm, TNumFlexIterator> infl(form, TNumFlexIterator(*this));
    return infl.InflectSupported(requested);
}

bool TNumFlexParadigm::Inflect(const TWtringBuf& flexion, const TGramBitSet& requested,
                               Wtroka& restext, TGramBitSet& resgram) const {
    const TGrammarBunch* forms = Recognize(flexion);

    TNumFlexForm best;
    size_t bestNormal = 0;
    if (forms != NULL)
        for (TGrammarBunch::const_iterator it = forms->begin(); it != forms->end(); ++it) {
            TNumFlexForm cur(*it);
            if (InflectForm(cur, requested) && cur.Flexion != NULL) {
                size_t curNormal = (cur.Grammems & DefaultFeatures().NormalMutableSet).count();
                if (best.Flexion == NULL || bestNormal < curNormal) {
                    best = cur;
                    bestNormal = curNormal;
                }
/*
                else if (*best.Flexion != *cur.Flexion)     // ambiguity
                    return false;
*/
            }
        }

    if (best.Flexion == NULL) {
        TNumFlexForm cur;
        if (InflectForm(cur, requested) && cur.Flexion != NULL)
            best = cur;
    }

    if (best.Flexion != NULL) {
        restext = *best.Flexion;
        resgram = best.Grammems;
        return true;
    } else
        return false;
}

void TNumFlexByk::AddForms(const Wtroka& numeral, TNumFlexParadigm* dst, const TGramBitSet& filter) {
    yvector<TYandexLemma> lemmas;
    Wtroka text;

    NLemmer::AnalyzeWord(~numeral, +numeral, lemmas, Language);
    for (size_t i = 0; i < lemmas.size(); ++i) {
        TGramBitSet grammems;
        NSpike::ToGramBitset(lemmas[i].GetStemGram(), grammems);
        if (!grammems.HasAll(filter))
            continue;

        TAutoPtr<NLemmer::TFormGenerator> gen = lemmas[i].Generator();
        for (NLemmer::TFormGenerator& kit = *gen; kit.IsValid(); ++kit) {
            grammems = TYemmaInflector::Grammems(*kit);
            //filter excessive unnecessary forms (currently, only obsolete)
            if (grammems.Has(gObsolete))
                continue;
            kit->ConstructText(text);
            dst->AddForm(text, grammems);
        }
        break;
    }
}

bool TNumFlexByk::Split(const TWtringBuf& text, TWtringBuf& numeral, TWtringBuf& delim, TWtringBuf& flexion) const {
    size_t i = text.size();
    for (; i > 0; --i)
        if (!IsAlpha(text[i-1]))
            break;

    TWtringBuf f = TWtringBuf(~text + i, ~text + text.size());
    if (f.empty())
        return false;

    for (; i > 0; --i)
        if (!IsPunct(text[i-1]))
            break;

    TWtringBuf d = TWtringBuf(~text + i, ~f);
    for (; i > 0; --i)
        if (!IsCommonDigit(text[i-1]))
            break;

    TWtringBuf n = TWtringBuf(~text + i, ~d);
    if (!n.empty()) {
        numeral = n;
        delim = d;
        flexion = f;
        return true;
    } else
        return false;
}



class TNumFlexParadigmRus: public TNumFlexParadigm {
public:
    TNumFlexParadigmRus()
        : TNumFlexParadigm(4, 1)
    {
    }

    virtual Wtroka Normalize(const TWtringBuf& flex) const {
        TWtringBuf res = DefaultNormalize(flex);
        if (!res.empty() && res[0] == 0x44c) // 'ь'
            res.Skip(1);
        return ::ToWtring(res);
    }
};

class TNumLetniyParadigmRus: public TNumFlexParadigm {
public:
    TNumLetniyParadigmRus()
        : TNumFlexParadigm(6, 5)
    {
    }

    virtual Wtroka Normalize(const TWtringBuf& flex) const {
        TWtringBuf res(flex);
        while (!res.empty() && res[0] != 0x43b)  // 'л' (ищем начало корня "летн")
            res.Skip(1);
        return ::ToWtring(res);
    }
};

class TNumFlexBykRus: public TNumFlexByk {
public:
    TNumFlexBykRus();
    virtual const TNumFlexParadigm* Recognize(const TWtringBuf& numeral, const TWtringBuf& flexion) const;
private:
    TNumFlexParadigmRus Perviy;     // 1-ый, 5-тый, 10-й
    TNumFlexParadigmRus Vtoroy;     // 2-ой, 7-мой, 40-вой
    TNumFlexParadigmRus Tretiy;     // 3-ий
    TNumLetniyParadigmRus Stoletniy;  // 2-летний, 7-летний, 40-летний, 100-летний
};


TNumFlexBykRus::TNumFlexBykRus()
{
    Language = LANG_RUS;
    AddNumeralForms(UTF8ToWide("первый"), &Perviy);
    AddNumeralForms(UTF8ToWide("второй"), &Vtoroy);
    AddNumeralForms(UTF8ToWide("третий"), &Tretiy);

    AddNumeralForms(UTF8ToWide("четвертый"), &Perviy);
    AddNumeralForms(UTF8ToWide("шестой"), &Vtoroy);
    AddNumeralForms(UTF8ToWide("седьмой"), &Vtoroy);

    AddNumeralForms(UTF8ToWide("сороковой"), &Vtoroy);

    AddNumeralForms(UTF8ToWide("тысячный"), &Perviy);

    TGramBitSet flt(gAdjective, gFull);
    AddForms(UTF8ToWide("двухлетний"), &Stoletniy, flt);
    AddForms(UTF8ToWide("семилетний"), &Stoletniy, flt);
    AddForms(UTF8ToWide("сорокалетний"), &Stoletniy, flt);
    AddForms(UTF8ToWide("столетний"), &Stoletniy, flt);
    AddForms(UTF8ToWide("тысячелетний"), &Stoletniy, flt);
}

const TNumFlexParadigm* TNumFlexBykRus::Recognize(const TWtringBuf& numeral, const TWtringBuf& flexion) const {

    // 19-letniy
    if (flexion.size() >= 5 && Stoletniy.Recognize(flexion))
        return &Stoletniy;

    size_t L = numeral.size();
    if (L <= 0)
        return NULL;

    // 10-iy, 11-iy, 12, ... , 19-iy
    if (L >= 2 && numeral[L-2] == '1' && ::IsCommonDigit(numeral[L-1]))
        return &Perviy;

    switch (numeral[L-1]) {
        case '0':
            if (L >= 2 && ::IsCommonDigit(numeral[L-2]) && numeral[L-2] != '4')
                return &Perviy;   // 10-iy, 20-iy, 100-iy, 1000-iy, ...
            else
                return &Vtoroy;   // 0-oy, 40-oy
        case '1':
        case '4':
        case '5':
        case '9':
            return &Perviy;

        case '2':
        case '6':
        case '7':
        case '8':
            return &Vtoroy;

        case '3':
            return &Tretiy;

        default:
            return NULL;
    }
}

TNumFlexLemmer::TNumFlexLemmer()
    : DefaultByk(new TNumFlexByk)
{
    Byks[LANG_UNK] = DefaultByk;
    Byks[LANG_RUS] = new TNumFlexBykRus;
}



TNumeralAbbr::TNumeralAbbr(const TLangMask& langMask, const TWtringBuf& wordText)
    : Prefix(wordText)
    , Paradigm(NULL)
{
    const TNumFlexByk& byk = TNumFlexLemmer::Byk(langMask);
    if (byk.Split(wordText, Numeral, Delim, Flexion)) {
        Paradigm = byk.Recognize(Numeral, Flexion);
        Prefix = TWtringBuf(~wordText, ~Numeral);
    }
}


bool TNumeralAbbr::Inflect(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet* resgram) const {
    TGramBitSet rgram;
    Wtroka newflex;
    if (Paradigm != NULL && Paradigm->Inflect(Flexion, grammems, newflex, rgram)) {
        if (resgram != NULL)
            *resgram = rgram;
        ConstructText(newflex, restext);
        return true;
    } else
        return false;
}

void TNumeralAbbr::ConstructText(const TWtringBuf& newflex, Wtroka& text) const {
    text.reserve(Prefix.size() + Numeral.size() + Delim.size() + newflex.size());
    text.assign(~Prefix, +Prefix);
    text.append(~Numeral, +Numeral);
    text.append(~Delim, +Delim);
    text.append(~newflex, +newflex);
}


}   // namespace NInfl

