#pragma once

#include <string.h>
#include <util/system/defaults.h>

class Wtroka;
typedef Wtroka WStroka;

////////////////////////////////////////////////////////////////////////////////
//
// TCharSet
//

class TCharSet {
// Constructor
public:
    TCharSet() : Mask() {}
    TCharSet(const char* set) : Mask() { DoSet(set); }

// Methods
public:
    void Set(const char* set) { DoSet(set); }

    int Test(char ch) const {
        return Mask[(ui8)ch >> 5] & (1 << (ch & 0x1f));
    }

    int Test(wchar16 ch) const {
        if (ch >= 256)
            return 0;
        return Mask[ch >> 5] & (1 << (ch & 0x1f));
    }

// Helper methods
private:
    void DoSet(const char* set);

// Fields
private:
    ui32 Mask[8];
};

////////////////////////////////////////////////////////////////////////////////
//
// WCharSet
//

class TWCharSet {
    DECLARE_NOCOPY(TWCharSet);

// Constructor
public:
    TWCharSet(const wchar16* set);
    TWCharSet(const WStroka& set);
    ~TWCharSet();

// Methods
public:
    int Test(wchar16 ch) const {
        wchar16 lo = ch & 0xff;
        return Mask[Blocks[ch >> 8] + (lo >> 5)] & (1 << (lo & 0x1f));
    }

// Helper methods
private:
    void Init(const wchar16* set, size_t length);
    ui16 NewBlock();

// Fields
private:
    ui16 Blocks[256];
    ui32* Mask;
    int Size;
};
