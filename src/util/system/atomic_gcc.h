#pragma once

typedef volatile intptr_t TAtomic;

static inline intptr_t AtomicIncrement(TAtomic& p) {
    return __sync_add_and_fetch(&p, 1);
}

static inline intptr_t AtomicDecrement(TAtomic& p) {
    return __sync_sub_and_fetch(&p, 1);
}

static inline intptr_t AtomicAdd(TAtomic& p, intptr_t v) {
    return __sync_add_and_fetch(&p, v);
}

static inline intptr_t AtomicSwap(TAtomic& p, intptr_t v) {
    return __sync_lock_test_and_set(&p, v);
}

static inline bool AtomicCas(TAtomic* a, intptr_t exchange, intptr_t compare) {
    return __sync_bool_compare_and_swap(a, compare, exchange);
}

static inline intptr_t AtomicOr(TAtomic& a, intptr_t b) {
    return __sync_or_and_fetch(&a, b);
}

static inline intptr_t AtomicAnd(TAtomic& a, intptr_t b) {
    return __sync_and_and_fetch(&a, b);
}

static inline void AtomicBarrier() {
    __sync_synchronize();
}
