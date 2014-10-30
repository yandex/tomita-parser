#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/intrlist.h>
#include <util/memory/pool.h>

#include <library/iter/iter.h>

#include "common/secondhand.h"
#include "common/tokeniterator.h"
#include "common/tools.h"

#include "worditerator.h"
#include "inputwords.h"
#include "articlefilter.h"
#include "gzttrie.h"

#include "searchtree.h"



namespace NGzt
{

// Defined in this file:
template <typename TInput> class TArticleIter;
template <typename TInput> class TPhraseSearchState;
template <typename TInput> class TCompoundSearchState;




template <typename TInput>
class TArticleIter
{
    template <typename T> friend class TPhraseSearchState;
    template <typename T> friend class TCompoundSearchState;
public:
    typedef typename TWordIterator<TInput>::TChar TChar;
    typedef typename TStringSelector<TChar>::TString TString;
    typedef typename TGztTrie::TWordTrie TWordTrie;
    typedef typename TGztTrie::TPhraseTrie TPhraseTrie;
    typedef typename TGztTrie::TCompoundTrie TCompoundTrie;

    typedef TInputWord<TInput> TWord;

    inline TArticleIter();
    inline TArticleIter(const TGztTrie* trie, const TInput& text);

    inline void Reset(const TGztTrie* trie, const TInput& text);

    inline bool Ok() const {
        return Ok_;
    }

    inline TArticleIter& operator++() {
        if (FindNext())
            ++ArticleCount;
        return *this;
    }

    inline TArticleId operator*() const {
        YASSERT(Ok());
        return ArticleId;
    }

    // Position, length, used lemmas of found article. Be careful - returned object is valid only on current iteration step.
    inline const TPhraseWords<TInput>& GetWords() const {
        YASSERT(ArticleWordsPtr != NULL);
        return *ArticleWordsPtr;
    }

    // Original object on which word iteration is being done.
    inline const TInput* GetOriginalInput() const {
        return WordsIter.GetInput();
    }

    TString DebugFoundPhrase() const {
        return GetWords().DebugString();
    }

private:
    bool FindNext();
    bool FindNextInWords();

    inline bool MatchFilter(const NAux::TFilter& filter, TPhraseWords<TInput>& words, const yvector<size_t>& indexTable) const;

private: // data

    bool Ok_;

    const TGztTrie* Trie;
    TWordIterator<TInput> WordsIter;

    // Current incomplete phrases on PhraseTrie - in progress
    // Essentially a bunch of nodes from PhraseTrie
    TPhraseSearchState<TInput> Phrases;

    // Compound articles in progress
    TCompoundSearchState<TInput> Compounds;

    // Current found article id (offset) - only non-fake real user-defined simple or complex article
    TArticleId ArticleId;

    // Effective phrase words for current found ArticleId
    // Could point to either Phrases.Phrase (for simple article) or Compounds.Phrase (for complex article)
    TPhraseWords<TInput>* ArticleWordsPtr;

    // performance counters;
    size_t WordCount, PhraseCount, ArticleCount;

    DECLARE_NOCOPY(TArticleIter);
};



template <typename TInput>
class TPhraseSearchState
{
public:
    typedef typename TArticleIter<TInput>::TPhraseTrie TPhraseTrie;

    inline TPhraseSearchState(TArticleIter<TInput>* article_iter);
    void Reset(const TPhraseTrie* trie);

    inline void TakeNextWord();
    bool FindNext();
    inline bool FindNextInFilters();
    inline bool FindNextInSimpleArticles();

    inline const TPhraseWords<TInput>& GetWords() const {
        return Phrase;
    }

private:
    typedef typename TArticleIter<TInput>::TWord TWord;
    typedef typename TWordIterator<TInput>::TWordString TWordStr;

    inline bool FindWord(const TWtringBuf& text, TWordId& foundId) {
        return ArticleIter.Trie->FindWord(text, foundId);
    }

    inline TWordId FindWordId(const TWtringBuf& text) {
        TWordId id = 0;
        FindWord(text, id);
        return id;
    }

    TWordId SplitMultitoken(const TWtringBuf& text);

    template <bool doLemma, bool doExact>
    void TryAppendPlainWordId(TWordId id, TWord word);     // non-multitoken

    template <bool doLemma, bool doExact>
    void TryAppendMultitoken(const TWtringBuf& text, TWord word);

    template <bool doLemma, bool doExact>
    TWordId TryAppendWord(const TWtringBuf& text, TWord word, bool subTokens);

    template <bool doLemma, bool doExact>
    void TryAppendWord(const TWtringBuf& text, TWordId wordId, TWord word, bool subTokens);


    inline void TryAppendAsLemma(TWordId wordId, TWord word) {
        if (TWordIdTraits::CheckAndForce<false>(wordId))
            SearchTree.TryAppendWord(wordId, word);
    }

    inline void TryAppendAsExactForm(TWordId wordId, TWord word = TWord()) {
        if (TWordIdTraits::CheckAndForce<true>(wordId))
            SearchTree.TryAppendWord(wordId, word);
    }

    inline void TryAppendMetaAnyWord();
    bool ResetFoundArticle(TArticleId articleId);


private: // data

    TArticleIter<TInput>& ArticleIter;

    typedef TSearchTree<TPhraseTrie, TWord> TPhraseSearchTree;
    typedef typename TPhraseSearchTree::TBranchIterator TBranchIterator;
    typedef typename TPhraseSearchTree::TBranch TBranch;

    TPhraseSearchTree SearchTree;
    yvector<TWordId> Subtokens;     // used locally in TakeNextWord()

    // Currently found phrases (possibly imcomplete)
    // first item of this iterator (if not empty) corresponds to PhraseWords

    TBranchIterator BranchQueue;

    // Current found complete phrase (a sequence of words without homonymy, i.e. with specified lemmas)
    // Could correspond to several articles (even to several same article keys), still no filters is checked yet for this phrase
    // Caution: valid only during single iteration step
    yvector<TWord> Words;
    yvector<size_t> IndexTable;
    TPhraseWords<TInput> Phrase;        // light-weight wrapper above Words

    // Current set of simple articles (i.e. without assigned filters), already found
    // first item of this iterator corresponds to currently found article - ArticleIter->ArticleId
    TArticleFilter::TSimpleArticleIterator SimpleArticles;

    // Current set of article filters to apply to Phrase,
    // first item of this iterator corresponds to currently found article - ArticleIter->ArticleId
    TArticleFilter::TFilterIterator Filters;
};


template <typename TInput>
class TCompoundSearchState
{
public:
    typedef TPair<TArticleId, size_t> TFoundArticle;        // article_id + phrase length (in words)
    typedef typename TArticleIter<TInput>::TWord TWord;
    typedef typename TArticleIter<TInput>::TCompoundTrie TCompoundTrie;

    inline TCompoundSearchState(TArticleIter<TInput>* article_iter);
    void Reset(const TCompoundTrie* trie);

    inline void TakeNextArticle(TArticleId id);

    bool FindNext();
    inline bool FindNextInFilters();
    inline bool FindNextInSimpleArticles();

private:
    class TSaveGramPhraseWordIter: public NIter::TVectorIterator<const TWord> {
        typedef NIter::TVectorIterator<const TWord> TBase;
    public:
        TSaveGramPhraseWordIter(const TPhraseWords<TInput>& words)
            : TBase(*words.GetItems())
        {
        }

        TWord operator*() const {
            TWord ret = TBase::operator*();
            ret.SaveGrammems();
            return ret;
        }
    };

    // appends article @id to each item from Items
    inline void TryAppendArticle(TArticleId id, const TPhraseWords<TInput>& articleWords);
    bool ResetFoundArticle(TArticleId articleId);


private: // data
    TArticleIter<TInput>& ArticleIter;

    typedef TFullSearchTree<TCompoundTrie, TWord> TCompoundSearchTree;
    typedef typename TCompoundSearchTree::TBranchIterator TBranchIterator;
    typedef typename TCompoundSearchTree::TBranch TBranch;

    TCompoundSearchTree SearchTree;
    TBranchIterator BranchQueue;

    yvector<TWord> Words;
    yvector<size_t> IndexTable;
    TPhraseWords<TInput> Phrase;

    TArticleFilter::TSimpleArticleIterator SimpleArticles;
    TArticleFilter::TFilterIterator Filters;

    yvector<TFoundArticle> FoundArticles;
};





// ================== templates implementation =====================



// TArticleIter ==============================================

template <typename TInput>
inline TArticleIter<TInput>::TArticleIter()
    : Ok_(false)
    , Trie(NULL)
    , WordsIter()

    , Phrases(this)
    , Compounds(this)

    , ArticleId(0)
    , ArticleWordsPtr(NULL)

    , WordCount(0), PhraseCount(0), ArticleCount(0)
{
}

template <typename TInput>
inline TArticleIter<TInput>::TArticleIter(const TGztTrie* trie, const TInput& text)
    : Ok_(true)
    , Trie(trie)
    , WordsIter(text)

    , Phrases(this)
    , Compounds(this)

    , ArticleId(0)
    , ArticleWordsPtr(NULL)

    , WordCount(0), PhraseCount(0), ArticleCount(0)
{
    if (FindNextInWords())
        ArticleCount += 1;
}

template <typename TInput>
inline void TArticleIter<TInput>::Reset(const TGztTrie* trie, const TInput& text)
{
    Ok_ = true;
    Trie = trie;
    WordsIter = TWordIterator<TInput>(text);

    YASSERT(trie != NULL);

    Phrases.Reset(&trie->PhraseTrie);
    Compounds.Reset(&trie->CompoundTrie);

    ArticleId = 0;
    ArticleWordsPtr = NULL;

    WordCount = PhraseCount = ArticleCount = 0;

    if (FindNextInWords())
        ArticleCount += 1;
}

/*
the main loops of iteration (if written in Python generators style):

for word in input:
    collect word_ids from exact_form_trie and lemma_trie corresponding to @word
    for word_id in word_ids:
        append word_id to each of previously found chains of word-ids from phrase_trie (phrases_in_progress)
        also search this word_id from root of phrase_trie and append new chain to phrases_in_progress (if any)

    for phrase in phrases_in_progress:
        if phrase corresponds to some articles keys text:
            get associated with this articles filters (grammems, agreements, letter case, etc.) = filters_to_check
            for filter in filters_to_check:
                if phrase matches to the filter:
                    we have found an article
                    if article is not fake:
                        yield article


                    already_found_composite_articles = []
                    tail_articles_to_check = [article]

                    for tail_article in tail_articles_to_check:
                        removed tail_article from tail_articles_to_check
                        for composite in complex_in_progress:
                            if composite could be continued with tail_article:
                                append tail_article to composite (this would be next_level_complex)
                                if next_level_complex corresponds to some composite_article:
                                    if composite_article is not in already_found_composite_articles:
                                        get filters associated with this composite_article (complex_filters_to_check)
                                        for complex_filter in complex_filters_to_check:
                                            if complex_filter is passed:
                                                yield corresponding composite_article

                                            add composite_article to already_found_composite_articles
                                            append composite_article to tail_articles_to_check

*/


template <typename TInput>
bool TArticleIter<TInput>::FindNext()
{
    if (Compounds.FindNextInSimpleArticles() ||
        Compounds.FindNextInFilters() ||
        Compounds.FindNext())
        return true;

    if (Phrases.FindNextInSimpleArticles() ||
        Phrases.FindNextInFilters() ||
        Phrases.FindNext())
        return true;

    ++WordsIter;       // no more phrases in current word, read next one.
    return FindNextInWords();
}


template <typename TInput>
bool TArticleIter<TInput>::FindNextInWords()
{
    for (; WordsIter.Ok(); ++WordsIter)
    {
        WordCount += 1;
        Phrases.TakeNextWord();
        if (Phrases.FindNext())
            // note that current word is not changed now and remains the same during
            // subsequent sub-iterations (over phrases and filters to check)
            return true;
    }
    Ok_ = false;
    return false;
}

template <typename TInput>
inline bool TArticleIter<TInput>::MatchFilter(const NAux::TFilter& filter, TPhraseWords<TInput>& words, const yvector<size_t>& indexTable) const {
    if (EXPECT_TRUE(indexTable.empty()))
        return Trie->ArticleFilter.Match(filter, words);
    else
        return Trie->ArticleFilter.Match(filter, words, indexTable);
}

// TPhraseSearchState<TInput> =========================

template <typename TInput>
inline TPhraseSearchState<TInput>::TPhraseSearchState(TArticleIter<TInput>* article_iter)
    : ArticleIter(*article_iter)
    , SearchTree(ArticleIter.Trie ? &ArticleIter.Trie->PhraseTrie : NULL)
    , BranchQueue(SearchTree.IterBranches())
    , Phrase(ArticleIter.WordsIter, NULL)
{
}

template <typename TInput>
void TPhraseSearchState<TInput>::Reset(const TPhraseTrie* trie)
{
    SearchTree.Reset(trie);
    BranchQueue = SearchTree.IterBranches();
    Phrase.Reset(ArticleIter.WordsIter, NULL);
    Filters = TArticleFilter::TFilterIterator();
    SimpleArticles = TArticleFilter::TSimpleArticleIterator();
}

template <typename TInput>
TWordId TPhraseSearchState<TInput>::SplitMultitoken(const TWtringBuf& text) {
    TWordId mask = TWordIdTraits::EXACT_FORM_MASK | TWordIdTraits::LEMMA_MASK;
    TWordId id = 0;
    Subtokens.clear();
    for (TSubtokenIterator tok(text); tok.Ok(); ++tok) {
        if (!FindWord(*tok, id))
            return 0;
        mask &= id;
        if (mask == 0)      // at least one of flags from initial mask should remain.
            return 0;
        Subtokens.push_back(id);
    }
    return Subtokens.empty() ? 0 : mask;
}

template <bool exact>
class TForceMaskIterator: public NIter::TVectorIterator<const TWordId> {
    typedef TVectorIterator<const TWordId> TBase;
    using TBase::operator->;
    using TBase::operator*;
public:
    TForceMaskIterator(const yvector<TWordId>& ids)
        : TBase(ids)
    {
    }

    TWordId operator*() const {
        YASSERT(TWordIdTraits::Check<exact>(TBase::operator*()));
        return TBase::operator*() & TWordIdTraits::ForceMask<exact>();
    }
};

template <typename TInput>
template <bool doLemma, bool doExact>
inline void TPhraseSearchState<TInput>::TryAppendMultitoken(const TWtringBuf& text, TWord word) {
    TWordId mask = SplitMultitoken(text);
    if (doLemma && TWordIdTraits::IsLemma(mask))
        SearchTree.TryAppendMultitoken(TForceMaskIterator<false>(Subtokens), word);
    if (doExact && TWordIdTraits::IsExactForm(mask))
        SearchTree.TryAppendMultitoken(TForceMaskIterator<true>(Subtokens), word);
}

template <typename TInput>
template <bool doLemma, bool doExact>
inline void TPhraseSearchState<TInput>::TryAppendPlainWordId(TWordId wordId, TWord word) {
    // TODO: change an order: append as lemma first
    if (doExact)
        TryAppendAsExactForm(wordId, word);
    if (doLemma)
        TryAppendAsLemma(wordId, word);
}

template <typename TInput>
template <bool doLemma, bool doExact>
inline void TPhraseSearchState<TInput>::TryAppendWord(const TWtringBuf& text, TWordId wordId, TWord word, bool subTokens) {
    if (wordId != 0)
        TryAppendPlainWordId<doLemma, doExact>(wordId, word);

    if (subTokens)
        TryAppendMultitoken<doLemma, doExact>(text, word);
}

template <typename TInput>
template <bool doLemma, bool doExact>
inline TWordId TPhraseSearchState<TInput>::TryAppendWord(const TWtringBuf& text, TWord word, bool subTokens) {
    TWordId wordId = FindWordId(text);
    TryAppendWord<doLemma, doExact>(text, wordId, word, subTokens);
    return wordId;
}

template <typename TInput>
inline void TPhraseSearchState<TInput>::TakeNextWord()
{
    typedef typename TWordIterator<TInput>::TExactFormIter TExactFormIterator;
    typedef typename TWordIterator<TInput>::TLemmaIter TLemmaIterator;

    const TWordIterator<TInput>& wordIter = ArticleIter.WordsIter;
    bool checkSubTokens = wordIter.HasSubTokens();

    // also search from root (for sequences starting with current word)
    SearchTree.AppendRoot();

    TWordStr exactForm = wordIter.GetWordString();
    TWordId exactFormId = FindWordId(exactForm);
    bool hasSameLemma = false;

    // check all possible lemmas
    for (TLemmaIterator lemmaIter = wordIter.IterLemmas(); lemmaIter.Ok(); ++lemmaIter) {
        TWordId lemmaId = TryAppendWord<true, false>(wordIter.GetLemmaString(lemmaIter), TWord(lemmaIter), checkSubTokens);
        if (exactFormId != 0 && lemmaId == exactFormId)
            hasSameLemma = true;
    }

    // check exact wordform
    if (hasSameLemma)
        TryAppendWord<false, true>(exactForm, exactFormId, TWord(), checkSubTokens);
    else
        TryAppendWord<true, true>(exactForm, exactFormId, TWord(), checkSubTokens);

    // finally seach alternative exact forms
    for (TExactFormIterator exactIt = wordIter.IterExtraForms(); exactIt.Ok(); ++exactIt)
        TryAppendWord<true, true>(*exactIt, TWord(), checkSubTokens);

    // if should check for wildcard "*" - then append again lemmas and exact form
    TryAppendMetaAnyWord();

    SearchTree.SwitchNextState();
    BranchQueue = SearchTree.IterBranches();
}

template <typename TInput>
inline void TPhraseSearchState<TInput>::TryAppendMetaAnyWord() {
    if (ArticleIter.Trie->HasMetaAnyWord()) {
        typename TWordIterator<TInput>::TLemmaIter lemmaIter = ArticleIter.WordsIter.IterLemmas();
        if (lemmaIter.Ok())
            for (; lemmaIter.Ok(); ++lemmaIter)
                SearchTree.TryAppendWord(ArticleIter.Trie->MetaAnyWordId, TWord::AnyWord(lemmaIter));
        else
            SearchTree.TryAppendWord(ArticleIter.Trie->MetaAnyWordId, TWord::AnyWord());
    }
}

template <typename TInput>
bool TPhraseSearchState<TInput>::FindNext() {
    while (BranchQueue.Ok()) {
        const TBranch& branch = *BranchQueue;
        ++BranchQueue;

        TArticleBucket bucket;
        //check if built phrase has a final state (meaning it is a complete phrase with associated articles)
        if (branch.GetFinalData(bucket)) {
            branch.CopyTo(Words, IndexTable);
            Phrase.Reset(ArticleIter.WordsIter, &Words);
            Filters = ArticleIter.Trie->ArticleFilter.IterFiltered(bucket.FilterIndex);
            SimpleArticles = ArticleIter.Trie->ArticleFilter.IterSimple(bucket.SimpleIndex);

            if (FindNextInSimpleArticles() || FindNextInFilters())
                return true;
        }
    }
    return false;
}

template <typename TInput>
inline bool TPhraseSearchState<TInput>::FindNextInFilters()
{
    // perform filter check for found articles.
    while (Filters.Ok()) {
        Phrase.ResetGrammems();
        bool found = ArticleIter.MatchFilter(*Filters, Phrase, IndexTable) &&
                     ResetFoundArticle(Filters->ArticleId);
        ++Filters;
        if (found)
            return true;
    }
    return false;
}

template <typename TInput>
inline bool TPhraseSearchState<TInput>::FindNextInSimpleArticles()
{
    Phrase.ResetGrammems();
    while (SimpleArticles.Ok()) {
        bool found = ResetFoundArticle(*SimpleArticles);
        ++SimpleArticles;
        if (found)
            return true;
    }
    return false;
}

template <typename TInput>
bool TPhraseSearchState<TInput>::ResetFoundArticle(TArticleId articleId) {
    // We have just found an article, it could be either user article (defined in gzt-file)
    // or fake article as part of other complex article.
    // In both case we should pass this article further to CompoundTrie
    bool fake = IsFakeArticle(articleId);
    if (ArticleIter.Trie->IsCompoundElement(articleId)) {
        ArticleIter.Compounds.TakeNextArticle(articleId);
        if (fake)
            return ArticleIter.Compounds.FindNext();
    }

    if (fake)
        return false;

    // otherwise just reset to this article
    ArticleIter.ArticleId = articleId;
    ArticleIter.ArticleWordsPtr = &Phrase;
    return true;

}


// TCompoundSearchState<TInput> =========================

template <typename TInput>
inline TCompoundSearchState<TInput>::TCompoundSearchState(TArticleIter<TInput>* article_iter)
    : ArticleIter(*article_iter)
    , SearchTree(ArticleIter.Trie ? &ArticleIter.Trie->CompoundTrie : NULL)
    , BranchQueue(SearchTree.IterBranches())
    , Phrase(ArticleIter.WordsIter, NULL)
{
}

template <typename TInput>
void TCompoundSearchState<TInput>::Reset(const TCompoundTrie* trie)
{
    SearchTree.Reset(trie);
    BranchQueue = SearchTree.IterBranches();

    Phrase.Reset(ArticleIter.WordsIter, NULL);
    Filters = TArticleFilter::TFilterIterator();
    SimpleArticles = TArticleFilter::TSimpleArticleIterator();

    FoundArticles.clear();
}

template <typename TInput>
inline void TCompoundSearchState<TInput>::TakeNextArticle(TArticleId id)
{
    // we always start with non-composite article (i.e. from PhraseWords), but it could be a fake article (not defined by user explicitly).

    // here we collect all compound articles found during this step (they all will have current word from input as their last word)
    FoundArticles.clear();

//    LastProcessed = State.Last();
    TryAppendArticle(id, ArticleIter.Phrases.GetWords());
}

template <typename TInput>
inline void TCompoundSearchState<TInput>::TryAppendArticle(TArticleId id, const TPhraseWords<TInput>& articleWords)
{
    // special iterator over words calling SaveGrammems() on dereferencing
    TSaveGramPhraseWordIter wordIter(articleWords);
    SearchTree.TryAppendSequence(id, wordIter, articleWords.Pos(), articleWords.Size());
}

template <typename TInput>
bool TCompoundSearchState<TInput>::FindNext() {
    while (BranchQueue.Ok()) {
        const TBranch& branch = *BranchQueue;
        ++BranchQueue;

        TArticleBucket bucket;
        //check if empty string is in trie (meaning this is a leaf with final complex article)
        if (branch.GetFinalData(bucket)) {
            branch.CopyTo(Words, IndexTable);
            Phrase.Reset(ArticleIter.WordsIter, &Words);
            Filters = ArticleIter.Trie->ArticleFilter.IterFiltered(bucket.FilterIndex);
            SimpleArticles = ArticleIter.Trie->ArticleFilter.IterSimple(bucket.SimpleIndex);

            if (FindNextInSimpleArticles() || FindNextInFilters())
                return true;
        }
    }
    return false;
}

template <typename TInput>
inline bool TCompoundSearchState<TInput>::FindNextInFilters()
{
    // perform filter check for found articles.
    while (Filters.Ok()) {
        Phrase.ResetGrammems();
        bool found = ArticleIter.MatchFilter(*Filters, Phrase, IndexTable) &&
                     ResetFoundArticle(Filters->ArticleId);
        ++Filters;
        if (found)
            return true;
    }
    return false;
}



template <typename TInput>
inline bool TCompoundSearchState<TInput>::FindNextInSimpleArticles()
{
    Phrase.ResetGrammems();
    while (SimpleArticles.Ok()) {
        bool found = ResetFoundArticle(*SimpleArticles);
        ++SimpleArticles;
        if (found)
            return true;
    }
    return false;
}


template <typename TInput>
bool TCompoundSearchState<TInput>::ResetFoundArticle(TArticleId articleId) {
    size_t found_article_size = Phrase.Size();

    // make sure we have not already have this article in FoundArticles on same words (to avoid infinite recursion)
    // TODO: investigate if using yset<TFoundArticle> is faster rather than yvector for such checking
    //       or move this check out of filtering "if" block
    bool already_found = false;
    for (size_t i = 0; i < FoundArticles.size(); ++i)
        if (FoundArticles[i].first == articleId && FoundArticles[i].second == found_article_size) {
            already_found = true;
            break;
        }

    if (already_found)
        return false;

    FoundArticles.push_back(MakePair(articleId, found_article_size));

    if (ArticleIter.Trie->IsCompoundElement(articleId)) {
        // append this article to Items as well - it can be a component of other more complex article
        TryAppendArticle(articleId, Phrase);
    }

    if (IsFakeArticle(articleId))
        return false;

    // and return this found article to user
    ArticleIter.ArticleId = articleId;
    ArticleIter.ArticleWordsPtr = &Phrase;

    return true;
}


} // namespace NGzt
