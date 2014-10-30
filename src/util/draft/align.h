#pragma once

/*
 * ideas from
 *   http://www.boost.org/doc/libs/1_53_0/boost/type_traits/alignment_of.hpp
 *   http://stackoverflow.com/questions/364483/determining-the-alignment-of-c-c-structures-in-relation-to-its-members
 *   Chromium'a aligned_memory.h: http://stackoverflow.com/questions/4583125/char-array-as-storage-for-placement-new?rq=1
 */

#include <util/system/defaults.h>


#ifndef MIN
#   define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#if !defined(__GNUC__) && !defined(__clang__)

template <typename T>
struct TAlignmentHelper
{
    char C;
    T    T1;

    TAlignmentHelper();
};

#endif

template<class T>
struct TAlignOf
{
    const static size_t Value =
#if defined(__GNUC__) || defined(__clang__)
            __alignof__(T)
#elif defined(_MSC_VER)
            MIN(sizeof(TAlignmentHelper<T>) - sizeof(T), __alignof(T))
#else
            MIN(sizeof(TAlignmentHelper<T>) - sizeof(T), sizeof(T))
#endif
            ;
};



#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ >= 4)) || (defined(__GNUC__) && (__GNUC__ > 4))

template <size_t Size, size_t Align>
struct TAlignedMemory;

#if defined(__GNUC__)
    #define DECLARE_ALIGNED(alignment) __attribute__((aligned(alignment)))
#else // _MSC_VER
    #define DECLARE_ALIGNED(alignment)   __declspec(align(alignment))
#endif


#define DECLARE_ONE_ALIGNED_MEMORY(alignment) \
template <size_t Size> \
struct DECLARE_ALIGNED(alignment) TAlignedMemory<Size, alignment> { \
    alias_hack char Mem[Size]; \
};

DECLARE_ONE_ALIGNED_MEMORY(1)
DECLARE_ONE_ALIGNED_MEMORY(2)
DECLARE_ONE_ALIGNED_MEMORY(4)
DECLARE_ONE_ALIGNED_MEMORY(8)
DECLARE_ONE_ALIGNED_MEMORY(16)
DECLARE_ONE_ALIGNED_MEMORY(32)
DECLARE_ONE_ALIGNED_MEMORY(64)
DECLARE_ONE_ALIGNED_MEMORY(128)
DECLARE_ONE_ALIGNED_MEMORY(256)
DECLARE_ONE_ALIGNED_MEMORY(512)
DECLARE_ONE_ALIGNED_MEMORY(1024)
DECLARE_ONE_ALIGNED_MEMORY(2048)
DECLARE_ONE_ALIGNED_MEMORY(4096)

#else

template <size_t Size, size_t Align>
struct TAlignedMemory
{
    alias_hack char Mem[Size];
} __attribute__((aligned(Align), __may_alias__));

#endif

template <class T>
struct TAlignedMemoryFor : public TAlignedMemory<sizeof(T), TAlignOf<T>::Value>
{
    typedef TAlignedMemory<sizeof(T), TAlignOf<T>::Value> TBase;

    T* Get()
    {
        return (T*)TBase::Mem;
    }

    const T* Get() const
    {
        return (const T*)TBase::Mem;
    }
};


