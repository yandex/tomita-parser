#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/singleton.h>

#include <library/iter/iter.h>

#include "common/compactstorage.h"
#include "common/packedint.h"
#include "common/serialize.h"
#include "filterdata.h"
#include "inputwords.h"


// forward declarations from base.proto
class TSearchKey;

DECLARE_BYTESIZE_PODTYPE(TGramBitSet);
DECLARE_BYTESIZE_PODTYPE(TLangMask);

namespace NGzt
{

//defined in this file:
class TArticleFilterBuilder;
class TArticleFilter;

typedef NAux::TArticleBucket TArticleBucket;

// forward declarations from binary.proto
namespace NBinaryFormat {
    class TWordIndexStorage;
    class TBitSetStorage;
    class TArticleFilter;
}

// specialization of TStorageProtoSerializer for TWordIndex, TGramBitSet and TLangMask
template <>
struct TStorageProtoSerializer<NBinaryFormat::TWordIndexStorage> {
    static void Save(NBinaryFormat::TWordIndexStorage& proto, TWordIndex obj);
    static void Load(const NBinaryFormat::TWordIndexStorage& proto, size_t index, TWordIndex& obj);
};

template <>
struct TStorageProtoSerializer<NBinaryFormat::TBitSetStorage> {
    template <typename TBitSet> static void Save(NBinaryFormat::TBitSetStorage& proto, const TBitSet& obj);
    template <typename TBitSet> static void Load(const NBinaryFormat::TBitSetStorage& proto, size_t index, TBitSet& obj);
};



class TArticleFilterBuilder
{
    friend class TArticleFilter;
public:
    TArticleFilterBuilder()
        : FilteredArticles(*this)
        , EmptyFilterDataIndex(AddEmptyFilterData())
    {
    }

    TArticleBucket Add(const TSearchKey& key, const yvector<ui64>& capitalization,
                       TArticleId article_id, size_t phrase_length);
    TArticleBucket AddTo(TArticleBucket bucket, const TSearchKey& key, const yvector<ui64>& capitalization,
                         TArticleId article_id, size_t phrase_length);

    inline NIter::TIterator<NAux::TFilter> IterFiltered(NAux::TFilterIndex index) const {
        return FilteredArticles[index];
    }

    //typedef TPackedIntVectorsVectorBuilder<TArticleId> TSimpleArticleStorageBuilder;
    //typedef TSimpleArticleStorageBuilder::TSubVectorIter TSimpleArticleIterator;
    typedef TCompactStorageBuilder<TArticleId, false> TSimpleArticleStorageBuilder;
    typedef NIter::TIterator<TArticleId> TSimpleArticleIterator;

    inline TSimpleArticleIterator IterSimple(NAux::TSimpleArticleIndex index) const {
        return SimpleArticles[index];
    }

    void Save(TOutputStream* out) const;
    void Save(NBinaryFormat::TArticleFilter& proto, TBlobCollection& blobs) const;

    size_t ByteSize() const {
        return ::ByteSize(Grammems) + ::ByteSize(WordGrammems)
             + ::ByteSize(LangMasks) + ::ByteSize(WordLanguages)
             + ::ByteSize(Capitalizations) + ::ByteSize(WordCases)
             + ::ByteSize(GleicheWords) + ::ByteSize(Agreements)
             + ::ByteSize(FilterData)
             + ::ByteSize(FilteredArticles) + ::ByteSize(SimpleArticles);
    }

    Stroka DebugByteSize() const {
        TStringStream str;
        str << "ArticleFilter:\n" << DEBUG_BYTESIZE(FilterData) << DEBUG_BYTESIZE(FilteredArticles) << DEBUG_BYTESIZE(SimpleArticles);
        return str;
    }

private:
    NAux::TWordGramIndex AddWordGrams(const ymap<TWordIndex, yvector<TGramBitSet> >& word_grams);
    NAux::TWordLangIndex AddWordLangs(const ymap<TWordIndex, TLangMask>& word_langs);
    NAux::TWordCaseIndex AddWordCases(ymap<TWordIndex, yset<NAux::TCapitalization> >& word_cases,
                                      const yvector<ui64>& capitalization_masks);

    void MakeWordGramFilter(const TSearchKey& key, size_t phrase_length, NAux::TWordGramIndex& allowed, NAux::TWordGramIndex& forbidden);
    void MakeWordLangFilter(const TSearchKey& key, size_t phrase_length, NAux::TWordLangIndex& allowed, NAux::TWordLangIndex& forbidden);
    void MakeWordCaseFilter(const TSearchKey& key, size_t phrase_length, NAux::TWordCaseIndex& allowed, NAux::TWordCaseIndex& forbidden,
                            const yvector<ui64>& capitalization);

    NAux::TAgrIndex MakeAgrFilter(const TSearchKey& key, size_t phrase_length);

    NAux::TFilterDataIndex AddFilterData(const TSearchKey& key, const yvector<ui64>& capitalization, size_t phrase_length);
    NAux::TFilterDataIndex AddEmptyFilterData();    // called once in constructor

    inline bool IsEmptyFilter(NAux::TFilterDataIndex index) const {
        return index == EmptyFilterDataIndex;
    }

    NAux::TFilterIndex RemoveArticleFilters(NAux::TFilterIndex filterIndex, TArticleId articleId);

    friend class TFilterStorageBuilder;
    NAux::TFilter MergeFilters(const NAux::TFilter& f1, const NAux::TFilter& f2);

private:
    TCompactStorageBuilder<TGramBitSet> Grammems;
    TCompactStorageBuilder<NAux::TWordGram> WordGrammems;

    TCompactStorageBuilder<TLangMask> LangMasks;
    TCompactStorageBuilder<NAux::TWordLang> WordLanguages;

    TCompactStorageBuilder<NAux::TCapitalization> Capitalizations;
    TCompactStorageBuilder<NAux::TWordCase> WordCases;

    TCompactStorageBuilder<TWordIndex> GleicheWords;
    TCompactStorageBuilder<NAux::TAgr> Agreements;

    TCompactStorageBuilder<NAux::TFilterData> FilterData;

    //TCompactStorageBuilder<NAux::TFilter> Filters;
    TFilterStorageBuilder FilteredArticles;                 // Stores articles with non-empty filters

    //TCompactStorageBuilder<TArticleId, false> SimpleArticles;      // Stores articles without filters (just TArticleId sets)
    TSimpleArticleStorageBuilder SimpleArticles;   // Stores articles without filters (just TArticleId sets)


    const NAux::TFilterDataIndex EmptyFilterDataIndex;
};


class TArticleFilter
{
public:
    typedef TCompactStorage<NAux::TFilter>::TIter TFilterIterator;
    typedef TCompactStorage<TArticleId>::TIter TSimpleArticleIterator;
    //typedef TPackedIntVectorsVector<TArticleId>::TSubVectorIter TSimpleArticleIterator;

    void Load(TMemoryInput* input);
    void Load(const NBinaryFormat::TArticleFilter& proto, const TBlobCollection& blobs);

    inline TFilterIterator IterFiltered(NAux::TFilterIndex index) const {
        return FilteredArticles[index];
    }

    inline TSimpleArticleIterator IterSimple(NAux::TSimpleArticleIndex index) const {
        return SimpleArticles[index];
    }

    template <typename TInput>
    inline bool Match(const NAux::TFilter& filter, TPhraseWords<TInput>& words) const;

    template <typename TInput>
    inline bool Match(const NAux::TFilter& filter, TPhraseWords<TInput>& words, const yvector<size_t>& indexTable) const;

private:
    class TNoTransform {
    public:
        inline size_t operator()(size_t index) const {
            return index;
        }
    };

    class TTableTransform {
    public:
        TTableTransform(const yvector<size_t>& table)
            : Table(table)
        {
        }

        inline size_t operator()(size_t index) const {
            return Table[index];
        }

    private:
        const yvector<size_t>& Table;
    };

    template <typename TInput, typename TIndexTransform = TNoTransform>
    class TMatcher {
    public:
        // Takes non-const TPhraseWords<TInput>& words, it is important
        // Based on result of filtering it could remove some homoforms of a word.
        TMatcher(const TArticleFilter& filter, TPhraseWords<TInput>& words, const TIndexTransform& index)
            : Filter(filter)
            , Words(words)
            , CorrectIndex(index)
        {
        }

        inline bool Match(const NAux::TFilter& filter) const;

        // Grammems not only checked but also filtered, so the name of this method differs from other.
        bool FilterGrammems(NAux::TWordGramIndex wg_index, bool allow) const;

        bool CheckLanguage(NAux::TWordLangIndex wl_index, bool allow) const;
        bool CheckCapitalization(NAux::TWordCaseIndex wc_index, bool allow) const;
        bool CheckSingleWordCapitalization(size_t word_index, const NAux::TCapitalization& capitalization, bool allow) const;
        bool CheckAgreements(NAux::TAgrIndex agr_index) const;
        bool CheckSingleAgreement(const NAux::TAgr& agr) const;


    private:
        const TArticleFilter& Filter;
        TPhraseWords<TInput>& Words;
        TIndexTransform CorrectIndex;       // used as CorrectIndex(size_t)
    };



private: //data//
    TCompactStorage<TGramBitSet> Grammems;
    TCompactStorage<NAux::TWordGram> WordGrammems;

    TCompactStorage<TLangMask> LangMasks;
    TCompactStorage<NAux::TWordLang> WordLanguages;

    TCompactStorage<NAux::TCapitalization> Capitalizations;
    TCompactStorage<NAux::TWordCase> WordCases;

    TCompactStorage<TWordIndex> GleicheWords;
    TCompactStorage<NAux::TAgr> Agreements;

    TCompactStorage<NAux::TFilterData> FilterData;
    TCompactStorage<NAux::TFilter> FilteredArticles;    // Articles with non-empty filters
    TCompactStorage<TArticleId> SimpleArticles;         // Articles without filters (just TArticleId sets)
    //TPackedIntVectorsVector<TArticleId> SimpleArticles;   // Articles without filters (just TArticleId sets)
};



namespace NGleiche {
    void StrongNormalize(TGramBitSet& gram, TMask mask);
    bool IsAgreed(const TGramBitSet& common_grammems, TMask mask);
}   // namespace NGleiche


bool ParseSingleGrammemSet(TStringBuf grammems, TGramBitSet& result);




// template definitions

template <typename TInput>
inline bool TArticleFilter::Match(const NAux::TFilter& filter, TPhraseWords<TInput>& words) const {
    return TMatcher<TInput, TNoTransform>(*this, words, TNoTransform()).Match(filter);
}

template <typename TInput>
inline bool TArticleFilter::Match(const NAux::TFilter& filter, TPhraseWords<TInput>& words, const yvector<size_t>& indexTable) const {
    TTableTransform transform(indexTable);
    return TMatcher<TInput, TTableTransform>(*this, words, transform).Match(filter);
}

template <typename TInput, typename TIndexTransform>
inline bool TArticleFilter::TMatcher<TInput, TIndexTransform>::Match(const NAux::TFilter& filter) const
{
    const NAux::TFilterData& fdata = Filter.FilterData.GetSingleItem(filter.DataIndex);

    if (!FilterGrammems(fdata.ForbiddenGramIndex, false) ||
        !FilterGrammems(fdata.AllowedGramIndex, true))
        return false;

    if (!CheckAgreements(fdata.AgrIndex))
        return false;

    if (!CheckLanguage(fdata.ForbiddenLangIndex, false) ||
        !CheckLanguage(fdata.AllowedLangIndex, true))
        return false;

    if (!CheckCapitalization(fdata.ForbiddenCaseIndex, false) ||
        !CheckCapitalization(fdata.AllowedCaseIndex, true))
        return false;

    return true;
}

template <typename TInput, typename TIndexTransform>
bool TArticleFilter::TMatcher<TInput, TIndexTransform>::FilterGrammems(NAux::TWordGramIndex wg_index, bool allow) const
{
    // TODO: fix case when there are several item in @wg with same Word (this is related to building of filters)
    for (TCompactStorage<NAux::TWordGram>::TIter wg = Filter.WordGrammems[wg_index]; wg.Ok(); ++wg) {
        size_t word_index = CorrectIndex(wg->Word);
        TCompactStorage<TGramBitSet>::TIter gr_filter = Filter.Grammems[wg->Index];
        // If the word was found by exact form (i.e. it is written in text and it the key in same way)
        // but not as ANYWORD (mask '*')
        // do not apply grammar filters at all!
        // Plus, no-grammems matches any grammems set, so continue checking next gram filters
        if (!gr_filter.Ok() || Words.ExactWordFromKey(word_index))
            continue;

        typename TPhraseWords<TInput>::TGrammemIter gr_word = Words.IterGrammems(word_index);
        // TODO: factor this O(n*n) algorithm out, or do some caching.
        for (; gr_word.Ok(); ++gr_word) {
            TGramBitSet tmp = *gr_word;
            bool has_match = false;
            for (TCompactStorage<TGramBitSet>::TIter it = gr_filter; it.Ok(); ++it)
                if (tmp.HasAll(*it)) {
                    has_match = true;
                    break;
                }

            // remove non-matching form from current word
            if (has_match != allow)
                gr_word.RemoveCurrent();
        }

        // if we have already cleaned the word from all its forms, we may not check further
        if (!Words.HasGrammems(word_index))
            return false;
    }

    return true;
}

template <typename TInput, typename TIndexTransform>
bool TArticleFilter::TMatcher<TInput, TIndexTransform>::CheckLanguage(NAux::TWordLangIndex wl_index, bool allow) const
{
    for (TCompactStorage<NAux::TWordLang>::TIter wl = Filter.WordLanguages[wl_index]; wl.Ok(); ++wl) {
        TCompactStorage<TLangMask>::TIter lang_filter = Filter.LangMasks[wl->Index];

        size_t word_index = CorrectIndex(wl->Word);

        // skip language checks for exact forms only (do filter lemmas with LANG_UNK!)
        if (!Words.IsLemma(word_index))
            continue;

        docLanguage word_lang = Words.GetLanguage(word_index);
        bool has_match = false;
        for (; lang_filter.Ok(); ++lang_filter)
            if (lang_filter->SafeTest(word_lang)) {
                has_match = true;
                break;
            }
        if (has_match != allow)
            return false;
    }
    return true;
}

template <typename TInput, typename TIndexTransform>
bool TArticleFilter::TMatcher<TInput, TIndexTransform>::CheckCapitalization(NAux::TWordCaseIndex wc_index, bool allow) const
{
    // TODO: fix case when there are several item in @wc with same Word (this is related to building of filters)
    for (TCompactStorage<NAux::TWordCase>::TIter wc = Filter.WordCases[wc_index]; wc.Ok(); ++wc) {
        TCompactStorage<NAux::TCapitalization>::TIter cap_filter = Filter.Capitalizations[wc->Index];
        size_t word_index = CorrectIndex(wc->Word);
        bool has_match = false;
        for (; cap_filter.Ok(); ++cap_filter)
            if (CheckSingleWordCapitalization(word_index, *cap_filter, allow)) {
                has_match = true;
                break;
            }
        if (has_match != allow)
            return false;
    }
    return true;
}

template <typename TInput, typename TIndexTransform>
bool TArticleFilter::TMatcher<TInput, TIndexTransform>::CheckSingleWordCapitalization(size_t word_index, const NAux::TCapitalization& capitalization, bool allow) const
{
    bool match;
    switch (capitalization.Type) {
        case TCapitalizationFilter::ANY:   return true;
        case TCapitalizationFilter::EXACT: return Words.IsMaskCase(word_index, capitalization.CustomMask);
        case TCapitalizationFilter::LOWER:
            match = Words.IsLowerCase(word_index);
            break;
        case TCapitalizationFilter::TITLE:
            match = Words.IsTitleCase(word_index);
            break;
        case TCapitalizationFilter::UPPER:
            match = Words.IsUpperCase(word_index);
            break;
        default:
            ythrow yexception() << "Unknown capitalization " << capitalization.Type;
    }
    return (allow || !match) ? match : !Words.StartsWithDigit(word_index);
}



template <typename TInput, typename TIndexTransform>
bool TArticleFilter::TMatcher<TInput, TIndexTransform>::CheckAgreements(NAux::TAgrIndex agr_index) const
{
    TCompactStorage<NAux::TAgr>::TIter agr_iter = Filter.Agreements[agr_index];
    for (; agr_iter.Ok(); ++agr_iter)
        if (!CheckSingleAgreement(*agr_iter))
            return false;
    return true;
}

template <typename TInput, typename TIndexTransform>
bool TArticleFilter::TMatcher<TInput, TIndexTransform>::CheckSingleAgreement(const NAux::TAgr& agr) const
{
    TCompactStorage<TWordIndex>::TIter word_iter = Filter.GleicheWords[agr.WordsIndex];

    // Attention: if the word is found by exact form (i.e. it is written in text and it the key in same way)
    // and it was not found with '*' mask
    // do not apply agreement filters to it at all!

    // Find the first found-by-lemma word in Words
    if (word_iter.Ok() && Words.ExactWordFromKey(CorrectIndex(*word_iter)))
        ++word_iter;

    // empty set of words is considered to be agreed
    if (!word_iter.Ok())
        return true;

    // agreed homoforms of all words in agreement set (take initial state from first word in set)
    yvector<TGramBitSet> agreed_forms;
    typename TPhraseWords<TInput>::TGrammemIter gram_it = Words.IterGrammems(CorrectIndex(*word_iter));
    for (; gram_it.Ok(); ++gram_it) {
        agreed_forms.push_back(*gram_it);
        NGleiche::StrongNormalize(agreed_forms.back(), agr.GleicheTypes);
    }
    int agreed_size = agreed_forms.size();

    for (++word_iter; word_iter.Ok(); ++word_iter) {
        size_t word_index = CorrectIndex(*word_iter);
        if (Words.ExactWordFromKey(word_index))
            continue;   // ignore found-by-exact-form words (and not found with '*' mask)

        for (int i = 0; i < agreed_size; ++i) {
            TGramBitSet& agreed_form = agreed_forms[i];
            bool is_agreed = false;
            // check if @agreed_form is agree with some of forms of current word
            for (gram_it = Words.IterGrammems(word_index); gram_it.Ok(); ++gram_it) {
                TGramBitSet form = *gram_it;
                NGleiche::StrongNormalize(form, agr.GleicheTypes);
                form &= agreed_form;
                if (NGleiche::IsAgreed(form, agr.GleicheTypes)) {
                    agreed_form = form;
                    is_agreed = true;
                    break;
                }
            }
            if (!is_agreed) {
                agreed_size -= 1;
                if (agreed_size == 0)
                    return false;
                // to remove this disagreed form just replace it with last one
                agreed_form = agreed_forms[agreed_size];
                i -= 1;
            }
        }
    }

    for (word_iter = Filter.GleicheWords[agr.WordsIndex]; word_iter.Ok(); ++word_iter) {
        size_t word_index = CorrectIndex(*word_iter);
        if (Words.ExactWordFromKey(word_index))
            continue;   // ignore found-by-exact-form words

        for (gram_it = Words.IterGrammems(word_index); gram_it.Ok(); ++gram_it) {
            bool is_agreed = false;
            TGramBitSet form = *gram_it;
            NGleiche::StrongNormalize(form, agr.GleicheTypes);
            for (int i = 0; i < agreed_size; ++i)
                if (NGleiche::IsAgreed(form & agreed_forms[i], agr.GleicheTypes)) {
                    is_agreed = true;
                    break;
                }
            if (!is_agreed)
                gram_it.RemoveCurrent();
        }
        if (!Words.HasGrammems(word_index))
            return false;
    }
    return true;
}


} // namespace NGzt

