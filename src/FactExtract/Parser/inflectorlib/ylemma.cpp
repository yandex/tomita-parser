#include "ylemma.h"
#include "inflector.h"

#include <FactExtract/Parser/common/utilit.h>

namespace NInfl {

// TFormOps<TWordformKit> specialization
template <>
struct TFormOps<TWordformKit> {
    static bool IsValid(const TWordformKit& /*form*/) {
        return true;
    }

    static inline TGramBitSet ExtractGrammems(const TWordformKit& form) {
        return TYemmaInflector::Grammems(form);
    }

    static inline size_t CommonPrefixSize(const TWordformKit& form1, const TWordformKit& form2) {
        return form1.CommonPrefixLength(form2);
    }

    static Stroka DebugString(const TWordformKit& form) {
        TStringStream out;
        Wtroka text;
        form.ConstructText(text);
        out << text << "::" << ExtractGrammems(form).ToString(",");
        return out.Str();
    }
};

class TWordformKitIterator {
public:
    TWordformKitIterator(const TYandexLemma* lemma)
        : Lemma(lemma)
    {
    }

    bool Restart() {
        Generator = Lemma->Generator();
        return Ok();
    }

    bool Ok() const {
        YASSERT(Generator.Get() != NULL);
        return Generator->IsValid();
    }

    void operator++() {
        YASSERT(Generator.Get() != NULL);
        ++(*Generator);
    }

    const TWordformKit& operator* () const {
        YASSERT(Generator.Get() != NULL);
        return **Generator;
    }

private:
    const TYandexLemma* Lemma;
    TAutoPtr<NLemmer::TFormGenerator> Generator;
};

bool TYemmaInflector::Inflect(const TGramBitSet& requested, bool strict) {
    TInflector<TWordformKit, TWordformKitIterator> infl(CurrentBest, TWordformKitIterator(Original));
    return infl.Inflect(requested, strict);
}

bool TYemmaInflector::InflectSupported(const TGramBitSet& requested) {
    TInflector<TWordformKit, TWordformKitIterator> infl(CurrentBest, TWordformKitIterator(Original));
    return infl.InflectSupported(requested);
}

void TYemmaInflector::ApplyTo(TYandexLemma& word) const {
    TChar buffer[MAXWORD_BUF];
    TWtringBuf text = CurrentBest.ConstructText(buffer, MAXWORD_BUF);
    NLemmerAux::TYandexLemmaSetter setter(word);
    setter.SetNormalizedForma(~text, +text);
    const char* flexgr = CurrentBest.GetFlexGram();
    setter.SetFlexGrs(&flexgr, 1);
}

Stroka TYemmaInflector::DebugString() const {
    TYandexLemma copy = *Original;
    ApplyTo(copy);
    return Substitute("$0 ($1 + $2)",
                      WideToChar(copy.GetForma(), copy.GetFormLength()),
                      TGramBitSet::FromBytes(copy.GetStemGram()).ToString(","),
                      TGramBitSet::FromBytes(copy.GetFlexGram()[0]).ToString(","));
}




void TCollocation::AddDependency(size_t from, size_t to, const TGramBitSet& agreeTo,
                                 const TGramBitSet& enforceFrom, const TGramBitSet& enforceTo) {
    Agreements.push_back();
    Agreements.back().From = from;
    Agreements.back().To = to;
    Agreements.back().AgreeTo = agreeTo;
    Agreements.back().EnforceFrom = enforceFrom;
    Agreements.back().EnforceTo = enforceTo;
}


bool TCollocation::Inflect(size_t from, const TGramBitSet& requested) {
    if (!Words[from].Inflect(requested, false))
        return false;

    TGramBitSet mainGrammems = Words[from].Grammems();
    TGramBitSet enforceFrom;

    for (size_t i = 0; i < Agreements.size(); ++i) {
        const TDependency& dep = Agreements[i];
        if (dep.From == from) {
            TGramBitSet agreedTo = (mainGrammems & dep.AgreeTo) | dep.EnforceTo;
            if (!Words[dep.To].InflectSupported(agreedTo))
                return false;
            enforceFrom |= dep.EnforceFrom;
        }
    }

    // if there is enforce-from grammems, try modifying original word again with new requirement
    return enforceFrom.none() || Words[from].Inflect(requested | enforceFrom, false);
}

Stroka TCollocation::DebugString() const {
    if (Words.empty())
        return Stroka();
    Stroka res = Words[0].DebugString();
    for (size_t i = 1; i < Words.size(); ++i) {
        res += " ";
        res += Words[i].DebugString();
    }
    return res;
}


}   // NInfl
