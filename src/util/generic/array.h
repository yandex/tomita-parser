#pragma once

#include "algorithm.h"
#include "typetraits.h"
#include "yexception.h"

#include <util/system/defaults.h>

#include <stlport/new>

#define _STLP_USE_NO_IOSTREAMS
#include <stlport/iterator>
#undef _STLP_USE_NO_IOSTREAMS
#include <stlport/cstddef>

namespace NTr1 {
    // Adopted from std::tr1::array
    template <typename Tp, size_t Nm>
    struct array {
        typedef Tp                                       value_type;
        typedef value_type&                              reference;
        typedef const value_type&                        const_reference;
        typedef value_type*                              iterator;
        typedef const value_type*                        const_iterator;
        typedef size_t                                   size_type;
        typedef ptrdiff_t                                difference_type;
        typedef NStl::reverse_iterator<iterator>         reverse_iterator;
        typedef NStl::reverse_iterator<const_iterator>   const_reverse_iterator;

        enum {
            ArraySize = Nm
        };

        // Support for zero-sized arrays mandatory.
        // P is public, because aggregate type cannot have public data members
        value_type P[Nm ? Nm : 1];

        // No explicit construct/copy/destroy for aggregate type.

        void assign(const value_type& u) {
            NStl::fill_n(begin(), size(), u);
        }

        void swap(array& other) {
            NStl::swap_ranges(begin(), end(), other.begin());
        }

        // Iterators.
        iterator begin() {
            return iterator(&P[0]);
        }

        const_iterator begin() const {
            return const_iterator(&P[0]);
        }

        iterator end() {
            return iterator(&P[Nm]);
        }

        const_iterator end() const {
            return const_iterator(&P[Nm]);
        }

        reverse_iterator rbegin() {
            return reverse_iterator(end());
        }

        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }

        reverse_iterator rend() {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        // Capacity.
        size_type size() const {
            return Nm;
        }

        size_type max_size() const {
            return Nm;
        }

        bool empty() const {
            return size() == 0;
        }

        // Element access.
        reference operator[](size_type n) {
            YASSERT(n < size());
            return P[n];
        }

        const_reference operator[](size_type n) const {
            YASSERT(n < size());
            return P[n];
        }

        reference at(size_type n) {
            BoundCheck<Nm>(n);
            return P[n];
        }

        const_reference at(size_type n) const {
            BoundCheck<Nm>(n);
            return P[n];
        }

        reference front() {
            return *begin();
        }

        const_reference front() const {
            return *begin();
        }

        reference back() {
            return Nm ? *(end() - 1) : *end();
        }

        const_reference back() const {
            return Nm ? *(end() - 1) : *end();
        }

        Tp* data() {
            return &P[0];
        }

        const Tp* data() const {
            return &P[0];
        }

        size_type operator+ () const throw () {
            return size();
        }

        Tp* operator~ () throw () {
            return data();
        }

        const Tp* operator~ () const throw () {
            return data();
        }

        int ysize() const throw () {
            return (int)size();
        }
    private:
        template <size_t Mm>
        typename TEnableIf<Mm, void>::TResult
        BoundCheck(size_type n) const {
            if (EXPECT_FALSE(n >= Mm))
                ThrowRangeError("array");
        }

        template <size_t Mm>
        typename TEnableIf<!Mm, void>::TResult
        BoundCheck(size_type) const {
            ThrowRangeError("array");
        }
    };

    // array comparisons.
    template <typename Tp, size_t Nm>
    inline bool operator==(const array<Tp, Nm>& one, const array<Tp, Nm>& two)  {
        return NStl::equal(one.begin(), one.end(), two.begin());
    }

    template <typename Tp, size_t Nm>
    inline bool operator!=(const array<Tp, Nm>& one, const array<Tp, Nm>& two) {
        return !(one == two);
    }

    template <typename Tp, size_t Nm>
    inline bool operator<(const array<Tp, Nm>& a, const array<Tp, Nm>& b) {
        return NStl::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template <typename Tp, size_t Nm>
    inline bool operator>(const array<Tp, Nm>& one, const array<Tp, Nm>& two) {
        return two < one;
    }

    template <typename Tp, size_t Nm>
    inline bool operator<=(const array<Tp, Nm>& one, const array<Tp, Nm>& two) {
        return !(one > two);
    }

    template <typename Tp, size_t Nm>
    inline bool operator>=(const array<Tp, Nm>& one, const array<Tp, Nm>& two) {
        return !(one < two);
    }
}

template <typename T, size_t N>
class yarray: public NTr1::array<T, N> {
public:
    typedef NTr1::array<T, N> TBase;
    typedef typename TBase::size_type size_type;

    size_type operator+ () const throw () {
        return this->size();
    }

    T* operator~ () throw () {
        return this->data();
    }

    const T* operator~ () const throw () {
        return this->data();
    }

    int ysize() const throw () {
        return (int)TBase::size();
    }
};
