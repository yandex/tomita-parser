#pragma once

#include "helpers.h"

#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/hash.h>


template <bool threadSafe>
class TArticleHierarchy {
public:
    TArticleHierarchy(const NGzt::TProtoPool& descriptors, const NGzt::TFieldDescriptor& refField);
    ~TArticleHierarchy();

    inline bool IsStrictAncestor(const NGzt::TArticlePtr& parent, const NGzt::TArticlePtr& child) {
        return  IsStrictAncestor(parent.GetId(), child);
    }

    inline bool IsRelated(const NGzt::TArticlePtr& a1, const NGzt::TArticlePtr& a2) {
        return a1.GetId() == a2.GetId() || IsStrictAncestor(a2, a1) || IsStrictAncestor(a1, a2);
    }


    inline bool IsStrictAncestor(ui32 parentId, ui32 childId, const NGzt::TArticlePtr& loader) {
        return  IsStrictAncestor(parentId, GetItem(childId), loader);
    }

    inline bool IsRelated(ui32 id1, ui32 id2, const NGzt::TArticlePtr& loader) {
        return id1 == id2 || IsStrictAncestor(id2, id1, loader) || IsStrictAncestor(id1, id2, loader);
    }



    inline bool IsExactType(const NGzt::TArticlePtr& art) const {
        return art.GetType() == RefField->containing_type();
    }

    inline bool IsSubType(const NGzt::TArticlePtr& art) const {
        return Descriptors->IsSubType(art.GetType(), RefField->containing_type());
    }

private:

    inline NGzt::TRefIterator IterParents(const NGzt::TArticlePtr& art) const;


    struct TItem;
    TItem& GetItem(ui32 id);

    bool CheckRef(ui32 parentId, TItem& child, const NGzt::TRefIterator& ref);
    bool IsStrictAncestor(ui32 parentId, TItem& childItem, const NGzt::TArticlePtr& loader);

    inline bool IsStrictAncestor(ui32 parentId, const NGzt::TArticlePtr& child) {
        return IsStrictAncestor(parentId, GetItem(child.GetId()), child);
    }


private:
    const NGzt::TProtoPool* Descriptors;
    const NGzt::TFieldDescriptor* RefField;

    class TImpl;
    THolder<TImpl> Impl;
};







template <bool threadSafe>
struct TArticleHierarchy<threadSafe>::TItem {
    const ui32 Id;

    // Refer to parent geo-object,
    // if Next == Root - item does not have a parent
    // if Next == NULL - the parent is still not cached
    TItem* volatile Next;

    inline TItem(ui32 id = 0)
        : Id(id)
        , Next(NULL)
    {
    }

    static inline bool SetIfNull(TItem* volatile& src, TItem& dst) {
        return src == NULL && TAtomicCasSelector<threadSafe>::Do(&src, &dst, (TItem*)NULL);
    }

    inline bool SetNext(TItem& next) {
        return SetIfNull(Next, next);
    }
};


template <bool threadSafe>
class TArticleHierarchy<threadSafe>::TImpl {
public:

    inline TImpl() {
    }

    ~TImpl() {
        for (typename TIndex::iterator it = ItemIndex.begin(); it != ItemIndex.end(); ++it)
            delete it->second;
    }

    TItem& GetItemOrInsert(ui32 id) {
        TItem* null = NULL;
        typename TIndex::value_type v(id, null);
        TPair<typename TIndex::iterator, bool> ins;
        {
            typename TGuardSelector<TMutex, threadSafe>::TResult guard(IndexLock);
            ins = ItemIndex.insert(v);
        }
        if (ins.first->second == NULL) {
            THolder<TItem> newItem(new TItem(id));
            if (TItem::SetIfNull(ins.first->second, *newItem))
                newItem.Release();
        }
        return *(ins.first->second);
    }

    inline TItem& SetParent(TItem& child, ui32 parentId) {
        TItem& parent = GetItemOrInsert(parentId);
        child.SetNext(parent);
        return parent;     // note that @parent is not necessarily will be equal to child.Next
    }

    inline void SetParentToRoot(TItem& child) {
        child.SetNext(Root);
    }

    inline bool IsRoot(const TItem& item) const {
        return &item == &Root;
    }

    // Find item with specified @id among @start's parents, or highest non-NULL item from cached hierarchy
    inline TItem& FindParent(TItem& start, ui32 id) const {
        TItem* cur = &start;
        while (cur->Next != NULL) {
            cur = cur->Next;
            if (cur->Id == id)
                break;
        }
        return *cur;
    }

private:
    TItem Root;
    typedef yhash<ui32, TItem* volatile> TIndex;
    TIndex ItemIndex;
    TMutex IndexLock;

    DECLARE_NOCOPY(TImpl);
};






template <bool threadSafe>
TArticleHierarchy<threadSafe>::TArticleHierarchy(const NGzt::TProtoPool& descriptors, const NGzt::TFieldDescriptor& refField)
    : Descriptors(&descriptors)
    , RefField(&refField)
    , Impl(new TImpl)
{
}

template <bool threadSafe>
TArticleHierarchy<threadSafe>::~TArticleHierarchy() {
}


template <bool threadSafe>
inline NGzt::TRefIterator TArticleHierarchy<threadSafe>::IterParents(const NGzt::TArticlePtr& art) const {
    if (IsExactType(art))
        return NGzt::TRefIterator(art, RefField);
    else if (IsSubType(art))
        return NGzt::TRefIterator(art, RefField->number());
    else
        return NGzt::TRefIterator(art, (const NGzt::TFieldDescriptor*)NULL);    // empty iterator
}

template <bool threadSafe>
typename TArticleHierarchy<threadSafe>::TItem& TArticleHierarchy<threadSafe>::GetItem(ui32 id) {
    return Impl->GetItemOrInsert(id);
}

template <bool threadSafe>
bool TArticleHierarchy<threadSafe>::CheckRef(ui32 parentId, TItem& child, const NGzt::TRefIterator& ref) {
    TItem& refItem = Impl->SetParent(child, ref.GetId());
    if (parentId == ref.GetId())   // compare ids first, this just saves one deserialization
        return true;
    else
        return IsStrictAncestor(parentId, refItem, ref.LoadArticle());
}

template <bool threadSafe>
bool TArticleHierarchy<threadSafe>::IsStrictAncestor(ui32 parentId, TItem& childItem, const NGzt::TArticlePtr& loader) {
    // check if some of @childItem's parents in geopart hierarchy is equal to @parentId
    TItem& found = Impl->FindParent(childItem, parentId);

    if (Impl->IsRoot(found))
        return false;       // all hierarchy already cached, no such parent

    if (found.Id == parentId)
        return true;

    // otherwise, deserialize referred parent articles and check them recursively

    // @loader is not required to correspond to @childItem or @parentId, it just used to load other articles
    NGzt::TArticlePtr parent = loader.LoadByOffset(found.Id);
    NGzt::TRefIterator it = IterParents(parent);
    if (!it.Ok())
        Impl->SetParentToRoot(found);    // no parents - the top of hierarchy
    else
        for (; it.Ok(); ++it)
            if (CheckRef(parentId, found, it))
                return true;

    return false;
}
