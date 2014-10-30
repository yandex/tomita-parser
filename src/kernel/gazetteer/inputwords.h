#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/charset/wide.h>
#include <library/lemmer/dictlib/grammarhelper.h>
#include <bitset>
#include "worditerator.h"



namespace NGzt {

// This is essentially TWordIterator::TLemmaIter + some extra info about filtered grammems (homoforms).
// TInputWord is not generally visible to user and mostly accessed via TPhraseWords.
// But anyway it is fully usable class
template <typename TInput>
class TInputWord
{
    typedef TWordIterator<TInput> TWordIter;
    typedef typename TWordIter::TLemmaIter TLemmaIter;
    typedef typename TWordIter::TGrammemIter TSourceGrammemIter;

    // The size of bitset is selected to be 1-word (in this case a specialized, more fast, version of bitset is used)
    // So if some word has >32 homoforms in single paradigm - they would not be filtered starting from 32
    // (this should not be very critical)
    enum { MAX_HOMOFORM_COUNT = 32 };
    typedef std::bitset<MAX_HOMOFORM_COUNT> TMask;

public:
    inline TInputWord()
        : LemmaIter(TLemmaIter())
        , IsAnyWord(false)
    {
    }

    inline explicit TInputWord(TLemmaIter lemma)
        : LemmaIter(lemma)
        , IsAnyWord(false)
    {
    }

    // static constructor for making word, found with '*' mask
    static inline TInputWord AnyWord(TLemmaIter lemma = TLemmaIter()) {
        TInputWord res(lemma);
        res.IsAnyWord = true;
        return res;
    }

    // if true then the word corresponds to lemma
    // if false then then word corresponds to exact word from input sequence
    inline bool IsLemma() const {
        return LemmaIter.Ok();
    }

    inline const TLemmaIter& GetLemmaIter() const {
        return LemmaIter;
    }

    inline bool ExactWordFromKey() const {
        // If the word was found as exact-form and not as '*' mask.
        // I.e. there is a key in the found article
        // with exactly same word in corresponding position.
        return !IsLemma() && !IsAnyWord;
    }

    class TGrammemIter {
    public:
        inline explicit TGrammemIter(const TInputWord* word)
            : Word(word), MutableWord(NULL)
            //, SourceGrammems(TWordIter::IterGrammems(word->LemmaIter))
            , SourceGrammems(word->IsLemma()? TWordIter::IterGrammems(word->LemmaIter) : TSourceGrammemIter())
            , Index(0), BadCount(0)
        {
            FindFirst();
        }

        inline explicit TGrammemIter(TInputWord* word)
            : Word(word), MutableWord(word)
            //, SourceGrammems(TWordIter::IterGrammems(word->LemmaIter))
            , SourceGrammems(word->IsLemma()? TWordIter::IterGrammems(word->LemmaIter) : TSourceGrammemIter())
            , Index(0), BadCount(0)
        {
            FindFirst();
        }

        // implements Iterator interface
        inline void operator++() {
            ++SourceGrammems;
            ++Index;
            FindNext();
        }

        inline bool Ok() const {
            return Word->ShouldIterGrammems() && SourceGrammems.Ok();
        }

        inline TGramBitSet operator*() const {
            return *SourceGrammems;
        }

        // Marks current homoform as bad. On next iteration this homoform will be skipped.
        // Does not move iterator to next item - you should do it yourself.
        void RemoveCurrent();

    private:
        void FindFirst() {
            if (Word->ShouldIterGrammems())
                FindNext();
        }

        void FindNext();

        const TInputWord* Word;
        TInputWord* MutableWord;

        TSourceGrammemIter SourceGrammems;
        size_t Index;
        size_t BadCount;
    };

    inline TGrammemIter IterGrammems() const {
        return TGrammemIter(this);
    }

    inline TGrammemIter IterGrammems() {
        return TGrammemIter(this);
    }

    inline TSourceGrammemIter IterAllGrammems() const {
        return TWordIter::IterGrammems(LemmaIter);
    }

    inline void RestoreGrammems() {
        BadHomoforms = InitialBadHomoforms;
    }

    // set current homoforms to be initial state
    inline void SaveGrammems() {
        InitialBadHomoforms = BadHomoforms;
    }

private:

    inline bool ShouldIterGrammems() const {
        static const TMask all = ~TMask();
        return BadHomoforms != all;
    }

    TLemmaIter LemmaIter;
    TMask BadHomoforms, InitialBadHomoforms;
    bool IsAnyWord;     // true, if the word was found with '*' wildcard
};


// Sub-phrase from input word sequence corresponding (possibly) to some found article
// It contains a fixed lemma for each word of this sub-sequence
// (i.e. no homonymy at morph paradigm level, but still could be homonymous forms within paradigm - homoforms)
// TPhraseWords is strongly connected to TWordIterator and can live only during its single iteration.
// When switching to next word from input, all previuosly created TPhraseWords instances become invalid.
// <obsolete>Use TArticleEntry to make static version of such fixed input subsequence (not depending on TWordIterator).</obsolete>
template <typename TInput>
class TPhraseWords
{
    typedef TWordIterator<TInput> TWordIter;
    typedef typename TWordIter::TWordString TWordString;
    typedef typename TWordIter::TChar TChar;
    typedef typename TWordIter::TLemmaIter TLemmaIter;
    typedef TInputWord<TInput> TWord;

    typedef typename TStringSelector<TChar>::TString TString;
public:

    inline TPhraseWords(TWordIter word_iter, yvector<TWord>* words) {
        Reset(word_iter, words);
    }

    inline void Reset(TWordIter word_iter, yvector<TWord>* words) {
        WordIter = word_iter;
        Words = words;
        FirstWordIndex_ = (words != NULL) ? WordIter.ReverseNumberToIndex((TReverseWordNumber)words->size()) : 0;
    }

    // only restore grammems (before applying next filter from FiltersToCheck, for example)
    inline void ResetGrammems() {
        if (Words != NULL)
            for (typename yvector<TWord>::iterator it = Words->begin(); it != Words->end(); ++it)
                it->RestoreGrammems();
    }

    inline void SaveGrammems() {
        if (Words != NULL)
            for (typename yvector<TWord>::const_iterator it = Words->begin(); it != Words->end(); ++it)
                it->SaveGrammems();
    }

    inline size_t Size() const {
        YASSERT(Words != NULL);
        return Words->size();
    }

    // Index of first word relative to WordIterator words sequence.
    inline size_t FirstWordIndex() const {
        return FirstWordIndex_;
    }

    // Index of last word relative to WordIterator words sequence.
    inline size_t LastWordIndex() const {
        return WordIter.ReverseNumberToIndex(1);
    }

    // just synonym of FirstWordIndex for compatibility - position of phrase in source word sequence
    inline size_t Pos() const {
        return FirstWordIndex_;
    }

    inline const yvector<TWord>* GetItems() const {
        return Words;
    }

    TWordString GetString(size_t word_index) const;
    TWordString GetOriginalString(size_t word_index) const;
    docLanguage GetLanguage(size_t word_index) const;

    bool StartsWithDigit(size_t word_index) const;
    bool IsUpperCase(size_t word_index) const;
    bool IsTitleCase(size_t word_index) const;
    bool IsLowerCase(size_t word_index) const;
    bool IsMaskCase(size_t word_index, ui64 mask) const;

    inline bool IsLemma(size_t word_index) const {
        return GetItem(word_index).IsLemma();
    }

    inline TLemmaIter GetLemma(size_t word_index) const {
        return GetItem(word_index).GetLemmaIter();
    }

    inline bool ExactWordFromKey(size_t word_index) const {
        return GetItem(word_index).ExactWordFromKey();
    }

    // there are two variants of grammem iteration: const iteration and non-const iteration with possibility to remove some grammems from word
    typedef typename TWord::TGrammemIter TGrammemIter;

    inline TGrammemIter IterGrammems(size_t word_index) const {
        return GetItem(word_index).IterGrammems();
    }

    inline TGrammemIter IterGrammems(size_t word_index) {   // non-const iteration
        YASSERT(Words != NULL);
        return (*Words)[word_index].IterGrammems();
    }

    inline bool HasGrammems(size_t word_index) const {
        return IterGrammems(word_index).Ok();
    }

    inline typename TWordIter::TGrammemIter IterAllGrammems(size_t word_index) const {
        return GetItem(word_index).IterAllGrammems();
    }

   TString DebugString() const;

private:
    inline const TWord& GetItem(size_t word_index) const {
        YASSERT(Words != NULL);
        return (*Words)[word_index];
    }

private:
    TWordIter WordIter;
    yvector<TWord>* Words;
    size_t FirstWordIndex_;

    // should not be copied, because it does not hold actual lemmas data - just fragile links,
    // valid only until next word from input is processed.
    DECLARE_NOCOPY(TPhraseWords);
};



// Template methods implementations =========================================

template <typename TInput>
void TInputWord<TInput>::TGrammemIter::FindNext() {
    // skip homoforms marked as "bad"
    while (SourceGrammems.Ok()) {
        if (Index >= Word->BadHomoforms.size() || !Word->BadHomoforms.test(Index))
            return;

        ++BadCount;
        ++SourceGrammems;
        ++Index;
    }
    // if all iterated items are bad - do not iterate over them next time.
    if (BadCount >= Index && MutableWord != NULL)
        MutableWord->BadHomoforms = ~TMask();
}

template <typename TInput>
void TInputWord<TInput>::TGrammemIter::RemoveCurrent() {
    // should not be called on with MutableWord==NULL;
    YASSERT(MutableWord != NULL && Ok());
    if (Index < Word->BadHomoforms.size()) {
        MutableWord->BadHomoforms.set(Index);
        ++BadCount;
    }
}




template <typename TInput>
typename TPhraseWords<TInput>::TWordString TPhraseWords<TInput>::GetString(size_t word_index) const {
    if (IsLemma(word_index))
        return WordIter.GetLemmaString(GetLemma(word_index)) ;
    else
        return WordIter.GetWordString(Pos() + word_index);
}

template <typename TInput>
typename TPhraseWords<TInput>::TWordString TPhraseWords<TInput>::GetOriginalString(size_t word_index) const {
    return WordIter.GetOriginalWordString(Pos() + word_index);
}

template <typename TInput>
docLanguage TPhraseWords<TInput>::GetLanguage(size_t word_index) const {
    if (IsLemma(word_index))
        return WordIter.GetLanguage(GetLemma(word_index));
    else
        return LANG_UNK;
}



template <typename TInput>
bool TPhraseWords<TInput>::StartsWithDigit(size_t word_index) const {
    TWordString word = GetOriginalString(word_index);
    typename TWordString::const_iterator it = word.begin();
    if (it == word.end())
        return false;
    return WordIter.CharTest().IsDigit(*it);
}

template <typename TInput>
bool TPhraseWords<TInput>::IsUpperCase(size_t word_index) const {
    TWordString word = GetOriginalString(word_index);
    for (typename TWordString::const_iterator it = word.begin(); it != word.end(); ++it)
        if (WordIter.CharTest().IsLower(*it))
            return false;
    return true;
}

template <typename TInput>
bool TPhraseWords<TInput>::IsLowerCase(size_t word_index) const {
    TWordString word = GetOriginalString(word_index);
    for (typename TWordString::const_iterator it = word.begin(); it != word.end(); ++it)
        if (WordIter.CharTest().IsUpper(*it))
            return false;
    return true;
}

template <typename TInput>
bool TPhraseWords<TInput>::IsTitleCase(size_t word_index) const {
    TWordString word = GetOriginalString(word_index);
    typename TWordString::const_iterator it = word.begin();
    if (it != word.end()) {
        if (WordIter.CharTest().IsLower(*it))
            return false;
        for (++it; it != word.end(); ++it)
            if (WordIter.CharTest().IsUpper(*it))
                return false;
    }
    return true;
}

template <typename TInput>
bool TPhraseWords<TInput>::IsMaskCase(size_t word_index, ui64 mask) const {
    TWordString word = GetOriginalString(word_index);
    typename TWordString::const_iterator it = word.begin();
    for (; mask != 0 && it != word.end(); ++it, mask >>= 1)
        if (mask & 1) {
            if (WordIter.CharTest().IsLower(*it))
                return false;
        }
        else if (WordIter.CharTest().IsUpper(*it))
            return false;

    for (; it != word.end(); ++it)
        if (WordIter.CharTest().IsUpper(*it))
            return false;

    return true;
}

namespace {

template <typename TStr>
static inline TStr DebugStringRecode(const Stroka& str) {
    return str;
}

template <>
inline Wtroka DebugStringRecode<Wtroka>(const Stroka& str) {
    return CharToWide(str);
}

}

template <typename TInput>
typename TPhraseWords<TInput>::TString TPhraseWords<TInput>::DebugString() const {
    TString res;
    for (size_t i = 0; i < Size(); ++i) {
        TWordString str = WordIter.GetOriginalWordString(Pos() + i);
        res += TString(~str, +str) + ' ';
    }

    res += DebugStringRecode<TString>(" -> ");
    for (size_t i = 0; i < Size(); ++i) {
        TWordString str = GetString(i);
        res += TString(~str, +str);
        if (IsLemma(i)) {
            Stroka gram_str;
            for (TGrammemIter gram = IterGrammems(i); gram.Ok(); ++gram) {
                if (!gram_str.empty())
                    gram_str += "|";
                gram_str += (*gram).ToString(",");
            }
            if (!gram_str.empty())
                res += DebugStringRecode<TString>("(" + gram_str + ")");
        }
        else
            res += '!';

        res += ' ';
    }
    return res;
}



} // namespace NGzt
