#pragma once

#include <util/system/defaults.h>

#if defined(_MSC_VER) && _MSC_VER <= 1500
    #define nullptr NULL
    #define static_assert(condition, message)
#endif

#ifdef WALL
#define DECLARE_COPY(aClass) \
    public: \
        aClass(const aClass&); \
        aClass& operator=(const aClass&);
#else
#define DECLARE_COPY(aClass)
#endif

#define ARRAY_SSIZE(arr) (int)(sizeof(arr) / sizeof(arr[0]))

// Switch statement for C-strings
#define CASE_STRCMP(str, value)    case sizeof(value) - 1: if (strcmp(str, value) == 0)
#define ELSE_IF_STRCMP(str, value) else if (strcmp(str, value) == 0)

class Wtroka;
typedef Wtroka WStroka;

// Data load mode
enum ELoadMode {
    LM_DEFAULT,
    LM_MMAP, // memory map
    LM_PRECHARGE, // memory map and precharge
    LM_READ // read data into memory
};

#define DECLARE_ENUM_BIT_OPERATORS(Enum) \
    inline Enum operator|(Enum first, Enum second) { \
        return static_cast<Enum>(static_cast<int>(first) | second); \
    } \
    inline Enum& operator|=(Enum& first, Enum second) { \
        return first = first | second; \
    }

namespace NSize {
    const size_t KB = 1 << 10;
    const size_t MB = 1 << 20;
    const size_t GB = 1 << 30;
} // namespace NSize

////////////////////////////////////////////////////////////////////////////////
//
// IDisposable
//

class IDisposable {
public:
    virtual ~IDisposable() {}
};
