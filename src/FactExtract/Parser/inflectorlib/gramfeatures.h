#pragma once

#include <util/generic/vector.h>
#include <library/lemmer/dictlib/grammarhelper.h>

#include "better.h"

namespace NInfl {

struct TFeature {
    enum EType {
        PoS, SubPoS,
        Gender, Case, Number, Anim,
        Tense, Person, Perfect,
        Transitive, Vox,
        Poss, Predic,
        Det,
        Max
    };

    typedef TEnumBitSet<EType, TFeature::PoS, TFeature::Max> TMask;

    EType Type;
    yvector<TGrammar> OrderedValues;
    TGramBitSet Set;

    // if true, several values within this feature are treated
    // as several alternatives (one could be replaced with another).
    bool IsHomogeneous;

    TFeature(EType type, bool homogeneous, const TGrammar* values, size_t count);

    bool HasAny(const TGramBitSet& grammems) const {
        return Set.HasAny(grammems);
    }
};


class TFeatureHolder {
public:
    yvector<TFeature> Features;
    yvector<TFeature::EType> MutableFeatures;
    TGramBitSet NormalMutableSet;

    TGramBitSet MutableSet, FreeSet, UnwantedSet, RestrictedSet, ClearedSet, RequiredSet;
    yvector<TFeature::EType> GrammemTypes;

public:
    TFeatureHolder();

    inline const TGramBitSet& BitSet(TFeature::EType type) const { return Features[type].Set; }
    inline const TGramBitSet BitSet(TFeature::EType type1, TFeature::EType type2) const { return BitSet(type1) | BitSet(type2); }
    inline const TGramBitSet BitSet(TFeature::EType type1, TFeature::EType type2, TFeature::EType type3) const { return BitSet(type1, type2) | BitSet(type3); }
    inline const TGramBitSet BitSet(TFeature::EType type1, TFeature::EType type2, TFeature::EType type3, TFeature::EType type4) const { return BitSet(type1, type2, type3) | BitSet(type4); }

    inline const yvector<TGrammar>& Ordered(TFeature::EType type) const {
        return Features[type].OrderedValues;
    }



    inline TFeature::EType TypeOf(TGrammar grammem) const {
        YASSERT(grammem >= gFirst && grammem < gMax);
        return GrammemTypes[grammem - gFirst];
    }

    inline TGramBitSet GetFeature(const TGramBitSet& grammems, TFeature::EType type) const {
        return Features[type].Set & grammems;
    }

    inline bool HasFeature(const TGramBitSet& grammems, TFeature::EType type) const {
        return Features[type].Set.HasAny(grammems);
    }

    TGramBitSet ExtendAllFeatures(const TGramBitSet& grammems) const;
    TGramBitSet ReplaceFeatures(const TGramBitSet& grammems, const TGramBitSet& features) const;

    TFeature::TMask SupportedFeatures(const TGramBitSet& grammems) const;
    TFeature::TMask SupportedMutableFeatures(const TGramBitSet& grammems) const;

    bool IsControversial(const TGramBitSet& g1, const TGramBitSet& g2, const TFeature& feature) const;
    bool NonControversial(const TGramBitSet& g1, const TGramBitSet& g2) const;
    TFeature::TMask ControversialFeatures(const TGramBitSet& g1, const TGramBitSet& g2) const;
    TGramBitSet ExtendControversial(const TGramBitSet& g1, const TGramBitSet& g2) const;


    TGramBitSet NormalMutable(const TGramBitSet& grammems) const;
    bool IsNormalOnly(const TGramBitSet& grammems) const {
        return NormalMutableSet.HasAll(grammems);
    }

    inline bool HasNormal(TFeature::EType type, const TGramBitSet& gram) const {
        return gram.Has(Features[type].OrderedValues[0]);
    }


    inline TGramBitSet Mutable(const TGramBitSet& gram) const {
        return gram & MutableSet;
    }

    // new - undesirable, missing - undesirable, i.e. existing - preferable
    // compared only if other types are same
    inline TGramBitSet Free(const TGramBitSet& gram) const {
        return gram & FreeSet;
    }

    // new - undesirable (unless explicitly requested), missing - undesirable, existing - preferable
    inline TGramBitSet Unwanted(const TGramBitSet& gram) const {
        return gram & UnwantedSet;
    }

    // new - forbidden, missing - undesirable (unless explicitly requested), existing - preferable
    inline TGramBitSet Restricted(const TGramBitSet& gram) const {
        return gram & RestrictedSet;
    }

    // new - forbidden, existing - undesirable, missing - preferable (unless explicitly requested)
    inline TGramBitSet Cleared(const TGramBitSet& gram) const {
        return gram & ClearedSet;
    }

    // new - forbidden, missing - forbidden (unless explicitly specified)
    inline TGramBitSet Required(const TGramBitSet& gram) const {
        return gram & RequiredSet;
    }


private:
    void Add(TFeature::EType type, bool homogeneous, const TGrammar* values, size_t count);
};

inline const TFeatureHolder& DefaultFeatures() {
    return Default<TFeatureHolder>();
}

struct TAgreementMeasure {
    size_t ArgeedExplicitly;
    size_t AgreedImplicitly;
    size_t Disagreed;

    TAgreementMeasure()
        : ArgeedExplicitly(0), AgreedImplicitly(0), Disagreed(Max<size_t>())
    {}

    TAgreementMeasure(const TGramBitSet& g1, const TGramBitSet& g2);
    NCmp::EResult Compare(const TAgreementMeasure& a) const;

    bool ResetIfBetter(const TAgreementMeasure& a) {
        if (a.Compare(*this) != NCmp::BETTER)
            return false;
        *this = a;
        return true;
    }

    bool ResetIfBetter(const TGramBitSet& g1, const TGramBitSet& g2) {
        return ResetIfBetter(TAgreementMeasure(g1, g2));
    }
};

}   // NInfl
