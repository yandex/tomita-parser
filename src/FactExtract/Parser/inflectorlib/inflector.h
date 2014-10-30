#pragma once

#include "gramfeatures.h"
#include "formselector.h"

#include <library/lemmer/dictlib/grammarhelper.h>

namespace NInfl {


// Generic inflector
template <typename TForm, typename TFormIterator>
class TInflector {

public:
    TInflector(TForm& currentBest, const TFormIterator& iter)
        : CurrentBest(currentBest)
        , Iterator(iter)
        , SupportedIsCollected(false)
    {
    }

    // If @strict then find a form with exactly @requested grammems in all corresponding features.
    // Otherwise loosen the restrictions and search a form with grammems at least non-controversial to @requested.
    bool Inflect(const TGramBitSet& requested, bool strict);

    bool InflectStrict(TGrammar requested) {
        return Inflect(TGramBitSet(requested), true);
    }

    bool InflectSupported(const TGramBitSet& requested);

    inline bool IsNonControversial(TGrammar value) {
        return IsNonControversial(Grammems(), value);
    }

    TGramBitSet Grammems() const {
        return TFormOps<TForm>::ExtractGrammems(CurrentBest);
    }

private:
    // Return true if @formgram's @feature-subset grammems are empty or has specified @value.
    // This could be used as an inflection result if we cannot find exact match for some grammems combinations.
    static bool IsNonControversial(const TGramBitSet& formgram, TGrammar value) {
        TGramBitSet kitGram = DefaultFeatures().BitSet(DefaultFeatures().TypeOf(value)) & formgram;
        return kitGram.none() || kitGram.Has(value);
    }

    static inline TGramBitSet ReplaceGram(const TGramBitSet& gram, TGrammar from, TGrammar to) {
        TGramBitSet r = gram;
        r.Reset(from);
        r.Set(to);
        return r;
    }

private:
    TForm& CurrentBest;
    TFormIterator Iterator;

    bool SupportedIsCollected;
    TGramBitSet SupportedGrammems;
};




// Template methods implementation

template <typename TForm, typename TFormIterator>
bool TInflector<TForm, TFormIterator>::Inflect(const TGramBitSet& requested, bool strict) {
//    Cerr << "Inflecting to " << requested.ToString(",") << Endl;
    const TFeatureHolder& ff = DefaultFeatures();

    TGramBitSet grammems = Grammems();
    // Mega-optimization: if current form is valid and already has all requested grammems, just return it.
    if (TFormOps<TForm>::IsValid(CurrentBest))
        if (grammems.HasAll(requested) && !grammems.HasAny(ff.ClearedSet))
            return true;

    if (!Iterator.Restart())
        return false;

    TGramBitSet mutableGoal = ff.ReplaceFeatures(ff.Mutable(grammems), requested);
    TGramBitSet required = ff.Required(grammems);
    TGramBitSet free = ff.Free(grammems);
    TGramBitSet unwanted = ff.Unwanted(grammems);
    TGramBitSet restricted = ff.Restricted(grammems);
    TGramBitSet cleared = ff.Cleared(grammems);

    // if requested non-mutable grammems, fix our sets accordingly
    if (requested.HasAny(~ff.MutableSet)) {
        required = ff.ReplaceFeatures(required, ff.Required(requested));
        free = ff.ReplaceFeatures(free, ff.Free(requested));
        unwanted = ff.ReplaceFeatures(unwanted, ff.Unwanted(requested));
        restricted = ff.ReplaceFeatures(restricted, ff.Restricted(requested));
        cleared = ff.ReplaceFeatures(cleared, ff.Cleared(requested));
    }

    TFormSelector<TForm> best(*Iterator, CurrentBest, strict);
    bool found = false;

//    Cerr << "requested=" << requested.ToString(",") << ", ";
//    Cerr << "mutableGoal=" << mutableGoal.ToString(",") << ", ";
//    Cerr << "required=" << required.ToString(",") << ", ";
//    Cerr << "restricted=" << restricted.ToString(",") << ", ";
//    Cerr << "cleared=" << cleared.ToString(",") << ", ";
//    Cerr << "unwanted=" << unwanted.ToString(",") << ", ";
//    Cerr << "free=" << free.ToString(",") << Endl;

//    Cerr << "CurrentBest(0) is " << TFormOps<TForm>::DebugString(CurrentBest) << Endl;

    for (; Iterator.Ok(); ++Iterator) {
        TFormSelector<TForm> current(*Iterator, CurrentBest, strict);
        if (current.SelectIfBetter(best, requested, mutableGoal, required, restricted, cleared, unwanted, free)) {
//            Cerr << "Found better: " << TFormOps<TForm>::DebugString(current.BestForm) << Endl;
            found = true;
            best = current;
        }
        if (!SupportedIsCollected)
            SupportedGrammems |= current.Grammems;
    }

    SupportedIsCollected = true;

    if (!found)
        return false;

    CurrentBest = best.BestForm;
//    Cerr << "FinalBest is " << TFormOps<TForm>::DebugString(CurrentBest) << Endl;
    YASSERT(TFormOps<TForm>::IsValid(CurrentBest));
    return true;
}

template <typename TForm, typename TFormIterator>
bool TInflector<TForm, TFormIterator>::InflectSupported(const TGramBitSet& requested) {
    if (Inflect(requested, false))
        return true;

    YASSERT(SupportedIsCollected);
    TGramBitSet supported;
    const TFeatureHolder& ff = DefaultFeatures();
    for (size_t i = 0; i < ff.Features.size(); ++i) {
        TGramBitSet freq = requested & ff.Features[i].Set;
        if (SupportedGrammems.HasAll(freq) || (SupportedGrammems & ff.Features[i].Set).count() > 1)
            supported |= freq;
    }

    if (supported.any()) {

        if (Inflect(supported, false))
            return true;

        // some special hacks

        // special treatement of gMasFem: try inflecting to both genders
        if (supported.Has(gMasFem)) {
            if (Inflect(ReplaceGram(supported, gMasFem, gMasculine), false))
                return true;
            if (Inflect(ReplaceGram(supported, gMasFem, gFeminine), false))
                return true;
        }

        if (supported.Has(gLocative) && Inflect(ReplaceGram(supported, gLocative, gAblative), false))
            return true;
    }

    return false;
}

}   // namespace NInfl
