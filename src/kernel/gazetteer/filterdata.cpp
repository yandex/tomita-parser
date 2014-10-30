#include "filterdata.h"
#include <util/string/vector.h>

#include <kernel/gazetteer/base.pb.h>
#include <kernel/gazetteer/binary.pb.h>

#include "articlefilter.h"

namespace NGzt {

namespace NAux
{

const ui64 TCapitalization::LOWER_MASK = 0;
const ui64 TCapitalization::TITLE_MASK = 1;
const ui64 TCapitalization::UPPER_MASK = Max<ui64>();

void TSingleWordFilter::SaveToStorage(NGzt::NBinaryFormat::TSingleWordFilterStorage& proto) const {
    proto.AddWord(Word);
    proto.AddIndex_(Index);
}

void TSingleWordFilter::LoadFromStorage(const NGzt::NBinaryFormat::TSingleWordFilterStorage& proto, size_t index) {
    Word = static_cast<TWordIndex>(proto.GetWord(index));
    Index = proto.GetIndex_(index);
}

void TSingleWordFilter::Save(TOutputStream* output) const {
    ::Save(output, Word);
    ::Save(output, Index);
}

void TSingleWordFilter::Load(TInputStream* input) {
    ::Load(input, Word);
    ::Load(input, Index);
}

void TCapitalization::SaveToStorage(NGzt::NBinaryFormat::TCapitalizationStorage& proto) const {
    proto.AddType(Type);
    proto.AddCustomMask(IsCustom() ? CustomMask : 0);
}

void TCapitalization::LoadFromStorage(const NGzt::NBinaryFormat::TCapitalizationStorage& proto, size_t index) {
    Type = proto.GetType(index);
    CustomMask = IsCustom() ? proto.GetCustomMask(index) : 0;
}

void TCapitalization::Save(TOutputStream* output) const {
    ::Save(output, Type);
    if (IsCustom())
        ::Save(output, CustomMask);
}
void TCapitalization::Load(TInputStream* input) {
    ::Load(input, Type);
    if (IsCustom())
        ::Load(input, CustomMask);
}


void TAgr::SaveToStorage(NGzt::NBinaryFormat::TAgrStorage& proto) const {
    proto.AddWordsIndex(WordsIndex);
    TStringOutput str(*proto.AddGleicheTypes());
    ::Save(&str, GleicheTypes);
}

void TAgr::LoadFromStorage(const NGzt::NBinaryFormat::TAgrStorage& proto, size_t index) {
    WordsIndex = proto.GetWordsIndex(index);
    TStringInput str(proto.GetGleicheTypes(index));
    ::Load(&str, GleicheTypes);
}

void TAgr::Save(TOutputStream* output) const {
    ::Save(output, WordsIndex);
    ::Save(output, GleicheTypes);
}

void TAgr::Load(TInputStream* input) {
    ::Load(input, WordsIndex);
    ::Load(input, GleicheTypes);
}

void TFilterData::SaveToStorage(NGzt::NBinaryFormat::TFilterDataStorage& proto) const {
    proto.AddAllowedGramIndex(AllowedGramIndex);
    proto.AddAllowedCaseIndex(AllowedCaseIndex);
    proto.AddAllowedLangIndex(AllowedLangIndex);

    proto.AddForbiddenGramIndex(ForbiddenGramIndex);
    proto.AddForbiddenCaseIndex(ForbiddenCaseIndex);
    proto.AddForbiddenLangIndex(ForbiddenLangIndex);

    proto.AddAgrIndex(AgrIndex);

}

void TFilterData::LoadFromStorage(const NGzt::NBinaryFormat::TFilterDataStorage& proto, size_t index) {
    AllowedGramIndex = proto.GetAllowedGramIndex(index);
    AllowedCaseIndex = proto.GetAllowedCaseIndex(index);
    AllowedLangIndex = proto.GetAllowedLangIndex(index);

    ForbiddenGramIndex = proto.GetForbiddenGramIndex(index);
    ForbiddenCaseIndex = proto.GetForbiddenCaseIndex(index);
    ForbiddenLangIndex = proto.GetForbiddenLangIndex(index);

    AgrIndex = proto.GetAgrIndex(index);
}

void TFilterData::Save(TOutputStream* output) const {
    ::Save(output, AllowedGramIndex);
    ::Save(output, AllowedCaseIndex);
    ::Save(output, AllowedLangIndex);

    ::Save(output, ForbiddenGramIndex);
    ::Save(output, ForbiddenCaseIndex);
    ::Save(output, ForbiddenLangIndex);

    ::Save(output, AgrIndex);
}

void TFilterData::Load(TInputStream* input) {
    ::Load(input, AllowedGramIndex);
    ::Load(input, AllowedCaseIndex);
    ::Load(input, AllowedLangIndex);

    ::Load(input, ForbiddenGramIndex);
    ::Load(input, ForbiddenCaseIndex);
    ::Load(input, ForbiddenLangIndex);

    ::Load(input, AgrIndex);
}

void TFilter::Save(TOutputStream* output) const {
    ::Save(output, ArticleId);
    ::Save(output, DataIndex);
}

void TFilter::Load(TInputStream* input) {
    ::Load(input, ArticleId);
    ::Load(input, DataIndex);
}

}   // namespace NAux


TFilterStorageBuilder::TIndex TFilterStorageBuilder::PushBackSingle(const NAux::TFilter& item)
{
    FilterIndex[item.DataIndex].push_back();
    TArticle2Filter& p = FilterIndex[item.DataIndex].back();
    p.ArticleId = item.ArticleId;
    p.Index = TBase::NextItemIndex();
    TBase::PushBackSingle(item);
    return p.Index;
}

TFilterStorageBuilder::TIndex TFilterStorageBuilder::Add(const NAux::TFilter& item)
{
    // Here we allow having several same TFilter with different TIndex - for sake of quick index update
    // in most cases article filters are added sequentially with same article id adjacent,
    // so check the last available article-id in FilterIndex
    if (item.DataIndex >= FilterIndex.size()) {
        FilterIndex.resize(item.DataIndex + 1);
        return PushBackSingle(item);
    } else if (FilterIndex[item.DataIndex].empty() || FilterIndex[item.DataIndex].back().ArticleId != item.ArticleId)
        return PushBackSingle(item);
    else
        return FilterIndex[item.DataIndex].back().Index;
}

TFilterStorageBuilder::TIndex TFilterStorageBuilder::AddTo(TIndex index, const NAux::TFilter& item)
{
    if (IsEmptySet(index)) {
        return Add(item);

    } else if (TBase::IsSingleItem(index)) {
        // if existing filter for same article - try merging
        const NAux::TFilter& existingFilter = TBase::GetSingleItem(index);
        if (existingFilter.ArticleId == item.ArticleId)
            return (existingFilter == item) ? index : Add(Host->MergeFilters(existingFilter, item));

        TBase::TItemSet newSet;
        newSet.insert(index);       // existing
        newSet.insert(Add(item));   // new
        return AddSet(newSet);

    } else {
        TBase::TRefItemSet& existingSet = TBase::GetMutableItemSet(index);
        YASSERT(existingSet.size() > 1);
        for (TBase::TItemSet::iterator it = existingSet.begin(); it != existingSet.end(); ++it) {
            TIndex existingIndex = *it;
            const NAux::TFilter& existingFilter = TBase::GetSingleItem(existingIndex);
            if (existingFilter.ArticleId == item.ArticleId) {
                if (existingFilter == item)
                    return index;

                TIndex newIndex = Add(Host->MergeFilters(existingFilter, item));
                existingSet.erase(it);

                // an existing set could have external users of such set index already.
                // look at RefCount to determine if we could modify this set or should create new one
                if (existingSet.RefCount > 1) {
                    existingSet.RefCount -= 1;
                    TBase::TItemSet newSet(existingSet);    // copy without existingIndex
                    existingSet.insert(existingIndex);      // restore
                    newSet.insert(newIndex);
                    return AddSet(newSet);
                } else {
                    existingSet.insert(newIndex);
                    return index;
                }
            }
        }

        // no filter with this article id, just add to set (existing or new one)
        TIndex newIndex = Add(item);

        if (existingSet.RefCount > 1) {
            existingSet.RefCount -= 1;
            TBase::TItemSet newSet(existingSet);
            newSet.insert(newIndex);
            return AddSet(newSet);
        } else {
            existingSet.insert(newIndex);
            return index;
        }
    }
}


}   // NGzt
