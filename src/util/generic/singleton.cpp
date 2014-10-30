#include "singleton.h"

#include <cstring>

void NPrivate::FillWithTrash(void* ptr, size_t len) {
#if defined(NDEBUG)
    UNUSED(ptr);
    UNUSED(len);
#else
    memset(ptr, 0xBA, len);
#endif
}
