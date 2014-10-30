#pragma once

#include "static_ctor.h"
#include <util/charset/doccodes.h>

class TMutex;

////////////////////////////////////////////////////////////////////////////////
//
// Charset
//

class TCharset {
    DECLARE_NOCOPY(TCharset);
    DECLARE_STATIC_CTOR_AND_DTOR;

// Constructor/Destructor
protected:
    TCharset(ECharset code) : Code(code) {}
public:
    virtual ~TCharset() {}

// Static methods
public:
    static const TCharset* ForCode(ECharset code);

// Methods
public:
    virtual void Encode(
        const wchar16 chars[], size_t charCount,
        char bytes[], size_t byteCount,
        size_t* charsUsed, size_t* bytesUsed) const = 0;

    virtual void Decode(
        const char bytes[], size_t byteCount, bool endOfInput,
        wchar16 chars[], size_t charCount,
        size_t* bytesUsed, size_t* charsUsed) const = 0;

// Helper methods
private:
    static const TCharset* CreateCharset(ECharset code);

// Fields
protected:
    const ECharset Code;
private:
    static const char ClassName[];
    static const TCharset* Charsets[];
    static TMutex Mutex;
    static bool Ready;
};
