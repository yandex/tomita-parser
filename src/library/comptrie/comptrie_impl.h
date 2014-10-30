#pragma once

#include <util/generic/list.h>
#include <util/generic/buffer.h>
#include <util/generic/noncopyable.h>
#include <util/generic/pair.h>
#include <util/generic/ptr.h>
#include <util/generic/static_assert.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/memory/pool.h>
#include <util/memory/tempbuf.h>
#include <util/memory/smallobj.h>
#include <util/stream/ios.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/string/traits.h>
#include <util/system/align.h>

#include "array_with_size.h"
#include "compact_vector.h"

#include "comptrie_packers.h"

#ifndef COMPTRIE_DATA_CHECK
#define COMPTRIE_DATA_CHECK 1
#endif

#define COMPTRIE_MAX2(a, b) (a) > (b) ? (a) : (b)
#define COMPTRIE_MAX3(a, b, c) COMPTRIE_MAX2(COMPTRIE_MAX2(a, b), c)

// NCompactTrie

namespace NCompactTrie {
    const char MT_FINAL    = '\x80';
    const char MT_NEXT     = '\x40';
    const char MT_SIZEMASK = '\x07';
    const size_t MT_LEFTSHIFT  = 3;
    const size_t MT_RIGHTSHIFT = 0;

    FORCED_INLINE size_t UnpackOffset(const char* p, size_t len);
    size_t MeasureOffset(size_t offset);
    size_t PackOffset(char* buffer, size_t offset);
    static inline ui64 ArcSaveOffset(size_t offset, TOutputStream& os);
    FORCED_INLINE char LeapByte(const char* &datapos, const char* dataend, char label);

    template <class T>
    inline static size_t ExtraBits() {
        return (sizeof(T) - 1) * 8;
    }

    static inline bool IsEpsilonLink(const char flags) {
        return !(flags & (MT_FINAL | MT_NEXT));
    }

    static inline void TraverseEpsilon(const char*& datapos) {
        const char flags = *datapos;
        if (!IsEpsilonLink(flags)) {
            return;
        }
        const size_t offsetlength = flags & MT_SIZEMASK;
        const size_t offset = UnpackOffset(datapos + 1, offsetlength);
        YASSERT(offset);
        datapos += offset;
    }

    static inline size_t LeftOffsetLen(const char flags) {
        return (flags >> MT_LEFTSHIFT) & MT_SIZEMASK;
    }

    static inline size_t RightOffsetLen(const char flags) {
        return flags & MT_SIZEMASK;
    }

    void ShowProgress(size_t n); // just print dots
}

namespace NCompTriePrivate {
    template <typename TChar>
    struct TStringForChar {
    };

    template <>
    struct TStringForChar<char> {
        typedef Stroka TResult;
    };

    template <>
    struct TStringForChar<wchar16> {
        typedef Wtroka TResult;
    };
};

namespace NCompTriePrivate {
    struct TCmp {
        template <class T>
        inline bool operator() (const T& l, const T& r) {
            return (unsigned char)(l.Label[0]) < (unsigned char)(r.Label[0]);
        }

        template <class T>
        inline bool operator() (const T& l, char r) {
            return (unsigned char)(l.Label[0]) < (unsigned char)r;
        }
    };
}

namespace NCompactTrie {
    static inline ui64 ArcSaveOffset(size_t offset, TOutputStream& os) {
        using namespace NCompactTrie;

        if (!offset)
            return 0;

        char buf[16];
        size_t len = PackOffset(buf, offset);
        os.Write(buf, len);
        return len;
    }



    // Unpack the offset to the next node. The encoding scheme can store offsets
    // up to 7 bytes; whether they fit into size_t is another issue.
    FORCED_INLINE size_t UnpackOffset(const char* p, size_t len) {
        size_t result = 0;

        while (len--)
            result = ((result << 8) | (*(p++) & 0xFF));

        return result;
    }

    // Auxiliary function: consumes one character from the input. Advances the data pointer
    // to the position immediately preceding the value for the link just traversed (if any);
    // returns flags associated with the link. If no arc with the required label is present,
    // zeroes the data pointer.
    FORCED_INLINE char LeapByte(const char* &datapos, const char* dataend, char label) {
        while (datapos < dataend) {
            size_t offsetlength, offset;
            const char* startpos = datapos;
            char flags = *(datapos++);

            if (IsEpsilonLink(flags)) {
                // Epsilon link - jump to the specified offset without further checks.
                // These links are created during minimization: original uncompressed
                // tree does not need them. (If we find a way to package 3 offset lengths
                // into 1 byte, we could get rid of them; but it looks like they do no harm.
                YASSERT(datapos < dataend);
                offsetlength = flags & MT_SIZEMASK;
                offset = UnpackOffset(datapos, offsetlength);
                if (!offset)
                    break;
                datapos = startpos + offset;

                continue;
            }

            char ch = *(datapos++);

            // Left branch
            offsetlength = LeftOffsetLen(flags);
            if ((unsigned char)label < (unsigned char)ch) {
                offset = UnpackOffset(datapos, offsetlength);
                if (!offset)
                    break;

                datapos = startpos + offset;

                continue;
            }

            datapos += offsetlength;

            // Right branch
            offsetlength = RightOffsetLen(flags);
            if ((unsigned char)label > (unsigned char)ch) {
                offset = UnpackOffset(datapos, offsetlength);

                if (!offset)
                    break;

                datapos = startpos + offset;

                continue;
            }

            // Got a match; return position right before the contents for the label
            datapos += offsetlength;
            return flags;
        }

        // if we got here, we're past the dataend - bail out ASAP
        datapos = 0;
        return 0;
    }

    template <typename TSymbol, class TPacker>
    FORCED_INLINE bool Advance(const char*& datapos, const char* const dataend, const char*& value,
        TSymbol label, TPacker packer) {

        YASSERT(datapos < dataend);
        char flags = MT_NEXT;
        for (int i = (int)ExtraBits<TSymbol>(); i >= 0; i -= 8) {
            flags = LeapByte(datapos, dataend, (char)(label >> i));
            if (!datapos) {
                return false; // no such arc
            }

            value = NULL;

            YASSERT(datapos <= dataend);
            if ((flags & MT_FINAL)) {
                value = datapos;
                datapos += packer.SkipLeaf(datapos);
            }

            if (!(flags & MT_NEXT)) {
                if (i == 0) {
                    datapos = NULL;
                    return true;
                }
                return false; // no further way
            }

            TraverseEpsilon(datapos);
            if (i == 0) { // last byte, and got a match
                return true;
            }
        }

        return false;
    }

}
