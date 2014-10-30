#pragma once

#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/memory/alloc.h>
#include <util/generic/typetraits.h>

#include <new>

struct autoarray_getindex {
    autoarray_getindex() {}
};

struct aarr_b0 {
    aarr_b0() {}
};

struct aarr_nofill {
    aarr_nofill() {}
};

template<typename T>
struct ynd_type_traits {
    enum {
        empty_destructor = TTypeTraits<T>::IsPod,
    };
};

template<class T>
class autoarray
{
protected:
    T *arr;
    size_t _size;
private:
    void AllocBuf(size_t siz) {
        arr = 0;
        _size = 0;
        if (siz) {
            arr = (T*)y_allocate(sizeof(T) * siz);
            _size = siz;
        }
    }
public:
    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    autoarray()
        : arr(0)
        , _size(0)
    {
    }
    autoarray(size_t siz)
    {
        AllocBuf(siz);
        T *curr = arr;
        try {
            for (T *end = arr + _size; curr != end; ++curr)
                new(curr) T();
        } catch(...) {
            for (--curr; curr >= arr; --curr)
                curr->~T();
            y_deallocate(arr);
            throw;
        }
    }
    template <class A>
    explicit
    autoarray(size_t siz, A &fill)
    {
        AllocBuf(siz);
        T *curr = arr;
        try {
            for (T *end = arr + _size; curr != end; ++curr)
                new(curr) T(fill);
        } catch(...) {
            for (--curr; curr >= arr; --curr)
                curr->~T();
            y_deallocate(arr);
            throw;
        }
    }
    explicit
    autoarray(size_t siz, const autoarray_getindex &)
    {
        AllocBuf(siz);
        size_t nCurrent = 0;
        try {
            for (nCurrent = 0; nCurrent < _size; ++nCurrent)
                new(&arr[nCurrent]) T(nCurrent);
        } catch(...) {
            for (size_t n = 0; n < nCurrent; ++n)
                arr[n].~T();
            y_deallocate(arr);
            throw;
        }
    }
    explicit
    autoarray(size_t siz, const aarr_b0 &)
    {
        AllocBuf(siz);
        memset(arr, 0, _size * sizeof(T));
    }
    explicit
    autoarray(size_t siz, const aarr_nofill &)
    {
        AllocBuf(siz);
    }
    template <class A>
    explicit
    autoarray(const A* fill, size_t siz)
    {
        AllocBuf(siz);
        size_t nCurrent = 0;
        try {
            for (nCurrent = 0; nCurrent < _size; ++nCurrent)
                new(&arr[nCurrent]) T(fill[nCurrent]);
        } catch(...) {
            for (size_t n = 0; n < nCurrent; ++n)
                arr[n].~T();
            y_deallocate(arr);
            throw;
        }
    }
    template <class A, class B>
    explicit
    autoarray(const A* fill, const B* cfill, size_t siz)
    {
        AllocBuf(siz);
        size_t nCurrent = 0;
        try {
            for (nCurrent = 0; nCurrent < _size; ++nCurrent)
                new(&arr[nCurrent]) T(fill[nCurrent], cfill);
        } catch(...) {
            for (size_t n = 0; n < nCurrent; ++n)
                arr[n].~T();
            y_deallocate(arr);
            throw;
        }
    }
    template <class A>
    explicit
    autoarray(const A* fill, size_t initsiz, size_t fullsiz)
    {
        AllocBuf(fullsiz);
        size_t nCurrent = 0;
        try {
            for (nCurrent = 0; nCurrent < ((initsiz < _size) ? initsiz : _size); ++nCurrent)
                new(&arr[nCurrent]) T(fill[nCurrent]);
            for (; nCurrent < _size; ++nCurrent)
                new(&arr[nCurrent]) T();
        } catch(...) {
            for (size_t n = 0; n < nCurrent; ++n)
                arr[n].~T();
            y_deallocate(arr);
            throw;
        }
    }
    template <class A>
    explicit
    autoarray(const A* fill, size_t initsiz, size_t fullsiz, const T& dummy)
    {
        AllocBuf(fullsiz);
        size_t nCurrent = 0;
        try {
            for (nCurrent = 0; nCurrent < ((initsiz < _size) ? initsiz : _size); ++nCurrent)
                new(&arr[nCurrent]) T(fill[nCurrent]);
            for (; nCurrent < _size; ++nCurrent)
                new(&arr[nCurrent]) T(dummy);
        } catch(...) {
            for (size_t n = 0; n < nCurrent; ++n)
                arr[n].~T();
            y_deallocate(arr);
            throw;
        }
    }

    template <class... R>
    explicit
    autoarray(size_t siz, const R&... fill)
    {
        AllocBuf(siz);
        T *curr = arr;
        try {
            for (T *end = arr + _size; curr != end; ++curr)
                new(curr) T(fill...);
        } catch(...) {
            for (--curr; curr >= arr; --curr)
                curr->~T();
            y_deallocate(arr);
            throw;
        }
    }
    ~autoarray()
    {
        if (_size) {
            if (!ynd_type_traits<T>::empty_destructor)
                for (T *curr = arr, *end = arr + _size; curr != end; ++curr)
                    curr->~T();
            y_deallocate(arr);
        }
    }
    T &operator[](size_t pos)
    {
        YASSERT(pos < _size);
        return arr[pos];
    }
    const T &operator[](size_t pos) const
    {
        YASSERT(pos < _size);
        return arr[pos];
    }
    size_t size() const {
        return _size;
    }
    void swap(autoarray &with) {
        T *tmp_arr = arr;
        size_t tmp_size = _size;
        arr = with.arr;
        _size = with._size;
        with.arr = tmp_arr;
        with._size = tmp_size;
    }
    void resize(size_t siz) {
        autoarray<T> tmp(arr, _size, siz);
        swap(tmp);
    }
    void resize(size_t siz, const T& dummy) {
        autoarray<T> tmp(arr, _size, siz, dummy);
        swap(tmp);
    }
    T* rawpointer() { return arr; }
    const T* operator~() const { return arr; }
    T* begin() { return arr; }
    T* end() { return arr + _size; }
    T &back() { YASSERT(_size); return arr[_size - 1]; }
    bool empty() const { return !_size; }
    bool operator !() const { return !_size; }
    size_t operator +() const { return _size; }
    const T* begin() const { return arr; }
    const T* end() const { return arr + _size; }
    const T &back() const { YASSERT(_size); return arr[_size - 1]; }
    //operator T*() { return arr; }
    DECLARE_NOCOPY(autoarray);
};

template<class T>
inline bool operator==(const autoarray<T> &a, const autoarray<T> &b)
{
    size_t count = a.size();
    if (count != b.size())
        return false;
    for (size_t i = 0; i < count; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

template<class T>
class automatrix : private autoarray<T>
{
  private:
    const size_t _N;
  public:
    explicit
    automatrix(size_t M, size_t N)
        : autoarray<T>(M*N), _N(N)
    {
    }
    explicit
    automatrix(size_t M)
        : autoarray<T>(M*M), _N(M)
    {
    }
    template <class A>
    explicit
    automatrix(size_t M, size_t N, const A &fill)
        : autoarray<T>(M*N, fill), _N(N)
    {
    }
    template <class A>
    explicit
    automatrix(size_t M, const A &fill)
        : autoarray<T>(M*M, fill), _N(M)
    {
    }
    const T& GetAt(size_t i, size_t j) const {
        return autoarray<T>::operator[](i*_N+j);
    }
    T& GetAtForWrite(size_t i, size_t j) {
        return autoarray<T>::operator[](i*_N+j);
    }
    template<class B>
    void PutAt(size_t i, size_t j, const B& el) {
        autoarray<T>::operator[](i*_N+j) = el;
    }
    DECLARE_NOCOPY(automatrix);
};
