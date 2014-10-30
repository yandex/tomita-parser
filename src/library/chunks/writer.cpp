#include <util/ysaveload.h>

#include "writer.h"

static inline void WriteAux(TOutputStream* out, const yvector<ui64>& data) {
    ::SavePodArray(out, ~data, +data);
}

/*************************** TBuffersWriter ***************************/

TChunkedDataWriter::TChunkedDataWriter(TOutputStream& slave)
    : Slave(slave)
    , Offset(0)
{
}

TChunkedDataWriter::~TChunkedDataWriter() throw () {
}

void TChunkedDataWriter::NewBlock() {
    if (Offsets.size()) {
        Lengths.push_back(Offset - Offsets.back());
    }

    Pad(16);
    Offsets.push_back(Offset);
}

void TChunkedDataWriter::WriteFooter() {
    Lengths.push_back(Offset - Offsets.back());
    WriteAux(this, Lengths);
    WriteAux(this, Offsets);
    WriteBinary<ui64>(Offsets.size());
    WriteBinary<ui64>(Version);
    WriteBinary<ui64>(0);
}

size_t TChunkedDataWriter::GetCurrentBlockOffset() const {
    YASSERT(!Offsets.empty());
    YASSERT(Offset >= Offsets.back());
    return Offset - Offsets.back();
}

size_t TChunkedDataWriter::GetBlockCount() const {
    return Offsets.size();
}
