#include "gramfeatures.h"


namespace NInfl {


TFeature::TFeature(EType type, bool homogeneous, const TGrammar* values, size_t count)
    : Type(type)
    , IsHomogeneous(homogeneous)
{
    OrderedValues.resize(count);
    for (size_t i = 0; i < count; ++i) {
        YASSERT(!Set.Has(values[i]));   // should be unique
        OrderedValues[i] = values[i];
        Set.Set(values[i]);
    }
}

TFeatureHolder::TFeatureHolder() {

    GrammemTypes.resize(gMax - gFirst, TFeature::Max);

    TGrammar pos[] = {gSubstantive, gAdjective, gVerb, gParticiple, gAdverb, gNumeral, gAdjNumeral,
                      gSubstPronoun, gAdjPronoun, gPraedic, gPreposition, gConjunction, gInterjunction,
                      gParenth, gParticle};
    Add(TFeature::PoS, false, pos, ARRAY_SIZE(pos));

    TGrammar subpos[] = {gShort, gComparative, gSuperlative, gInfinitive, gImperative, gGerund, gDerivedAdjective};
    Add(TFeature::SubPoS, false, subpos, ARRAY_SIZE(subpos));

    TGrammar genders[] = {gMasculine, gFeminine, gNeuter, gMasFem};
    Add(TFeature::Gender, true, genders, ARRAY_SIZE(genders));

    TGrammar cases[] = {gNominative, gGenitive, gDative, gAccusative, gInstrumental, gAblative,
                        gPartitive, gLocative, gVocative, gElative};
    Add(TFeature::Case, true, cases, ARRAY_SIZE(cases));

    TGrammar numbers[] = {gSingular, gPlural};
    Add(TFeature::Number, true, numbers, ARRAY_SIZE(numbers));

    TGrammar anim[] = {gAnimated, gInanimated, gHuman};
    Add(TFeature::Anim, true, anim, ARRAY_SIZE(anim));

    TGrammar tenses[] = {gPresent, gNotpast, gPast, gFuture, gPast2};
    Add(TFeature::Tense, true, tenses, ARRAY_SIZE(tenses));

    TGrammar persons[] = {gPerson1, gPerson2, gPerson3};
    Add(TFeature::Person, true, persons, ARRAY_SIZE(persons));

    TGrammar perfect[] = {gImperfect, gPerfect};
    Add(TFeature::Perfect, true, perfect, ARRAY_SIZE(perfect));

    TGrammar transitive[] = {gTransitive, gIntransitive};
    Add(TFeature::Transitive, true, transitive, ARRAY_SIZE(transitive));

    TGrammar vox[] = {gActive, gPassive, gReflexive, gImpersonal, gMedial};
    Add(TFeature::Vox, true, vox, ARRAY_SIZE(vox));

    TGrammar possessive[] = {gPossessive, gPoss1pSg, gPoss1pPl, gPoss2pSg, gPoss2pPl, gPoss3p};
    Add(TFeature::Poss, true, possessive, ARRAY_SIZE(possessive));

    TGrammar predic[] = {gPredic1pSg, gPredic1pPl, gPredic2pSg, gPredic2pPl, gPredic3pSg, gPredic3pPl};
    Add(TFeature::Predic, true, predic, ARRAY_SIZE(predic));

    TGrammar det[] = {gDefinite, gIndefinite};
    Add(TFeature::Det, true, det, ARRAY_SIZE(det));


    YASSERT(Features.size() == TFeature::Max);

    MutableFeatures.push_back(TFeature::Case);
    MutableFeatures.push_back(TFeature::Number);
    MutableFeatures.push_back(TFeature::Gender);
    MutableFeatures.push_back(TFeature::Person);
    MutableFeatures.push_back(TFeature::Anim);
    //MutableFeatures.push_back(TFeature::Tense);   - participles do not support modification of Tense

    for (size_t i = 0; i < MutableFeatures.size(); ++i) {
        MutableSet |= BitSet(MutableFeatures[i]);
        NormalMutableSet.Set(Ordered(MutableFeatures[i])[0]);
    }

    // Could be lost and gained freely, existing state is preferable
    FreeSet = TGramBitSet(gFull, gIndicative, gAuxVerb) |
              Features[TFeature::Tense].Set;

    // Could be lost. Could be gained only if there is no other choice. Existing state is preferable
    UnwantedSet = TGramBitSet(gInformal, gDistort, gContracted, gObscene, gRare) |
                  TGramBitSet(gAwkward, gObsolete);

    // Could be lost, but could never be gained from nothing. Existing state is preferable
    RestrictedSet = TGramBitSet(gGerund, gInfinitive) |
                    TGramBitSet(gImperative, gConditional, gSubjunctive) |
                    TGramBitSet(/*gShort,*/ gComparative, gSuperlative) |           // gShort is required (gosudarstvenno-pravovogo)
                    TGramBitSet(gConverb);

    // new - forbidden, existing - undesirable, missing - preferable (unless explicitly requested)
    ClearedSet = Features[TFeature::Poss].Set |
                 Features[TFeature::Predic].Set |
                 TGramBitSet(gDerivedAdjective);

    RequiredSet = ~(MutableSet | FreeSet | UnwantedSet | RestrictedSet | ClearedSet);
}

void TFeatureHolder::Add(TFeature::EType type, bool homogeneous, const TGrammar* values, size_t count) {
    YASSERT((size_t)type == Features.size());
    Features.push_back(TFeature(type, homogeneous, values, count));

    const TFeature& f = Features.back();
    for (size_t i = 0; i < f.OrderedValues.size(); ++i) {
        TGrammar val = f.OrderedValues[i];
        if (val >= gFirst && val < gMax) {
            TFeature::EType& gtype = GrammemTypes[val - gFirst];
            if (gtype != type) {
                if (gtype != TFeature::Max)
                    ythrow yexception() << "Several features for grammem is not allowed";
                gtype = type;
            }
        }
    }
}

TGramBitSet TFeatureHolder::ExtendAllFeatures(const TGramBitSet& grammems) const {
    TGramBitSet res = grammems;
    for (size_t i = 0; i < Features.size(); ++i)
        if (HasFeature(grammems, Features[i].Type))
            res |= Features[i].Set;
    return res;
}

TGramBitSet TFeatureHolder::ReplaceFeatures(const TGramBitSet& grammems, const TGramBitSet& features) const {
    return (grammems & ~ExtendAllFeatures(features)) | features;
}

TFeature::TMask TFeatureHolder::SupportedFeatures(const TGramBitSet& grammems) const {
    TFeature::TMask res;
    for (size_t i = 0; i < Features.size(); ++i)
        if (HasFeature(grammems, Features[i].Type))
            res.Set(Features[i].Type);
    return res;
}

TFeature::TMask TFeatureHolder::SupportedMutableFeatures(const TGramBitSet& grammems) const {
    TFeature::TMask res;
    for (size_t i = 0; i < MutableFeatures.size(); ++i)
        if (HasFeature(grammems, MutableFeatures[i]))
            res.Set(MutableFeatures[i]);
    return res;
}

static inline bool IsExplicitControversial(const TGramBitSet& f1, const TGramBitSet& f2, const TFeature& feature) {
    return feature.IsHomogeneous ? (!f1.HasAll(f2) && !f2.HasAll(f1)) : f1 != f2;
}

bool TFeatureHolder::IsControversial(const TGramBitSet& g1, const TGramBitSet& g2, const TFeature& feature) const {
    // @candidate's feature either empty or contains all of @requested feature grammems (for non-homogeneous) or vice versa.
    if (!g1.HasAny(feature.Set) || !g2.HasAny(feature.Set))
        return false;
    return IsExplicitControversial(g1 & feature.Set, g2 & feature.Set, feature);
}

TFeature::TMask TFeatureHolder::ControversialFeatures(const TGramBitSet& g1, const TGramBitSet& g2) const {
    TFeature::TMask res;
    for (size_t i = 0; i < Features.size(); ++i)
        if (IsControversial(g1, g2, Features[i]))
            res.Set(Features[i].Type);

    return res;
}

TGramBitSet TFeatureHolder::ExtendControversial(const TGramBitSet& g1, const TGramBitSet& g2) const {
    TGramBitSet res;
    for (size_t i = 0; i < Features.size(); ++i)
        if (IsControversial(g1, g2, Features[i]))
            res |= Features[i].Set;

    return res;
}

bool TFeatureHolder::NonControversial(const TGramBitSet& g1, const TGramBitSet& g2) const {
    for (size_t i = 0; i < Features.size(); ++i)
        if (IsControversial(g1, g2, Features[i]))
            return false;
    return true;
}

TGramBitSet TFeatureHolder::NormalMutable(const TGramBitSet& grammems) const {
    TGramBitSet res;
    for (size_t i = 0; i < MutableFeatures.size(); ++i) {
        TFeature::EType type = MutableFeatures[i];
        if (HasFeature(grammems, type))
            res |= BitSet(type);
    }
    return res & NormalMutableSet;
}



static inline void IncAgreementStat(const TGramBitSet& g1, const TGramBitSet& g2, const TFeature& feature,
                                    size_t& agreedExplicit, size_t& agreedImplicit, size_t& disagreed) {
    TGramBitSet f1 = g1 & feature.Set;
    TGramBitSet f2 = g2 & feature.Set;

    bool has1 = f1.any();
    bool has2 = f2.any();
    if (has1 != has2)
        ++agreedImplicit;       // one has feature, other has not
    else if (has1) {
        if (IsExplicitControversial(f1, f2, feature))
            ++disagreed;
        else
            ++agreedExplicit;
    }
}

TAgreementMeasure::TAgreementMeasure(const TGramBitSet& g1, const TGramBitSet& g2)
    : ArgeedExplicitly(0), AgreedImplicitly(0), Disagreed(0)
{
    for (size_t i = 0; i < DefaultFeatures().Features.size(); ++i)
        IncAgreementStat(g1, g2, DefaultFeatures().Features[i], ArgeedExplicitly, AgreedImplicitly, Disagreed);

    // implicit agreements are counted only if there is an explicit one
    if (ArgeedExplicitly == 0)
        AgreedImplicitly = 0;
}

NCmp::EResult TAgreementMeasure::Compare(const TAgreementMeasure& a) const {
    NCmp::EResult res = NCmp::MoreIsBetter(ArgeedExplicitly, a.ArgeedExplicitly);
    if (res == NCmp::EQUAL) {
        res = NCmp::MoreIsWorse(Disagreed, a.Disagreed);
        if (res == NCmp::EQUAL)
            res = NCmp::MoreIsBetter(AgreedImplicitly, a.AgreedImplicitly);
    }
    return res;
}

}   // NInfl
