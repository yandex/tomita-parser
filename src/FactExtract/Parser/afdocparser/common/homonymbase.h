#pragma once

#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/ptr.h>
#include <library/lemmer/dictlib/grammarhelper.h>
#include <library/iter/set.h>

#include <FactExtract/Parser/afdocparser/rus/morph.h>

using NSpike::TGrammarBunch;

class THomonymGrammems {
public:
    inline THomonymGrammems() {
    }

    inline THomonymGrammems(const TGramBitSet& gram) {
        Reset(gram);
    }

    inline THomonymGrammems(const TGrammarBunch& forms) {
        Reset(forms);
    }

    inline void Reset() {
        Forms_.clear();
        All_.Reset();
    }

    inline void Reset(const TGramBitSet& gram) {
        Forms_.clear();
        All_ = gram;
    }

    inline void Reset(const TGrammarBunch& forms) {
        Forms_ = forms;
        All_ = TGramBitSet::UniteGrammems(Forms_);
    }

    inline void ResetAbbrev() {
        static const TGramBitSet mask = TGramBitSet(gSubstantive) | NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers;
        Reset(mask);
    }

    inline void ResetSingleForm(const TGramBitSet& form) {
        Forms_.clear();
        Forms_.insert(form);
        All_ = form;
    }

    inline void Swap(THomonymGrammems& grammems) {
        ::DoSwap(Forms_, grammems.Forms_);
        ::DoSwap(All_, grammems.All_);
    }

    void SetPOS(const TGramBitSet& POS);

    void Replace(TGrammar oldGr, TGrammar newGr);
    void Delete(TGrammar gr) { Replace(gr, gInvalid); }
    void Add(TGrammar gr)    { Replace(gInvalid, gr); }

    void Replace(const TGramBitSet& oldGr, const TGramBitSet& newGr);
    void Delete(const TGramBitSet& gr) { Replace(gr, TGramBitSet()); }
    void Add(const TGramBitSet& gr)    { Replace(TGramBitSet(), gr); }

    bool operator== (const THomonymGrammems& grammems) const {
        return All_ == grammems.All_ && Forms_ == grammems.Forms_;
    }

    bool operator< (const THomonymGrammems& grammems) const {
        if (All_ != grammems.All_)
            return All_ < grammems.All_;
        else
            return Forms_ < grammems.Forms_;
    }

    void AddForm(const TGramBitSet& grammems);

    THomonymGrammems& operator |= (const THomonymGrammems& grammems) {
        Forms_.insert(grammems.Forms().begin(), grammems.Forms().end());
        All_ |= grammems.All();
        return *this;
    }

    inline const TGramBitSet& All() const {
        return All_;
    }

    inline const TGrammarBunch& Forms() const {
        return Forms_;
    }

    typedef NIter::TSetIterator<const TGramBitSet> TFormIter;
    TFormIter IterForms() const {
        return TFormIter(Forms_);
    }

    bool Empty() const {
        return All_.none() && Forms_.empty();
    }

    bool HasForms() const {
        return !Forms_.empty();
    }

    inline bool Has(const TGrammar grammem) const {
        return All_.Has(grammem);
    }

    inline bool HasAll(const TGramBitSet& grammems) const {
        return All_.HasAll(grammems);
    }

    inline bool HasAny(const TGramBitSet& grammems) const {
        return All_.HasAny(grammems);
    }

    bool HasFormWith(const TGramBitSet& grammems) const;

    // helpers
    inline bool HasAllCases() const {
        return HasAll(NSpike::AllMajorCases);
    }

    inline bool IsIndeclinable() const {
        return HasAllCases() && HasAll(NSpike::AllNumbers);
    }

    inline bool HasNominativeOnly() const {
        return (All_ & NSpike::AllCases) == TGramBitSet(gNominative);
    }

    TGramBitSet GetPOS() const {
        return All_ & TMorph::AllPOS();
    }

private:
    TGrammarBunch Forms_;    // grammems of homonymic forms
    TGramBitSet All_;        // union of Forms

};

class CHomonymBase: public TRefCounted<CHomonymBase>
{
public:
    CHomonymBase(docLanguage lang, const Wtroka& sLemma = Wtroka())
        : IsDictionaryHom(false)
        , Lang(lang)
        , Lemma(sLemma)
    {
    }

    virtual ~CHomonymBase() {
    }

    void InitText(const Wtroka& text, bool isDictHom = false) {
        SetIsDictionary(isDictHom);
        SetLemma(text);
    }

    void SetLemma(const Wtroka& lemma);

    void SetGrammems(const THomonymGrammems& grammems) {
        Grammems = grammems;
    }

    void SetIsDictionary(bool isDictHom) {
        IsDictionaryHom = isDictHom;
    }

    void Init(const Wtroka& lemma, const TGrammarBunch& formsgrammars, bool isDictHom = false);

    void ResetYLemma(const TYandexLemma& lemma) {
        if (YLemma.Get() != NULL && YLemma.RefCount() == 1)
            *YLemma = lemma;
        else
            YLemma.Reset(new TYandexLemma(lemma));
    }

    const TYandexLemma* GetYLemma() const {
        return YLemma.Get();
    }

    virtual void AddLemmaPrefix(const Wtroka&) {
    }

    virtual Wtroka GetLemma() const {
        return Lemma;
    }

    inline bool HasGrammem(const TGrammar grammem) const {
        return Grammems.Has(grammem);
    }

    bool IsDictionary() const {
        return IsDictionaryHom;
    }

    inline bool IsGeo() const {
        return HasGrammem(gGeo);
    }

    bool IsName() const {
        static const TGramBitSet names(gSurname, gFirstName, gPatr);
        return Grammems.HasAny(names) && (HasGrammem(gSingular) || !HasGrammem(gPlural));
    }

    // specific morph features testers
    bool IsFullAdjective() const  { return TMorph::IsFullAdjective(Grammems.All()); }   //without gShort!
    bool IsFullParticiple() const { return TMorph::IsFullParticiple(Grammems.All()); } //without gShort!
    bool IsShortAdjectiveOrParticiple() const;
    bool IsPersonalVerb() const   { return TMorph::IsPersonalVerb(Grammems.All()); }
    bool IsPresentTense() const   { return TMorph::IsPresentTense(Grammems.All()); }
    bool IsFutureTense() const    { return TMorph::IsFutureTense(Grammems.All()); }
    bool IsPastTense() const      { return TMorph::IsPastTense(Grammems.All()); }
    // TODO: move these methods to place where they are called and used
    bool IsConj() const;
    bool IsSubConj() const        { return HasGrammem(gSubConj) || HasGrammem(gPronounConj); }
    bool IsSimConj() const        { return HasGrammem(gSimConj); }

    bool IsMorphNoun() const;
    bool IsMorphAdj() const;

    // general POS testers
    static bool HasPOS(const TGramBitSet& grammems, const TGrammar& POS);       // for simple POS represented with single grammem: gSubstantive, gVerb (personal verb), gAdjective (not short), etc.
    static bool HasPOS(const TGramBitSet& grammems, const TGramBitSet& POS);    // for complex POS represented with a set of grammems: gAdjective + gShort, for example
    bool HasPOS(const TGrammar& POS) const { return HasPOS(Grammems.All(), POS); }
    bool HasPOS(const TGramBitSet& POS) const { return HasPOS(Grammems.All(), POS); }
    bool HasAnyOfPOS(const TGramBitSet& POS) const;         //for checking on simple POS, (e.g. not gAdjective + gShort)
    bool HasAnyOfPOS(const TGrammarBunch& POS) const;
    bool HasSamePOS(const CHomonymBase& hom) const;         // true if POS of @this and @hom are same
    bool HasUnknownPOS() const { return Grammems.GetPOS().none(); }

    TGramBitSet GetMinimalPOS() const;      //returns minimal number of grammems to identify POS of homonym

protected:
    virtual void CopyTo(CHomonymBase* pHom) const
    {
        pHom->IsDictionaryHom = IsDictionaryHom;
        pHom->Lemma = Lemma;
        pHom->Grammems = Grammems;
        pHom->YLemma = YLemma;
    }

protected:
    bool    IsDictionaryHom; // True if homonym is taken from dictionary;

    TSharedPtr<TYandexLemma> YLemma;

public:
    const docLanguage Lang;
    Wtroka Lemma;
    THomonymGrammems Grammems;

    typedef THomonymGrammems::TFormIter TFormIter;
};

