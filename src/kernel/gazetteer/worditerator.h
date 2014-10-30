#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/string/vector.h>
#include <util/charset/unidata.h>

#include <FactExtract/Parser/lemmerlib/lemmerlib.h>

#ifndef TOMITA_EXTERNAL
#include <library/lemmer/core/wordinstance.h>
#endif

#include <library/lemmer/dictlib/grammarhelper.h>
#include <library/iter/iter.h>

namespace NGzt
{

// Number of word relative to the end of key phrase (the last word is number 1, the word before last is number 2, etc.).
// No more than 255 words per key, 0 means "all words of key".
typedef ui8 TReverseWordNumber;

// word index, forward order, 0-based, for internal usage
typedef ui8 TWordIndex;



// String<TChar> - Stroka or Wtroka
template <typename TChar>
struct TStringSelector {};

template <>
struct TStringSelector<char> { typedef Stroka TString; };

template <>
struct TStringSelector<wchar16> { typedef Wtroka TString; };


//iterator over separated stem and flex grammems, joins them to get single grammems of lemma
class TStemFlexGramIterator {
public:
    TStemFlexGramIterator()
        : StemGram(NULL), FlexGram()
    {
    }

    TStemFlexGramIterator(const TGramBitSet& stem_gram, const yvector<TGramBitSet>& flex_gram)
        : StemGram(&stem_gram), FlexGram(flex_gram)
    {
        if (!FlexGram.Ok() && StemGram->any())
            // when there is no @flex_gram, the iterator still will do a single iteration over @stem_gram
            FlexGram.Reset(StemGram, StemGram + 1);
    }

    inline bool Ok() const {
        return FlexGram.Ok();
    }

    inline void operator++() {
        ++FlexGram;
    }

    // return a COPY instead of ref
    typedef TGramBitSet TValueRef;

    inline TGramBitSet operator*() const {
        return *StemGram | *FlexGram;
    }

private:
    const TGramBitSet* StemGram;
    NIter::TVectorIterator<const TGramBitSet> FlexGram;
};

class TYandexLemmaGramIter {
public:
    TYandexLemmaGramIter()
        : StemGram(NULL)
    {
    }

    TYandexLemmaGramIter(const TYandexLemma& lemma)
        : StemGram(lemma.GetStemGram()), FlexGram(lemma.GetFlexGram(), lemma.FlexGramNum())
    {
        StemGramBitSet = TGramBitSet::FromBytes(StemGram);
        // when there is no @FlexGram, the iterator still will do a single iteration over @StemGram
        if (!FlexGram.Ok() && StemGramBitSet.any())
            FlexGram.Reset(&StemGram, &StemGram + 1);
    }

    inline bool Ok() const {
        return FlexGram.Ok();
    }

    inline void operator++() {
        ++FlexGram;
    }

    // return a COPY instead of ref
    typedef TGramBitSet TValueRef;

    inline TGramBitSet operator*() const {
        return StemGramBitSet | TGramBitSet::FromBytes(*FlexGram);
    }

private:
    typedef const char* TGramStr;

    TGramStr StemGram;
    TGramBitSet StemGramBitSet;
    NIter::TVectorIterator<const TGramStr> FlexGram;
};

typedef NIter::TValueIterator<const TWtringBuf> TDefaultExactFormIter;


class TWideCharTester {
public:
    inline bool IsUpper(wchar16 ch) const {
        return ::IsUpper(static_cast<wchar32>(ch));
    }

    inline bool IsLower(wchar16 ch) const {
        return ::IsLower(static_cast<wchar32>(ch));
    }

    inline bool IsDigit(wchar16 ch) const {
        return ::IsDigit(static_cast<wchar32>(ch));
    }
};

template <ECharset Encoding>
class TFixedEncodingCharTester {
public:
    inline TFixedEncodingCharTester() {
        YASSERT(SingleByteCodepage(Encoding));
        CP = CodePageByCharset(Encoding);
        YASSERT(CP != NULL);
    }

    inline bool IsUpper(char ch) const {
        return CP->IsUpper(static_cast<ui8>(ch));
    }

    inline bool IsLower(char ch) const {
        return CP->IsLower(static_cast<ui8>(ch));
    }

    inline bool IsDigit(char ch) const {
        return CP->IsDigit(static_cast<ui8>(ch));
    }

private:
    const CodePage* CP;
};


template <typename TInput>
class TWordIteratorTraits {
public:
    // following types should be defined for each TInput specialization:

    //default variant
    typedef wchar16 TChar;
    typedef Wtroka TWordString;
    typedef NIter::TValueIterator<const TWordString> TExactFormIter;
    typedef TStemFlexGramIterator TGrammemIter;
    typedef TWideCharTester TCharTester;
};


template <typename TInput>
class TWordIterator
{
public:
    typedef TWordIteratorTraits<TInput> TTraits;
    typedef typename TTraits::TChar TChar;
    typedef typename TTraits::TWordString TWordString;
    typedef typename TTraits::TExactFormIter TExactFormIter;
    typedef typename TTraits::TLemmaIter TLemmaIter;
    typedef typename TTraits::TGrammemIter TGrammemIter;
    typedef typename TTraits::TCharTester TCharTester;

    inline TWordIterator()
        : Input(NULL), WordCount(0), Ok_(false)
    {
    }

    explicit inline TWordIterator(const TInput& input)
        : Input(&input), WordCount(0), Ok_(true)
    {
        // skip to the first word or to the end of input.
        operator++();
    }

    inline TWordIterator& operator++() {
        Ok_ = Next();
        return *this;
    }

    inline bool Ok() const {
        return Ok_;
    }

    // next 4 methods - should be defined for each specialization of TInput (they don't have default template implementation)

    TWordString GetWordString(size_t word_index) const;     // this is assumed to be normalized text of original source word
    TLemmaIter IterLemmas(size_t word_index) const;
    static TWordString GetLemmaString(const TLemmaIter& lemma);     // here and in next similar methods @lemma must not be empty iterator!
    static TGrammemIter IterGrammems(const TLemmaIter& lemma);


    // This is exactly original word without normalizations (at least with original capitalization)
    // However, default implementation returns the same as GetWordString, i.e. all words will be treated
    // as lower-cased by capitalization filters.
    TWordString GetOriginalWordString(size_t word_index) const {
        return GetWordString(word_index);
    }

    // Default language is unknown (no language filtration)
    static inline docLanguage GetLanguage(const TLemmaIter& /*lemma*/) {
        return LANG_UNK;
    }

    // Iterate over other possible exact forms (used, for example, with untransliteration)
    // A default implementation is empty iterator
    TExactFormIter IterExtraForms(size_t /*word_index*/) const {
        return TExactFormIter();
    }

    // If true then this word will be analysed for sub-tokens
    // and each sub-token will be search consequetively
    bool HasSubTokens(size_t /*wordIndex*/) const {
        return false;
    }


    // Special object for checking if a char from input is upper or lower case.
    // Default checker (TWideCharTester) is for wide-chars only, it treats all input chars as wchar32
    // For single-byte strings use TFixedEncodingCharTester
    inline const TCharTester& CharTest() const {
        return CharTester_;
    }


    // Access to last word (with reverse number 1):

    inline TWordString GetWordString() const {
        return GetWordString(LastWordIndex());
    }

    TExactFormIter IterExtraForms() const {
        return IterExtraForms(LastWordIndex());
    }

    inline TLemmaIter IterLemmas() const {
        return IterLemmas(LastWordIndex());
    }

    bool HasSubTokens() const {
        return HasSubTokens(LastWordIndex());
    }

    size_t ReverseNumberToIndex(TReverseWordNumber word_number) const {
        YASSERT(WordCount >= word_number);
        return WordCount - word_number;
    }

    inline const TInput* GetInput() const {
        return Input;
    }

protected:
    size_t LastWordIndex() const {
        return ReverseNumberToIndex(1);
    };

private:

    // Switch to next word from Input.
    // Returns false and does nothing if we already at the end of input.
    // Default implementation - implies that TInput has size()
    bool Next() {
       if (WordCount < Input->size()) {
            WordCount += 1;
            return true;
        }
        return false;
    }

private: //data//
    const TInput* Input;
    size_t WordCount;       // number of words seen in input
    bool Ok_;

    TCharTester CharTester_;
};





// ============= Specializations of TWordIterator for different type of input text ==============


// TWordIterator specialization for yvector<Wtroka> with only EXACT_FORMS search
typedef TWordIterator<VectorWtrok> TVectorWtrokIterator;
template <> class TWordIteratorTraits<VectorWtrok> {
public:
    typedef wchar16 TChar;
    typedef Wtroka TWordString;
    typedef NIter::TValueIterator<const TWordString> TExactFormIter;
    typedef NIter::TVectorIterator<const Wtroka> TLemmaIter;
    typedef NIter::TVectorIterator<const TGramBitSet> TGrammemIter;
    typedef TWideCharTester TCharTester;
};


} // namespace NGzt
