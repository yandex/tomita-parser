#pragma once

#include <util/system/yassert.h>
#include <util/generic/buffer.h>
#include <util/generic/static_assert.h>

/// References a C-styled array
template <typename T>
class TRegion {
public:
    typedef T TValue;
    typedef T* TIterator;
    typedef const T* TConstIterator;

    typedef TIterator iterator;
    typedef TConstIterator const_iterator;

    inline TRegion()
        : DataPtr(0)
        , Len(0)
    {
    }

    inline TRegion(T* data, size_t len)
        : DataPtr(data)
        , Len(len)
    {
    }

    inline TRegion(T* begin, T* end)
        : DataPtr(begin)
        , Len(end - begin)
    {
        YASSERT(end >= begin);
    }

    inline TRegion(T& t)
        : DataPtr(&t)
        , Len(1)
    {
    }

    template <typename T2, size_t N>
    inline TRegion(T2(&array)[N])
        : DataPtr(array)
        , Len(N)
    {
        STATIC_ASSERT(sizeof(T) == sizeof(T2));
    }

    template <class T2>
    inline TRegion(const TRegion<T2>& t)
        : DataPtr(t.Data())
        , Len(t.Size())
    {
        STATIC_ASSERT(sizeof(T) == sizeof(T2));
    }

    inline const T* Data() const throw () {
        return DataPtr;
    }

    inline T* Data() throw () {
        return DataPtr;
    }

    inline const T* data() const throw () {
        return Data();
    }

    inline T* data() throw () {
        return Data();
    }

    inline size_t Size() const throw () {
        return Len;
    }

    inline size_t size() const throw () {
        return Size();
    }

    inline bool Empty() const throw () {
        return !Len;
    }

    inline bool empty() const throw () {
        return Empty();
    }

    inline bool operator<(const TRegion& rhs) const {
        return (DataPtr < rhs.DataPtr) || (DataPtr == rhs.DataPtr && Len < rhs.Len);
    }

    inline bool operator==(const TRegion& rhs) const {
        return (DataPtr == rhs.DataPtr) && (Len == rhs.Len);
    }

    /*
     * some helpers...
     */
    inline T* operator~() throw () {
        return Data();
    }

    inline const T* operator~() const throw () {
        return Data();
    }

    inline size_t operator+ () const throw () {
        return Size();
    }

    /*
     * iterator-like interface
     */
    inline TConstIterator Begin() const throw () {
        return DataPtr;
    }

    inline const_iterator begin() const throw () {
        return Begin();
    }

    inline TIterator Begin() throw () {
        return DataPtr;
    }

    inline iterator begin() throw () {
        return Begin();
    }

    inline TConstIterator End() const throw () {
        return DataPtr + Len;
    }

    inline const_iterator end() const throw () {
        return End();
    }

    inline TIterator End() throw () {
        return DataPtr + Len;
    }

    inline iterator end() throw () {
        return End();
    }

    inline const T& operator[](size_t idx) const throw () {
        YASSERT(idx < Len);
        return Data()[idx];
    }

    inline T& operator[](size_t idx) throw () {
        YASSERT(idx < Len);
        return Data()[idx];
    }

private:
    T* DataPtr;
    size_t Len;
};

typedef TRegion<const char> TDataRegion;
typedef TRegion<char> TMemRegion;
