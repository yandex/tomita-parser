#pragma once

#include "defaults.h"

#include <util/generic/typetraits.h>
#include <util/generic/static_assert.h>

#if defined(__GNUC__) && (__GNUC__ > 3) && !defined(_i386_)
    #include "atomic_gcc.h"
#elif defined(__GNUC__) && (defined(_i386_) || defined(_x86_64_))
    #include "atomic_x86.h"
#elif defined(_MSC_VER)
    #include "atomic_win.h"
#else
    #error todo
#endif

#if defined(__GNUC__)
    #define ATOMIC_COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")
#endif

#if !defined(ATOMIC_COMPILER_BARRIER)
    #define ATOMIC_COMPILER_BARRIER()
#endif

typedef TTypeTraits<TAtomic>::TNonVolatile TAtomicBase;

STATIC_ASSERT(sizeof(TAtomic) == sizeof(void*));

static inline TAtomicBase AtomicSub(TAtomic& a, TAtomicBase v) {
    return AtomicAdd(a, -v);
}

#if defined(_x86_64_)
    #if defined(__GNUC__)
        static inline TAtomicBase AtomicGet(const TAtomic& a) {
            register TAtomicBase tmp;

            __asm__ __volatile__ (
                  "movq %1, %0"
                : "=r" (tmp)
                : "m" (a)
                :
            );

            return tmp;
        }

        static inline void AtomicSet(TAtomic& a, TAtomicBase v) {
            __asm__ __volatile__ (
                  "movq %1, %0"
                : "=m" (a)
                : "r" (v)
                :
            );
        }
    #elif defined(_win_)
        //TODO
        #define USE_GENERIC_SETGET
    #else
        #error unsupported platform
    #endif

#elif defined(_arm64_)
    static inline TAtomicBase AtomicGet(const TAtomic& a) {
        register TAtomicBase tmp;

        __asm__ __volatile__ (
              "ldar %x[value], %[ptr]  \n\t"
              : [value]"=r" (tmp)
              : [ptr]"Q" (a)
              : "memory"
              );

        return tmp;
    }

    static inline void AtomicSet(TAtomic& a, TAtomicBase v) {
        __asm__ __volatile__ (
              "stlr %x[value], %[ptr]  \n\t"
              : [ptr]"=Q" (a)
              : [value]"r" (v)
              : "memory"
        );
    }

#elif defined(_i386_) || defined(_arm32_)
    #define USE_GENERIC_SETGET
#else
    #error unsupported platform
#endif

#if defined(USE_GENERIC_SETGET)
static inline TAtomicBase AtomicGet(const TAtomic& a) {
    return a;
}

static inline void AtomicSet(TAtomic& a, TAtomicBase v) {
    a = v;
}
#endif

template <typename T>
inline TAtomic* AsAtomicPtr(T volatile* val) {
    return reinterpret_cast<TAtomic*>(val);
}

template <typename T>
inline const TAtomic* AsAtomicPtr(T const volatile* val) {
    return reinterpret_cast<const TAtomic*>(val);
}

#if !defined(HAVE_CUSTOM_LOCKS)
static inline bool AtomicTryLock(TAtomic* a) {
    return AtomicCas(a, 1, 0);
}

static inline bool AtomicTryAndTryLock(TAtomic* a) {
    return (AtomicGet(*a) == 0) && AtomicTryLock(a);
}

static inline void AtomicUnlock(TAtomic* a) {
    ATOMIC_COMPILER_BARRIER();
    AtomicSet(*a, 0);
}

template <typename T>
inline bool AtomicCas(T* volatile* target, T* exchange, T* compare) {
    return AtomicCas(AsAtomicPtr(target), (TAtomicBase) exchange, (TAtomicBase) compare);
}
#endif

template <typename T>
inline intptr_t AtomicOr(T volatile& a, T b) {
    return AtomicOr(*AsAtomicPtr(&a), (TAtomicBase)b);
}

template <typename T>
inline intptr_t AtomicAnd(T volatile& a, T b) {
    return AtomicAnd(*AsAtomicPtr(&a), (TAtomicBase)b);
}

#include "atomic_ops.h"
