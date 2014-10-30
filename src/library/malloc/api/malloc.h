#pragma once

namespace NMalloc {
    struct TMallocInfo {
        TMallocInfo();

        const char* Name;
    };

    // this function should be implemented by malloc implementations
    TMallocInfo MallocInfo();
}
