
#include "pool.h"

#include <util/generic/singleton.h>

TMemoryPool::IGrowPolicy* TMemoryPool::TLinearGrow::Instance() throw () {
    return SingletonWithPriority<TLinearGrow, 0>();
}

TMemoryPool::IGrowPolicy* TMemoryPool::TExpGrow::Instance() throw () {
    return SingletonWithPriority<TExpGrow, 0>();
}
