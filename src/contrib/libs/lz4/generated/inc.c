#define refTables RENAME(refTables)
#define LZ4_NbCommonBytes RENAME(LZ4_NbCommonBytes)
#define LZ4_compressCtx RENAME(LZ4_compressCtx)
#define LZ4_compress64kCtx RENAME(LZ4_compress64kCtx)
#define LZ4_compress_limitedOutput RENAME(LZ4_compress_limitedOutput)
#define LZ4_compress RENAME(LZ4_compress)
#define U16_S RENAME(U16_S)
#define _U16_S RENAME(_U16_S)
#define U32_S RENAME(U32_S)
#define _U32_S RENAME(_U32_S)
#define U64_S RENAME(U64_S)
#define _U64_S RENAME(_U64_S)
#define ytbl RENAME(ytbl)

#include "lz4.c"

static struct TLZ4Methods ytbl = {
      LZ4_compress
    , LZ4_compress_limitedOutput
};

#undef ytbl
#undef refTables
#undef LZ4_NbCommonBytes
#undef LZ4_compressCtx
#undef LZ4_compress64kCtx
#undef LZ4_compress_limitedOutput
#undef LZ4_compress
#undef U16_S
#undef _U16_S
#undef U32_S
#undef _U32_S
#undef U64_S
#undef _U64_S
#undef YVERSION
#undef A16
#undef A32
#undef A64
#undef AARCH
#undef BYTE
#undef COPYLENGTH
#undef GCC_VERSION
#undef HASH64KTABLESIZE
#undef HASHLOG64K
#undef HASHTABLESIZE
#undef HASH_LOG
#undef HASH_MASK
#undef HEAPMODE
#undef HTYPE
#undef INITBASE
#undef LASTLITERALS
#undef LZ4_64KLIMIT
#undef LZ4_ARCH64
#undef LZ4_BIG_ENDIAN
#undef LZ4_BLINDCOPY
#undef LZ4_COPYPACKET
#undef LZ4_COPYSTEP
#undef LZ4_FORCE_SW_BITCOUNT
#undef LZ4_FORCE_UNALIGNED_ACCESS
#undef LZ4_HASH64K_FUNCTION
#undef LZ4_HASH64K_VALUE
#undef LZ4_HASH_FUNCTION
#undef LZ4_HASH_VALUE
#undef LZ4_READ_LITTLEENDIAN_16
#undef LZ4_SECURECOPY
#undef LZ4_WILDCOPY
#undef LZ4_WRITE_LITTLEENDIAN_16
#undef MAXD_LOG
#undef MAX_DISTANCE
#undef MEMORY_USAGE
#undef MFLIMIT
#undef MINLENGTH
#undef MINMATCH
#undef ML_BITS
#undef ML_MASK
#undef NOTCOMPRESSIBLE_DETECTIONLEVEL
#undef RUN_BITS
#undef RUN_MASK
#undef S32
#undef SKIPSTRENGTH
#undef STACKLIMIT
#undef STEPSIZE
#undef U16
#undef U32
#undef U64
#undef UARCH
#undef expect
#undef likely
#undef lz4_bswap16
#undef matchlimit
#undef restrict
#undef unlikely
