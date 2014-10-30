#pragma once

#include "common/serialize.h"

#include <util/generic/stroka.h>
#include <util/stream/base.h>

namespace NGzt {

// This number is increased only when binary format is changed to an extent
// that old and new formats become incompatible. This should be now quite
// rare situation as most of gazetteer control data is now stored as protobuf
// object in the binary and thus could be easily refactored without breaking
// version compatibility. Still there are several blocks in the binary which
// are not protobuf-serialized, and this number is to indicate their modification.

//static const ui32 GZT_BINARY_VERSION = 2;     // zipping of NBinary::TGazetteer is cancelled
  static const ui32 GZT_BINARY_VERSION = 3;     // introduced articles without filters

class TGztBinaryVersion {
public:
    static inline void Save(TOutputStream* output) {
        SaveProtectedSize(output, GZT_BINARY_VERSION);
    }

    static inline bool Load(TInputStream* input) {
        size_t version = 0;
        return LoadProtectedSize(input, version) && version == GZT_BINARY_VERSION;
    }

    // load version and fail if it is wrong
    static inline void Verify(TInputStream* input) {
        if (!Load(input)) {
            Stroka msg = "Failed to load gazetteer data: the binary is incompatible with your program. "
                         "You should either re-compiler your .gzt.bin, or rebuild your program from corresponding version of sources.\n";
            // TODO: add binary filename to message (this will require to use Stroka constructor instead of TBlob constructor)
            Cerr << msg << Endl;
            ythrow yexception() << msg;
        }
    }
};

}   // namespace NGzt

