#include "input.h"
#include "output.h"
#include "helpers.h"

#include <util/memory/tempbuf.h>
#include <util/system/defaults.h>
#include <util/generic/buffer.h>

ui64 TransferData(TInputStream* in, TOutputStream* out, TBuffer& b) {
    return TransferData(in, out, b.Data(), b.Size());
}

ui64 TransferData(TInputStream* in, TOutputStream* out, size_t sz) {
    TTempBuf tmp(sz);
    return TransferData(in, out, tmp.Data(), tmp.Size());
}

ui64 TransferData(TInputStream* in, TOutputStream* out) {
    TTempBuf tmp;
    return TransferData(in, out, tmp.Data(), tmp.Size());
}

ui64 TransferData(TInputStream* in, TOutputStream* out, void* buf, size_t sz) {
    ui64   totalReaded = 0;
    size_t readed;

    while ((readed = in->Read(buf, sz)) != 0) {
        out->Write(buf, readed);
        totalReaded += readed;
    }

    return totalReaded;
}
