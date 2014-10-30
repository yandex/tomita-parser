#include <util/generic/cast.h>
#include <util/memory/blob.h>

#include "reader.h"

template <typename T>
static inline void ReadAux(const char* data, T* aux, T count, yvector<const char*>* result) {
    result->resize(count);
    for (size_t i = 0; i < count; ++i) {
        (*result)[i] = data + aux[i];
    }
}

TChunkedDataReader::TChunkedDataReader(const TBlob& blob)
    : Version(0)
    , Size(0)
{
    const char* cdata = blob.AsCharPtr();
    const size_t size = blob.Size();
    if (size < 4)
        ythrow yexception() << "empty file with chunks";
    ui32 last = ((ui32*)(cdata + size))[-1];

    if (last != 0) { // old version file
        ui32* aux = (ui32*)(cdata + size);
        ui32 count = last;
        Size = size - (count + 1) * sizeof(ui32);

        aux -= (count + 1);
        ReadAux<ui32>(cdata, aux, count, &Offsets);
        return;
    }

    ui64* aux = (ui64*)(cdata + size);
    Version = aux[-2];
    VERIFY(Version > 0, "Invalid version");

    ui64 count = aux[-3];

    aux -= (count + 3);
    ReadAux<ui64>(cdata, aux, count, &Offsets);

    aux -= count;
    Lengths.resize(count);
    for (size_t i = 0; i < count; ++i) {
        Lengths[i] = IntegerCast<size_t>(aux[i]);
    }
}


