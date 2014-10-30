#pragma once

#include "gramfeatures.h"
#include "better.h"
#include <library/lemmer/dictlib/grammarhelper.h>



namespace NInfl {


template <typename TForm>
struct TFormOps {
    static bool IsValid(const TForm& form);
    static TGramBitSet ExtractGrammems(const TForm& form);
    static size_t CommonPrefixSize(const TForm& form1, const TForm& form2);

    static Stroka DebugString(const TForm& form);
};

template <typename TForm>
class TFormSelector {
public:
    TFormSelector(const TForm& kit, const TForm& original, bool strict);

    bool SelectIfBetter(const TFormSelector& currentBest, const TGramBitSet& requested, const TGramBitSet& mutableGoal,
                        const TGramBitSet& required, const TGramBitSet& restricted, const TGramBitSet& cleared,
                        const TGramBitSet& unwanted, const TGramBitSet& free);

private:
    bool CalcMutableFeature(TFeature::EType feature, const TGramBitSet& requested, const TGramBitSet& mutableGoal);

    NCmp::EResult CompareMutableFeatures(const TFormSelector& currentBest) const;
    NCmp::EResult CompareFreeFeatures(const TFormSelector& currentBest) const;

public:
    TForm BestForm;
    TGramBitSet Grammems;

private:
    const TForm* Original;

    size_t MutableMissing, MutableNew;
    size_t MutableNormal;

    size_t ClearedMissing;

    size_t UnwantedNew, RestrictedUnwantedMissing;
    size_t FreeMissing, FreeNew;

    size_t PrefixLen;

    bool StrictRequested;
};

template <typename TForm>
TFormSelector<TForm>::TFormSelector(const TForm& kit, const TForm& original, bool strict)
    : BestForm(kit)
    , Original(&original)
    , MutableMissing(Max<size_t>())     // the worst
    , MutableNew(0)
    , MutableNormal(0)
    , ClearedMissing(0)
    , UnwantedNew(0)
    , RestrictedUnwantedMissing(0)
    , FreeMissing(0)
    , FreeNew(0)
    , PrefixLen(0)
    , StrictRequested(strict)
{
    Grammems = TFormOps<TForm>::ExtractGrammems(BestForm);
}

template <typename TForm>
bool TFormSelector<TForm>::SelectIfBetter(const TFormSelector& currentBest, const TGramBitSet& requested, const TGramBitSet& mutableGoal,
                                          const TGramBitSet& required, const TGramBitSet& restricted, const TGramBitSet& cleared,
                                          const TGramBitSet& unwanted, const TGramBitSet& free)
{
    const TFeatureHolder& holder = Default<TFeatureHolder>();

    // required grammems should be exactly the same
    if (holder.Required(Grammems) != required)
        return false;
    // no new restricted and cleared grammems
    TGramBitSet fRestricted = holder.Restricted(Grammems);
    TGramBitSet fCleared = holder.Cleared(Grammems);
    if (fRestricted.HasAny(~restricted) || fCleared.HasAny(~cleared))
        return false;

    // if some non-mutable grammems requested explicitly do not accept a form without these grammems
    if (!Grammems.HasAll(requested & ~holder.MutableSet))
        return false;


    MutableMissing = 0;     // init correctly in order to compare with @currentBest

    for (size_t i = 0; i < holder.MutableFeatures.size(); ++i) {
        if (!CalcMutableFeature(holder.MutableFeatures[i], requested, mutableGoal))
            return false;
    }

    // if worse, return as soon as possible
    NCmp::EResult state = CompareMutableFeatures(currentBest);
    if (state == NCmp::WORSE)
        return false;

    // @state=BETTER here means that @this is already better than @currentBest,
    // so do not waste time on further comparisions, just calculate all features

    ClearedMissing += (cleared & ~fCleared).count();
    if (state == NCmp::EQUAL) {
        state = NCmp::MoreIsBetter(ClearedMissing, currentBest.ClearedMissing);    // more is better
        if (state == NCmp::WORSE)
            return false;
    }

    TGramBitSet fUnwanted = holder.Unwanted(Grammems);
    UnwantedNew += (~unwanted & fUnwanted).count();

    if (state == NCmp::EQUAL) {
        state = NCmp::MoreIsWorse(UnwantedNew, currentBest.UnwantedNew);
        if (state == NCmp::WORSE)
            return false;
    }

    RestrictedUnwantedMissing += (unwanted & ~fUnwanted).count();
    RestrictedUnwantedMissing += (restricted & ~fRestricted).count();

    if (state == NCmp::EQUAL) {
        state = NCmp::MoreIsWorse(RestrictedUnwantedMissing, currentBest.RestrictedUnwantedMissing);
        if (state == NCmp::WORSE)
            return false;
    }

    TGramBitSet fFree = holder.Free(Grammems);
    FreeMissing += (free & ~fFree).count();
    FreeNew += (~free & fFree).count();

    if (state == NCmp::EQUAL) {
        state = CompareFreeFeatures(currentBest);
        if (state == NCmp::WORSE)
            return false;
    }

    // last resort
    PrefixLen = TFormOps<TForm>::CommonPrefixSize(BestForm, *Original);
    return state == NCmp::BETTER || PrefixLen > currentBest.PrefixLen;
}

template <typename TForm>
bool TFormSelector<TForm>::CalcMutableFeature(TFeature::EType feature, const TGramBitSet& requested, const TGramBitSet& mutableGoal) {

    const TGramBitSet& fset = DefaultFeatures().BitSet(feature);

    TGramBitSet current = Grammems & fset;        // current grammems of @feature
    TGramBitSet desired = mutableGoal & fset;     // desired grammems of @feature
    if (!current.HasAll(desired)) {
        // Decline this form if @current value of @feature is controversial with @requested.
        // For @StrictRequested mode, do not accept even empty value if this @feature is among @requested.
        if (current.any() || (StrictRequested && requested.HasAny(fset)))
            return false;
        else
            MutableMissing += 1;
    }
    if (!desired.HasAll(current))
        MutableNew += 1;
    if (DefaultFeatures().HasNormal(feature, current))
        MutableNormal += 1;
    return true;
}

template <typename TForm>
NCmp::EResult TFormSelector<TForm>::CompareMutableFeatures(const TFormSelector& currentBest) const {
    // return as soon as we find out that @this is worse compared to @currentBest
    NCmp::EResult res = NCmp::MoreIsWorse(MutableMissing, currentBest.MutableMissing);
    if (res == NCmp::EQUAL) {
        res = NCmp::MoreIsWorse(MutableNew, currentBest.MutableNew);
        if (res == NCmp::EQUAL)
            return NCmp::MoreIsBetter(MutableNormal, currentBest.MutableNormal);    // more is better here
    }
    return res;
}

template <typename TForm>
NCmp::EResult TFormSelector<TForm>::CompareFreeFeatures(const TFormSelector& currentBest) const {
    NCmp::EResult res = NCmp::MoreIsWorse(FreeMissing, currentBest.FreeMissing);
    if (res == NCmp::EQUAL)
        return NCmp::MoreIsWorse(FreeNew, currentBest.FreeNew);
    return res;
}

}   // namespace NInfl
