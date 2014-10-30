#pragma once

#include <util/stream/output.h>
#include <cstddef>

namespace NCompactTrie {

    class TReverseNodeEnumerator {
    public:
        virtual ~TReverseNodeEnumerator() {}
        virtual bool Move() = 0;
        virtual size_t GetLeafLength() const = 0;
        virtual size_t RecreateNode(char* buffer, size_t resultLength) = 0;
    };

    size_t WriteTrieBackwards(TOutputStream& os, TReverseNodeEnumerator& enumerator, bool verbose);

}
