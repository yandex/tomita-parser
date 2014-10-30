#pragma once

#include "utility.h"
#include "mapfindptr.h"
#include "pair.h"

#include <stlport/numeric>
#include <stlport/algorithm>
#include <stlport/iterator>

template <class T>
static inline void QuickSort(T f, T l) {
    NStl::sort(f, l);
}

template <class T, class C>
static inline void QuickSort(T f, T l, C c) {
    NStl::sort(f, l, c);
}

template <class T>
static inline void Sort(T f, T l) {
    NStl::sort(f, l);
}

template <class T, class C>
static inline void Sort(T f, T l, C c) {
    NStl::sort(f, l, c);
}

template <class T>
static inline void MergeSort(T f, T l) {
    NStl::stable_sort(f, l);
}

template <class T, class C>
static inline void MergeSort(T f, T l, C c) {
    NStl::stable_sort(f, l, c);
}

template <class T>
static inline void StableSort(T f, T l) {
    NStl::stable_sort(f, l);
}

template <class T, class C>
static inline void StableSort(T f, T l, C c) {
    NStl::stable_sort(f, l, c);
}

template <class T>
static inline void PartialSort(T f, T m, T l) {
    NStl::partial_sort(f, m, l);
}

template <class T, class C>
static inline void PartialSort(T f,T m, T l, C c) {
    NStl::partial_sort(f, m, l, c);
}

template <class T, class R>
static inline R PartialSortCopy(T f, T l, R of, R ol) {
    return NStl::partial_sort_copy(f, l, of, ol);
}

template <class T, class R, class C>
static inline R PartialSortCopy(T f, T l, R of, R ol, C c) {
    return NStl::partial_sort_copy(f, l, of, ol, c);
}

template <class I, class T>
static inline I Find(I f, I l, const T& v) {
    return NStl::find(f, l, v);
}

template <class I, class P>
static inline I FindIf(I f, I l, P p) {
    return NStl::find_if(f, l, p);
}

template<class I, class P>
static inline bool AllOf(I f, I l, P pred) {
    while (f != l) {
        if (!pred(*f))  {
            return false;
        }
        ++f;
    }
    return true;
}

template<class C, class P>
static inline bool AllOf(const C& c, P pred) {
    return AllOf(c.begin(), c.end(), pred);
}

template<class I, class P>
static inline bool AnyOf(I f, I l, P pred) {
    while (f != l) {
        if (pred(*f))  {
            return true;
        }
        ++f;
    }
    return false;
}

template<class C, class P>
static inline bool AnyOf(const C& c, P pred) {
    return AnyOf(c.begin(), c.end(), pred);
}

template<class I, class T>
inline bool IsIn(I f, I l, const T& v) {
    return Find(f, l, v) != l;
}

template<class C, class T>
inline bool IsIn(const C& c, const T& e) {
    return NStl::find(c.begin(), c.end(), e) != c.end();
}

// FindIfPtr - return NULL if not found. Works for arrays, containers, iterators
template <class I, class P>
static inline auto FindIfPtr(I f, I l, P pred) -> decltype(&*f) {
    I found = FindIf(f, l, pred);
    return (found != l) ? &*found : NULL;
}

template <class C, class P>
static inline auto FindIfPtr(C&& c, P pred) -> decltype(&*c.begin()) {
    return FindIfPtr(c.begin(), c.end(), pred);
}

template <size_t N, class T, class P>
static inline T* FindIfPtr(T (&c)[N], P pred) {
    return FindIfPtr(c, c + N, pred);
}

//EqualToOneOf(x, "apple", "orange") means (x == "apple" || x == "orange")
template <typename T>
inline bool EqualToOneOf(const T&) {
    return false;
}

template <typename T, typename U, typename... Other>
inline bool EqualToOneOf(const T& x, const U& y, const Other&... other) {
    return x == y || EqualToOneOf(x, other...);
}

template <class I>
static inline void PushHeap(I f, I l) {
    NStl::push_heap(f, l);
}

template <class I, class C>
static inline void PushHeap(I f, I l, C c) {
    NStl::push_heap(f, l, c);
}

template <class I>
static inline void PopHeap(I f, I l) {
    NStl::pop_heap(f, l);
}

template <class I, class C>
static inline void PopHeap(I f, I l, C c) {
    NStl::pop_heap(f, l, c);
}

template <class I>
static inline void MakeHeap(I f, I l) {
    NStl::make_heap(f, l);
}

template <class I, class C>
static inline void MakeHeap(I f, I l, C c) {
    NStl::make_heap(f, l, c);
}

template <class I>
static inline void SortHeap(I f, I l) {
    NStl::sort_heap(f, l);
}

template <class I, class C>
static inline void SortHeap(I f, I l, C c) {
    NStl::sort_heap(f, l, c);
}

template <class I, class T>
static inline I LowerBound(I f, I l, const T& v) {
    return NStl::lower_bound(f, l, v);
}

template <class I, class T, class C>
static inline I LowerBound(I f, I l, const T& v, C c) {
    return NStl::lower_bound(f, l, v, c);
}

template <class I, class T>
static inline I UpperBound(I f, I l, const T& v) {
    return NStl::upper_bound(f, l, v);
}

template <class I, class T, class C>
static inline I UpperBound(I f, I l, const T& v, C c) {
    return NStl::upper_bound(f, l, v, c);
}

template <class T>
static inline T Unique(T f, T l) {
    return NStl::unique(f, l);
}

template <class T, class P>
static inline T Unique(T f, T l, P p) {
    return NStl::unique(f, l, p);
}

template <class T1, class T2>
static inline bool Equal(T1 f1, T1 l1, T2 f2) {
    return NStl::equal(f1, l1, f2);
}

template <class T1, class T2, class P>
static inline bool Equal(T1 f1, T1 l1, T2 f2, P p) {
    return NStl::equal(f1, l1, f2, p);
}

template <class TI, class TO>
static inline TO Copy(TI f, TI l, TO t) {
    return NStl::copy(f, l, t);
}

template <class TI, class TO>
static inline TO UniqueCopy(TI f, TI l, TO t) {
    return NStl::unique_copy(f, l, t);
}

template <class TI, class TO, class TP>
static inline TO UniqueCopy(TI f, TI l, TO t, TP p) {
    return NStl::unique_copy(f, l, t, p);
}

template <class TI, class TP>
static inline TI RemoveIf(TI f, TI l, TP p) {
    return NStl::remove_if(f, l, p);
}

template <class TI, class TO, class TP>
static inline TO RemoveCopyIf(TI f, TI l, TO t, TP p) {
    return NStl::remove_copy_if(f, l, t, p);
}

template <class TI, class TO>
static inline TO ReverseCopy(TI f, TI l, TO t) {
    return NStl::reverse_copy(f, l, t);
}

template <class TI1, class TI2, class TO>
static inline TO SetUnion(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p) {
    return NStl::set_union(f1, l1, f2, l2, p);
}

template <class TI1, class TI2, class TO, class TC>
static inline TO SetUnion(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p, TC c) {
    return NStl::set_union(f1, l1, f2, l2, p, c);
}

template <class TI1, class TI2, class TO>
static inline TO SetDifference(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p) {
    return NStl::set_difference(f1, l1, f2, l2, p);
}

template <class TI1, class TI2, class TO, class TC>
static inline TO SetDifference(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p, TC c) {
    return NStl::set_difference(f1, l1, f2, l2, p, c);
}

template <class TI1, class TI2, class TO>
static inline TO SetSymmetricDifference(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p) {
    return NStl::set_symmetric_difference(f1, l1, f2, l2, p);
}

template <class TI1, class TI2, class TO, class TC>
static inline TO SetSymmetricDifference(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p, TC c) {
    return NStl::set_symmetric_difference(f1, l1, f2, l2, p, c);
}

template <class TI1, class TI2, class TO>
static inline TO SetIntersection(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p) {
    return NStl::set_intersection(f1, l1, f2, l2, p);
}

template <class TI1, class TI2, class TO, class TC>
static inline TO SetIntersection(TI1 f1, TI1 l1, TI2 f2, TI2 l2, TO p, TC c) {
    return NStl::set_intersection(f1, l1, f2, l2, p, c);
}

template <class I, class T>
static inline void Fill(I f, I l, const T& v) {
    NStl::fill(f, l, v);
}

template<class T>
void Reverse(T f, T l) {
    NStl::reverse(f, l);
}

template<class T>
void Rotate(T f, T m, T l) {
    NStl::rotate(f, m, l);
}

template<typename It, typename Val>
static inline Val Accumulate(It begin, It end, Val val) {
    return NStl::accumulate(begin, end, val);
}

template<typename It, typename Val, typename BinOp>
static inline Val Accumulate(It begin, It end, Val val, BinOp binOp) {
    return NStl::accumulate(begin, end, val, binOp);
}

template <typename TVector>
static inline typename TVector::value_type Accumulate(const TVector& v, typename TVector::value_type val = typename TVector::value_type()) {
    return Accumulate(v.begin(), v.end(), val);
}

template <typename TVector, typename BinOp>
static inline typename TVector::value_type Accumulate(const TVector& v, typename TVector::value_type val, BinOp binOp) {
    return Accumulate(v.begin(), v.end(), val, binOp);
}

template<typename It1, typename It2, typename Val, typename BinOp1, typename BinOp2>
static inline Val InnerProduct(It1 begin1, It1 end1, It2 begin2, Val val, BinOp1 binOp1, BinOp2 binOp2) {
    return NStl::inner_product(begin1, end1, begin2, val, binOp1, binOp2);
}

template <typename TVector>
static inline typename TVector::value_type InnerProduct(const TVector& lhs, const TVector& rhs, typename TVector::value_type val = typename TVector::value_type()) {
    return NStl::inner_product(lhs.begin(), lhs.end(), rhs.begin(), val);
}

template <typename TVector, typename BinOp1, typename BinOp2>
static inline typename TVector::value_type InnerProduct(const TVector& lhs, const TVector& rhs, typename TVector::value_type val, BinOp1 binOp1, BinOp2 binOp2) {
    return NStl::inner_product(lhs.begin(), lhs.end(), rhs.begin(), val, binOp1, binOp2);
}

template<class T>
static inline T MinElement(T begin, T end) {
    return NStl::min_element(begin, end);
}

template<class T, class C>
static inline T MinElement(T begin, T end, C comp) {
    return NStl::min_element(begin, end, comp);
}

template<class T>
static inline T MaxElement(T begin, T end) {
    return NStl::max_element(begin, end);
}

template<class T, class C>
static inline T MaxElement(T begin, T end, C comp) {
    return NStl::max_element(begin, end, comp);
}

template<class TI, class TOp>
void ForEach(TI f, TI l, TOp op) {
    NStl::for_each(f, l, op);
}

template<class T1, class T2, class O>
static inline void Transform(T1 b, T1 e, T2 o, O f) {
    NStl::transform(b, e, o, f);
}

template<class T1, class T2, class T3, class O>
static inline void Transform(T1 b1, T1 e1, T2 b2, T3 o, O f) {
    NStl::transform(b1, e1, b2, o, f);
}

template <class T, class V>
inline typename NStl::iterator_traits<T>::difference_type Count(T first, T last, const V& value) {
    return NStl::count(first, last, value);
}

template<class I1, class I2>
static inline TPair<I1, I2> Mismatch(I1 b1, I1 e1, I2 b2) {
    return NStl::mismatch(b1, e1, b2);
}

template<class I1, class I2, class P>
static inline TPair<I1, I2> Mismatch(I1 b1, I1 e1, I2 b2, P p) {
    return NStl::mismatch(b1, e1, b2, p);
}

// no standard implementation until C++14
template<class I1, class I2>
static inline TPair<I1, I2> Mismatch(I1 b1, I1 e1, I2 b2, I2 e2) {
    while (b1 != e1 && b2 != e2 && *b1 == *b2) {
        ++b1;
        ++b2;
    }
    return MakePair(b1, b2);
}

template<class I1, class I2, class P>
static inline TPair<I1, I2> Mismatch(I1 b1, I1 e1, I2 b2, I2 e2, P p) {
    while (b1 != e1 && b2 != e2 && p(*b1, *b2)) {
        ++b1;
        ++b2;
    }
    return MakePair(b1, b2);
}

template <class It, class Val>
static inline bool BinarySearch(It begin, It end, const Val& val) {
    return NStl::binary_search(begin, end, val);
}

template <class It, class Val, class Comp>
static inline bool BinarySearch(It begin, It end, const Val& val, Comp comp) {
    return NStl::binary_search(begin, end, val, comp);
}
