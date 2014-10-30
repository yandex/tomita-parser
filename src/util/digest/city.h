#pragma once

#include <util/generic/pair.h>
#include <util/generic/utility.h>

typedef TPair<ui64, ui64> uint128;

inline ui64 Uint128Low64(const uint128& x) {
    return x.first;
}

inline ui64 Uint128High64(const uint128& x) {
    return x.second;
}

// Hash function for a byte array.
ui64 CityHash64(const char* buf, size_t len);

// Hash function for a byte array.  For convenience, a 64-bit seed is also
// hashed into the result.
ui64 CityHash64WithSeed(const char* buf, size_t len, ui64 seed);

// Hash function for a byte array.  For convenience, two seeds are also
// hashed into the result.
ui64 CityHash64WithSeeds(const char* buf, size_t len, ui64 seed0, ui64 seed1);

// Hash function for a byte array.
uint128 CityHash128(const char* s, size_t len);

// Hash function for a byte array.  For convenience, a 128-bit seed is also
// hashed into the result.
uint128 CityHash128WithSeed(const char* s, size_t len, uint128 seed);

// Hash 128 input bits down to 64 bits of output.
// This is intended to be a reasonably good hash function.
inline ui64 Hash128to64(const uint128& x) {
    // Murmur-inspired hashing.
    const ui64 kMul = 0x9ddfea08eb382d69ULL;
    ui64 a = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
    a ^= (a >> 47);
    ui64 b = (Uint128High64(x) ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}
