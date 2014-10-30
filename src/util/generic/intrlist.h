#pragma once

#include "utility.h"
#include "noncopyable.h"

#include <util/system/yassert.h>

class TSizeCount {
public:
    inline TSizeCount() throw ()
        : Size_(0)
    {
    }

    inline size_t Size() const throw () {
        return Size_;
    }

    template <class T>
    inline void operator()(const T&) throw () {
        ++Size_;
    }

private:
    size_t Size_;
};

/*
 * two-way linked list
 */
template <class T>
class TIntrusiveListItem: public TNonCopyable {
private:
    typedef TIntrusiveListItem<T> TListItem;

public:
    inline TIntrusiveListItem() throw ()
        : Next_(this)
        , Prev_(Next_)
    {
    }

    inline ~TIntrusiveListItem() throw () {
        Unlink();
    }

public:
    inline bool Empty() const throw () {
        return (Prev_ == this) && (Next_ == this);
    }

    inline void Unlink() throw () {
        if (Empty()) {
            return;
        }

        Prev_->SetNext(Next_);
        Next_->SetPrev(Prev_);

        SetEnd();
    }

    inline void LinkBefore(TListItem* before) throw () {
        Unlink();
        LinkBeforeNoUnlink(before);
    }

    inline void LinkBeforeNoUnlink(TListItem* before) throw () {
        TListItem* const after = before->Prev();

        after->SetNext(this);
        SetPrev(after);
        SetNext(before);
        before->SetPrev(this);
    }

    inline void LinkBefore(TListItem& before) throw () {
        LinkBefore(&before);
    }

    inline void LinkAfter(TListItem* after) throw () {
        Unlink();
        LinkBeforeNoUnlink(after->Next());
    }

    inline void LinkAfter(TListItem& after) throw () {
        LinkAfter(&after);
    }

public:
    inline TListItem* Prev() throw () {
        return Prev_;
    }

    inline const TListItem* Prev() const throw () {
        return Prev_;
    }

    inline TListItem* Next() throw () {
        return Next_;
    }

    inline const TListItem* Next() const throw () {
        return Next_;
    }

public:
    inline void SetEnd() throw () {
        Prev_ = this;
        Next_ = Prev_;
    }

    inline void SetNext(TListItem* item) throw () {
        Next_ = item;
    }

    inline void SetPrev(TListItem* item) throw () {
        Prev_ = item;
    }

public:
    inline T* Node() throw () {
        return static_cast<T*>(this);
    }

    inline const T* Node() const throw () {
        return static_cast<const T*>(this);
    }

private:
    TListItem* Next_;
    TListItem* Prev_;
};

template <class T>
class TIntrusiveList: public TNonCopyable {
private:
    typedef TIntrusiveListItem<T> TListItem;

    template <class TListItem, class TNode>
    class TIteratorBase {
    public:
        typedef TListItem TItem;
        typedef TNode& TReference;
        typedef TNode* TPointer;

        inline TIteratorBase() throw ()
            : Item_(0)
        {
        }

        template <class TListItem_, class TNode_>
        inline TIteratorBase(const TIteratorBase<TListItem_, TNode_>& right) throw ()
            : Item_(right.Item())
        {
        }

        inline TIteratorBase(TItem* item) throw ()
            : Item_(item)
        {
        }

        inline TItem* Item() const throw () {
            return Item_;
        }

        inline void Next() throw () {
            Item_ = Item_->Next();
        }

        inline void Prev() throw () {
            Item_ = Item_->Prev();
        }

        template <class TListItem_, class TNode_>
        inline bool operator==(const TIteratorBase<TListItem_, TNode_>& right) const throw () {
            return Item() == right.Item();
        }

        template <class TListItem_, class TNode_>
        inline bool operator!=(const TIteratorBase<TListItem_, TNode_>& right) const throw () {
            return Item() != right.Item();
        }

        inline TIteratorBase& operator++() throw () {
            Next();

            return *this;
        }

        inline TIteratorBase operator++(int) throw () {
            TIteratorBase ret(*this);

            Next();

            return ret;
        }

        inline TIteratorBase& operator--() throw () {
            Prev();

            return *this;
        }

        inline TIteratorBase operator--(int) throw () {
            TIteratorBase ret(*this);

            Prev();

            return ret;
        }

        inline TReference operator*() const throw () {
            return *Item_->Node();
        }

        inline TPointer operator->() const throw () {
            return Item_->Node();
        }

    private:
        TItem* Item_;
    };

    template <class TIterator>
    class TReverseIteratorBase {
    public:
        typedef typename TIterator::TItem TItem;
        typedef typename TIterator::TReference TReference;
        typedef typename TIterator::TPointer TPointer;

        inline TReverseIteratorBase() throw () {
        }

        template <class TIterator_>
        inline TReverseIteratorBase(const TReverseIteratorBase<TIterator_>& right) throw ()
            : Current_(right.Current_)
        {
        }

        inline explicit TReverseIteratorBase(TIterator item) throw ()
            : Current_(item)
        {
        }

        inline TIterator Base() const throw () {
            return Current_;
        }

        inline TItem* Item() const throw () {
            TIterator ret = Current_;

            return (--ret).Item();
        }

        inline void Next() throw () {
            Current_.Prev();
        }

        inline void Prev() throw () {
            Current_.Next();
        }

        template <class TIterator_>
        inline bool operator==(const TReverseIteratorBase<TIterator_>& right) const throw () {
            return Base() == right.Base();
        }

        template <class TIterator_>
        inline bool operator!=(const TReverseIteratorBase<TIterator_>& right) const throw () {
            return Base() != right.Base();
        }

        inline TReverseIteratorBase& operator++() throw () {
            Next();

            return *this;
        }

        inline TReverseIteratorBase operator++(int) throw () {
            TReverseIteratorBase ret(*this);

            Next();

            return ret;
        }

        inline TReverseIteratorBase& operator--() throw () {
            Prev();

            return *this;
        }

        inline TReverseIteratorBase operator--(int) throw () {
            TReverseIteratorBase ret(*this);

            Prev();

            return ret;
        }

        inline TReference operator*() const throw () {
            TIterator ret = Current_;

            return *--ret;
        }

        inline TPointer operator->() const throw () {
            TIterator ret = Current_;

            return &*--ret;
        }

    private:
        TIterator Current_;
    };

public:
    typedef TIteratorBase<TListItem, T> TIterator;
    typedef TIteratorBase<const TListItem, const T> TConstIterator;

    typedef TReverseIteratorBase<TIterator> TReverseIterator;
    typedef TReverseIteratorBase<TConstIterator> TConstReverseIterator;

    typedef TIterator iterator;
    typedef TConstIterator const_iterator;

    typedef TReverseIterator reverse_iterator;
    typedef TConstReverseIterator const_reverse_iterator;

public:
    inline void Swap(TIntrusiveList& right) throw () {
        TIntrusiveList temp;

        temp.Append(right);
        YASSERT(right.Empty());
        right.Append(*this);
        YASSERT(this->Empty());
        this->Append(temp);
        YASSERT(temp.Empty());
    }

public:
    inline bool Empty() const throw () {
        return End_.Empty();
    }

    inline size_t Size() const throw () {
        TSizeCount sizeCount;

        ForEach(sizeCount);

        return sizeCount.Size();
    }

    inline void Clear() throw () {
        End_.Unlink();
    }

public:
    inline TIterator Begin() throw () {
        return ++End();
    }

    inline TIterator End() throw () {
        return TIterator(&End_);
    }

    inline TConstIterator Begin() const throw () {
        return ++End();
    }

    inline TConstIterator End() const throw () {
        return TConstIterator(&End_);
    }

    inline TReverseIterator RBegin() throw () {
        return TReverseIterator(End());
    }

    inline TReverseIterator REnd() throw () {
        return TReverseIterator(Begin());
    }

    inline TConstReverseIterator RBegin() const throw () {
        return TConstReverseIterator(End());
    }

    inline TConstReverseIterator REnd() const throw () {
        return TConstReverseIterator(Begin());
    }

public:
    inline T* Back() throw () {
        return End_.Prev()->Node();
    }

    inline T* Front() throw () {
        return End_.Next()->Node();
    }

    inline const T* Back() const throw () {
        return End_.Prev()->Node();
    }

    inline const T* Front() const throw () {
        return End_.Next()->Node();
    }

    inline void PushBack(TListItem* item) throw () {
        item->LinkBefore(End_);
    }

    inline void PushFront(TListItem* item) throw () {
        item->LinkAfter(End_);
    }

    inline T* PopBack() throw () {
        TListItem* const ret = End_.Prev();

        ret->Unlink();

        return ret->Node();
    }

    inline T* PopFront() throw () {
        TListItem* const ret = End_.Next();

        ret->Unlink();

        return ret->Node();
    }

    inline void Append(TIntrusiveList& list) throw () {
        Cut(list.Begin(), list.End(), End());
    }

    inline static void Cut(TIterator begin, TIterator end, TIterator pasteBefore) throw () {
        if (begin == end) {
            return;
        }

        TListItem* const cutFront = begin.Item();
        TListItem* const gapBack = end.Item();

        TListItem* const gapFront = cutFront->Prev();
        TListItem* const cutBack = gapBack->Prev();

        gapFront->SetNext(gapBack);
        gapBack->SetPrev(gapFront);

        TListItem* const pasteBack = pasteBefore.Item();
        TListItem* const pasteFront = pasteBack->Prev();

        pasteFront->SetNext(cutFront);
        cutFront->SetPrev(pasteFront);

        cutBack->SetNext(pasteBack);
        pasteBack->SetPrev(cutBack);
    }

public:
    template <class TFunctor>
    inline void ForEach(TFunctor& functor) {
        TIterator i = Begin();

        while (i != End()) {
            functor(&*(i++));
        }
    }

    template <class TFunctor>
    inline void ForEach(TFunctor& functor) const {
        TConstIterator i = Begin();

        while (i != End()) {
            functor(&*(i++));
        }
    }

    template <class TComparer>
    inline void QuickSort(const TComparer& comparer) {
        if (Begin() == End() || ++Begin() == End()) {
            return;
        }

        T* const pivot = PopFront();
        TIntrusiveList bigger;
        TIterator i = Begin();

        while (i != End()) {
            if (comparer(*pivot, *i)) {
                bigger.PushBack(&*i++);
            } else {
                ++i;
            }
        }

        this->QuickSort(comparer);
        bigger.QuickSort(comparer);

        PushBack(pivot);
        Append(bigger);
    }

private:
    TListItem End_;
};

template <class T, class D>
class TIntrusiveListWithAutoDelete: public TIntrusiveList<T> {
public:
    typedef typename TIntrusiveList<T>::TIterator TIterator;
    typedef typename TIntrusiveList<T>::TConstIterator TConstIterator;

    typedef typename TIntrusiveList<T>::TReverseIterator TReverseIterator;
    typedef typename TIntrusiveList<T>::TConstReverseIterator TConstReverseIterator;

    typedef TIterator iterator;
    typedef TConstIterator const_iterator;

    typedef TReverseIterator reverse_iterator;
    typedef TConstReverseIterator const_reverse_iterator;

public:
    inline ~TIntrusiveListWithAutoDelete() throw () {
        this->Clear();
    }

public:
    inline void Clear() throw () {
        while (!this->Empty()) {
            D::Destroy(this->PopFront());
        }
    }

    inline static void Cut(TIterator begin, TIterator end) throw () {
        TIntrusiveListWithAutoDelete<T, D> temp;
        Cut(begin, end, temp.End());
    }

    inline static void Cut(TIterator begin, TIterator end, TIterator pasteBefore) throw () {
        TIntrusiveList<T>::Cut(begin, end, pasteBefore);
    }
};

/*
 * one-way linked list
 */
template <class T>
class TIntrusiveSListItem {
private:
    typedef TIntrusiveSListItem<T> TListItem;

public:
    inline TIntrusiveSListItem() throw ()
        : Next_(0)
    {
    }

    inline ~TIntrusiveSListItem() throw () {
    }

    inline bool IsEnd() const throw () {
        return Next_ == 0;
    }

    inline TListItem* Next() throw () {
        return Next_;
    }

    inline const TListItem* Next() const throw () {
        return Next_;
    }

    inline void SetNext(TListItem* item) throw () {
        Next_ = item;
    }

public:
    inline T* Node() throw () {
        return static_cast<T*>(this);
    }

    inline const T* Node() const throw () {
        return static_cast<const T*>(this);
    }

private:
    TListItem* Next_;
};

template <class T>
class TIntrusiveSList {
private:
    typedef TIntrusiveSListItem<T> TListItem;

public:
    template <class TListItem, class TNode>
    class TIteratorBase {
    public:
        inline TIteratorBase(TListItem* item) throw ()
            : Item_(item)
        {
        }

        inline void Next() throw () {
            Item_ = Item_->Next();
        }

        inline bool operator==(const TIteratorBase& right) const throw () {
            return Item_ == right.Item_;
        }

        inline bool operator!=(const TIteratorBase& right) const throw () {
            return Item_ != right.Item_;
        }

        inline TIteratorBase& operator++() throw () {
            Next();

            return *this;
        }

        inline TIteratorBase operator++(int) throw () {
            TIteratorBase ret(*this);

            Next();

            return ret;
        }

        inline TNode& operator*() throw () {
            return *Item_->Node();
        }

        inline TNode* operator->() throw () {
            return Item_->Node();
        }

    private:
        TListItem* Item_;
    };

public:
    typedef TIteratorBase<TListItem, T> TIterator;
    typedef TIteratorBase<const TListItem, const T> TConstIterator;

    typedef TIterator iterator;
    typedef TConstIterator const_iterator;

public:
    inline TIntrusiveSList() throw ()
        : Begin_(0)
    {
    }

    inline void Swap(TIntrusiveSList& right) throw () {
        DoSwap(Begin_, right.Begin_);
    }

public:
    inline bool Empty() const throw () {
        return Begin_ == 0;
    }

    inline size_t Size() const throw () {
        TSizeCount sizeCount;

        ForEach(sizeCount);

        return sizeCount.Size();
    }

    inline void Clear() throw () {
        Begin_ = 0;
    }

public:
    inline TIterator Begin() throw () {
        return TIterator(Begin_);
    }

    inline TIterator End() throw () {
        return TIterator(0);
    }

    inline TConstIterator Begin() const throw () {
        return TConstIterator(Begin_);
    }

    inline TConstIterator End() const throw () {
        return TConstIterator(0);
    }

public:
    inline T* Front() throw () {
        YASSERT(Begin_);
        return Begin_->Node();
    }

    inline const T* Front() const throw () {
        YASSERT(Begin_);
        return Begin_->Node();
    }

    inline void PushFront(TListItem* item) throw () {
        item->SetNext(Begin_);
        Begin_ = item;
    }

    inline T* PopFront() throw () {
        YASSERT(Begin_);

        TListItem* const ret = Begin_;
        Begin_ = Begin_->Next();

        return ret->Node();
    }

public:
    inline void Reverse() throw () {
        TIntrusiveSList temp;

        while (!Empty()) {
            temp.PushFront(PopFront());
        }

        this->Swap(temp);
    }

    template <class TFunctor>
    inline void ForEach(TFunctor& functor) const {
        TListItem* i = Begin_;

        while (i) {
            TListItem* const next = i->Next();
            functor(i->Node());
            i = next;
        }
    }

private:
    TListItem* Begin_;
};
