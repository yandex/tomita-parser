#include "gzttrie.h"
#include "options.h"

#include <util/stream/factory.h>
#include <library/lemmer/alpha/abc.h>
#include <library/token/charfilter.h>
#include <library/unicode_normalization/normalization.h>

#include <kernel/gazetteer/base.pb.h>
#include <kernel/gazetteer/binary.pb.h>

namespace NGzt
{

// Auxiliary structure describing parameters of each word in search-key
struct TGztTrieBuilder::TKeyWordInfo {
    TKeyWordInfo()
        : Morph(TMorphMatchRule::ALL_FORMS)
        , Language(LANG_UNK)
        , CapitalizationMask(0)
        , Id(0)
        , IsArticle(false)
        , IsAnyWord(false)
    {
    }

    TWtringBuf OriginalText;    // standard unicode normalization + removed insignificant diacritic (accents, for example), depending on language.
    TWtringBuf LowerCasedText;  // + to_lower()
    TWtringBuf NormalizedText;  // + TLanguage::Convert()   - transforms 'yo' into 'e', for example, for russian

    TMorphMatchRule::EType Morph;
    docLanguage Language;       // main language from TLanguageFilter.allow
    ui64 CapitalizationMask;    // capitalized letters mask, only for words having case=EXACT, otherwise 0
    TWordId Id;                 // ArticleId if IsArticle, otherwise WordId
    yset<TWordId> Lemmas;       // Ids of lemmas for key with morph = ALL_LEMMA_FORMS

    bool IsArticle;
    bool IsAnyWord;
};

struct TGztTrieBuilder::TState {
    yvector<TGztTrieBuilder::TKeyWordInfo> Words;
    yvector<ui64> Capitalization;
    yvector<TWordId> WordIds;
    yvector<TArticleId> CompoundArticle;
#ifdef GZT_DEBUG
    NTools::TPerformanceCounter WordCounter;
    NTools::TPerformanceCounter PhraseCounter;
    NTools::TPerformanceCounter CompoundCounter;
    NTools::TPerformanceCounter FakeCounter;

    TState()
        : WordCounter("word trie", 500000)
        , PhraseCounter("phrase trie", 2000000)
        , CompoundCounter("compound trie", 100000)
        , FakeCounter("fake articles", 100000)
    {
    }
#endif


};


TGztTrieBuilder::TGztTrieBuilder(TArticlePoolBuilder* articles)
    : MetaAnyWordId(0), NextWordId(1), FakeArticleCount(0), ArticlePool(articles)
    , Current(new TState)
{
}

TGztTrieBuilder::~TGztTrieBuilder() {
    // for sake of THolder<TState>
}

inline TArticleId TGztTrieBuilder::NextFakeArticleId()
{
    // to make sure we do not intersect real article ids space - place them after max offset
    FakeArticleCount += 1;
    TArticleId res = Max<TArticleId>() - FakeArticleCount;
    if (EXPECT_TRUE(IsFakeArticle(res)))
        return res;

    ythrow yexception() << "Fake article id limit reached.";
}

inline bool TGztTrieBuilder::IsCachedArticleIdentifier(const TStrBuf& word, TArticleId* article_id) const
{
    TCachedArticlesHash::const_iterator cached = CachedArticleIdentifiers.Find(word);
    if (cached != CachedArticleIdentifiers.End()) {
        *article_id = cached->second;
        return true;
    } else
        return false;
}

bool TGztTrieBuilder::IsArticleIdentifier(const TStrBuf& word, TArticleId* article_id)
{
    if (!HasArticleMarker(word))
        return false;

    if (IsCachedArticleIdentifier(word, article_id))
        return true;

    TStaticCharCodec<> encoder;
    TStringBuf utf8_name = encoder.ToUtf8(word, CODES_UTF8).SubStr(1);

    // articles searched first, so if there is an article and folder with same name
    // the article will be always selected, and the folder will be hidden.
    if (ArticlePool->FindOffsetByName(utf8_name, *article_id) /*|| IsFolder(utf8_name, article_id)*/) {
        CachedArticleIdentifiers.Insert(word, *article_id);
        return true;
    }

    // warning
    Cerr << "A word \"$" << utf8_name << "\" is not a name of known article, it will be treated literally." << Endl;
    return false;
}

void TGztTrieBuilder::SplitKeyToWords(const TStrBuf& key, const TOptions& options) {
    TStrBuf text = StripString(key);
    while (!text.empty()) {
        TStrBuf word = text.NextTok(SPACE);
        text = StripString(text);
        if (!word.empty()) {
            Current->Words.push_back();
            Current->Words.back().OriginalText = word;
            if (options.GztOptions.GetWildCards().GetAnyWord() && IsAnyWord(word))
                Current->Words.back().IsAnyWord = true;
        }
    }
}

static TWtringBuf StoreWord(const TWtringBuf& word, Wtroka& storage) {
    YASSERT(storage.capacity() > storage.size() + word.size());     // should not re-allocate @storage buffer
    storage.append(~word, +word);
    return TWtringBuf(storage.c_str() + storage.size() - word.size(), word.size());
}

void TGztTrieBuilder::CollectKeyWords(const Wtroka& key_text, const TSearchKey& key, const TOptions& options)
{
    Current->Words.clear();
    SplitKeyToWords(key_text, options);

    CollectWordLanguage(key);

    TmpWordStorage.clear();
    TmpWordStorage.reserve(key_text.size() * 4);      // have a reserve to avoid reallocations
    for (size_t i = 0; i < Current->Words.size(); ++i)
        NormalizeKeyWord(Current->Words[i]);
}

void TGztTrieBuilder::NormalizeKeyWord(TKeyWordInfo& kw)
{
    docLanguage lang = /*(kw.Language == LANG_UNK) ? GuessLanguage(kw.OriginalText) :*/ kw.Language;
    const NLemmer::TAlphabetWordNormalizer* awn = NLemmer::GetAlphaRulesAnyway(lang);
    static const NLemmer::TAlphabetWordNormalizer* awnRus = NLemmer::GetAlphaRulesAnyway(LANG_RUS);

    TmpNormalized.reserve(kw.OriginalText.size() * 4);
    TmpNormalized.assign(~kw.OriginalText, +kw.OriginalText);

    awn->SoftDecompose(TmpNormalized);
    awn->SoftCompose(TmpNormalized);
    if (TmpNormalized != kw.OriginalText)
        kw.OriginalText = StoreWord(TmpNormalized, TmpWordStorage);

    // normalize case
    kw.LowerCasedText = kw.OriginalText;
    awn->ToLower(TmpNormalized);
    if (TmpNormalized != kw.LowerCasedText)
        kw.LowerCasedText = StoreWord(TmpNormalized, TmpWordStorage);

    // lemmer conversion (to normalize some letters, for example, 'yo' in russian)
    // if the language is not specified explicitly, use Russian converter (i.e. TGeneralCyrilicConverter)
    kw.NormalizedText = kw.LowerCasedText;
    if (lang == LANG_UNK)
        awnRus->Convert(TmpNormalized);
    else
        awn->Convert(TmpNormalized);
    if (TmpNormalized != kw.NormalizedText)
        kw.NormalizedText = StoreWord(TmpNormalized, TmpWordStorage);
}

void TGztTrieBuilder::CollectWordMorph(const TSearchKey& key)
{
    for (int m = 0; m < key.morph_size(); ++m) {
        const TMorphMatchRule& key_morph = key.morph(m);
        if (key_morph.word_size() == 0)
            // apply this morph-type to all words of the key
            for (size_t w = 0; w < Current->Words.size(); ++w)
                Current->Words[w].Morph = key_morph.type();
        else
            for (int i = 0; i < key_morph.word_size(); ++i)
                if (key_morph.word(i) <= Current->Words.size())
                    Current->Words[key_morph.word(i) - 1].Morph = key_morph.type();
    }
}

void TGztTrieBuilder::CollectWordLanguage(const TSearchKey& key)
{
    for (int m = 0; m < key.lang_size(); ++m) {
        const TLanguageFilter& langfilter = key.lang(m);
        if (langfilter.allow_size() == 0)
            continue;

        // select first language or russian
        docLanguage lang = static_cast<docLanguage>(langfilter.allow(0));
        for (int i = 1; i < langfilter.allow_size(); ++i)
            if (static_cast<docLanguage>(langfilter.allow(i)) == LANG_RUS) {
                lang = LANG_RUS;
                break;
            }

        if (langfilter.word_size() == 0) {
            // apply lang to all words of the key
            for (size_t w = 0; w < Current->Words.size(); ++w)
                Current->Words[w].Language = lang;
        } else for (int i = 0; i < langfilter.word_size(); ++i)
            if (langfilter.word(i) <= Current->Words.size())
                Current->Words[langfilter.word(i) - 1].Language = lang;
    }
}

void TGztTrieBuilder::CollectWordCapitalization(const TSearchKey& key)
{
    bool has_any = false;
    for (int m = 0; m < key.case__size(); ++m) {
        const TCapitalizationFilter& key_case = key.case_(m);

        bool has_exact = false;
        for (int k = 0; k < key_case.allow_size(); ++k)
            if (key_case.allow(k) == TCapitalizationFilter::EXACT) {
                has_exact = true;
                break;
            }

        if (!has_exact)
            for (int k = 0; k < key_case.forbid_size(); ++k)
                if (key_case.forbid(k) == TCapitalizationFilter::EXACT) {
                    has_exact = true;
                    break;
                }

        if (has_exact) {
            if (key_case.word_size() == 0)
                for (size_t w = 0; w < Current->Words.size(); ++w)
                    if (!Current->Words[w].IsArticle) {
                        Current->Words[w].CapitalizationMask = GetCapitalizationMask(Current->Words[w].OriginalText);
                        has_any = true;
                    }
            else
                for (int i = 0; i < key_case.word_size(); ++i)
                    if (key_case.word(i) <= Current->Words.size() && !Current->Words[key_case.word(i) - 1].IsArticle) {
                        size_t w = key_case.word(i) - 1;
                        Current->Words[w].CapitalizationMask = GetCapitalizationMask(Current->Words[w].OriginalText);
                        has_any = true;
                    }
        }
    }

    if (has_any) {
        Current->Capitalization.resize(Current->Words.size());
        for (size_t i = 0; i < Current->Words.size(); ++i)
            Current->Capitalization[i] = Current->Words[i].CapitalizationMask;
    } else
        Current->Capitalization.clear();
}

inline TWordId TGztTrieBuilder::UseNextWordId()
{
    #ifdef GZT_DEBUG
        ++Current->WordCounter;
    #endif
    return TWordIdTraits::Shift(NextWordId++);
}

TWordId TGztTrieBuilder::AddWord(const TStrBuf& word, bool exact, bool lemma)
{
    TWordId* wordId = NULL;
    if (WordTrie.Insert(~word, +word, 0, &wordId))
        *wordId = UseNextWordId();  // already shifted

    // set necessary form flags to the word in the trie
    *wordId = TWordIdTraits::SetFlags(*wordId, exact, lemma);

    // but return only once encoded id (either exact-form, or lemma, not both)
    return TWordIdTraits::SetFlags(TWordIdTraits::ClearFlags(*wordId), exact, lemma);
}

TWordId TGztTrieBuilder::AddWord(const TKeyWordInfo& word, bool exact, bool lemma)
{
    if (word.IsAnyWord) {
        if (!HasMetaAnyWord()) {
            MetaAnyWordId = UseNextWordId();
            YASSERT(MetaAnyWordId != 0);
        }
        return MetaAnyWordId;
    }

    // first insert most normalized form
    TWordId normId = AddWord(word.NormalizedText, exact, lemma);

    // next, lower-cased form, if it differs, with same word id
    // thus, non-language-converted words can also be searched by gazetteer.
    TWordId* tmpId = NULL;
    if (~word.LowerCasedText != ~word.NormalizedText) {
        // normId already encoded
        if (!WordTrie.Insert(~word.LowerCasedText, +word.LowerCasedText, normId, &tmpId))
            *tmpId = TWordIdTraits::SetFlags(*tmpId, exact, lemma);      // set necessary form flags
    }

    // TODO: if we are inserting at ExactFormTrie, insert only LowerCasedText, without NormalizedText
    // in order to preserve diacritic and search exact forms exactly as written (except capitalization).
    // To do or not to do?

    return normId;
}

void TGztTrieBuilder::AddWordWithMorphType(TKeyWordInfo& word)
{
    switch (word.Morph) {
    case TMorphMatchRule::EXACT_FORM:
        word.Id = AddWord(word, true, false);
        break;

    case TMorphMatchRule::ALL_FORMS:
        word.Id = AddWord(word, false, true);
        break;

    case TMorphMatchRule::ALL_LEMMA_FORMS:
        AddLemmasOfWord(word);
        break;

    default:
        ythrow yexception() << "Unknown morph type " << static_cast<ui32>(word.Morph) << ".";
    }
}

void TGztTrieBuilder::BuildCompoundArticle(const yvector<TWordId>& word_ids, yvector<TArticleId>& compound)
{
    YASSERT(compound.empty());
    // take sequential word parts from @words and save them as fake articles.
    size_t start = 0, stop = 0;
    for (; stop < word_ids.size(); ++stop)
        if (Current->Words[stop].IsArticle) {
            if (stop > start)
                compound.push_back(AddFakeArticle(PhraseTrie, ~word_ids + start, stop - start));
            compound.push_back(word_ids[stop]);  // it is really article id (for words corresponding to article identifiers)
            start = stop + 1;
        }

    if (stop > start)
        compound.push_back(AddFakeArticle(PhraseTrie, ~word_ids + start, stop - start));
}

void TGztTrieBuilder::AddArticle(TArticleId article_id, const yvector<TWordId>& word_ids, const TSearchKey& key, bool is_compound)
{
    if (is_compound) {
        Current->CompoundArticle.clear();
        BuildCompoundArticle(word_ids, Current->CompoundArticle);
        // save all fake/non-fake article sequence as complex phrase corresponding to given @article_id
        AddToTrie(article_id, CompoundTrie, ~Current->CompoundArticle, +Current->CompoundArticle, Current->Capitalization, key);
        #ifdef GZT_DEBUG
            ++Current->CompoundCounter;
        #endif
    } else {
        // add whole key (phrase) to phrase trie,
        // i.e. append article's filter to phrase filters
        AddToTrie(article_id, PhraseTrie, ~word_ids, +word_ids, Current->Capitalization, key);
        #ifdef GZT_DEBUG
            ++Current->PhraseCounter;
        #endif
    }
}

void TGztTrieBuilder::ProcessPrefixes(TKeyWordInfo& word, bool& hasInnerArticles, const TOptions& options) {
    TArticleId artid = 0;
    if (options.GztOptions.GetWildCards().GetArticleSubst() && IsArticleIdentifier(word.OriginalText, &artid)) {
        word.Id = artid;
        word.IsArticle = true;
        hasInnerArticles = true;
        return;
    }

    // A shortcut for EXACT_FORM: [key = "!abracadabra"] has same meaning as [key = {"abracadabra" EXACT_FORM}]
    if (options.GztOptions.GetWildCards().GetExactForm() && HasExactFormMarker(word.OriginalText)) {
        word.OriginalText.Skip(1);
        word.LowerCasedText.Skip(1);
        word.NormalizedText.Skip(1);
        word.Morph = TMorphMatchRule::EXACT_FORM;
    }
}

// Helper class for iteration over all combinations of lemma (when ALL_LEMMA_FORMS options is used)
class TCombinationIterator {
public:
    TCombinationIterator(const yvector<TWordId>& single_words, const yvector< yset<TWordId> >& combinations);

    inline bool Ok() const {
        return Ok_;
    }

    void operator++();

    inline const yvector<TWordId>& operator*() const {
        return Current;
    }

private:
    const yvector< yset<TWordId> >& Source;
    yvector<TWordId> Current;

    yvector<size_t> Indexes;
    yvector< yset<TWordId>::const_iterator > State;

    bool Ok_;
};

void TGztTrieBuilder::AddCurrentKeyWords(const TSearchKey& key, TArticleId article_id, const TOptions& options)
{
    if (EXPECT_FALSE(Current->Words.size() <= 0))
        ythrow yexception() << "Empty article keys are not allowed.";

    // used for ALL_LEMMA_FORMS - one word could give several lemmas, and then we need to take all combinations into account
    bool has_combinations = false;
    bool has_inner_articles = false;

    CollectWordMorph(key);

    // preprocess words prefixes
    for (size_t w = 0; w < Current->Words.size(); ++w)
        ProcessPrefixes(Current->Words[w], has_inner_articles, options);

    // collect word capitalization masks (it is important to do it after removing all special prefixes from words)
    CollectWordCapitalization(key);

    // add each word (which is not article) to char trie corresponding to its morph extension type.
    // also, collect all Ids into separate array
    Current->WordIds.resize(Current->Words.size());
    for (size_t w = 0; w < Current->Words.size(); ++w) {
        if (!Current->Words[w].IsArticle) {
            AddWordWithMorphType(Current->Words[w]);
            if (Current->Words[w].Lemmas.size() >= 2)
                has_combinations = true;
        }
        Current->WordIds[w] = Current->Words[w].Id;
    }

    if (!has_combinations)
        AddArticle(article_id, Current->WordIds, key, has_inner_articles);
    else {
        yvector< yset<TWordId> > combinations(Current->Words.size());
        for (size_t w = 0; w < Current->Words.size(); ++w)
            combinations[w] = Current->Words[w].Lemmas;

        for (TCombinationIterator it(Current->WordIds, combinations); it.Ok(); ++it)
            AddArticle(article_id, *it, key, has_inner_articles);
    }
}


void TGztTrieBuilder::AddRawText(const Stroka& key_text, const TSearchKey& key, TArticleId article_id, const TOptions& options)
{
    // recode to unicode, split into words and normalize
    ::UTF8ToWide(key_text, TmpOriginalKey);

    // using NFC normalization scheme instead of NFKC to avoid decomposing of № to No
    TmpOriginalKey = ::Normalize<NUnicode::NFC>(TmpOriginalKey);   // unicode decomposition+composition

    CollectKeyWords(TmpOriginalKey, key, options);
    AddCurrentKeyWords(key, article_id, options);

    // custom tokenization
    if (key.has_tokenize())
        ReTokenize(TmpOriginalKey, key, article_id, options);
}

void TGztTrieBuilder::ReTokenize(Wtroka& key_text, const TSearchKey& key, TArticleId article_id, const TOptions& options) {
    TRetokenizer retokenizer(key);
    if (retokenizer.Do(key_text)) {
         CollectKeyWords(key_text, retokenizer.GetKey(), options);
         AddCurrentKeyWords(retokenizer.GetKey(), article_id, options);
     }
}

ui64 TGztTrieBuilder::GetCapitalizationMask(const TWtringBuf& word)
{
    ui64 res = 0;
    size_t len = Min<size_t>(word.size(), 64);
    for (size_t i = 0; i < len; ++i)
        if (::IsUpper(static_cast<wchar32>(word[i])))
            res |= static_cast<ui64>(1) << i;

    return res;
}

static inline TWtringBuf LemmaText(const TYandexLemma& lemma) {
    return TWtringBuf(lemma.GetText(), lemma.GetTextLength());
}

void TGztTrieBuilder::AddLemmasOfWord(TKeyWordInfo& word)
{
    // use only word specified language (or Russian, if none)
    docLanguage lang = word.Language;
    if (lang == LANG_UNK)
        lang = LANG_RUS;
    TWLemmaArray lemmata;
    NLemmer::AnalyzeWord(~word.OriginalText, +word.OriginalText, lemmata, lang);

    // optimize frequent case
    if (lemmata.empty() || (lemmata.size() == 1 && word.NormalizedText == LemmaText(lemmata[0])))
        word.Id = AddWord(word, false, true);
    else {
        bool isLemma = false;   // exact form is same as lemma?
        bool hasGoodLemma = false;  // if no, there is a chance we could have chosen wrong language
        for (size_t i = 0; i < lemmata.size(); ++i) {
            word.Lemmas.insert(AddWord(LemmaText(lemmata[i]), false, true));
            if (lemmata[i].GetQuality() == TYandexLemma::QDictionary)
                hasGoodLemma = true;
            if (!hasGoodLemma && !isLemma && word.NormalizedText == LemmaText(lemmata[i]))
                isLemma = true;
        }
        if (!isLemma && !hasGoodLemma)
            // add original text from key as exact form
            // to guarantee it will be found on same text in input.
            word.Lemmas.insert(AddWord(word, true, false));

        word.Id = *word.Lemmas.begin();
    }
}

TArticleId TGztTrieBuilder::AddFakeArticle(TIndexTrieBuilder& trie, const ui32* words, size_t count,
                                           bool force /*=false*/)
{
    if (!force) {
        // check if there is already fake article for such words in @trie
        TArticleBucket bucket;
        if (trie.Find(words, count, &bucket)) {
            for (TArticleFilterBuilder::TSimpleArticleIterator it = ArticleFilter.IterSimple(bucket.SimpleIndex); it.Ok(); ++it)
                if (IsFakeArticle(*it))
                    return *it;
            /* a fake article is always a simple article, so do not check FilteredArticles
            for (TIterator<NAux::TFilter> it = ArticleFilter.IterFiltered(bucket.FilterIndex); it.Ok(); ++it)
                if (IsFakeArticle(it->ArticleId))
                    return it->ArticleId;
            */
        }
    }

    TArticleId fake_id = NextFakeArticleId();
    AddFakeToTrie(fake_id, trie, words, count);
    #ifdef GZT_DEBUG
        ++Current->FakeCounter;
    #endif
    return fake_id;
}

void TGztTrieBuilder::AddFakeToTrie(TArticleId fake_id, TIndexTrieBuilder& trie, const ui32* words, size_t count)
{
    static const TSearchKey empty_key;   // fake articles are not filtered by grammems and other filters, so provide empty key
    static const yvector<ui64> empty_capitalization;
    AddToTrie(fake_id, trie, words, count, empty_capitalization, empty_key);
}

void TGztTrieBuilder::AddToTrie(TArticleId article_id, TIndexTrieBuilder& trie,
                                const ui32* words, size_t word_count,
                                const yvector<ui64>& capitalization, const TSearchKey& key)
{
    TArticleBucket* bucket = NULL;
    if (trie.Insert(words, word_count, TArticleBucket(), &bucket))
        *bucket = ArticleFilter.Add(key, capitalization, article_id, word_count);
    else
        *bucket = ArticleFilter.AddTo(*bucket, key, capitalization, article_id, word_count);

    // remember all compound components
    if (&trie == &CompoundTrie)
        for (size_t i = 0; i < word_count; ++i)
            CompoundElements.insert(words[i]);
}

void TGztTrieBuilder::Add(const TSearchKey& key, ui32 article_id, const TOptions& options)
{
    if (key.type() == TSearchKey::FILE)
        for (int i = 0; i < key.text_size(); ++i)
            AddKeyFile(key.text(i), key, article_id, options);
    else if (key.type() == TSearchKey::CUSTOM)
        for (int i = 0; i < key.text_size(); ++i)
            ArticlePool->AddCustomArticle(article_id, key.text(i));
    else
        for (int i = 0; i < key.text_size(); ++i)
            AddRawText(key.text(i), key, article_id, options);
}

void TGztTrieBuilder::AddKeyFile(const Stroka& filename, const TSearchKey& key, ui32 article_id, const TOptions& options)
{
    THolder<TInputStream> input(ArticlePool->GetSourceTree().OpenVirtualFile(filename));
    if (!input)
        ythrow yexception() << "Cannot read file with keys: " << filename << ".";

    Stroka line;
    Wtroka tmp;
    while (input->ReadLine(line))
        if (!::StripString(TStringBuf(line)).empty()) {
            if (options.Encoding != CODES_UTF8)
                ::WideToUTF8(::CharToWide(line, tmp, options.Encoding), line);
            AddRawText(line, key, article_id, options);
        }

    ArticlePool->AddImportedFile(filename);
}

void TGztTrieBuilder::Save(NBinaryFormat::TGztTrie& proto, TBlobCollection& blobs) const
{
    // first save TChr size and then disallow loading this data into trie with other TChr size
    proto.SetCharSize(sizeof(TChr));
    proto.SetMetaAnyWordId(MetaAnyWordId);

    // for compatibility - save empty trie-builder for exact-form-trie and lemma-trie
    TWordTrieBuilder emptyWordTrie;

    blobs.SaveCompactTrie("ExactFormTrie", emptyWordTrie, proto.MutableExactFormTrieRef()->MutableBlobKey());
    blobs.SaveCompactTrie("LemmaTrie", emptyWordTrie, proto.MutableLemmaTrieRef()->MutableBlobKey());
    blobs.SaveCompactTrie("WordTrie", WordTrie, proto.MutableWordTrieRef()->MutableBlobKey());
    blobs.SaveCompactTrie("PhraseTrie", PhraseTrie, proto.MutablePhraseTrieRef()->MutableBlobKey());
    blobs.SaveCompactTrie("CompoundTrie", CompoundTrie, proto.MutableCompoundTrieRef()->MutableBlobKey());

    ArticleFilter.Save(*proto.MutableArticleFilter(), blobs);

    blobs.SaveObject("CompoundElements", CompoundElements, proto.MutableCompoundElementsRef()->MutableBlobKey());
}

void TGztTrieBuilder::Save(TOutputStream* ostream) const
{
    SaveAsProto<NBinaryFormat::TGztTrie>(ostream, *this);
}


// TGztTrie =====================================================

template <typename TTrie>
static inline bool InitTrieFromBlob(const TBlobCollection& blobs, const NBinaryFormat::TBlobRef& ref, TTrie& trie) {
    if (blobs.HasBlob(ref.GetBlobKey())) {
        trie.Init(blobs[ref.GetBlobKey()]);
        return true;
    } else
        return false;
}


void TGztTrie::Load(const NBinaryFormat::TGztTrie& proto, const TBlobCollection& blobs)
{
    // DEPRECATED: gzttrie is now only on wide-char (wchar16) strings
    if (proto.GetCharSize() != sizeof(TChr))
        ythrow yexception() << "Cannot load " << proto.GetCharSize() << "-byte dictionary into "
                            << sizeof(TChr) << "-byte gazetteer (incompatible or corrupted binary supplied).";

    MetaAnyWordId = proto.GetMetaAnyWordId();

    // skip legacy tries
    //ExactFormTrie.Init(blobs[proto.GetExactFormTrieRef().GetBlobKey()]);
    //LemmaTrie.Init(blobs[proto.GetLemmaTrieRef().GetBlobKey()]);
    InitTrieFromBlob(blobs, proto.GetWordTrieRef(), WordTrie);
    InitTrieFromBlob(blobs, proto.GetPhraseTrieRef(), PhraseTrie);
    InitTrieFromBlob(blobs, proto.GetCompoundTrieRef(), CompoundTrie);

    ArticleFilter.Load(proto.GetArticleFilter(), blobs);

    blobs.LoadObject(CompoundElements, proto.GetCompoundElementsRef().GetBlobKey());
}

void TGztTrie::Load(TMemoryInput* input)
{
    LoadAsProto<NBinaryFormat::TGztTrie>(input, *this);
}




// TCombinationIterator =========================================

TCombinationIterator::TCombinationIterator(const yvector<TWordId>& single_words, const yvector< yset<TWordId> >& combinations)
    : Source(combinations)
    , Current(Source.size())
    , Ok_(true)
{
    YASSERT(single_words.size() == combinations.size());

    ui64 count = 1;
    for (size_t i = 0; i < Source.size(); ++i)
    {
        if (Source[i].empty())
            Current[i] = single_words[i];
        else if (Source[i].size() == 1)
            Current[i] = *Source[i].begin();
        else
        {
            Indexes.push_back(i);
            State.push_back(Source[i].begin());
            Current[i] = *State.back();

            // count number of combinations to prevent combinatorial explosion
            count *= Source[i].size();
            if (count >= 100000)
                ythrow yexception() << "Too many combinations of lemmas (at least " << count << ").";
        }
    }
}

void TCombinationIterator::operator++()
{
    for (size_t i = 0; i < Indexes.size(); ++i)
    {
        ++State[i];
        size_t src_index = Indexes[i];
        if (State[i] != Source[src_index].end())
        {
            Current[src_index] = *State[i];
            return;
        }
        else
        {
            State[i] = Source[src_index].begin();
            Current[src_index] = *State[i];
        }
    }
    Ok_ = false;
}

} // namespace NGzt













