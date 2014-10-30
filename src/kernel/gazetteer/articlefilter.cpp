#include "articlefilter.h"
#include "common/serialize.h"
#include "common/recode.h"

#include <util/string/vector.h>
#include <util/generic/static_assert.h>

#include <kernel/gazetteer/base.pb.h>
#include <kernel/gazetteer/binary.pb.h>


namespace NGzt {
namespace NGleiche {

struct TCategoriesHolder {
    yvector<TGramBitSet> Categories;
    yvector<TGramBitSet> Activators;
    yhash<TGleicheFilter::EType, TMask> GleicheTypeToMask;
    TGramBitSet AllGrammems;

    TCategoriesHolder();
};

TCategoriesHolder::TCategoriesHolder()
    : Categories(gcMax), Activators(gcMax)
{
    Categories[gcCase] = NSpike::AllMajorCases;
    Categories[gcGender] = TGramBitSet(gMasculine, gFeminine, gNeuter);
    Categories[gcNumber] = NSpike::AllNumbers;
    Categories[gcPerson] = NSpike::AllPersons;
    Categories[gcTense] = TGramBitSet(gPresent, gPast, gFuture);
    Categories[gcAnim] = TGramBitSet(gAnimated, gInanimated);

    Categories[gcFemGender] = Categories[gcGender];       // special
    Categories[gcNominative] = TGramBitSet(gNominative);        // special

    for (size_t i = 0; i < Categories.size(); ++i)
        Activators[i] = Categories[i];
    Activators[gcNominative] = NSpike::AllMajorCases;

    // when checking case, always check animated/inanimated agreement too
    GleicheTypeToMask[TGleicheFilter::CASE] = TMask(gcCase, gcAnim);
    GleicheTypeToMask[TGleicheFilter::GENDER] = TMask(gcGender);
    GleicheTypeToMask[TGleicheFilter::NUMBER] = TMask(gcNumber);
    GleicheTypeToMask[TGleicheFilter::TENSE] = TMask(gcTense);
    GleicheTypeToMask[TGleicheFilter::PERSON] = TMask(gcPerson);

    GleicheTypeToMask[TGleicheFilter::SUBJECT_PREDICATE] = TMask(gcNominative) | TMask(gcGender, gcNumber, gcTense, gcPerson);
    GleicheTypeToMask[TGleicheFilter::GENDER_NUMBER_CASE] = TMask(gcGender, gcNumber, gcCase, gcAnim);
    GleicheTypeToMask[TGleicheFilter::NUMBER_CASE] = TMask(gcNumber, gcCase, gcAnim);
    GleicheTypeToMask[TGleicheFilter::GENDER_NUMBER] = TMask(gcGender, gcNumber);
    GleicheTypeToMask[TGleicheFilter::GENDER_CASE] = TMask(gcGender, gcCase, gcAnim);
    GleicheTypeToMask[TGleicheFilter::FEM_CASE] = TMask(gcFemGender, gcCase, gcAnim);

    for (size_t i = 0; i < Categories.size(); ++i)
        AllGrammems |= Categories[i];
}

inline const yvector<TGramBitSet>& GetCategories()
{
    return Singleton<TCategoriesHolder>()->Categories;
}

inline const yvector<TGramBitSet>& GetActivators()
{
    return Singleton<TCategoriesHolder>()->Activators;
}

inline const TGramBitSet& GetCategoriesGrammems()
{
    return Singleton<TCategoriesHolder>()->AllGrammems;
}

inline TMask GetMask(TGleicheFilter::EType gleiche_type)
{
    const yhash<TGleicheFilter::EType, TMask>& table = Singleton<TCategoriesHolder>()->GleicheTypeToMask;
    yhash<TGleicheFilter::EType, TMask>::const_iterator res = table.find(gleiche_type);
    YASSERT(res != table.end());
    return res->second;
}

void StrongNormalize(TGramBitSet& gram, TMask mask)
{
    gram = ::NGleiche::NormalizeAll(gram);
    TGramBitSet tmp;
    static const yvector<TGramBitSet>& categories = GetCategories();
    static const yvector<TGramBitSet>& activators = GetActivators();
    bool has_any_category = false;
    for (size_t i = 0; i < categories.size(); ++i)
        if (mask.Test(TCategory(i)))
        {
            if (gram.HasAny(activators[i]))
            {
                tmp |= gram & categories[i];
                has_any_category = true;
            }
            else
                // if @gram does not have any grammem of category X activator, set default grammems of this category
                tmp |= categories[i];
        }

    // some exceptions:
    // gPlural is agreed with all genders
    if (mask.Test(gcGender) && !has_any_category && gram.Has(gPlural))
        has_any_category = true;


    if (has_any_category)
    {
        // special handling of gcFemGender:
        // if @gram is common noun (i.e., not proper name) then make it agreeable with words of feminine gender.
        static const TGramBitSet fixed_gender = TGramBitSet(gSurname, gFirstName, gGeo, gProper);
        if (mask.Test(gcFemGender) && gram.Has(gSubstantive) && !gram.HasAny(fixed_gender))
            tmp.Set(gFeminine);

        gram = tmp;
    }
    else
        gram.Reset();
}

bool IsAgreed(const TGramBitSet& common_grammems, TMask mask)
{
    static const yvector<TGramBitSet>& categories = GetCategories();
    for (size_t i = 0; i < categories.size(); ++i)
        if (mask.Test(TCategory(i)) && !common_grammems.HasAny(categories[i]))
            return false;
    return true;
}

}   // NGleiche



bool ParseSingleGrammemSet(TStringBuf grammems, TGramBitSet& result)
{
    while (!grammems.empty()) {
        TStringBuf gram = grammems.NextTok(',');
        TGrammar code = TGrammarIndex::GetCode(gram);
        if (code == gInvalid)
            throw yexception() << "Unknown grammem \"" << gram << "\".";
        else
            result.Set(code);
    }
    return result.any();
}

void ParseGramBitSets(const TGrammemSet& proto_grammems, yvector<TGramBitSet>& result)
{
    TStaticCharCodec<> codec;
    for (int i = 0; i < proto_grammems.g_size(); ++i)
    {
        // currently grammem names stored as win1251
        TStringBuf blocks = codec.Utf8ToChar(proto_grammems.g(i), CODES_WIN);
        while (!blocks.empty())
        {
            TGramBitSet tmp;
            if (ParseSingleGrammemSet(blocks.NextTok('|'), tmp))
                result.push_back(tmp);
        }
    }
}

static void CollectWordIndexes(const NProtoBuf::RepeatedField<NProtoBuf::uint32>& words,
                               size_t phrase_length, yset<TWordIndex>& result)
{
    // word = 0 means "all words"!
    for (int w = 0; w < words.size(); ++w)
        if (words.Get(w) > 0 && words.Get(w) <= phrase_length)
            result.insert(static_cast<TWordIndex>(words.Get(w) - 1));
    // when no word specified in filter - apply this filter to ALL words of phrase.
    if (words.size() == 0)
        for (TWordIndex i = 0; i < phrase_length; ++i)
            result.insert(i);
}


// Adds all grammem sets from @src to @dst.
// If some grammem set pair from @src and @dst are included one in other
// then the bigger is placed in @dst, the smaller is discarded.
void MergeGramBitSets(const yvector<TGramBitSet>& src, yvector<TGramBitSet>& dst)
{
    // TODO: re-factor this function to avoid O(N*N) complexity.
    for (size_t i = 0; i < src.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < dst.size(); ++j) {
            if (dst[j].HasAll(src[i])) {
                found = true;
                break;
            } else if (src[i].HasAll(dst[j])) {
                dst[j] = src[i];
                found = true;
                break;
            }
        }
        if (!found)
            dst.push_back(src[i]);
    }
}


void TArticleFilterBuilder::Save(NBinaryFormat::TArticleFilter& proto, TBlobCollection& blobs) const
{

    Grammems.SaveToProto(*proto.MutableGrammems());
    WordGrammems.SaveToProto(*proto.MutableWordGrammems());
    Capitalizations.SaveToProto(*proto.MutableCapitalizations());
    WordCases.SaveToProto(*proto.MutableWordCases());
    GleicheWords.SaveToProto(*proto.MutableGleicheWords());
    Agreements.SaveToProto(*proto.MutableAgreements());
    LangMasks.SaveToProto(*proto.MutableLangMasks());
    WordLanguages.SaveToProto(*proto.MutableWordLanguages());
    FilterData.SaveToProto(*proto.MutableFilterData());

    // Save filters directly as blob (they can exceed max proto-message size of 64 MB)
    blobs.SaveObject("Filters", FilteredArticles, proto.MutableFiltersRef()->MutableBlobKey());
    blobs.SaveObject("SimpleArticles", SimpleArticles, proto.MutableSimpleArticlesRef()->MutableBlobKey());
}

void TArticleFilterBuilder::Save(TOutputStream* output) const
{
    SaveAsProto<NBinaryFormat::TArticleFilter>(output, *this);
}

void TArticleFilter::Load(const NBinaryFormat::TArticleFilter& proto, const TBlobCollection& blobs)
{
    Grammems.LoadFromProto(proto.GetGrammems(), proto.GetGrammems().ItemsSize());
    WordGrammems.LoadFromProto(proto.GetWordGrammems(), proto.GetWordGrammems().WordSize());
    Capitalizations.LoadFromProto(proto.GetCapitalizations(), proto.GetCapitalizations().TypeSize());
    WordCases.LoadFromProto(proto.GetWordCases(), proto.GetWordCases().WordSize());
    GleicheWords.LoadFromProto(proto.GetGleicheWords(), proto.GetGleicheWords().ItemsSize());
    Agreements.LoadFromProto(proto.GetAgreements(), proto.GetAgreements().WordsIndexSize());
    LangMasks.LoadFromProto(proto.GetLangMasks(), proto.GetLangMasks().ItemsSize());
    WordLanguages.LoadFromProto(proto.GetWordLanguages(), proto.GetWordLanguages().WordSize());
    FilterData.LoadFromProto(proto.GetFilterData(), proto.GetFilterData().AllowedGramIndexSize());

    if (!proto.GetFiltersRef().GetBlobKey().empty() && blobs.HasBlob(proto.GetFiltersRef().GetBlobKey()))
        blobs.LoadObject(FilteredArticles, proto.GetFiltersRef().GetBlobKey());
    else
        // FilteredArticles.LoadFromProto(proto.GetFilters(), proto.GetFilters().ArticleIdSize());
        // a fallback from prevous line is removed, it was for transition only (14.03.2011)
        ythrow yexception() << "No filter blob in gazetteer, possibly old binary.";

    blobs.LoadObject(SimpleArticles, proto.GetSimpleArticlesRef().GetBlobKey());
}

void TArticleFilter::Load(TMemoryInput* input)
{
    LoadAsProto<NBinaryFormat::TArticleFilter>(input, *this);
}


static void CollectGramBitSets(const NProtoBuf::RepeatedPtrField<TGrammemSet>& gram_filter,
                               const yset<TWordIndex>& words, ymap<TWordIndex, yvector<TGramBitSet> >& result)
{
    yvector<TGramBitSet> parsed_grams;
    for (int i = 0; i < gram_filter.size() ; ++i)
        ParseGramBitSets(gram_filter.Get(i), parsed_grams);

    if (!parsed_grams.empty())
        for (yset<TWordIndex>::const_iterator w = words.begin(); w != words.end(); ++w)
        {
            yvector<TGramBitSet>& res_grams = result[*w];
            res_grams.insert(res_grams.end(), parsed_grams.begin(), parsed_grams.end());
        }
}

void TArticleFilterBuilder::MakeWordGramFilter(const TSearchKey& key, size_t phrase_length,
                                               NAux::TWordGramIndex& allowed_index, NAux::TWordGramIndex& forbidden_index)
{
    ymap<TWordIndex, yvector<TGramBitSet> > allowed, forbidden;
    yset<TWordIndex> words;
    for (int i = 0; i < key.gram_size(); ++i)
    {
        CollectWordIndexes(key.gram(i).word(), phrase_length, words);
        CollectGramBitSets(key.gram(i).allow(), words, allowed);
        CollectGramBitSets(key.gram(i).forbid(), words, forbidden);
        words.clear();
    }

    allowed_index = AddWordGrams(allowed);
    forbidden_index = AddWordGrams(forbidden);
}

NAux::TWordGramIndex TArticleFilterBuilder::AddWordGrams(const ymap<TWordIndex, yvector<TGramBitSet> >& word_grams)
{
    yvector<NAux::TWordGramIndex> indexes;
    indexes.reserve(word_grams.size());
    for (ymap<TWordIndex, yvector<TGramBitSet> >::const_iterator it = word_grams.begin(); it != word_grams.end(); ++it)
    {
        TCompactStorageIndex gram_index = Grammems.AddRange(it->second.begin(), it->second.end());
        indexes.push_back(WordGrammems.Add(NAux::TWordGram(it->first, gram_index)));
    }
    NAux::TWordGramIndex ret = WordGrammems.Merge(indexes.begin(), indexes.end());
    return ret;
}



static void CollectLangMask(const NProtoBuf::RepeatedField<NProtoBuf::int32>& lang_filter,
                             const yset<TWordIndex>& words, ymap<TWordIndex, TLangMask>& result)
{
    TLangMask mask;
    for (int i = 0; i < lang_filter.size() ; ++i) {
        YASSERT(lang_filter.Get(i) > LANG_UNK && lang_filter.Get(i) <= LANG_MAX);
        mask.SafeSet(static_cast<docLanguage>(lang_filter.Get(i)));
    }

    if (mask.any())
        for (yset<TWordIndex>::const_iterator w = words.begin(); w != words.end(); ++w)
            result[*w] |= mask;
}

void TArticleFilterBuilder::MakeWordLangFilter(const TSearchKey& key, size_t phrase_length,
                                               NAux::TWordLangIndex& allowed_index, NAux::TWordLangIndex& forbidden_index)
{
    ymap<TWordIndex, TLangMask> allowed, forbidden;
    yset<TWordIndex> words;
    for (int i = 0; i < key.lang_size(); ++i)
    {
        CollectWordIndexes(key.lang(i).word(), phrase_length, words);
        CollectLangMask(key.lang(i).allow(), words, allowed);
        CollectLangMask(key.lang(i).forbid(), words, forbidden);
        words.clear();
    }

    allowed_index = AddWordLangs(allowed);
    forbidden_index = AddWordLangs(forbidden);
}

NAux::TWordLangIndex TArticleFilterBuilder::AddWordLangs(const ymap<TWordIndex, TLangMask>& word_langs)
{
    yvector<NAux::TWordLangIndex> indexes;
    indexes.reserve(word_langs.size());
    for (ymap<TWordIndex, TLangMask>::const_iterator it = word_langs.begin(); it != word_langs.end(); ++it)
    {
        YASSERT(it->second.any());
        TCompactStorageIndex lang_index = LangMasks.Add(it->second);
        indexes.push_back(WordLanguages.Add(NAux::TWordLang(it->first, lang_index)));
    }
    return WordLanguages.Merge(indexes.begin(), indexes.end());
}

static void CollectCapitalizations(const NProtoBuf::RepeatedField<NProtoBuf::int32>& capitalizations,
                                   const yset<TWordIndex>& words, ymap<TWordIndex, yset<NAux::TCapitalization> >& result)
{
    yset<NAux::TCapitalization> tmp;
    for (int i = 0; i < capitalizations.size(); ++i)
        tmp.insert(NAux::TCapitalization(capitalizations.Get(i), 0));
    if (!tmp.empty())
        for (yset<TWordIndex>::const_iterator w = words.begin(); w != words.end(); ++w)
            result[*w].insert(tmp.begin(), tmp.end());
}

void TArticleFilterBuilder::MakeWordCaseFilter(const TSearchKey& key, size_t phrase_length,
                                               NAux::TWordCaseIndex& allowed_index, NAux::TWordCaseIndex& forbidden_index,
                                               const yvector<ui64>& capitalization_masks)
{
    ymap<TWordIndex, yset<NAux::TCapitalization> > allowed, forbidden;
    yset<TWordIndex> words;
    for (int i = 0; i < key.case__size(); ++i)
    {
        CollectWordIndexes(key.case_(i).word(), phrase_length, words);
        CollectCapitalizations(key.case_(i).allow(), words, allowed);
        CollectCapitalizations(key.case_(i).forbid(), words, forbidden);
        words.clear();
    }
    allowed_index = AddWordCases(allowed, capitalization_masks);
    forbidden_index = AddWordCases(forbidden, capitalization_masks);
}


NAux::TWordCaseIndex TArticleFilterBuilder::AddWordCases(ymap<TWordIndex, yset<NAux::TCapitalization> >& word_cases,
                                                         const yvector<ui64>& capitalization_masks)
{
    using NAux::TCapitalization;
    yvector<NAux::TWordCaseIndex> indexes;

    for (ymap<TWordIndex, yset<TCapitalization> >::iterator it = word_cases.begin(); it != word_cases.end(); ++it)
    {
        yset<TCapitalization>& cases = it->second;
        yset<TCapitalization>::iterator exact_case = cases.find(TCapitalizationFilter::EXACT);
        if (exact_case != cases.end())
        {
            YASSERT(capitalization_masks.size() > 0);
            cases.erase(exact_case);

            ui64 mask = capitalization_masks[it->first];
            if (mask == TCapitalization::LOWER_MASK)
                cases.insert(TCapitalization(TCapitalizationFilter::LOWER));
            else if (mask == TCapitalization::TITLE_MASK)
                cases.insert(TCapitalization(TCapitalizationFilter::TITLE));
            else if (mask == TCapitalization::UPPER_MASK)
                cases.insert(TCapitalization(TCapitalizationFilter::UPPER));
            else
                cases.insert(TCapitalization(TCapitalizationFilter::EXACT, mask));
        }
        TCompactStorageIndex index = Capitalizations.AddRange(cases.begin(), cases.end());
        indexes.push_back(WordCases.Add(NAux::TWordCase(it->first, index)));
    }

    return WordCases.Merge(indexes.begin(), indexes.end());
}

NAux::TAgrIndex TArticleFilterBuilder::MakeAgrFilter(const TSearchKey& key, size_t phrase_length)
{
    yvector<NAux::TWordGramIndex> agr_indices(key.agr_size());
    for (int i = 0; i < key.agr_size(); ++i)
    {
        const TGleicheFilter& agr = key.agr(i);
        yset<TWordIndex> words;
        CollectWordIndexes(agr.word(), phrase_length, words);
        // ignore agreements on single word or empty set of words
        if (words.size() < 2)
            continue;
        NAux::TWordListIndex words_index = GleicheWords.AddRange(words.begin(), words.end());

        NGleiche::TMask gleiche_mask;
        for (int j = 0; j < agr.type_size(); ++j)
        {
            // convert aliases to canonical type ( % 100 )
            TGleicheFilter::EType canon_type = static_cast<TGleicheFilter::EType>(agr.type(j) % 100);
            gleiche_mask |= NGleiche::GetMask(canon_type);
        }
        agr_indices[i] = Agreements.Add(NAux::TAgr(words_index, gleiche_mask));
    }
    return Agreements.Merge(agr_indices.begin(), agr_indices.end());
}

NAux::TFilterDataIndex TArticleFilterBuilder::AddFilterData(const TSearchKey& key, const yvector<ui64>& capitalization,
                                                            size_t phrase_length)
{
    // gram = "..."
    NAux::TWordGramIndex allowed_gram, forbidden_gram;
    MakeWordGramFilter(key, phrase_length, allowed_gram, forbidden_gram);

    // lang = "..."
    NAux::TWordLangIndex allowed_lang, forbidden_lang;
    MakeWordLangFilter(key, phrase_length, allowed_lang, forbidden_lang);

    // case = "..."
    NAux::TWordCaseIndex allowed_case, forbidden_case;
    MakeWordCaseFilter(key, phrase_length, allowed_case, forbidden_case, capitalization);

    // agr = "..."
    NAux::TAgrIndex agr_index = MakeAgrFilter(key, phrase_length);

    return FilterData.Add(NAux::TFilterData(allowed_gram, forbidden_gram, allowed_case, forbidden_case,
                                            allowed_lang, forbidden_lang, agr_index));
}

NAux::TFilterDataIndex TArticleFilterBuilder::AddEmptyFilterData() {
    const TSearchKey emptyKey;
    const yvector<ui64> emptyCapitalization;
    return AddFilterData(emptyKey, emptyCapitalization, 0);
}


TArticleBucket TArticleFilterBuilder::Add(const TSearchKey& key, const yvector<ui64>& capitalization,
                                        TArticleId article_id, size_t phrase_length)
{
    // this will associate a single article-filter with some phrase.
    NAux::TFilterDataIndex dataIndex = AddFilterData(key, capitalization, phrase_length);
    TArticleBucket bucket;
    if (IsEmptyFilter(dataIndex)) {
        bucket.SimpleIndex = SimpleArticles.Add(article_id);
        bucket.FilterIndex = FilteredArticles.EmptySetIndex;
    } else {
        bucket.SimpleIndex = SimpleArticles.EmptySetIndex;
        bucket.FilterIndex = FilteredArticles.Add(NAux::TFilter(article_id, dataIndex));
    }
    return bucket;
}

NAux::TFilter TArticleFilterBuilder::MergeFilters(const NAux::TFilter& f1, const NAux::TFilter& f2)
{
    YASSERT(f1.ArticleId == f2.ArticleId);

    const NAux::TFilterData& fdata1 = FilterData.GetSingleItem(f1.DataIndex);
    const NAux::TFilterData& fdata2 = FilterData.GetSingleItem(f2.DataIndex);

    // TODO: merge same word-number filters more accurately
    NAux::TFilterData data(WordGrammems.Merge(fdata1.AllowedGramIndex, fdata2.AllowedGramIndex),
                           WordGrammems.Merge(fdata1.ForbiddenGramIndex, fdata2.ForbiddenGramIndex),
                           WordCases.Merge(fdata1.AllowedCaseIndex, fdata2.AllowedCaseIndex),
                           WordCases.Merge(fdata1.ForbiddenCaseIndex, fdata2.ForbiddenCaseIndex),
                           WordLanguages.Merge(fdata1.AllowedLangIndex, fdata2.AllowedLangIndex),
                           WordLanguages.Merge(fdata1.ForbiddenLangIndex, fdata2.ForbiddenLangIndex),
                           Agreements.Merge(fdata1.AgrIndex, fdata2.AgrIndex));

    return NAux::TFilter(f1.ArticleId, FilterData.Add(data));
}

TArticleBucket TArticleFilterBuilder::AddTo(TArticleBucket bucket, const TSearchKey& key, const yvector<ui64>& capitalization,
                                            TArticleId article_id, size_t phrase_length)
{
    // Add article-filter to some phrase associations,
    // but only if there is no filter for such article in already.
    // If there is then new article-filter merged to corresponding existing article-filter.

    NAux::TFilterDataIndex dataIndex = AddFilterData(key, capitalization, phrase_length);
    if (IsEmptyFilter(dataIndex)) {
        bucket.SimpleIndex = SimpleArticles.AddTo(bucket.SimpleIndex, article_id);
        // all previously added filters for this article becomes unnecessary
        bucket.FilterIndex = RemoveArticleFilters(bucket.FilterIndex, article_id);
    } else if (!SimpleArticles.HasItemAt(bucket.SimpleIndex, article_id))
        bucket.FilterIndex = FilteredArticles.AddTo(bucket.FilterIndex, NAux::TFilter(article_id, dataIndex));

    return bucket;
}

NAux::TFilterIndex TArticleFilterBuilder::RemoveArticleFilters(NAux::TFilterIndex filterIndex, TArticleId articleId) {
    for (NIter::TIterator<NAux::TFilter> it = IterFiltered(filterIndex); it.Ok(); ++it)
        if (it->ArticleId == articleId) {
            return FilteredArticles.RemoveFrom(filterIndex, *it);
        }

    return filterIndex;
}

void TStorageProtoSerializer<NBinaryFormat::TWordIndexStorage>::Save(NBinaryFormat::TWordIndexStorage& proto, TWordIndex obj) {
    proto.AddItems(obj);
}

void TStorageProtoSerializer<NBinaryFormat::TWordIndexStorage>::Load(const NBinaryFormat::TWordIndexStorage& proto,
                                                                     size_t index, TWordIndex& obj) {
    obj = static_cast<TWordIndex>(proto.GetItems(index));
}

template <typename TBitSet>
void TStorageProtoSerializer<NBinaryFormat::TBitSetStorage>::Save(NBinaryFormat::TBitSetStorage& proto, const TBitSet& obj) {
    TStringOutput str(*proto.AddItems());
    ::Save(&str, obj);
}

template <typename TBitSet>
void TStorageProtoSerializer<NBinaryFormat::TBitSetStorage>::Load(const NBinaryFormat::TBitSetStorage& proto, size_t index, TBitSet& obj) {
    TStringInput str(proto.GetItems(index));
    ::Load(&str, obj);
}


} // namespace NGzt

