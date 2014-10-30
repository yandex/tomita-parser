#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/set.h>
#include <util/generic/hash_set.h>
#include <util/memory/blob.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <util/stream/file.h>
#include <library/comptrie/comptrie.h>

#include "common/tools.h"
#include "common/recode.h"
#include "common/serialize.h"
#include "common/triebuilder.h"

#include "articlefilter.h"
#include "protopool.h"
#include "articlepool.h"
#include "tokenize.h"



// for TCompactTrieBuilder::TKey
namespace NGzt { typedef ui32 TWordId; }

template <>
struct TCharStringTraits<NGzt::TWordId> {
    typedef yvector<NGzt::TWordId> TString;

    struct TBuffer {
        const NGzt::TWordId* Beg;
        const size_t Len;

        TBuffer(const TString& s)
            : Beg(~s)
            , Len(+s)
        {}

        const NGzt::TWordId* operator~() const {
            return Beg;
        }

        size_t operator+() const {
            return Len;
        }
    };
};


namespace NGzt
{


// Defined in this file:
class TGztTrieBuilder;
class TGztTrie;

// Forward declarations from articleiter.h
template <typename TInput> class TArticleIter;
template <typename TInput> class TPhraseSearchState;
template <typename TInput> class TCompoundSearchState;


class TOptions;

// Forward declaration from binary.proto
namespace NBinaryFormat { class TGztTrie; }


inline bool IsFakeArticle(TArticleId id) {
    return id > TArticlePool::MaxValidOffset();
}

struct TWordIdTraits {
    static const TWordId EXACT_FORM_MASK = 1;
    static const TWordId LEMMA_MASK = 2;
    static const TWordId MAX_WORD_ID = ~((TWordId)0) >> 2;

    static inline bool IsExactForm(TWordId encoded) {
        return encoded & EXACT_FORM_MASK;
    }

    static inline bool IsLemma(TWordId encoded) {
        return encoded & LEMMA_MASK;
    }

    static inline TWordId Shift(TWordId word) {
        return word << 2;
    }

    static inline TWordId SetFlags(TWordId shifted, bool exact, bool lemma) {
        return shifted | (exact ? EXACT_FORM_MASK : 0) | (lemma ? LEMMA_MASK : 0);
    }

    static inline TWordId ClearFlags(TWordId shifted) {
        return shifted & ~(EXACT_FORM_MASK | LEMMA_MASK);
    }

    static inline TWordId ForceExactForm(TWordId shifted) {
        return (shifted & ~LEMMA_MASK) | EXACT_FORM_MASK;
    }

    static inline TWordId ForceLemma(TWordId shifted) {
        return (shifted & ~EXACT_FORM_MASK) | LEMMA_MASK;
    }

    template <bool exact>
    static inline bool Check(TWordId shifted) {
        // hoping compiler will optimize ? out
        return exact ? IsExactForm(shifted) : IsLemma(shifted);
    }

    template <bool exact>
    static inline TWordId ForceMask() {
        return exact ? ~LEMMA_MASK : ~EXACT_FORM_MASK;
    }

    template <bool exact>
    static inline bool CheckAndForce(TWordId& shifted) {
        if (Check<exact>(shifted)) {
            shifted &= ForceMask<exact>();
            return true;
        } else
            return false;
    }
};

class TGztTrie
{
    template <typename TInput> friend class TArticleIter;
    template <typename TInput> friend class TPhraseSearchState;
    template <typename TInput> friend class TCompoundSearchState;

    typedef wchar16 TChr;
    typedef Wtroka TString;
    typedef TCompactTrie<TChr, TWordId> TWordTrie;
    typedef TCompactTrie<TWordId, NAux::TArticleBucket, NAux::TArticleBucketPacker> TPhraseTrie;
    typedef TCompactTrie<TArticleId, NAux::TArticleBucket, NAux::TArticleBucketPacker> TCompoundTrie;

public:
    inline TGztTrie()
        : MetaAnyWordId(0)
    {
    }

    template <typename TInput>
    inline void IterArticles(const TInput& text, TArticleIter<TInput>* iter) const {
        // Iteration without creating new iterator, re-using memory of specified existing @iter.
        // This is helpful when searching in many relatively short queries (as opposed to search in single very long text)
        iter->Reset(this, text);
    }

    template <typename TInput>
    inline bool HasArticles(const TInput& text) const {
        return IterArticles(text).Ok();
    }

    template <typename TInput>
    inline TArticleId FindFirst(const TInput& text) const;

    inline bool FindWord(const TWtringBuf& word, TWordId& encoded) const {
        return WordTrie.Find(~word, +word, &encoded);
    }

    inline bool HasLemma(const TWtringBuf& lemma) const {
        TWordId wordId;
        return FindWord(lemma, wordId) && TWordIdTraits::IsLemma(wordId);
    }

    inline bool HasExactWord(const TWtringBuf& word) const {
        TWordId wordId;
        return FindWord(word, wordId) && ( TWordIdTraits::IsLemma(wordId) ||
                                           TWordIdTraits::IsExactForm(wordId));
    }

    inline bool IsCompoundElement(TArticleId id) const {
        return CompoundElements.find(id) != CompoundElements.end();
    }

    // Load trie without copying from specified memory stream
    void Load(TMemoryInput* input);
    void Load(const NBinaryFormat::TGztTrie& proto, const TBlobCollection& blobs);

private: // methods

    inline bool HasMetaAnyWord() const {
        return MetaAnyWordId != 0;
    }

private: // data

    TWordId MetaAnyWordId;      // any word (corresponding to "*" wildcard), if 0 - no wildcards in this gzt (for performance)

    // leaf contains TWordId:
    // sequence of chars -> word id
    TWordTrie WordTrie;

    // leaf contains index of item in ArticleFilter
    // sequence of word_ids -> article filter index
    TPhraseTrie PhraseTrie;

    // sequence of article ids -> article id
    // "complex" = article composed of other articles, complex article
    TCompoundTrie CompoundTrie;

    // for checking if an article should be passed to compound search
    yhash_set<TArticleId> CompoundElements;

    TArticleFilter ArticleFilter;

    DECLARE_NOCOPY(TGztTrie);
};






class TGztTrieBuilder
{
public:
    typedef wchar16 TChr;
    typedef Wtroka TString;
    typedef TWtringBuf TStrBuf;

    typedef TMoreCompactTrieBuilder<TChr, TWordId> TWordTrieBuilder;
    typedef TMoreCompactTrieBuilder<TWordId, NAux::TArticleBucket, NAux::TArticleBucketPacker> TPhraseTrieBuilder;
    typedef TMoreCompactTrieBuilder<TArticleId, NAux::TArticleBucket, NAux::TArticleBucketPacker> TCompoundTrieBuilder;

    explicit TGztTrieBuilder(TArticlePoolBuilder* articles);
    ~TGztTrieBuilder();

    // base.proto TSearchKey keys
    void Add(const TSearchKey& key, TArticleId article_id, const TOptions& options);

    void Save(TOutputStream* output) const;
    void Save(NBinaryFormat::TGztTrie& proto, TBlobCollection& blobs) const;

    static ui64 GetCapitalizationMask(const TStrBuf& word);

    Stroka DebugByteSize() const {
        TStringStream str;
        str << "GztTrieBuilder:\n" << DEBUG_BYTESIZE(WordTrie) << DEBUG_BYTESIZE(PhraseTrie) << DEBUG_BYTESIZE(CompoundTrie);
        str << DEBUG_BYTESIZE(CompoundElements) << DEBUG_BYTESIZE(ArticleFilter);
        str << "{\n" << ArticleFilter.DebugByteSize() << "}\n";
        str << DEBUG_BYTESIZE(Folders) << DEBUG_BYTESIZE(CachedArticleIdentifiers);
        return str;
    }

private:
    inline bool HasMetaAnyWord() const {
        return MetaAnyWordId != 0;
    }

    // Try recognizing @word as article identifier.
    // If successful, return True and set @article_id.
    bool IsArticleIdentifier(const TStrBuf& word, TArticleId* article_id);

    inline bool IsCachedArticleIdentifier(const TStrBuf& word, TArticleId* article_id) const;

    struct TKeyWordInfo;
    struct TState;

    void SplitKeyToWords(const TStrBuf& key, const TOptions& options);
    void ProcessPrefixes(TKeyWordInfo& kw, bool& hasInnerArticles, const TOptions& options);
    void NormalizeKeyWord(TKeyWordInfo& kw);

    void CollectKeyWords(const Wtroka& decomposed_key_text, const TSearchKey& key, const TOptions& options);
    void CollectWordMorph(const TSearchKey& key);
    void CollectWordLanguage(const TSearchKey& key);
    void CollectWordCapitalization(const TSearchKey& key);


    void AddKeyFile(const Stroka& filename, const TSearchKey& key, TArticleId article_id, const TOptions& options);

    // raw text here is single-byte encoded string which should be a literal key text (e.g, not file-name)
    void AddRawText(const Stroka& key_text, const TSearchKey& key, TArticleId article_id, const TOptions& options);
    void AddCurrentKeyWords(const TSearchKey& key, TArticleId article_id, const TOptions& options);
    void ReTokenize(Wtroka& key_text, const TSearchKey& key, TArticleId article_id, const TOptions& options);



    // add given word to proper trie corresponding to specified morph_type
    void AddWordWithMorphType(TKeyWordInfo& word);

    inline TWordId UseNextWordId();

    TWordId AddWord(const TStrBuf& word, bool exact, bool lemma);
    TWordId AddWord(const TKeyWordInfo& word, bool exact, bool lemma);
    void AddLemmasOfWord(TKeyWordInfo& word);

    inline TArticleId NextFakeArticleId();
    void BuildCompoundArticle(const yvector<TWordId>& word_ids, yvector<TArticleId>& compound);


    typedef TMoreCompactTrieBuilder<ui32, NAux::TArticleBucket, NAux::TArticleBucketPacker> TIndexTrieBuilder;

    TArticleId AddFakeArticle(TIndexTrieBuilder& trie, const ui32* words, size_t word_count, bool force = false);
    void AddArticle(TArticleId article_id, const yvector<TWordId>& word_ids, const TSearchKey& key, bool is_compound);

    void AddFakeToTrie(TArticleId fake_id, TIndexTrieBuilder& trie, const ui32* words, size_t count);
    void AddToTrie(TArticleId article_id, TIndexTrieBuilder& trie,
                   const ui32* words, size_t word_count,
                   const yvector<ui64>& capitalization, const TSearchKey& key);

private:
    TWordId MetaAnyWordId;      // any word (corresponding to "*" wildcard), 0 means - no wildcards in this gzt (improves performance)
    TWordId NextWordId;
    size_t FakeArticleCount;
    TArticlePoolBuilder* ArticlePool;

    // tries of words: leaves contain TWordId
    TWordTrieBuilder WordTrie;    // all possible words to search (with flags encoding if it is a lemma of exact form)

    // trie of phrases, leaves contain index from ArticleFilter;
    TPhraseTrieBuilder PhraseTrie;

    // trie of complex phrases (composed of other articles), leaves contain index from ArticleFilter;
    TCompoundTrieBuilder CompoundTrie;
    // contains all elements from CompoundTrie (nodes, not leaves)
    yset<TArticleId> CompoundElements;


    TArticleFilterBuilder ArticleFilter;

    typedef TStringDictionary<Stroka, TArticleId> TFoldersHash;
    TFoldersHash Folders;        // article name prefix (utf-8) -> related articles, similiar to file-system tree

    // To avoid constant lookups in ArticlePool namespace (which includes re-encodings)
    // store small number of article identifiers here.
    typedef TStringDictionary<TString, TArticleId> TCachedArticlesHash;
    TCachedArticlesHash CachedArticleIdentifiers;

    //current state vectors (all used during AddPhrase)
    THolder<TState> Current;

    // buffers for decoding from single-byte to wide-chars and normalization
    Wtroka TmpOriginalKey, TmpWordStorage, TmpNormalized;

    DECLARE_NOCOPY(TGztTrieBuilder);
};

} // namespace NGzt
