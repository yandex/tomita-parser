#pragma once

#include <stlport/memory>

#include <cstdlib>

extern void ThrowBadAlloc();

inline void* y_allocate(size_t n) {
    void* r = malloc(n);

    if (r == 0) {
        ThrowBadAlloc();
    }

    return r;
}

inline void y_deallocate(void* p) {
    free(p);
}

/**
 * Behavior of realloc from C++99 to C++11 changed (http://www.cplusplus.com/reference/cstdlib/realloc/).
 *
 * Our implementation work as C++99: if new_sz == 0 free will be called on 'p' and nullptr returned.
 */
inline void* y_reallocate(void* p, size_t new_sz) {
    if (!new_sz) {
        if (p) {
            free(p);
        }
        return nullptr;
    }

    void* r = realloc(p, new_sz);

    if (r == 0) {
        ThrowBadAlloc();
    }

    return r;
}

#define __STL_DEFAULT_ALLOCATOR(T) NStl::allocator<T >
#define DEFAULT_ALLOCATOR(T) __STL_DEFAULT_ALLOCATOR(T)
#define REBIND_ALLOCATOR(A, T) typename A::template rebind<T >::other
#define FIX2ARG(x, y) x, y

class IAllocator {
    public:
        struct TBlock {
            void* Data;
            size_t Len;
        };

        virtual ~IAllocator() {
        }

        virtual TBlock Allocate(size_t len) = 0;
        virtual void Release(const TBlock& block) = 0;
};

class TDefaultAllocator: public IAllocator {
    public:
        virtual TBlock Allocate(size_t len) {
            TBlock ret = {y_allocate(len), len};

            return ret;
        }

        virtual void Release(const TBlock& block) {
            y_deallocate(block.Data);
        }

        static IAllocator* Instance() throw ();
};
