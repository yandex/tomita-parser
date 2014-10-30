#pragma once

#include "leaf_skipper.h"
#include <cstddef>

class TOutputStream;

namespace NCompactTrie {
// Return value: size of the resulting trie.
size_t RawCompactTrieFastLayoutImpl(TOutputStream& os, const NCompactTrie::TOpaqueTrie& trie, bool verbose);

// Return value: size of the resulting trie.
template<class TPacker>
size_t CompactTrieMakeFastLayoutImpl(TOutputStream& os, const char *data, size_t datalength, bool verbose, const TPacker* packer)
{
    TPackerLeafSkipper<TPacker> skipper(packer);
    TOpaqueTrie trie(data, datalength, skipper);
    return RawCompactTrieFastLayoutImpl(os, trie, verbose);
}

} // namespace NCompactTrie
