#include "homonym.h"

#include <FactExtract/Parser/common/utilit.h>

#include <FactExtract/Parser/inflectorlib/gramfeatures.h>
#include <FactExtract/Parser/inflectorlib/numeral.h>

#include <FactExtract/Parser/afdocparser/rus/homonym.h>
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>


static bool IsForeign(const TWtringBuf& text, docLanguage lang) {
    for (size_t i = 0; i < text.size(); ++i)
        if (::IsAlpha(text[i]) && !NStr::IsLangAlpha(text[i], lang))
            return true;

    return false;
}

inline Wtroka ExtractText(const CWord& word, const CWordVector& allWords) {
    Wtroka res = word.IsMultiWord() ? allWords.ToString(word) : word.GetText();
    if (word.IsFirstInSentence() && !word.HasAtLeastTwoUpper()) {
        // Remove title-case for the first word of a sentence (not having other upper-cased letters)
        TMorph::ToLower(res);
    }
    return res;
}

bool THomonymInflector::FindInForms(const THomonymGrammems& forms, const TGramBitSet& grammems, TGramBitSet& resgram) {
    using NInfl::DefaultFeatures;
    using NInfl::TFeature;
    if (forms.HasForms()) {
        for (THomonymGrammems::TFormIter it = forms.IterForms(); it.Ok(); ++it)
            if (it->HasAll(grammems)) {
                resgram = *it;
                return true;
            }
    } else if (forms.IsIndeclinable() && DefaultFeatures().BitSet(TFeature::Case, TFeature::Number).HasAll(grammems)) {
        resgram = grammems;
        return true;
    }

    return false;
}

bool THomonymInflector::FindInForms(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) const {
    YASSERT(Hom != NULL);
    if (!FindInForms(Hom->Grammems, grammems, resgram))
        return false;

    restext = ExtractText(Word, AllWords);
    return true;
}

bool THomonymInflector::AsForeign(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) const {
    if (::IsForeign(Word.GetText(), TMorph::GetMainLanguage())) {
        restext = ::ExtractText(Word, AllWords);
        resgram = grammems;
        return true;
    } else
        return false;
}

bool THomonymInflector::Inflect(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) {

    // if the word contains foreign alphabetical chars, or we already have a homonym with all requested @grammems
    // return original text without modification

    if (Hom != NULL && FindInForms(grammems, restext, resgram))
        return true;

    if (AsForeign(grammems, restext, resgram))
        return true;

    return MakeComplexWord(grammems, false).Inflect(grammems, restext, &resgram);
}


TGramBitSet THomonymInflector::SelectBestHomoform(const CHomonym& hom, const TGramBitSet& desired) {
    if (!hom.Grammems.HasForms())
        return TGramBitSet();

    TGramBitSet res;
    size_t bestCommon = 0;
    bool bestSingular = 0;
    size_t bestNormal = 0;
    // select a form with the maximum of @desired grammems
    for (THomonymGrammems::TFormIter it = hom.Grammems.IterForms(); it.Ok(); ++it) {
        size_t curCommon = (*it & desired).count();
        bool curSingular = it->Has(gSingular);
        size_t curNormal = NInfl::DefaultFeatures().NormalMutable(*it).count();
        bool better = false;
        if (bestCommon < curCommon)
            better = true;
        else if (bestCommon == curCommon) {
            if (curSingular && !bestSingular)
                better = true;
            else if (curSingular == bestSingular && bestNormal < curNormal)
                better = true;
        }

        if (better) {
            res = *it;
            bestCommon = curCommon;
            bestSingular = curSingular;
            bestNormal = curNormal;
        }
    }
    return res;
}

const CHomonym* THomonymInflector::SelectBestHomonym(const CWord& word, const TGramBitSet& desired) {
    const CHomonym* res = NULL;
    size_t bestCommon = 0;
    for (CWord::SHomIt it = word.IterHomonyms(); it.Ok(); ++it) {
        TGramBitSet curGram = SelectBestHomoform(*it, desired);
        size_t curCommon = (curGram & desired).count();
        if (res == NULL || bestCommon < curCommon) {
            res = &*it;
            bestCommon = curCommon;
        }
    }
    return res;
}

NInfl::TComplexWord THomonymInflector::MakeComplexWord(const TGramBitSet& grammems, bool known) const {

    Wtroka text = ExtractText(Word, AllWords);
    if (Hom == NULL)
        return NInfl::TComplexWord(Word.m_lang, text, Wtroka(), known ? grammems : TGramBitSet());

    const TYandexLemma* yemma = Hom->GetYLemma();
    Wtroka lemma;
    if (yemma == NULL && (Hom->IsDictionary() || !NInfl::TComplexWord::IsEqualText(Hom->Lemma, text)))
        lemma = Hom->Lemma;
        // otherwise do not use such weird lemma for inflection

    TGramBitSet form = known ? grammems : SelectBestHomoform(*Hom, grammems);

    return NInfl::TComplexWord(Word.m_lang, text, lemma, form, yemma);
}
