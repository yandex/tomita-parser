#pragma once

#include <cstddef>

namespace NCompactTrie {

class TNode;

static const size_t NPOS = static_cast<size_t>(-1);

class TWriteableNode {
public:

    const char* LeafPos;
    size_t LeafLength;

    size_t ForwardOffset;
    size_t LeftOffset;
    size_t RightOffset;
    char Label;

    TWriteableNode();
    explicit TWriteableNode(const TNode& node);

    // When you call this, the offsets should be relative to the end of the node. Use NPOS to indicate an absent offset.
    size_t Pack(char* buffer) const;

private:
    size_t Measure() const;
};


}
