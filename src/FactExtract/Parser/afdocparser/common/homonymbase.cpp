#include "homonymbase.h"


void THomonymGrammems::SetPOS(const TGramBitSet& POS) {
    const TGramBitSet& all = TMorph::AllPOS();
    All_.ReplaceByMask(POS, all);
    TGrammarBunch tmpForms;
    for (TFormIter it = IterForms(); it.Ok(); ++it) {
        TGramBitSet gr = *it;
        gr.ReplaceByMask(POS, all);
        if (gr.any())
            tmpForms.insert(gr);
    }
    Forms_.swap(tmpForms);
}

void THomonymGrammems::AddForm(const TGramBitSet& grammems) {
    if (grammems.any())
        Forms_.insert(grammems);
    All_ |= grammems;
}

void THomonymGrammems::Replace(TGrammar grOld, TGrammar grNew) {
    if ((grOld == gInvalid || !Has(grOld)) && (grNew == gInvalid || Has(grNew)))
        return;

    All_.SafeReset(grOld);
    All_.SafeSet(grNew);

    TGrammarBunch tmp;
    for (TFormIter it(Forms_); it.Ok(); ++it) {
        TGramBitSet grammems = *it;
        grammems.SafeReset(grOld);
        grammems.SafeSet(grNew);
        if (grammems.any())
            tmp.insert(grammems);
    }
    Forms_.swap(tmp);
}

void THomonymGrammems::Replace(const TGramBitSet& grOld, const TGramBitSet& grNew) {
    if ((grOld.none() || !HasAny(grOld)) && (grNew.none() || HasAll(grNew)))
        return;

    TGramBitSet noOld = ~grOld;

    All_ &= noOld;
    All_ |= grNew;

    TGrammarBunch tmp;
    for (TFormIter it(Forms_); it.Ok(); ++it) {
        TGramBitSet grammems = *it;
        grammems &= noOld;
        grammems |= grNew;
        if (grammems.any())
            tmp.insert(grammems);
    }
    Forms_.swap(tmp);
}

bool THomonymGrammems::HasFormWith(const TGramBitSet& grammems) const {
     if (HasForms()) {
        for (TFormIter it = IterForms(); it.Ok(); ++it)
            if (it->HasAll(grammems))
                return true;
        return false;
    } else
        return HasAll(grammems);        // for half-made homonyms
}

void CHomonymBase::SetLemma(const Wtroka& lemma) {
    Lemma = lemma;
    TMorph::ToLower(Lemma);
    TMorph::ReplaceYo(Lemma);
}

void CHomonymBase::Init(const Wtroka& lemma, const TGrammarBunch& formsgrammars, bool isDictHom) {
    InitText(lemma, isDictHom);
    SetGrammems(formsgrammars);
}

bool CHomonymBase::HasAnyOfPOS(const TGramBitSet& POS) const {
    static const TGramBitSet exceptions(gVerb, gAdjective, gParticiple);

    if (POS.HasAny(exceptions)) {
        if ((POS.Has(gVerb) && IsPersonalVerb()) ||
            (POS.Has(gAdjective) && IsFullAdjective()) ||
            (POS.Has(gParticiple) && IsFullParticiple()))
            return true;
        else
            return Grammems.HasAny(POS & ~exceptions);
    } else
        return Grammems.HasAny(POS);
}

bool CHomonymBase::HasAnyOfPOS(const TGrammarBunch& POS) const {
    for (TFormIter it(POS); it.Ok(); ++it)
        if (HasPOS(*it))
            return true;
    return false;
}

bool CHomonymBase::HasSamePOS(const CHomonymBase& hom) const {
    return GetMinimalPOS() == hom.GetMinimalPOS();
}

//returns minimal number of grammems required to identify POS of homonym (most specific grammem(s))
TGramBitSet CHomonymBase::GetMinimalPOS() const {
    TGramBitSet res = Grammems.GetPOS();
    size_t count = res.count();
    if (count >= 2) {
        //take max (lexicographically) POS grammem - assumed to be most specific (!)
        TGrammar last_found = res.FindFirst();
        TGrammar next = res.FindNext(last_found);
        while (next < TGramBitSet::End) {
            last_found = next;
            next = res.FindNext(last_found);
        }
        if (last_found == gShort || last_found == gPossessive) {
            // if count == 2 - consider remained grammem to be the one required to specify POS
            if (count > 2)
                res &= TGramBitSet(last_found, gAdjective, gParticiple);
        } else
            res = TGramBitSet(last_found);
    }
    return res;
}

bool CHomonymBase::HasPOS(const TGramBitSet& grammems, const TGrammar& POS) {     //static
    switch (POS) {
    case gVerb:
        return TMorph::IsPersonalVerb(grammems);
    case gParticiple:
        return TMorph::IsFullParticiple(grammems);
    case gAdjective:
        return TMorph::IsFullAdjective(grammems);
    default:
        return grammems.Has(POS);
    }
}

bool CHomonymBase::HasPOS(const TGramBitSet& grammems, const TGramBitSet& POS) {   //static
    return (POS.count() == 1) ? HasPOS(grammems, POS.FindFirst()) : grammems.HasAll(POS);
}

bool CHomonymBase::IsShortAdjectiveOrParticiple() const {
    static const TGramBitSet mask(gAdjective, gParticiple);
    return Grammems.HasAny(mask) && HasGrammem(gShort);
}

bool CHomonymBase::IsConj() const {
    static const TGramBitSet mask(gConjunction, gSubConj, gSimConj, gPronounConj, gCorrelateConj);
    return Grammems.HasAny(mask);
}

bool CHomonymBase::IsMorphNoun() const
{
    static const TGramBitSet mask(gSubstantive, gSubstPronoun, gPronounConj);
    return Grammems.HasAny(mask);
}

bool CHomonymBase::IsMorphAdj() const
{
    static const TGramBitSet mask(gAdjPronoun, gAdjective, gParticiple, gAdjNumeral);
    return Grammems.HasAny(mask) && !HasGrammem(gShort);       // to avoid counting short adjectives or participles

}

