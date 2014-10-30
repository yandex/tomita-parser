#pragma once

#include "abstract.h"

#include <kernel/gazetteer/gazetteer.h>



template <bool threadSafe>
class TGeoHierarchy: public TArticleHierarchy<threadSafe> {
    typedef TArticleHierarchy<threadSafe> TBase;
public:
    TGeoHierarchy(const NGzt::TProtoPool& descriptors, const NGzt::TFieldDescriptor& refField)
        : TBase(descriptors, refField)
    {
    }

    inline bool IsGeoArticle(const NGzt::TArticlePtr& a) const {
        return TBase::IsSubType(a);
    }

    bool IsRelated(const NGzt::TArticlePtr& a1, const NGzt::TArticlePtr& a2);
    bool IsStrictAncestor(const NGzt::TArticlePtr& parent, const NGzt::TArticlePtr& child);

private:
    ui32 FindAbstract(const NGzt::TArticlePtr& art, NGzt::TArticlePtr& res);
    ui32 FindAbstractOrSelf(const NGzt::TArticlePtr& art, NGzt::TArticlePtr& res);

private:
    yhash<ui32, ui32> Abstract;      // for article "_g_Moskva/ru" will hold abstract (lang-independent) article "_g_Moscow"
    TMutex Lock;

};



template <bool threadSafe>
ui32 TGeoHierarchy<threadSafe>::FindAbstract(const NGzt::TArticlePtr& art, NGzt::TArticlePtr& res) {
    typename TGuardSelector<TMutex, threadSafe>::TResult guard(Lock);

    TPair<yhash<ui32, ui32>::iterator, bool> ins = Abstract.insert(MakePair(art.GetId(), Max<ui32>()));
    if (ins.second) {
        TWtringBuf title = art.GetTitle();
        const size_t pos = title.rfind(NGzt::ARTICLE_FOLDER_SEPARATOR);
        if (pos != TWtringBuf::npos && pos != 0) {
            res = art.LoadByTitle(title.SubStr(0, pos));
            ins.first->second = res.GetId();
        }
    }
    return ins.first->second;
}

template <bool threadSafe>
ui32 TGeoHierarchy<threadSafe>::FindAbstractOrSelf(const NGzt::TArticlePtr& art, NGzt::TArticlePtr& res) {
    ui32 id = FindAbstract(art, res);
    if (id == Max<ui32>()) {
        id = art.GetId();
        res = art;
    }
    return id;
}

template <bool threadSafe>
bool TGeoHierarchy<threadSafe>::IsRelated(const NGzt::TArticlePtr& a1, const NGzt::TArticlePtr& a2) {

    if (a1.GetId() == a2.GetId())
        return true;

    NGzt::TArticlePtr abs1, abs2;
    ui32 id1 = FindAbstractOrSelf(a1, abs1);
    ui32 id2 = FindAbstractOrSelf(a2, abs2);

    // a little bit ugly - avoiding second deserialization if possible
    if (!abs1.IsNull()) {
        if (!abs2.IsNull())
            return TBase::IsRelated(abs1, abs2);
        else
            return TBase::IsRelated(id2, id1, abs1);
    } else {
        if (!abs2.IsNull())
            return TBase::IsRelated(id1, id2, abs2);
        else
            return TBase::IsRelated(id1, id2, a1);
    }
}

template <bool threadSafe>
bool TGeoHierarchy<threadSafe>::IsStrictAncestor(const NGzt::TArticlePtr& parent, const NGzt::TArticlePtr& child) {
    NGzt::TArticlePtr parentA, childA;  // abstract lang-independent articles without lang-suffix
    ui32 parentId = FindAbstractOrSelf(parent, parentA);
    ui32 childId = FindAbstractOrSelf(child, childA);

    if (!parentA.IsNull() && !childA.IsNull())
        return TBase::IsStrictAncestor(parentA, childA);
    else
        // avoiding second deserialization if possible
        return TBase::IsStrictAncestor(parentId, childId, childA.IsNull() ? parent : childA);
}
