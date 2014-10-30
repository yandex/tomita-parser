#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// buf_ptr.h -- TBufPtr<T>: like TStringBuf, but for arbitrary types
//

#include <util/generic/utility.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>
#include <stddef.h>

// Class, that points to an array: it stores array start pointer and number of elements.
//
// - Can be used instead of vector<T> or (T* ptr, int size) pair in function arguments.
// - Can also replace common pattern of passing sub-vector via (vector<T>, index = 0, count = -1).
//   TBufPtr validates 'index' and 'count' arguments and sets appropriate pointers if count == -1;
// - Can be automatically constructed from any container with begin() and size() methods -- no
//   caller code modification needed!
//
// EXAMPLE
//     WStroka Join(char separator, const TBufPtr<const WStroka>& words)
//
//     // Return all pairs of adjacent strings: ["p[0] p[1]", ..., "p[N-1] p[N]"]
//     //
//     VectorWStrok Pairs(const VectorWStrok& phrase) {
//         VectorWStrok result;
//         for (size_t i = 0; i + 1 < phrase.size(); ++i)
//             const WStroka pair = Join(" ", TBufPtr<const WStroka>(phrase, i, 1));
//             result.push_back(pair);
//         }
//         return result;
//     }
//
// CONST-CORRECTNESS
//     TBufPtr<T>::operator[] has same const-semantics as operator[] for C++ pointers: pointed-to
//     objects *can* be modified via pointer, that is *const* (pointer 'T* const' is const
//     *itself*, meaning it cannot be re-pointed to another object), So if you want to pass
//     (const T*, int size) analog, use 'const TBufPtr<const T>&'.
//
template <typename T> class TBufPtr {
public:
    inline TBufPtr();
    inline TBufPtr(T* ptr, size_t count);
    inline TBufPtr(const TBufPtr<T>& other);
    inline TBufPtr<T>& operator=(const TBufPtr<T>& other);

    template <typename U> inline TBufPtr(U* _begin, U* _end);

    // Construct TBufPtr from const or non-const vector-like container (NStl::vector,
    // yvector, std::array, even TBufPtr)
    //
    template <typename TContainer>
        inline TBufPtr(TContainer& c, size_t index = 0, size_t count = Max<size_t>());

    template <typename TContainer>
        inline TBufPtr(const TContainer& c, size_t index = 0, size_t count = Max<size_t>());

    inline T& operator[](size_t index) const;
    inline T* begin() const;
    inline T* end() const;
    inline T* data() const;
    inline size_t size() const;
    inline bool empty() const;

    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

private:
    T* Ptr;
    size_t Size;
};

// Functions (a-la std::make_pair) that allow more compact initialization of TBufPtr:
//
//     void Foo(const TBufPtr<const TTypeWithANameWayLongerThanNeeded>& things);
//
//     int main() {
//         TTypeWithANameWayLongerThanNeeded* thingsBegin;
//         TTypeWithANameWayLongerThanNeeded* thingsEnd;
//
//         Foo(BufPtr(thingsBegin, thingsEnd));
//         //
//         // instead of
//         //
//         Foo(TBufPtr<const TTypeWithANameWayLongerThanNeeded>(thingsBegin, thingsEnd))
//     }
//
template <typename T> inline TBufPtr<T> BufPtr(T* ptr, size_t size);

template <typename TContainer> inline TBufPtr<typename TContainer::value_type>
    BufPtr(TContainer& c, size_t index = 0, size_t count = Max<size_t>());

template <typename TContainer> inline TBufPtr<const typename TContainer::value_type>
    BufPtr(const TContainer& c, size_t index = 0, size_t count = Max<size_t>());

template <typename TContainer> inline TBufPtr<const typename TContainer::value_type>
    ConstBufPtr(const TContainer& c, size_t index = 0, size_t count = Max<size_t>());

template <typename T> inline TBufPtr<T> BufPtr(T* begin, T* end);

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline TBufPtr<T>::TBufPtr()
    : Ptr(NULL)
    , Size(0)
{}

template <typename T>
inline TBufPtr<T>::TBufPtr(T* ptr, size_t count)
    : Ptr(ptr)
    , Size(count)
{}

template <typename T>
template <typename TContainer>
inline TBufPtr<T>::TBufPtr(TContainer& c, size_t index, size_t count)
    : Ptr(NULL)
    , Size(0)
{
    assert(index <= c.size());
    index = Min(index, c.size());
    count = Min(count, c.size() - index);

    Ptr = c.begin() + index;
    Size = count;
}

template <typename T>
template <typename TContainer>
inline TBufPtr<T>::TBufPtr(const TContainer& c, size_t index, size_t count)
    : Ptr(NULL)
    , Size(0)
{
    assert(index <= c.size());
    index = Min(index, c.size());
    count = Min(count, c.size() - index);

    Ptr = c.begin() + index;
    Size = count;
}

template <typename T>
template <typename U>
inline TBufPtr<T>::TBufPtr(U* _begin, U* _end)
    : Ptr(_begin)
    , Size(_end - _begin)
{
    assert(_begin <= _end);
}

template <typename T>
inline TBufPtr<T>::TBufPtr(const TBufPtr<T>& other)
    : Ptr(other.Ptr)
    , Size(other.Size)
{}

template <typename T>
TBufPtr<T>& TBufPtr<T>::operator=(const TBufPtr<T>& other) {
    Ptr = other.Ptr;
    Size = other.Size;
    return *this;
}

template <typename T>
inline T* TBufPtr<T>::begin() const {
    return Ptr;
}

template <typename T>
inline T* TBufPtr<T>::end() const {
    return Ptr + Size;
}

template <typename T>
inline T* TBufPtr<T>::data() const {
    return begin();
}

template <typename T>
inline size_t TBufPtr<T>::size() const {
    return Size;
}

template <typename T>
inline bool TBufPtr<T>::empty() const {
    return !Size;
}

template <typename T>
inline T& TBufPtr<T>::operator[](size_t index) const {
    assert(index < Size);
    return *(begin() + index);
}

// Shortcut constructors /////////////////////////////////////////////////////////////////////////

template <typename T> inline TBufPtr<T> BufPtr(T* ptr, size_t size) {
    return TBufPtr<T>(ptr, size);
}

template <typename TContainer> inline TBufPtr<typename TContainer::value_type>
BufPtr(TContainer& c, size_t index, size_t count) {
    return TBufPtr<typename TContainer::value_type>(c, index, count);
}

template <typename TContainer> inline TBufPtr<const typename TContainer::value_type>
BufPtr(const TContainer& c, size_t index, size_t count) {
    return TBufPtr<const typename TContainer::value_type>(c, index, count);
}

template <typename TContainer> inline TBufPtr<const typename TContainer::value_type>
ConstBufPtr(const TContainer& c, size_t index, size_t count) {
    return BufPtr(c, index, count);
}

template <typename T> inline TBufPtr<T> BufPtr(T* begin, T* end) {
    return TBufPtr<T>(begin, end);
}
