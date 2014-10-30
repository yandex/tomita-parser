#include "char_set.h"
#include "wcs.h"
#include <util/generic/stroka.h>

////////////////////////////////////////////////////////////////////////////////
//
// CharSet
//

void TCharSet::DoSet(const char* set) {
    for (const ui8* p = (const ui8*)set; *p; ++p)
        Mask[*p >> 5] |= 1 << (*p & 0x1f);
}

////////////////////////////////////////////////////////////////////////////////
//
// WCharSet
//

TWCharSet::TWCharSet(const wchar16* set)
  : Blocks(), Mask(0), Size(0)
{
    Init(set, wcslen(set));
}

TWCharSet::TWCharSet(const WStroka& set)
  : Blocks(), Mask(0), Size(0)
{
    Init(~set, set.length());
}

void TWCharSet::Init(const wchar16* set, size_t length) {
    NewBlock();

    for (const wchar16 *p = set, *end = set + length; p != end; ++p) {
        wchar16 hi = (wchar16)(*p >> 8);
        wchar16 lo = *p & 0xff;
        if (Blocks[hi] == 0)
            Blocks[hi] = NewBlock();
        Mask[Blocks[hi] + (lo >> 5)] |= (1 << (lo & 0x1f));
    }
}

TWCharSet::~TWCharSet() {
    delete[] Mask;
}

ui16 TWCharSet::NewBlock() {
    ui32* newMask = new ui32[Size + 8];
    memcpy(newMask, Mask, Size * sizeof(ui32));
    memset(newMask + Size, 0, 32);
    delete[] Mask;
    Mask = newMask;
    int off = Size;
    Size += 8;
    return (ui16)off;
}
