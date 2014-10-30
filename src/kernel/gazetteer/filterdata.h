#pragma once

#include <util/system/defaults.h>

#include <library/lemmer/dictlib/grammarhelper.h>
#include <library/lemmer/dictlib/gleiche.h>

#include <library/comptrie/comptrie_packers.h>

#include <kernel/gazetteer/base.pb.h>
#include "common/compactstorage.h"
#include "common/bytesize.h"

namespace NGzt
{

typedef ui32 TArticleId;        // this is essentially offset of serialized article in ArticlePool
                                // 0 .. Max<i32>() - real articles
                                // Max<i32>() + 1 .. Max<ui32>() - fake articles (parts of compounds)

typedef ui8 TWordIndex;         // word index, forward order, 0-based, for internal usage

class TFilterStorageBuilder;


// forward declarations from binary.proto
namespace NBinaryFormat {
class TSingleWordFilterStorage;
class TCapitalizationStorage;
class TAgrStorage;
class TFilterDataStorage;
class TFilterStorage;
class TWordIndexStorage;
class TBitSetStorage;
}   // namespace NBinaryFormat


// special agreement helpers
namespace NGleiche
{

// category of grammem agreement
enum TCategory
{
    gcCase,
    gcGender,
    gcNumber,
    gcPerson,
    gcTense,
    gcAnim,
    gcFemGender,    // special: agreement of fixed-feminine words with non-feminine descriptors
    gcNominative,   // special: agreemtent only between words in Nomintive or words without case
    gcMax
};

typedef TEnumBitSet<TCategory, gcCase, gcMax> TMask;

}  // namespace NGleiche





// Auxiliary structures to encode the filter
namespace NAux
{

typedef TCompactStorageIndex TWordGramIndex;
typedef TCompactStorageIndex TWordCaseIndex;
typedef TCompactStorageIndex TWordLangIndex;

typedef TCompactStorageIndex TWordListIndex;
typedef TCompactStorageIndex TAgrIndex;

typedef TCompactStorageIndex TFilterDataIndex;
typedef TCompactStorageIndex TFilterIndex;

typedef TVectorsVectorIndex TSimpleArticleIndex;

typedef TCompactStorageIndex TArticleBucketIndex;

// used for storing filters for [gram], [case] and [lang]
struct TSingleWordFilter
{
    TWordIndex Word;
    TCompactStorageIndex Index;

    inline TSingleWordFilter();
    inline TSingleWordFilter(TWordIndex word, TCompactStorageIndex index);
    inline bool operator == (const TSingleWordFilter& other) const;
    inline bool operator < (const TSingleWordFilter& other) const;
    void SaveToStorage(NGzt::NBinaryFormat::TSingleWordFilterStorage& proto) const;
    void LoadFromStorage(const NGzt::NBinaryFormat::TSingleWordFilterStorage& proto, size_t index);
    void Save(TOutputStream* output) const;
    void Load(TInputStream* input);

    DECLARE_BYTESIZE_PODTYPE_METHOD(TSingleWordFilter);
};

typedef TSingleWordFilter TWordGram;
typedef TSingleWordFilter TWordCase;
typedef TSingleWordFilter TWordLang;

struct TCapitalization
{
    // One of pre-defined types
    ui32 Type;

    // For Type == EXACT contains bitmask of the corresponding key-word from start up to the last upper-case letter
    // where 1 means an upper-case letter should be at this position of input word.
    // All letters after masked part should be lower-case.
    // For example, if [ key = { "YaNDex" case=EXACT } ] then CustomMask will be "1011" (lower bit is first).
    // Only first 64 letters are taken into account
    ui64 CustomMask;

    // masks for standard capitalization types
    static const ui64 LOWER_MASK;
    static const ui64 TITLE_MASK;
    static const ui64 UPPER_MASK;

    inline bool IsCustom() const;

    inline TCapitalization();
    inline TCapitalization(ui32 type, ui64 custom = 0);
    inline bool operator == (const TCapitalization& other) const;
    inline bool operator < (const TCapitalization& other) const;
    void SaveToStorage(NGzt::NBinaryFormat::TCapitalizationStorage& proto) const;
    void LoadFromStorage(const NGzt::NBinaryFormat::TCapitalizationStorage& proto, size_t index);
    void Save(TOutputStream* output) const;
    void Load(TInputStream* input);

    DECLARE_BYTESIZE_PODTYPE_METHOD(TCapitalization);
};

struct TAgr
{
    TWordListIndex WordsIndex;
    NGleiche::TMask GleicheTypes;

    inline TAgr();
    inline TAgr(TWordListIndex words_index, NGleiche::TMask gleiche_types);
    inline bool operator == (const TAgr& other) const;
    inline bool operator < (const TAgr& other) const;
    void SaveToStorage(NGzt::NBinaryFormat::TAgrStorage& proto) const;
    void LoadFromStorage(const NGzt::NBinaryFormat::TAgrStorage& proto, size_t index);
    void Save(TOutputStream* output) const;
    void Load(TInputStream* input);

    DECLARE_BYTESIZE_PODTYPE_METHOD(TAgr);
};

struct TFilterData
{
    TWordGramIndex AllowedGramIndex, ForbiddenGramIndex;
    TWordCaseIndex AllowedCaseIndex, ForbiddenCaseIndex;
    TWordLangIndex AllowedLangIndex, ForbiddenLangIndex;
    TAgrIndex AgrIndex;

    inline TFilterData();
    inline TFilterData(TWordGramIndex allowed_gram, TWordGramIndex forbidden_gram,
                       TWordCaseIndex allowed_case, TWordCaseIndex forbidden_case,
                       TWordLangIndex allowed_lang, TWordLangIndex forbidden_lang,
                       TAgrIndex agr_index);
    inline bool operator == (const TFilterData& other) const;
    inline bool operator < (const TFilterData& other) const;
    void SaveToStorage(NGzt::NBinaryFormat::TFilterDataStorage& proto) const;
    void LoadFromStorage(const NGzt::NBinaryFormat::TFilterDataStorage& proto, size_t index);
    void Save(TOutputStream* output) const;
    void Load(TInputStream* input);

    DECLARE_BYTESIZE_PODTYPE_METHOD(TFilterData);
};

struct TFilter
{
    TArticleId ArticleId;
    TFilterDataIndex DataIndex;

    inline TFilter();
    inline TFilter(TArticleId article_id, TFilterDataIndex data_index);
    inline bool operator == (const TFilter& other) const;
    inline bool operator < (const TFilter& other) const;
    void Save(TOutputStream* output) const;
    void Load(TInputStream* input);

    DECLARE_BYTESIZE_PODTYPE_METHOD(TFilter);
};

// a leaf in article tries
struct TArticleBucket
{
    TSimpleArticleIndex SimpleIndex;        // articles without filters
    TFilterIndex FilterIndex;               // articles with filters

    inline TArticleBucket();
    inline TArticleBucket(TSimpleArticleIndex simple, TFilterIndex filter);

    DECLARE_BYTESIZE_PODTYPE_METHOD(TArticleBucket);
};

class TArticleBucketPacker: public NCompactTrie::TPacker<ui32> {
public:
    typedef NCompactTrie::TPacker<ui32> TBase;

    void UnpackLeaf(const char* p, TArticleBucket& data) const {
        TBase::UnpackLeaf(p, data.SimpleIndex);
        p += TBase::SkipLeaf(p);
        TBase::UnpackLeaf(p, data.FilterIndex);
    }

    void PackLeaf(char* buffer, const TArticleBucket& data, size_t computedSize) const {
        size_t simpleSize = TBase::MeasureLeaf(data.SimpleIndex);
        TBase::PackLeaf(buffer, data.SimpleIndex, simpleSize);
        TBase::PackLeaf(buffer + simpleSize, data.FilterIndex, computedSize - simpleSize);
    }

    inline size_t MeasureLeaf(const TArticleBucket& data) const {
        return TBase::MeasureLeaf(data.SimpleIndex) + TBase::MeasureLeaf(data.FilterIndex);
    }

    size_t SkipLeaf(const char* p) const {
        size_t simpleSize = TBase::SkipLeaf(p);
        return simpleSize + TBase::SkipLeaf(p + simpleSize);
    }
};

// inline definitions

// TSingleWordFilter =========================

inline TSingleWordFilter::TSingleWordFilter()
    : Word(0), Index(0)
{
}

inline TSingleWordFilter::TSingleWordFilter(TWordIndex word, TCompactStorageIndex index)
    : Word(word), Index(index)
{
}

inline bool TSingleWordFilter::operator == (const TSingleWordFilter& other) const {
    return Word == other.Word && Index == other.Index;
}

inline bool TSingleWordFilter::operator < (const TSingleWordFilter& other) const {
    return Word < other.Word || Index < other.Index;
}

// TCapitalization =========================

inline TCapitalization::TCapitalization()
    : Type(TCapitalizationFilter::ANY)
    , CustomMask(0)
{
}

inline TCapitalization::TCapitalization(ui32 type, ui64 custom)
    : Type(type)
    , CustomMask(custom)
{
    YASSERT(custom == 0 || IsCustom());
}

inline bool TCapitalization::IsCustom() const {
    return Type == TCapitalizationFilter::EXACT;
}

inline bool TCapitalization::operator == (const TCapitalization& other) const {
    return Type == other.Type && CustomMask == other.CustomMask;
}

inline bool TCapitalization::operator < (const TCapitalization& other) const {
    return Type < other.Type || CustomMask < other.CustomMask;
}


// TAgr =========================

inline TAgr::TAgr()
    : WordsIndex(0)
{
}

inline TAgr::TAgr(TWordListIndex words_index, NGleiche::TMask gleiche_types)
    : WordsIndex(words_index), GleicheTypes(gleiche_types)
{
}

inline bool TAgr::operator == (const TAgr& other) const {
    return WordsIndex == other.WordsIndex && GleicheTypes == other.GleicheTypes;
}

inline bool TAgr::operator < (const TAgr& other) const {
    return WordsIndex < other.WordsIndex || GleicheTypes < other.GleicheTypes;
}

// TFilterData =========================

inline TFilterData::TFilterData()
    : AllowedGramIndex(0), ForbiddenGramIndex(0)
    , AllowedCaseIndex(0), ForbiddenCaseIndex(0)
    , AllowedLangIndex(0), ForbiddenLangIndex(0)
    , AgrIndex(0)
{
}

inline TFilterData::TFilterData(TWordGramIndex allowed_gram, TWordGramIndex forbidden_gram,
                                TWordCaseIndex allowed_case, TWordCaseIndex forbidden_case,
                                TWordLangIndex allowed_lang, TWordLangIndex forbidden_lang,
                                TAgrIndex agr_index)
    : AllowedGramIndex(allowed_gram), ForbiddenGramIndex(forbidden_gram)
    , AllowedCaseIndex(allowed_case), ForbiddenCaseIndex(forbidden_case)
    , AllowedLangIndex(allowed_lang), ForbiddenLangIndex(forbidden_lang)
    , AgrIndex(agr_index)
{
}

inline bool TFilterData::operator == (const TFilterData& other) const {
    return AllowedGramIndex == other.AllowedGramIndex
        && AllowedCaseIndex == other.AllowedCaseIndex
        && AllowedLangIndex == other.AllowedLangIndex

        && ForbiddenGramIndex == other.ForbiddenGramIndex
        && ForbiddenCaseIndex == other.ForbiddenCaseIndex
        && ForbiddenLangIndex == other.ForbiddenLangIndex

        && AgrIndex == other.AgrIndex;
}

inline bool TFilterData::operator < (const TFilterData& other) const {
    return AllowedGramIndex < other.AllowedGramIndex
        || ForbiddenGramIndex < other.ForbiddenGramIndex
        || AllowedCaseIndex < other.AllowedCaseIndex
        || ForbiddenCaseIndex < other.ForbiddenCaseIndex
        || AllowedLangIndex < other.AllowedLangIndex
        || ForbiddenLangIndex < other.ForbiddenLangIndex
        || AgrIndex < other.AgrIndex;
}

// TFilter =========================

inline TFilter::TFilter()
    : ArticleId(0) , DataIndex(0)
{
}

inline TFilter::TFilter(TArticleId article_id, TFilterDataIndex data_index)
    : ArticleId(article_id), DataIndex(data_index)
{
}

inline bool TFilter::operator == (const TFilter& other) const {
    return ArticleId == other.ArticleId && DataIndex == other.DataIndex;
}

inline bool TFilter::operator < (const TFilter& other) const {
    return ArticleId < other.ArticleId  || DataIndex < other.DataIndex;
}


// TArticleBucket =========================

inline TArticleBucket::TArticleBucket()
    : SimpleIndex(0), FilterIndex(0)
{
}

inline TArticleBucket::TArticleBucket(TSimpleArticleIndex simple, TFilterIndex filter)
    : SimpleIndex(simple), FilterIndex(filter)
{
}

} // namespace NAux

} // namespace NGzt


// Should define hashers for types from NAux here, in global namespace
template <>
struct hash<TGramBitSet> {
    size_t inline operator()(const TGramBitSet& s) const throw () {
        return s.GetHash();
    }
};

template <>
struct hash<NGzt::NAux::TSingleWordFilter> {
    inline size_t operator()(const NGzt::NAux::TSingleWordFilter& s) const throw () {
        return IntHash(s.Word) ^ IntHash(s.Index);
    }
};

template <>
struct hash<NGzt::NAux::TCapitalization> {
    inline ui64 operator()(const NGzt::NAux::TCapitalization& s) const throw () {
        return IntHash(s.Type) ^ IntHash(s.CustomMask);
    }
};

template <>
struct hash<TLangMask> {
    size_t inline operator()(const TLangMask& s) const throw () {
        return s.GetHash();
    }
};

template <>
struct hash<NGzt::NAux::TAgr> {
    inline size_t operator()(const NGzt::NAux::TAgr& s) const throw () {
        return IntHash(s.WordsIndex) ^ s.GleicheTypes.GetHash();
    }
};

template <>
struct hash<NGzt::NAux::TFilterData> {
    inline size_t operator()(const NGzt::NAux::TFilterData& s) const throw () {
        size_t res = IntHash(s.AgrIndex);
        res ^= IntHash(s.AllowedGramIndex);
        res ^= IntHash(s.ForbiddenGramIndex);
        res ^= IntHash(s.AllowedCaseIndex);
        res ^= IntHash(s.ForbiddenCaseIndex);
        res ^= IntHash(s.AllowedLangIndex);
        res ^= IntHash(s.ForbiddenLangIndex);
        return res;
    }
};

template <>
struct hash<NGzt::NAux::TFilter> {
    inline size_t operator()(const NGzt::NAux::TFilter& s) const throw () {
        return IntHash(s.ArticleId) ^ IntHash(s.DataIndex);
    }
};


namespace NGzt {

// forward declaration
class TArticleFilterBuilder;

// Specially optimized version of TCompactStorageBuilder for a lot of items
class TFilterStorageBuilder: private TCompactStorageBuilder<NAux::TFilter, false> {
    typedef TCompactStorageBuilder<NAux::TFilter, false> TBase;
public:

    TFilterStorageBuilder(TArticleFilterBuilder& host)
        : Host(&host)
    {
    }

    using TBase::TIndex;
    using TBase::EmptySetIndex;
    using TBase::IsEmptySet;
    using TBase::operator[];
    using TBase::RemoveFrom;

    TIndex Add(const NAux::TFilter& item);
    TIndex AddTo(TIndex index, const NAux::TFilter& item);

    using TBase::Save;
    using TBase::SaveToProto;
    using TBase::DebugString;

    size_t ByteSize() const {
        return TBase::ByteSize() + ::ByteSize(FilterIndex);
    }

private:
    TIndex PushBackSingle(const NAux::TFilter& item);

    struct TArticle2Filter {
        TArticleId ArticleId;
        TIndex Index;

        DECLARE_BYTESIZE_PODTYPE_METHOD(TArticle2Filter)
    };

    TArticleFilterBuilder* Host;
    yvector< yvector<TArticle2Filter> > FilterIndex;

};

} // namespace NGzt
