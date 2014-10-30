#pragma once

#include <library/lemmer/dictlib/grammarhelper.h>
#include <FactExtract/Parser/lemmerlib/lemmerlib.h>

#include "gramfeatures.h"
#include "inflector.h"


namespace NInfl {

// Yandex-lemma inflector
class TYemmaInflector {
public:
    TYemmaInflector(const TYandexLemma& word, size_t flexIndex)
        : Original(&word)
        , CurrentBest(word, flexIndex)
    {
        YASSERT(flexIndex < word.FlexGramNum());
    }

    // If @strict then find a form with exactly @requested grammems in all corresponding features.
    // Otherwise loosen the restrictions and search a form with grammems at least non-controversial to @requested.
    bool Inflect(const TGramBitSet& requested, bool strict);
    bool InflectSupported(const TGramBitSet& requested);

    void ApplyTo(TYandexLemma& word) const;

    static TGramBitSet Grammems(const TWordformKit& kit) {
        TGramBitSet grammems;
        NSpike::ToGramBitset(kit.GetStemGram(), kit.GetFlexGram(), grammems);
        return grammems;
    }

    TGramBitSet Grammems() const {
        return Grammems(CurrentBest);
    }

    void ConstructText(Wtroka& text) const {
        CurrentBest.ConstructText(text);
    }

    Stroka DebugString() const;

    static inline void ResetFlexGram(TYandexLemma& word, const char* fgram) {
        NLemmerAux::TYandexLemmaSetter(word).SetFlexGrs(&fgram, 1);
    }

private:
    const TYandexLemma* Original;
    TWordformKit CurrentBest;
};



struct TDependency {
    size_t From, To;
    TGramBitSet AgreeTo;        // these grammems are agreed in @To with @From
    TGramBitSet EnforceFrom;    // these are explicitly requested in @From when agreement is enforced
    TGramBitSet EnforceTo;      // these are explicitly requested in @To when agreement is enforced
};

class TCollocation {
public:
    //TCollocation();

    void AppendWord(TYandexLemma& word, size_t flexIndex) {
        Words.push_back(TYemmaInflector(word, flexIndex));
    }

    void AddDependency(size_t from, size_t to, const TGramBitSet& agreeTo,
                       const TGramBitSet& enforceFrom = TGramBitSet(),
                       const TGramBitSet& enforceTo = TGramBitSet());
    bool Inflect(size_t from, const TGramBitSet& requested);

    Stroka DebugString() const;

private:
    yvector<TYemmaInflector> Words;
    yvector<TDependency> Agreements;
};

}   // NInfl
