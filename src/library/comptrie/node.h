#pragma once

#include <cstddef>

namespace NCompactTrie {

class ILeafSkipper;

class TNode {
public:
    TNode();
    // Processes epsilon links and sets ForwardOffset to correct value. Assumes an epsilon link doesn't point to an epsilon link.
    TNode(const char * data, size_t offset, const ILeafSkipper& skipFunction);

    size_t GetOffset() const {
        return Offset;
    }

    const char* GetLeafPos() const {
        return LeafPos;
    }
    size_t GetLeafLength() const {
        return LeafLength;
    }
    size_t GetCoreLength() const {
        return CoreLength;
    }

    size_t GetForwardOffset() const {
        return ForwardOffset;
    }
    size_t GetLeftOffset() const {
        return LeftOffset;
    }
    size_t GetRightOffset() const {
        return RightOffset;
    }
    char GetLabel() const {
        return Label;
    }

    bool IsFinal() const {
        return LeafPos != nullptr;
    }

    bool HasEpsilonLinkForward() const {
        return ForwardOffset > Offset + CoreLength;
    }

    bool operator==(const TNode& other) const;
    bool operator !=(const TNode& other) const {
        return !(*this == other);
    }

private:
    size_t Offset;
    const char* LeafPos;
    size_t LeafLength;
    size_t CoreLength;

    size_t ForwardOffset;
    size_t LeftOffset;
    size_t RightOffset;
    char Label;
};

} // namespace NCompactTrie
