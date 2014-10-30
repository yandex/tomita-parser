#include "src_root.h"

#include <util/system/defaults.h>

namespace NPrivate {
    TStaticBuf StripSourceRoot(const TStaticBuf& cur, const TStaticBuf& f) throw () {
        const size_t l1 = cur.Len;
        const size_t l2 = sizeof("util/system/src_root.h") - 1;

        if (l1 < l2) {
            return f;
        }

        const size_t src_root_len = l1 - l2;

        if (src_root_len > f.Len) {
            return f;
        }

        return TStaticBuf(f.Data + src_root_len, f.Len - src_root_len);
    }
}
