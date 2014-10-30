#pragma once

#include "leaf_skipper.h"
#include <cstddef>

class TOutputStream;

namespace NCompactTrie {

size_t MeasureOffset(size_t offset);

// Return value: size of the minimized trie.
size_t RawCompactTrieMinimizeImpl(TOutputStream& os, const TOpaqueTrie& trie, bool verbose, size_t minMergeSize);

// Return value: size of the minimized trie.
template<class TPacker>
size_t CompactTrieMinimizeImpl(TOutputStream& os, const char *data, size_t datalength, bool verbose, const TPacker* packer)
{
    TPackerLeafSkipper<TPacker> skipper(packer);
    size_t minmerge = MeasureOffset(datalength);
    TOpaqueTrie trie(data, datalength, skipper);
    return RawCompactTrieMinimizeImpl(os, trie, verbose, minmerge);
}

} // namespace NCompactTrie
