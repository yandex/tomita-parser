#include "alloc.h"

#include <util/generic/singleton.h>

IAllocator* TDefaultAllocator::Instance() throw () {
    return SingletonWithPriority<TDefaultAllocator, 0>();
}
