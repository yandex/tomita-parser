#pragma once

#include "noncopyable.h"

template <class T, class C>
struct TAvlTreeItem;

template <class T, class C>
class TAvlTree: public TNonCopyable {
        typedef TAvlTreeItem<T, C> TTreeItem;
        friend struct TAvlTreeItem<T, C>;

        static inline T* AsT(const TTreeItem* item) throw () {
            return (T*)item;
        }

        class TIterator {
            public:
                inline TIterator(TTreeItem* p, const TAvlTree* t) throw ()
                    : Ptr_(p)
                    , Tree_(t)
                {
                }

                inline ~TIterator() throw () {
                }

                inline bool IsEnd() const throw () {
                    return Ptr_ == 0;
                }

                inline bool IsBegin() const throw () {
                    return Ptr_ == 0;
                }

                inline bool IsFirst() const throw () {
                    return Ptr_ && Ptr_ == Tree_->Head_;
                }

                inline bool IsLast() const throw () {
                    return Ptr_ && Ptr_ == Tree_->Tail_;
                }

                inline T& operator* () const throw () {
                    return *AsT(Ptr_);
                }

                inline T* operator-> () const throw () {
                    return AsT(Ptr_);
                }

                inline TTreeItem* Inc() throw () {
                    return Ptr_ = FindNext(Ptr_);
                }

                inline TTreeItem* Dec() throw () {
                    return Ptr_ = FindPrev(Ptr_);
                }

                inline TIterator& operator++ () throw () {
                    Inc(); return *this;
                }

                inline TIterator operator++ (int) throw () {
                    TIterator ret(*this); Inc(); return ret;
                }

                inline TIterator& operator-- () throw () {
                    Dec(); return *this;
                }

                inline TIterator operator-- (int) throw () {
                    TIterator ret(*this); Dec(); return ret;
                }

                inline TIterator Next() const throw () {
                    return ConstructNext(*this);
                }

                inline TIterator Prev() const throw () {
                    return ConstructPrev(*this);
                }

                inline bool operator== (const TIterator& r) const throw () {
                    return Ptr_ == r.Ptr_;
                }

                inline bool operator!= (const TIterator& r) const throw () {
                    return Ptr_ != r.Ptr_;
                }

            private:
                inline static TIterator ConstructNext(const TIterator& i) throw () {
                    return TIterator(FindNext(i.Ptr_), i.Tree_);
                }

                inline static TIterator ConstructPrev(const TIterator& i) throw () {
                    return TIterator(FindPrev(i.Ptr_), i.Tree_);
                }

                inline static TTreeItem* FindPrev(TTreeItem* el) throw () {
                    if (el->Left_ != 0) {
                        el = el->Left_;

                        while (el->Right_ != 0) {
                            el = el->Right_;
                        }
                    } else {
                        while (true) {
                            TTreeItem* last = el;
                            el = el->Parent_;

                            if (el == 0 || el->Right_ == last) {
                                break;
                            }
                        }
                    }

                    return el;
                }

                static TTreeItem* FindNext(TTreeItem* el) {
                    if (el->Right_ != 0) {
                        el = el->Right_;

                        while (el->Left_) {
                            el = el->Left_;
                        }
                    } else {
                        while (true) {
                            TTreeItem* last = el;
                            el = el->Parent_;

                            if (el == 0 || el->Left_ == last) {
                                break;
                            }
                        }
                    }

                    return el;
                }

            private:
                TTreeItem* Ptr_;
                const TAvlTree* Tree_;
        };

        static inline TIterator ConstructFirst(const TAvlTree* t) throw () {
            return TIterator(t->Head_, t);
        }

        static inline TIterator ConstructLast(const TAvlTree* t) throw () {
            return TIterator(t->Tail_, t);
        }

        static inline bool Compare(const TTreeItem& l, const TTreeItem& r) {
            return C::Compare(*AsT(&l), *AsT(&r));
        }

    public:
        typedef TIterator iterator;

        inline TAvlTree() throw ()
            : Root_(0)
            , Head_(0)
            , Tail_(0)
        {
        }

        inline ~TAvlTree() throw () {
            for (iterator it = Begin(); it != End();) {
                (it++)->TTreeItem::Unlink();
            }
        }

        inline T* Insert(TTreeItem* el, TTreeItem** lastFound = 0) throw () {
            el->Unlink();
            el->Tree_ = this;

            TTreeItem* curEl = Root_;
            TTreeItem* parentEl = 0;
            TTreeItem* lastLess = 0;

            while (true) {
                if (curEl == 0) {
                    AttachRebal(el, parentEl, lastLess);

                    if (lastFound != 0) {
                        *lastFound = el;
                    }

                    return AsT(el);
                }

                if (Compare(*el, *curEl)) {
                    parentEl = lastLess = curEl;
                    curEl = curEl->Left_;
                } else if (Compare(*curEl, *el)) {
                    parentEl = curEl;
                    curEl = curEl->Right_;
                } else {
                    if (lastFound != 0) {
                        *lastFound = curEl;
                    }

                    return 0;
                }
            }
        }

        inline T* Find(const TTreeItem* el) const throw () {
            TTreeItem* curEl = Root_;

            while (curEl) {
                if (Compare(*el, *curEl)) {
                    curEl = curEl->Left_;
                } else if (Compare(*curEl, *el)) {
                    curEl = curEl->Right_;
                } else {
                    return AsT(curEl);
                }
            }

            return 0;
        }

        inline T* LowerBound(const TTreeItem* el) const throw() {
            TTreeItem* curEl = Root_;
            TTreeItem* lowerBound = 0;

            while (curEl) {
                if (Compare(*el, *curEl)) {
                    lowerBound = curEl;
                    curEl = curEl->Left_;
                } else if (Compare(*curEl, *el)) {
                    curEl = curEl->Right_;
                } else {
                    return AsT(curEl);
                }
            }

            return AsT(lowerBound);
        }

        inline T* Erase(TTreeItem* el) throw () {
            if (el->Tree_ == this) {
                return this->EraseImpl(el);
            }

            return 0;
        }

        inline T* EraseImpl(TTreeItem* el) throw () {
            el->Tree_ = 0;

            TTreeItem* replacement;
            TTreeItem* fixfrom;
            long lheight, rheight;

            if (el->Right_) {
                replacement = el->Right_;

                while (replacement->Left_) {
                    replacement = replacement->Left_;
                }

                if (replacement->Parent_ == el) {
                    fixfrom = replacement;
                } else {
                    fixfrom = replacement->Parent_;
                }

                if (el == Head_) {
                    Head_ = replacement;
                }

                RemoveEl(replacement, replacement->Right_);
                ReplaceEl(el, replacement);
            } else if (el->Left_) {
                replacement = el->Left_;

                while (replacement->Right_) {
                    replacement = replacement->Right_;
                }

                if (replacement->Parent_ == el) {
                    fixfrom = replacement;
                } else {
                    fixfrom = replacement->Parent_;
                }

                if (el == Tail_) {
                    Tail_ = replacement;
                }

                RemoveEl(replacement, replacement->Left_);
                ReplaceEl(el, replacement);
            } else {
                fixfrom = el->Parent_;

                if (el == Head_) {
                    Head_ = el->Parent_;
                }

                if (el == Tail_) {
                    Tail_ = el->Parent_;
                }

                RemoveEl(el, 0);
            }

            if (fixfrom == 0) {
                return AsT(el);
            }

            RecalcHeights(fixfrom);

            TTreeItem* ub = FindFirstUnbalEl(fixfrom);

            while (ub) {
                lheight = ub->Left_ ? ub->Left_->Height_ : 0;
                rheight = ub->Right_ ? ub->Right_->Height_ : 0;

                if (rheight > lheight) {
                    ub = ub->Right_;
                    lheight = ub->Left_ ? ub->Left_->Height_ : 0;
                    rheight = ub->Right_ ? ub->Right_->Height_ : 0;

                    if (rheight > lheight) {
                        ub = ub->Right_;
                    } else if (rheight < lheight) {
                        ub = ub->Left_;
                    } else {
                        ub = ub->Right_;
                    }
                } else {
                    ub = ub->Left_;
                    lheight = ub->Left_ ? ub->Left_->Height_ : 0;
                    rheight = ub->Right_ ? ub->Right_->Height_ : 0;
                    if (rheight > lheight) {
                        ub = ub->Right_;
                    } else if (rheight < lheight) {
                        ub = ub->Left_;
                    } else {
                        ub = ub->Left_;
                    }
                }

                fixfrom = Rebalance(ub);
                ub = FindFirstUnbalEl(fixfrom);
            }

            return AsT(el);
        }

        inline iterator First() throw () {
            return ConstructFirst(this);
        }

        inline iterator Last() throw () {
            return ConstructLast(this);
        }

        inline iterator Begin() throw () {
            return First();
        }

        inline iterator End() throw () {
            return iterator(0, this);
        }

        inline bool Empty() const throw () {
            return const_cast<TAvlTree*>(this)->Begin() == const_cast<TAvlTree*>(this)->End();
        }

        template <class Functor>
        inline void ForEach(Functor& f) {
            iterator it = Begin();

            while (!it.IsEnd()) {
                iterator next = it; ++next;
                f(*it);
                it = next;
            }
        }

    private:
        inline TTreeItem* Rebalance(TTreeItem* n) throw () {
            long lheight, rheight;

            TTreeItem* a;
            TTreeItem* b;
            TTreeItem* c;
            TTreeItem* t1;
            TTreeItem* t2;
            TTreeItem* t3;
            TTreeItem* t4;

            TTreeItem* p = n->Parent_;
            TTreeItem* gp = p->Parent_;
            TTreeItem* ggp = gp->Parent_;

            if (gp->Right_ == p) {
                if (p->Right_ == n) {
                    a = gp;
                    b = p;
                    c = n;
                    t1 = gp->Left_;
                    t2 = p->Left_;
                    t3 = n->Left_;
                    t4 = n->Right_;
                } else {
                    a = gp;
                    b = n;
                    c = p;
                    t1 = gp->Left_;
                    t2 = n->Left_;
                    t3 = n->Right_;
                    t4 = p->Right_;
                }
            } else {
                if (p->Right_ == n) {
                    a = p;
                    b = n;
                    c = gp;
                    t1 = p->Left_;
                    t2 = n->Left_;
                    t3 = n->Right_;
                    t4 = gp->Right_;
                } else {
                    a = n;
                    b = p;
                    c = gp;
                    t1 = n->Left_;
                    t2 = n->Right_;
                    t3 = p->Right_;
                    t4 = gp->Right_;
                }
            }

            if (ggp == 0) {
                Root_ = b;
            } else if (ggp->Left_ == gp) {
                ggp->Left_ = b;
            } else {
                ggp->Right_ = b;
            }

            b->Parent_ = ggp;
            b->Left_ = a;
            a->Parent_ = b;
            b->Right_ = c;
            c->Parent_ = b;
            a->Left_ = t1;

            if (t1 != 0) {
                t1->Parent_ = a;
            }

            a->Right_ = t2;

            if (t2 != 0) {
                t2->Parent_ = a;
            }

            c->Left_ = t3;

            if (t3 != 0) {
                t3->Parent_ = c;
            }

            c->Right_ = t4;

            if (t4 != 0) {
                t4->Parent_ = c;
            }

            lheight = a->Left_ ? a->Left_->Height_ : 0;
            rheight = a->Right_ ? a->Right_->Height_ : 0;
            a->Height_ = (lheight > rheight ? lheight : rheight) + 1;

            lheight = c->Left_ ? c->Left_->Height_ : 0;
            rheight = c->Right_ ? c->Right_->Height_ : 0;
            c->Height_ = (lheight > rheight ? lheight : rheight) + 1;

            lheight = a->Height_;
            rheight = c->Height_;
            b->Height_ = (lheight > rheight ? lheight : rheight) + 1;

            RecalcHeights(ggp);

            return ggp;
        }

        inline void RecalcHeights(TTreeItem* el) throw () {
            long lheight, rheight, new_height;

            while (el) {
                lheight = el->Left_ ? el->Left_->Height_ : 0;
                rheight = el->Right_ ? el->Right_->Height_ : 0;

                new_height = (lheight > rheight ? lheight : rheight) + 1;

                if (new_height == el->Height_) {
                    return;
                } else {
                    el->Height_ = new_height;
                }

                el = el->Parent_;
            }
        }

        inline TTreeItem* FindFirstUnbalGP(TTreeItem* el) throw () {
            long lheight, rheight, balanceProp;
            TTreeItem* gp;

            if (el == 0 || el->Parent_ == 0 || el->Parent_->Parent_ == 0) {
                return 0;
            }

            gp = el->Parent_->Parent_;

            while (gp != 0) {
                lheight = gp->Left_ ? gp->Left_->Height_ : 0;
                rheight = gp->Right_ ? gp->Right_->Height_ : 0;
                balanceProp = lheight - rheight;

                if (balanceProp < -1 || balanceProp > 1) {
                    return el;
                }

                el = el->Parent_;
                gp = gp->Parent_;
            }

            return 0;
        }

        inline TTreeItem* FindFirstUnbalEl(TTreeItem* el) throw () {
            if (el == 0) {
                return 0;
            }

            while (el) {
                const long lheight = el->Left_ ? el->Left_->Height_ : 0;
                const long rheight = el->Right_ ? el->Right_->Height_ : 0;
                const long balanceProp = lheight - rheight;

                if (balanceProp < -1 || balanceProp > 1) {
                    return el;
                }

                el = el->Parent_;
            }

            return 0;
        }

        inline void ReplaceEl(TTreeItem* el, TTreeItem* replacement) throw () {
            TTreeItem* parent = el->Parent_;
            TTreeItem* left = el->Left_;
            TTreeItem* right = el->Right_;

            replacement->Left_ = left;

            if (left) {
                left->Parent_ = replacement;
            }

            replacement->Right_ = right;

            if (right) {
                right->Parent_ = replacement;
            }

            replacement->Parent_ = parent;

            if (parent) {
                if (parent->Left_ == el) {
                    parent->Left_ = replacement;
                } else {
                    parent->Right_ = replacement;
                }
            } else {
                Root_ = replacement;
            }

            replacement->Height_ = el->Height_;
        }

        inline void RemoveEl(TTreeItem* el, TTreeItem* filler) throw () {
            TTreeItem* parent = el->Parent_;

            if (parent) {
                if (parent->Left_ == el) {
                    parent->Left_ = filler;
                } else {
                    parent->Right_ = filler;
                }
            } else {
                Root_ = filler;
            }

            if (filler) {
                filler->Parent_ = parent;
            }

            return;
        }

        inline void AttachRebal(TTreeItem* el, TTreeItem* parentEl, TTreeItem* lastLess) {
            el->Parent_ = parentEl;
            el->Left_ = 0;
            el->Right_ = 0;
            el->Height_ = 1;

            if (parentEl != 0) {
                if (lastLess == parentEl) {
                    parentEl->Left_ = el;
                } else {
                    parentEl->Right_ = el;
                }

                if (Head_->Left_ == el) {
                    Head_ = el;
                }

                if (Tail_->Right_ == el) {
                    Tail_ = el;
                }
            } else {
                Root_ = el;
                Head_ = Tail_ = el;
            }

            RecalcHeights(parentEl);

            TTreeItem* ub = FindFirstUnbalGP(el);

            if (ub != 0) {
                Rebalance(ub);
            }
        }

    private:
        TTreeItem* Root_;
        TTreeItem* Head_;
        TTreeItem* Tail_;
};

template <class T, class C>
struct TAvlTreeItem: public TNonCopyable {
    public:
        typedef TAvlTree<T, C> TTree;
        friend class TAvlTree<T, C>;
        friend class TAvlTree<T, C>::TIterator;

        inline TAvlTreeItem() throw ()
            : Left_(0)
            , Right_(0)
            , Parent_(0)
            , Height_(0)
            , Tree_(0)
        {
        }

        inline ~TAvlTreeItem() throw () {
            Unlink();
        }

        inline void Unlink() throw () {
            if (Tree_) {
                Tree_->EraseImpl(this);
            }
        }

    private:
        TAvlTreeItem* Left_;
        TAvlTreeItem* Right_;
        TAvlTreeItem* Parent_;
        long Height_;
        TTree* Tree_;
};
