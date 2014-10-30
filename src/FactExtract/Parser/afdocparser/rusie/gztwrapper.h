#pragma once

#include <util/generic/hash.h>
#include <util/generic/stroka.h>

#include <kernel/gazetteer/gazetteer.h>
#include <library/iter/iter.h>

#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/rus/homonym.h>

using NIter::TVectorIterator;
using NIter::TSetIterator;

// Iterates over m_FormGrammems of specified homonym.
// If this homonym does not have m_FormGrammems, but has non-empty m_AllForms,
// then it is considered as the homonym's single form.
class THomoformsIterator
{
public:
    inline THomoformsIterator()
        : Forms(), CurForm(NULL)
    {
    }

    inline THomoformsIterator(const CHomonym& homonym)
        : Forms(homonym.Grammems.Forms()), CurForm(NULL)
    {
        if (Forms.Ok()) {
            CurForm = Forms.operator->();
            ++Forms;
        } else if (homonym.Grammems.All().any())
            CurForm = &homonym.Grammems.All();
    }

    inline bool Ok() const
    {
        return CurForm != NULL;
    }

    inline void operator++()
    {
        if (Forms.Ok()) {
            CurForm = Forms.operator->();
            ++Forms;
        } else
            CurForm = NULL;
    }

    inline const TGramBitSet& operator*() const
    {
        YASSERT(CurForm != NULL);
        return *CurForm;
    }

    inline const TGramBitSet* operator->() const
    {
        YASSERT(CurForm != NULL);
        return CurForm;
    }
private:
    TSetIterator<const TGramBitSet> Forms;
    const TGramBitSet* CurForm;
};

// Special lemma iterator: if a homonym is a hyphened word, also iterate over its last part as separate lemma
// This is required to find gzt-articles in specific case: e.g. find key 'centr' in word 'muzey-centr' [translit]
// Plus, finally iterates over m_PredictedHomonyms
class TWordRusHomonymIterator
{
public:
    inline TWordRusHomonymIterator()
        : CurrentHomonym(NULL)
        , Hyphenable(false)
        , HyphenChecked(false)
        , PredictedIndex(0)
    {
    }

    inline TWordRusHomonymIterator(const CWord& word)
        : CurrentHomonym(NULL)
        , HomIt(word.IterHomonyms())
        , Hyphenable(word.m_typ == Hyphen && !word.m_bUp)
        , HyphenChecked(false)
        , PredictedHomonyms(word.GetPredictedHomonyms())
        , PredictedIndex(0)
    {
        if (HomIt.Ok())
            SetCurrent(&*HomIt);
        else if (PredictedHomonyms.Ok())
            SetCurrent(PredictedHomonyms->Get());
    }

    inline bool Ok() const
    {
        return HomIt.Ok() || PredictedHomonyms.Ok();
    }

    void operator++();

    inline const Wtroka& GetLemmaText() const
    {
        return CurrentLemma;
    }

    inline const CHomonym& operator*() const
    {
        return *CurrentHomonym;
    }

    inline const CHomonym* operator->() const
    {
        return CurrentHomonym;
    }

    inline int GetID() const
    {
        return HomIt.Ok()? HomIt.GetID() : -1;
    }

    inline size_t GetPredictedIndex() const
    {
        YASSERT(!HomIt.Ok());
        return PredictedIndex;
    }

private:
    inline void SetCurrent(const CHomonym* hom)
    {
        CurrentHomonym = hom;
        CurrentLemma = hom->GetLemma();
    }

    const CHomonym* CurrentHomonym;
    Wtroka CurrentLemma;

    CWord::SHomIt HomIt;
    bool Hyphenable;
    bool HyphenChecked;

    NIter::TVectorIterator<const THomonymPtr> PredictedHomonyms;
    size_t PredictedIndex;

};

namespace NGzt
{

typedef TWordIterator<CWordVector> TRusWordsIterator;

template <>
class TWordIteratorTraits<CWordVector>
{
public:
    typedef wchar16 TChar;
    typedef Wtroka TWordString;
    typedef NIter::TValueIterator<const TWordString> TExactFormIter;
    typedef TWordRusHomonymIterator TLemmaIter;
    typedef THomoformsIterator TGrammemIter;
    typedef TWideCharTester TCharTester;
};

template <>
inline bool TWordIterator<CWordVector>::Next() {
    if (WordCount < Input->OriginalWordsCount()) {
        WordCount += 1;
        return true;
    }
    return false;
}

template <>
inline TWordIterator<CWordVector>::TWordString TWordIterator<CWordVector>::GetWordString(size_t word_index) const {
    // should return fully normalized word as it is checked directly against ExactFormTrie
    return (*Input)[word_index].GetLowerText();
}

template <>
inline TWordIterator<CWordVector>::TWordString TWordIterator<CWordVector>::GetOriginalWordString(size_t word_index) const {
    return (*Input)[word_index].GetText();
}

template <>
inline TWordIterator<CWordVector>::TLemmaIter TWordIterator<CWordVector>::IterLemmas(size_t word_index) const {
    return TWordRusHomonymIterator((*Input)[word_index]);
}

template <>
inline TWordIterator<CWordVector>::TWordString TWordIterator<CWordVector>::GetLemmaString(const TLemmaIter& lemma) {
    return lemma.GetLemmaText();
}

template <>
inline TWordIterator<CWordVector>::TGrammemIter TWordIterator<CWordVector>::IterGrammems(const TLemmaIter& lemma) {
    return THomoformsIterator(*lemma);
}

template <>
inline docLanguage TWordIterator<CWordVector>::GetLanguage(const TLemmaIter& lemma) {
    return lemma->Lang;
}

} // namespace NGzt
