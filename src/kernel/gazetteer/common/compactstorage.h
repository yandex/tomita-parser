#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/stream/input.h>
#include <util/stream/str.h>
#include <util/generic/ylimits.h>
#include <util/ysaveload.h>

#include <library/iter/iter.h>

#include "vector2d.h"
#include "hashindex.h"
#include "bytesize.h"

namespace NGzt
{


// defined in this file:
template <typename T> class TCompactStorage;
template <typename T, bool> class TCompactStorageBuilder;


// TCompactStorage =======================================

typedef ui32 TCompactStorageIndex;

template <typename T>
class TCompactStorage
{
public:
    typedef TCompactStorageIndex TIndex;

    static inline bool IsEmptySet(TIndex index) {
        return TCompactStorageBuilder<T, true>::IsEmptySet(index);  // empty set should always be the first set
    }

    inline TCompactStorage() {
    }

    // items iterator = mapped iterator: TIndex -> T
    friend class NIter::TMappedIterator<T, NIter::TValueIterator<const TIndex> , TCompactStorage<T> >;
    typedef NIter::TMappedIterator<T, NIter::TValueIterator<const TIndex> , TCompactStorage<T> > TIter;

    inline TIter operator[](TIndex index) const;

    // only for single items access (!)
    inline const T& GetSingleItem(TIndex index) const {
        return GetItem(index);
    }

    template <typename TStorageProto>
    void LoadFromProto(const TStorageProto& proto, size_t count);

    void Save(TOutputStream* stream) const;
    void Load(TInputStream* stream);

    size_t ByteSize() const {
        return ::ByteSize(Items) + ItemSets.ByteSize();
    }

private:
    friend class TCompactStorageBuilder<T, true>;
    friend class TCompactStorageBuilder<T, false>;

    // for TItemSetIterator to use @this as mapper
    inline const T& operator()(TIndex index) const {
        return GetItem(index);
    }

    // even indices correspond to single items from Items
    // odd indices - to set of items from ItemSets
    // overflow check is done on serialization.

    inline bool IsSingleItem(TIndex index) const {
        return (index & 1) == 0;
    }

    inline size_t GetItemIndex(TIndex index) const {
        YASSERT(IsSingleItem(index));
        return index >> 1;
    }

    inline size_t GetItemSetIndex(TIndex index) const {
        YASSERT(!IsSingleItem(index));
        return index >> 1;
    }

    inline const T& GetItem(TIndex index) const {
        return Items[GetItemIndex(index)];
    }

    inline NIter::TValueIterator<const TIndex> IterItemSet(TIndex index) const {
        return ItemSets[GetItemSetIndex(index)];
    }

    void Verify() const;

private: //data//
    yvector<T> Items;
    TVectorsVector<TIndex> ItemSets;

    DECLARE_NOCOPY(TCompactStorage);
};


template <typename TValue>
struct TSetHasher
{
    static const size_t Initial = 2166136261U;

    typedef yset<TValue> TSet;
    inline size_t operator()(const TSet& s) const {
        THash<TValue> hasher;
        size_t res = Initial;
        for (typename TSet::const_iterator it = s.begin(); it != s.end(); ++it)
            res = hasher(*it) ^ res;
        return res;
    }
};


// TCompactStorageBuilder =======================================

template <typename T, bool UniqSets = true>
class TCompactStorageBuilder
{
public:
    typedef TCompactStorageIndex TIndex;

    static const TIndex EmptySetIndex = 1;
    static inline bool IsEmptySet(TIndex index) {
        return index == EmptySetIndex;
    }

    inline TCompactStorageBuilder(TIndex maxIndex = Max<TIndex>())
        : MaxIndex(maxIndex)
    {
        // empty set should be the first set with fixed index
        AddEmptySet();
    }

    inline NIter::TIterator<T> operator[](TIndex index) const;

    // only for single items access (!)
    inline const T& GetSingleItem(TIndex index) const {
        return Data.GetItem(index);
    }

    inline bool HasItemAt(TIndex index, const T& item) const {
        typename TItemIndex::const_iterator it = ItemIndex.find(item);
        if (it == ItemIndex.end())
            return false;

        if (IsSingleItem(index))
            return index == it->second;
        else {
            const TItemSet& itemSet = GetItemSet(index);
            return itemSet.find(it->second) != itemSet.end();
        }
    }

    TIndex Add(const T& item);
    TIndex AddTo(TIndex index, const T& item);

    template <class TItemIter> TIndex AddRange(TItemIter b, TItemIter e);
    template <class TItemIter> TIndex AddRangeTo(TIndex index, TItemIter b, TItemIter e);

    TIndex Merge(TIndex index1, TIndex index2);
    template <class TIndexIter> TIndex Merge(TIndexIter index_begin, TIndexIter index_end);

    TIndex RemoveFrom(TIndex index, const T& item);

    void Save(TOutputStream* out) const;

    template <typename TStorageProto>
    void SaveToProto(TStorageProto& proto) const;

    Stroka DebugString() const;

    size_t ByteSize() const {
        return Data.ByteSize() + ItemSets.ByteSize() + ::ByteSize(ItemIndex);
    }

    Stroka DebugByteSize() const {
        TStringStream str;
        str << "CompactStorageBuilder:\n" << DEBUG_BYTESIZE(Data) << DEBUG_BYTESIZE(ItemSets) << DEBUG_BYTESIZE(ItemIndex);
        return str;
    }

protected:

    typedef yset<TIndex> TItemSet;
    class TRefItemSet: public TItemSet {
    public:
        size_t RefCount;

        inline TRefItemSet()
            : RefCount(0)
        {
        }

        inline size_t ByteSize() const {
            return sizeof(TRefItemSet) + ::OuterByteSize<TItemSet>(*this);
        }
    };

    // define item-set-iterator as mapped iterator: TIndex -> T
    typedef TCompactStorageBuilder<T, UniqSets> TSelf;
    typedef NIter::TSetIterator<const TIndex> TConstSetIterator;
    friend class NIter::TMappedIterator<T, TConstSetIterator, TSelf>;
    typedef NIter::TMappedIterator<T, TConstSetIterator, TSelf> TItemSetIterator;

    // for TItemSetIterator to use @this as mapper
    inline const T& operator()(TIndex index) const {
        return Data.GetItem(index);
    }

    inline bool IsSingleItem(TIndex index) const {
        return Data.IsSingleItem(index);
    }

    ui64 CalculateTotalSetsSize() const;
    void CheckOverflow() const;
    void Verify() const;

    void PushBackSingle(const T& item) {
        Data.Items.push_back(item);
    }

    TIndex AddSet(TItemSet& items);
    TIndex AddSetOrSingle(TItemSet& items);

    void AddEmptySet();

    template <class It>
    inline void AddRangeToSet(TItemSet& items, It b, It e) {
        for (; b != e; ++b)
            items.insert(Add(*b));
    }

    inline const TItemSet& GetItemSet(TIndex index) const {
        return ItemSets[Data.GetItemSetIndex(index)];
    }

    inline TRefItemSet& GetMutableItemSet(TIndex index) {
        return ItemSets[Data.GetItemSetIndex(index)];
    }

    inline NIter::TSetIterator<const TIndex> IterItemSet(TIndex index) const {
        return NIter::TSetIterator<const TIndex>(GetItemSet(index));
    }

    inline size_t ToItemIndex(size_t idx) const {
        return idx << 1;
    }

    inline size_t ToSetIndex(size_t idx) const {
        return (idx << 1) + 1;
    }

    inline size_t NextItemIndex() const {
        return ToItemIndex(Data.Items.size());
    }

    inline size_t NextSetIndex() const {
        return ToSetIndex(ItemSets.Size());
    }

protected: //data//

    TCompactStorage<T> Data;       //used for storing Items only!

    TIndexator<TRefItemSet, UniqSets, TSetHasher<TIndex> > ItemSets;

    typedef yhash<T, TIndex> TItemIndex;
    TItemIndex ItemIndex;

    const TIndex MaxIndex;

    DECLARE_NOCOPY(TCompactStorageBuilder);
};


// TStorageProtoSerializer =======================================
template <typename TStorageProto>
struct TStorageProtoSerializer
{
    template <typename T>
    static inline void Save(TStorageProto& proto, const T& obj) {
        obj.SaveToStorage(proto);
    }

    template <typename T>
    static inline void Load(const TStorageProto& proto, size_t index, T& obj) {
        obj.LoadFromStorage(proto, index);
    }
};


template <typename T>
inline typename TCompactStorage<T>::TIter TCompactStorage<T>::operator[](TIndex index) const
{
    if (IsSingleItem(index))
        return TIter(NIter::TValueIterator<const TIndex>(index), this);
    else
        return TIter(IterItemSet(index), this);
}

template <typename T, bool u>
inline NIter::TIterator<T> TCompactStorageBuilder<T, u>::operator[](TIndex index) const
{
    if (IsSingleItem(index))
        return NIter::TIterator<T>::FromScalar(&Data.GetItem(index));
    else
        return NIter::TIterator<T>::FromIterator(TItemSetIterator(IterItemSet(index), this));
}








template <typename T, bool u>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::Add(const T& item)
{
    TPair<typename TItemIndex::iterator, bool> res = ItemIndex.insert(TPair<T, TIndex>(item, NextItemIndex()));
    if (res.second)
        PushBackSingle(item);
    return (TIndex)(res.first->second); // overflow check will be done on serialization.
}

template <typename T, bool u>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::AddSet(TItemSet& items)
{
    YASSERT(items.size() != 1);  //a given @items should not be a set with single item (but it could be empty!)

    // a special case of empty set
    if (items.empty())
        return EmptySetIndex;

    TRefItemSet refItems;
    refItems.swap(items);
    size_t rawIndex = ItemSets.Add(refItems);
    ItemSets[rawIndex].RefCount += 1;
    return ToSetIndex(rawIndex);
}

template <typename T, bool u>
void TCompactStorageBuilder<T, u>::AddEmptySet() {
    // should be the first added set
    YASSERT(IsEmptySet(NextSetIndex()));
    TRefItemSet empty;
    ItemSets.Add(empty);
}

template <typename T, bool u>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::AddSetOrSingle(TItemSet& items)
{
    if (items.size() == 1)
        // just return index of already added item.
        return (TIndex)(*(items.begin()));
    else
        return AddSet(items);
}

template <typename T, bool u>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::AddTo(TIndex index, const T& item)
{
    TIndex item_index = Add(item);
    if (index == EmptySetIndex || index == item_index)
        return item_index;
    else if (IsSingleItem(index)) {
        TItemSet new_set;
        new_set.insert(index);
        new_set.insert(item_index);
        return AddSet(new_set);
    } else {
        TRefItemSet& existing_set = GetMutableItemSet(index);
        YASSERT(!existing_set.empty());
        // an existing set could have external users of such set index already.
        // look at RefCount to determine if we could modify this set or should create new one
        if (existing_set.RefCount > 1) {
            existing_set.RefCount -= 1;
            TItemSet new_set(existing_set);
            new_set.insert(item_index);
            return AddSet(new_set);
        } else {
            existing_set.insert(item_index);
            return index;
        }
    }
}

template <typename T, bool u>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::RemoveFrom(TIndex index, const T& item)
{
    typename TItemIndex::const_iterator it = ItemIndex.find(item);
    // no such @item in storage
    if (it == ItemIndex.end())
        return index;

    if (IsSingleItem(index)) {
        if (it->second == index)
            return EmptySetIndex;
        else
            return index;
    }

    TRefItemSet& existing_set = GetMutableItemSet(index);
    if (existing_set.empty())
        return EmptySetIndex;

    if (existing_set.RefCount > 1) {
        existing_set.RefCount -= 1;
        TItemSet new_set(existing_set);
        new_set.erase(it->second);
        return AddSetOrSingle(new_set);
    } else {
        // a special case: set of 2 items
        if (existing_set.size() == 2) {
            TIndex firstItem = *existing_set.begin();
            TIndex secondItem = *(++existing_set.begin());
            if (firstItem == it->second)
                return secondItem;
            else if (secondItem == it->second)
                return firstItem;
        }
        existing_set.erase(it->second);
        YASSERT(existing_set.size() > 1);
        return index;
    }
}

template <typename T, bool u>
template <typename It>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::AddRange(It b, It e)
{
    TItemSet new_set;
    AddRangeToSet(new_set, b, e);
    return AddSetOrSingle(new_set);
}

template <typename T, bool u>
template <typename It>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::AddRangeTo(TIndex index, It b, It e)
{
    TItemSet new_set;
    AddRangeToSet(new_set, b, e);
    if (IsSingleItem(index)) {
        new_set.insert(index);
        return AddSetOrSingle(new_set);
    } else {
        TRefItemSet& existing_set = GetMutableItemSet(index);
        if (!existing_set.empty()) {
            if (existing_set.RefCount <= 1) {
                existing_set.insert(new_set.begin(), new_set.end());
                return index;
            } else
                existing_set.RefCount -= 1;
        }
        new_set.insert(existing_set.begin(), existing_set.end());
        return AddSet(new_set);
    }
}

template <typename T, bool u>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::Merge(TIndex index1, TIndex index2)
{
    TIndex arr[2] = {index1, index2};
    return Merge<TIndex*>(arr, arr + 2);
}

template <typename T, bool u>
template <typename TIndexIter>
typename TCompactStorageBuilder<T, u>::TIndex TCompactStorageBuilder<T, u>::Merge(TIndexIter index_begin, TIndexIter index_end)
{
    TItemSet new_set;
    for (; index_begin != index_end; ++index_begin) {
        TIndex index = *index_begin;
        if (IsSingleItem(index))
            new_set.insert(index);
        else {
            const TItemSet& existing_set = GetItemSet(index);
            new_set.insert(existing_set.begin(), existing_set.end());
        }
    }
    return AddSetOrSingle(new_set);
}


template <typename T, bool u>
ui64 TCompactStorageBuilder<T, u>::CalculateTotalSetsSize() const
{
    ui64 res = 0;
    for (size_t i = 0; i < ItemSets.Size(); ++i)
        res += ItemSets[i].size();
    return res;
}

template <typename T, bool u>
void TCompactStorageBuilder<T, u>::CheckOverflow() const {
    if ((ui64)NextItemIndex() > (ui64)MaxIndex)
        ythrow yexception() << "Storage single items index limit is reached (" << NextItemIndex() << ", should be less than " << MaxIndex << ").";
    if ((ui64)NextSetIndex() > (ui64)MaxIndex)
        ythrow yexception() << "Storage item sets index limit is reached (" << NextSetIndex() << ", should be less than " << MaxIndex << ").";
}

template <typename T, bool u>
void TCompactStorageBuilder<T, u>::Verify() const {
    CheckOverflow();

    // make sure we have not spoiled EmptySetIndex
    if (ItemSets.Empty() || !ItemSets[0].empty())
        ythrow yexception() << "Storage empty set is corrupted (on serialization).";
}

template <typename T>
void TCompactStorage<T>::Verify() const {
    // first set (if any) should be an empty set
    if (ItemSets.Size() > 0 && ItemSets[0].Ok())
        ythrow yexception() << "Storage empty set is corrupted (on de-serialization).";
}

template <typename T, bool u>
Stroka TCompactStorageBuilder<T, u>::DebugString() const {
    TStringStream str;
    str << "TCompactStorage: " << Data.Items.size() << " unique items, ";
    str << ItemSets.Size() << " sets, ";
    str << CalculateTotalSetsSize() << " total in sets, ";
    str << sizeof(T) << " bytes per item";
    return str.Str();
}

template <typename T, bool u>
template <typename TStorageProto>
void TCompactStorageBuilder<T, u>::SaveToProto(TStorageProto& proto) const
{
    Verify();

    // join all ItemSets together in single long vector.
    TVectorsVector<TIndex> items_sets(ItemSets.Items());
    YASSERT(items_sets.TotalSize() == CalculateTotalSetsSize());

    for (size_t i = 0; i < Data.Items.size(); ++i)
        TStorageProtoSerializer<TStorageProto>::Save(proto, Data.Items[i]);
    items_sets.SaveToProto(*proto.MutableItemSets());
}

template <typename T>
template <typename TStorageProto>
void TCompactStorage<T>::LoadFromProto(const TStorageProto& proto, size_t count)
{
    Items.resize(count);
    for (size_t i = 0; i < count; ++i)
        TStorageProtoSerializer<TStorageProto>::Load(proto, i, Items[i]);
    ItemSets.LoadFromProto(proto.GetItemSets());

    Verify();
}


template <typename T, bool u>
void TCompactStorageBuilder<T, u>::Save(TOutputStream* out) const
{
    Verify();

    // join all ItemSets together in single long vector.
    TVectorsVector<TIndex> items_sets(ItemSets.Items());
    YASSERT(items_sets.TotalSize() == CalculateTotalSetsSize());

    ::Save(out, Data.Items);
    ::Save(out, items_sets);
}

template <typename T>
void TCompactStorage<T>::Save(TOutputStream* stream) const
{
    ::Save(stream, Items);
    ::Save(stream, ItemSets);
}

template <typename T>
void TCompactStorage<T>::Load(TInputStream* stream)
{
    ::Load(stream, Items);
    ::Load(stream, ItemSets);

    Verify();
}

} // namespace NGzt
