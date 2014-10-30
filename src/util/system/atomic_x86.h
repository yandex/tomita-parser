#pragma once

typedef volatile intptr_t TAtomic;

static inline intptr_t AtomicAdd(TAtomic& a, intptr_t b) {
    intptr_t tmp = b;

    __asm__ __volatile__(
        "lock\n\t"
        "xadd %0, %1\n\t"
        : "+r" (tmp), "+m" (a)
        :
        : "memory");

    return tmp + b;
}

static inline intptr_t AtomicIncrement(TAtomic& a) {
    return AtomicAdd(a, 1);
}

static inline intptr_t AtomicDecrement(TAtomic& a) {
    return AtomicAdd(a, -1);
}

static inline bool AtomicCas(TAtomic* a, intptr_t exchange, intptr_t compare) {
    char ret;

    __asm__ __volatile__ (
        "lock\n\t"
        "cmpxchg %3,%1\n\t"
        "sete %0\n\t"
        : "=q" (ret), "+m" (*(a)), "+a" (compare)
        : "r" (exchange)
        : "cc", "memory");

    return ret;
}

static inline intptr_t AtomicSwap(TAtomic& a, intptr_t b) {
    register intptr_t rv = b;

    __asm__ __volatile__ (
        "lock\n\t"
        "xchg %0, %1\n\t"
        : "+m" (a) , "+q" (rv)
        :
        : "cc");

    return rv;
}

static inline void AtomicBarrier() {
#if defined(_x86_64_)
    __asm__ __volatile__("mfence" : : : "memory");
#else
    TAtomic x = 0;
    AtomicSwap(x, 0);
#endif
}
