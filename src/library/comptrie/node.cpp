#include "node.h"
#include "leaf_skipper.h"
#include "comptrie_impl.h"

#include <util/system/yassert.h>

namespace NCompactTrie {

TNode::TNode()
    : Offset(0)
    , LeafPos(nullptr)
    , LeafLength(0)
    , CoreLength(0)
    , ForwardOffset(0)
    , LeftOffset(0)
    , RightOffset(0)
    , Label(0)
{
}

// We believe that epsilon links are found only on the forward-position and that afer jumping an epsilon link you come to an ordinary node.

TNode::TNode(const char * data, size_t offset, const ILeafSkipper& skipFunction)
    : Offset(offset)
    , LeafPos(nullptr)
    , LeafLength(0)
    , CoreLength(0)
    , ForwardOffset(0)
    , LeftOffset(0)
    , RightOffset(0)
    , Label(0)
{
    if (!data)
        return; // empty constructor

    const char* datapos = data + offset;
    char flags = *(datapos++);
    YASSERT(!IsEpsilonLink(flags));
    Label = *(datapos++);

    size_t leftsize = LeftOffsetLen(flags);
    LeftOffset = UnpackOffset(datapos, leftsize);
    if (LeftOffset) {
        LeftOffset += Offset;
    }
    datapos += leftsize;

    size_t rightsize = RightOffsetLen(flags);
    RightOffset = UnpackOffset(datapos, rightsize);
    if (RightOffset) {
        RightOffset += Offset;
    }
    datapos += rightsize;

    if (flags & MT_FINAL) {
        LeafPos = datapos;
        LeafLength = skipFunction.SkipLeaf(datapos);
    }

    CoreLength = 2 + leftsize + rightsize + LeafLength;
    if (flags & MT_NEXT) {
        ForwardOffset = Offset + CoreLength;
        // There might be an epsilon link at the forward position.
        // If so, set the ForwardOffset to the value that points to the link's end.
        const char* forwardPos = data + ForwardOffset;
        const char forwardFlags = *forwardPos;
        if (IsEpsilonLink(forwardFlags)) {
            // Jump through the epsilon link.
            size_t epsilonOffset = UnpackOffset(forwardPos + 1, forwardFlags & MT_SIZEMASK);
            if (!epsilonOffset) {
                ythrow yexception() << "Corrupted epsilon link";
            }
            ForwardOffset += epsilonOffset;
        }
    }
}

} // namespace NCompactTrie
