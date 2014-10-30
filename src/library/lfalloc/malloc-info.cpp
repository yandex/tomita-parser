#include <library/malloc/api/malloc.h>


using namespace NMalloc;


TMallocInfo NMalloc::MallocInfo() {
    TMallocInfo r;
    r.Name = "lfalloc";
    return r;
}
