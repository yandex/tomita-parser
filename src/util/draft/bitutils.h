#pragma once

#include <util/stream/str.h>
#include <util/system/align.h>

#ifdef _MSC_VER
#   include <intrin.h>
#endif

namespace NBitUtils {

// todo: use builtins where possible

FORCED_INLINE ui64 FastIfElse(bool cond, ui64 iftrue, ui64 iffalse) {
    // see http://www-graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
    return (-i64(cond) & (iftrue ^ iffalse)) ^ iffalse;
}

FORCED_INLINE ui64 FastZeroIfFalse(bool cond, ui64 iftrue) {
    return -i64(cond) & iftrue;
}

template <ui64 iftrue>
FORCED_INLINE ui64 FastZeroIfFalseK(bool cond) {
    return -i64(cond) & iftrue;
}

template <typename T>
FORCED_INLINE typename TTypeTraits<T>:: TUnsigned I2U(T x) {
    return T(x << 1ULL) ^ T(x >> ui64(sizeof(T) * 8 - 1));
}

template <typename T>
FORCED_INLINE typename TTypeTraits<T>:: TSigned U2I(T x) {
    return T(x >> 1ULL) ^ -T(x & 1ULL);
}

FORCED_INLINE ui64 FastMin(ui64 x, ui64 y) {
    return FastIfElse(x < y, x , y);
}

FORCED_INLINE ui64 FastMax(ui64 x, ui64 y) {
    return FastIfElse(x < y, y, x);
}

FORCED_INLINE ui64 Bits(ui64 bytes) {
    return bytes << 3ULL;
}

FORCED_INLINE ui64 Bytes(ui64 bits) {
    return bits >> 3ULL;
}

FORCED_INLINE ui64 BytesUp(ui64 bits) {
    return (bits + 7ULL) >> 3ULL;
}

FORCED_INLINE ui64 BitsUp(ui64 bits) {
    return Bits(BytesUp(bits));
}

template <ui64 by>
FORCED_INLINE ui64 GroupUp(ui64 val) {
    return (val + by - 1ULL) / by;
}

template <ui64 by>
FORCED_INLINE ui64 GroupBitsUp(ui64 off) {
    return GroupUp<by>(off) * by;
}

extern const ui64 WORD_MASK[];
extern const ui64 FLAG_MASK[];
extern const ui64 INVERSE_WORD_MASK[];

FORCED_INLINE ui64 Flag(ui64 bit) {
    return FLAG_MASK[bit];
}

FORCED_INLINE ui64 InverseMask(ui64 bits) {
    return INVERSE_WORD_MASK[bits];
}

FORCED_INLINE ui64 Mask(ui64 bits) {
    return WORD_MASK[bits];
}

FORCED_INLINE ui64 Mask(ui64 bits, ui64 skipbits) {
    return Mask(bits) << skipbits;
}

FORCED_INLINE ui64 InverseMask(ui64 bits, ui64 skipbits) {
    return ~Mask(bits, skipbits);
}

// FlagK<0> => 1, FlagK<1> => 10, ... FlagK<64> => 0
template <ui64 bits>
FORCED_INLINE ui64 FlagK() {
    return bits >= 64 ? 0 : 1ULL << (bits & 63);
}

// MaskK<0> => 0, MaskK<1> => 1, ... MaskK<64> => -1
template <ui64 bits>
FORCED_INLINE ui64 MaskK() {
    return FlagK<bits>() - 1;
}

template <ui64 bits>
FORCED_INLINE ui64 InverseMaskK() {
    return ~MaskK<bits>();
}

template <ui64 bits>
FORCED_INLINE ui64 MaskK(ui64 skipbits) {
    return MaskK<bits>() << skipbits;
}

template <ui64 bits>
FORCED_INLINE ui64 InverseMaskK(ui64 skipbits) {
    return ~MaskK<bits>(skipbits);
}

/**
 * Returns 0 - based position of the most significant bit.
 * 0 for 0.
 */
FORCED_INLINE ui64 MostSignificantBit(ui64 v) {
#ifdef __GNUC__
    ui64 res = v ? (63 - __builtin_clzll(v)) : 0;
#elif defined (_MSC_VER) && defined (_64_)
    unsigned long res = 0;
    if (v)
        _BitScanReverse64(&res, v);
#else
    ui64 res = 0;
    if (v)
        while (v >>= 1) res++;
#endif
    return res;
}

/**
 * Returns 1 - based position of the most significant bit.
 * 1 for 0.
 */
FORCED_INLINE ui64 SignificantLength(ui64 val) {
    return 1ULL + MostSignificantBit(val);
}

/**
 * returns the position of the least significant bit set to one
 */
FORCED_INLINE int LowestOneBitIdx(ui64 x)
{
    x &= -(i64)x;
    int r = 0;
    if (x & 0xffffffff00000000ULL) r += 32;
    if (x & 0xffff0000ffff0000ULL) r += 16;
    if (x & 0xff00ff00ff00ff00ULL) r += 8;
    if (x & 0xf0f0f0f0f0f0f0f0ULL) r += 4;
    if (x & 0xccccccccccccccccULL) r += 2;
    if (x & 0xaaaaaaaaaaaaaaaaULL) r += 1;
    return r;
}

FORCED_INLINE int LowestOneBitIdx(ui32 x)
{
    x &= -(i32)x;
    int r = 0;
    if (x & ui32(0xffff0000)) r += 16;
    if (x & ui32(0xff00ff00)) r += 8;
    if (x & ui32(0xf0f0f0f0)) r += 4;
    if (x & ui32(0xcccccccc)) r += 2;
    if (x & ui32(0xaaaaaaaa)) r += 1;
    return r;
}

FORCED_INLINE ui8 CeilLog2(ui64 x) {
    return (ui8)MostSignificantBit(x - 1) + 1;
}

namespace NPrivate {
// see http://www-graphics.stanford.edu/~seander/bithacks.html#ReverseParallel

FORCED_INLINE ui64 SwapOddEvenBits(ui64 v) {
    return ((v >> 1ULL)  & 0x5555555555555555ULL) | ((v & 0x5555555555555555ULL) << 1ULL);
}

FORCED_INLINE ui64 SwapBitPairs(ui64 v) {
    return ((v >> 2ULL)  & 0x3333333333333333ULL) | ((v & 0x3333333333333333ULL) << 2ULL);
}

FORCED_INLINE ui64 SwapNibbles(ui64 v) {
    return ((v >> 4ULL)  & 0x0F0F0F0F0F0F0F0FULL) | ((v & 0x0F0F0F0F0F0F0F0FULL) << 4ULL);
}

FORCED_INLINE ui64 SwapOddEvenBytes(ui64 v) {
    return ((v >> 8ULL)  & 0x00FF00FF00FF00FFULL) | ((v & 0x00FF00FF00FF00FFULL) << 8ULL);
}

FORCED_INLINE ui64 SwapBytePairs(ui64 v) {
    return ((v >> 16ULL) & 0x0000FFFF0000FFFFULL) | ((v & 0x0000FFFF0000FFFFULL) << 16ULL);
}

FORCED_INLINE ui64 SwapByteQuads(ui64 v) {
    return (v >> 32ULL) | (v << 32ULL);
}

}

FORCED_INLINE ui8 ReverseBytes(ui8 t) {
    return t;
}

FORCED_INLINE ui16 ReverseBytes(ui16 t) {
    using namespace NBitUtils::NPrivate;
    return (ui16)SwapOddEvenBytes(t);
}

FORCED_INLINE ui32 ReverseBytes(ui32 t) {
    using namespace NBitUtils::NPrivate;
    return (ui32)SwapBytePairs(SwapOddEvenBytes(t));
}

FORCED_INLINE ui64 ReverseBytes(ui64 t) {
    using namespace NBitUtils::NPrivate;
    return SwapByteQuads(SwapBytePairs(SwapOddEvenBytes(t)));
}

FORCED_INLINE ui8 ReverseBits(ui8 t) {
    using namespace NBitUtils::NPrivate;
    return (ui8)SwapNibbles(SwapBitPairs(SwapOddEvenBits(t)));
}

FORCED_INLINE ui16 ReverseBits(ui16 t) {
    using namespace NBitUtils::NPrivate;
    return (ui16)SwapOddEvenBytes(SwapNibbles(SwapBitPairs(SwapOddEvenBits(t))));
}

FORCED_INLINE ui32 ReverseBits(ui32 t) {
    using namespace NBitUtils::NPrivate;
    return (ui32)SwapBytePairs(SwapOddEvenBytes(SwapNibbles(SwapBitPairs(SwapOddEvenBits(t)))));
}

FORCED_INLINE ui64 ReverseBits(ui64 t) {
    using namespace NBitUtils::NPrivate;
    return SwapByteQuads(SwapBytePairs(SwapOddEvenBytes(SwapNibbles(SwapBitPairs(SwapOddEvenBits(t))))));
}

/*
 * referse first "bits" bits
 * 1000111000111000 , bits = 6 => 1000111000000111
 */
template <typename T>
FORCED_INLINE T ReverseBits(T v, ui64 bits) {
    return T(v & InverseMask(bits)) | T(ReverseBits(T(v & Mask(bits)))) >> (Bits(sizeof(T)) - bits);
}

/*
 * referse first "bits" bits starting from "skipbits" bits
 * 1000111000111000 , bits = 4, skipbits = 2 => 1000111000011100
 */
template <typename T>
FORCED_INLINE T ReverseBits(T v, ui64 bits, ui64 skipbits) {
    return (T(ReverseBits((v >> skipbits), bits)) << skipbits) | T(v & Mask(skipbits));
}

inline Stroka PrintBits(const char* a, const char* b, bool reverse = false) {
    Stroka s;
    TStringOutput out(s);
    for (const char* it = a; it != b; ++it) {
        if (it != a)
            out << ' ';

        ui8 b = *it;

        if (reverse)
            b = ReverseBits(b);

        for (ui32 mask = 1; mask < 0xff; mask <<= 1) {
            out << ((b & mask) ? '1' : '0');
        }
    }

    return s;
}

template <typename T>
inline Stroka PrintBits(T t, ui32 bits = Bits(sizeof(T))) {
    return PrintBits((char*)&t, ((char*)&t) + BytesUp(bits));
}

/*
 returns number of bits set; result is in most significatnt byte
*/
FORCED_INLINE ui64 ByteSums(ui64 x) {
    ui64 byteSums = x - ((x & 0xAAAAAAAAAAAAAAAAULL) >> 1);
    byteSums = (byteSums & 0x3333333333333333ULL) + ((byteSums >> 2) & 0x3333333333333333ULL);
    byteSums = (byteSums + (byteSums >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return byteSums * 0x0101010101010101ULL;
}

/*
returns number of bits set (aka. popcount)
*/
FORCED_INLINE int CountBits(ui64 x) {
    return ByteSums(x) >> 56;
}

#define ONES_STEP_4 (0x1111111111111111ULL)
#define ONES_STEP_8 (0x0101010101010101ULL)
#define ONES_STEP_9 (1ULL << 0 | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54)
#define MSBS_STEP_8 (0x80ULL * ONES_STEP_8)
#define MSBS_STEP_9 (0x100ULL * ONES_STEP_9)
#define INCR_STEP_8 (0x80ULL << 56 | 0x40ULL << 48 | 0x20ULL << 40 | 0x10ULL << 32 | 0x8ULL << 24 | 0x4ULL << 16 | 0x2ULL << 8 | 0x1)
#define LEQ_STEP_8(x,y) ((((((y) | MSBS_STEP_8) - ((x) & ~MSBS_STEP_8)) ^ (x) ^ (y)) & MSBS_STEP_8) >> 7)
#define UCOMPARE_STEP_9(x,y) (((((((x) | MSBS_STEP_9) - ((y) & ~MSBS_STEP_9)) | (x ^ y)) ^ (x | ~y)) & MSBS_STEP_9) >> 8)
#define ULEQ_STEP_9(x,y) (((((((y) | MSBS_STEP_9) - ((x) & ~MSBS_STEP_9)) | (x ^ y)) ^ (x & ~y)) & MSBS_STEP_9) >> 8)
#define ZCOMPARE_STEP_8(x) (((x | ((x | MSBS_STEP_8) - ONES_STEP_8)) & MSBS_STEP_8) >> 7)

/*
* returns the position of the leftmost bit set to one and preceded by k ones.
*/
FORCED_INLINE int SelectInWord(const ui64 x, const int k) {
    // Phase 1: sums by byte
    register ui64 byteSums = ByteSums(x);
    // Phase 2: compare each byte sum with k
    const ui64 kStep8 = k * 0x0101010101010101ULL;
    const ui64 place = (LEQ_STEP_8(byteSums, kStep8) * 0x0101010101010101ULL >> 53) & ~0x7;
    // Phase 3: Locate the relevant byte and make 8 copies with incrental masks
    const int byteRank = k - (((byteSums << 8) >> place) & 0xFF);
    const ui64 spreadBits = (x >> place & 0xFF) * 0x0101010101010101ULL & INCR_STEP_8;
    const ui64 bitSums = ZCOMPARE_STEP_8(spreadBits) * ONES_STEP_8;
    // Compute the inside-byte location and return the sum
    const ui64 byteRankStep8 = byteRank * 0x0101010101010101ULL;
    return (int)(place + (LEQ_STEP_8(bitSums, byteRankStep8) * 0x0101010101010101ULL >> 56));
}

}
