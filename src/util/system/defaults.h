#pragma once

#include "platform.h"

#if defined _unix_
#   define LOCSLASH_C '/'
#   define LOCSLASH_S "/"
#else
#   define LOCSLASH_C '\\'
#   define LOCSLASH_S "\\"
#endif // _unix_

#if defined(__INTEL_COMPILER) && defined(__cplusplus)
    #include <new>
#endif

// low and high parts of integers
#if !defined(_win_)
#   include <sys/param.h>
#endif

#if defined (BSD)
#   include <machine/endian.h>
#   if (BYTE_ORDER == LITTLE_ENDIAN)
#    define _little_endian_
#   elif (BYTE_ORDER == BIG_ENDIAN)
#    define _big_endian_
#   else
#    error unknown endian not supported
#   endif
#elif (defined (_sun_) && !defined (__i386__)) || defined (_hpux_) || defined (WHATEVER_THAT_HAS_BIG_ENDIAN)
#   define _big_endian_
#else
#   define _little_endian_
#endif

// alignment
#if (defined (_sun_) && !defined (__i386__)) ||  defined (_hpux_) || defined(__alpha__) || defined(__ia64__) || defined (WHATEVER_THAT_NEEDS_ALIGNING_QUADS)
#   define _must_align8_
#endif

#if (defined (_sun_) && !defined (__i386__)) ||  defined (_hpux_) || defined(__alpha__) || defined(__ia64__) || defined (WHATEVER_THAT_NEEDS_ALIGNING_LONGS)
#   define _must_align4_
#endif

#if (defined (_sun_) && !defined (__i386__)) ||  defined (_hpux_) || defined(__alpha__) || defined(__ia64__) || defined (WHATEVER_THAT_NEEDS_ALIGNING_SHORTS)
#   define _must_align2_
#endif

#include <inttypes.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

#define LL(number)   INT64_C(number)
#define ULL(number)  UINT64_C(number)

// Макросы для типов size_t и ptrdiff_t
#if defined(_32_)
#   if defined(_darwin_)
#       define  PRISZT "lu"
#   elif !defined(_cygwin_)
#       define  PRISZT PRIu32
#   else
#       define  PRISZT "u"
#   endif
#   define  SCNSZT SCNu32
#   define  PRIPDT PRIi32
#   define  SCNPDT SCNi32
#   define  PRITMT PRIi32
#   define  SCNTMT SCNi32
#elif defined(_64_)
#   if defined(_darwin_)
#       define  PRISZT "lu"
#   else
#       define  PRISZT PRIu64
#   endif
#   define  SCNSZT SCNu64
#   define  PRIPDT PRIi64
#   define  SCNPDT SCNi64
#   define  PRITMT PRIi64
#   define  SCNTMT SCNi64
#else
#   error "Unsupported platform"
#endif

#if defined(__GNUC__) && (__GNUC__> 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define alias_hack __attribute__((__may_alias__))
#endif

#ifndef alias_hack
#define alias_hack
#endif

typedef ui16 alias_hack ui16a;
typedef ui32 alias_hack ui32a;
typedef ui64 alias_hack ui64a;
#if defined(__cplusplus)
#if defined(_big_endian_)
union u_u16 {
  ui16a v;
  struct {
    ui8 hi8, lo8;
  } u;
} alias_hack;
union u_u32 {
  ui32a v;
  float alias_hack f;
  struct {
    u_u16 hi16, lo16;
  } u;
} alias_hack;
union u_u64 {
  ui64a v;
  double alias_hack f;
  struct {
    u_u32 hi32, lo32;
  } u;
} alias_hack;
#else /* _little_endian_ */
union u_u16 {
  ui16a v;
  struct {
    ui8 lo8, hi8;
  } alias_hack u;
} alias_hack;
union u_u32 {
  ui32a v;
  float alias_hack f;
  struct {
    u_u16 lo16, hi16;
  } u;
} alias_hack;
union u_u64 {
  ui64a v;
  double alias_hack f;
  struct {
    u_u32 lo32, hi32;
  } u;
} alias_hack;
#endif
#endif

#ifdef CHECK_LO_HI_MACRO_USAGE

inline void check_64(const ui64 &) {}
inline void check_64(const i64 &) {}
inline void check_64(const double &) {}
inline void check_32(const ui32 &) {}
inline void check_32(const i32 &) {}
inline void check_32(const float &) {}
inline void check_16(const ui16 &) {}
inline void check_16(const i16 &) {}

#define LO_32(x) (check_64(x),(*(u_u64*)&x).u.lo32.v)
#define HI_32(x) (check_64(x),(*(u_u64*)&x).u.hi32.v)
#define LO_16(x) (check_32(x),(*(u_u32*)&x).u.lo16.v)
#define HI_16(x) (check_32(x),(*(u_u32*)&x).u.hi16.v)
#define LO_8(x)  (check_16(x),(*(u_u16*)&x).u.lo8)
#define HI_8(x)  (check_16(x),(*(u_u16*)&x).u.hi8)
#define LO_8_LO_16(x) (check_32(x),(*(u_u32*)&x).u.lo16.u.lo8)
#define HI_8_LO_16(x) (check_32(x),(*(u_u32*)&x).u.lo16.u.hi8)

#else

#define LO_32(x) (*(u_u64*)&x).u.lo32.v
#define HI_32(x) (*(u_u64*)&x).u.hi32.v
#define LO_16(x) (*(u_u32*)&x).u.lo16.v
#define HI_16(x) (*(u_u32*)&x).u.hi16.v
#define LO_8(x)  (*(u_u16*)&x).u.lo8
#define HI_8(x)  (*(u_u16*)&x).u.hi8
#define LO_8_LO_16(x) (*(u_u32*)&x).u.lo16.u.lo8
#define HI_8_LO_16(x) (*(u_u32*)&x).u.lo16.u.hi8

#endif // CHECK_LO_HI_MACRO_USAGE

#if !defined (DONT_USE_SUPERLONG) && !defined (SUPERLONG_MAX)

#define SUPERLONG_MAX ~LL(0)

typedef i64 SUPERLONG;

#endif // SUPERLONG

//#if (WCHAR_MAX == 0x0000FFFFul)  typedef wchar_t wchar16;
// missing format specifications
// UCS-2, native byteorder
typedef ui16 wchar16;

// internal symbol type: UTF-16LE
typedef wchar16 TChar;

typedef ui32 wchar32;

typedef ui32 udochandle;

#if defined (_MSC_VER)
    #include <basetsd.h>
    typedef SSIZE_T ssize_t;
    #define HAVE_SSIZE_T 1
    #include <wchar.h>
#endif

#include <sys/types.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#   define PRAGMA(x)    _Pragma(#x)
#   define RCSID(idstr) PRAGMA(comment(exestr,idstr))
#else
#   define RCSID(idstr) static const char rcsid[] = idstr
#endif

#ifdef __GNUC__
    #define YDEPRECATED(message) __attribute__((deprecated(message)))
#elif defined(_MSC_VER)
    #if _MSC_VER >= 1400
        #define YDEPRECATED(message) __declspec(deprecated(message))
    #elif _MSC_VER >= 1300
        #define YDEPRECATED(message) __declspec(deprecated)
    #endif
#else
    #define YDEPRECATED(message)
#endif

/// Deprecated. Use TNonCopyable instead (util/generic/noncopyable.h)
#define DECLARE_NOCOPY(aClass) \
    private: \
       aClass(const aClass&); \
       aClass& operator=(const aClass&);

#ifdef _win_
#   include <malloc.h>
#elif defined(_sun_)
#   include <alloca.h>
#endif

#ifdef __GNUC__
#define printf_format(n,m) __attribute__((__format__(__printf__, n, m)))
#endif

#ifndef printf_format
#define printf_format(n,m)
#endif

#ifdef __GNUC__
#define YNORETURN __attribute__ ((__noreturn__))
#endif

#ifndef YNORETURN
#define YNORETURN
#endif

#ifdef _MSC_VER
#define YNORETURN_B __declspec(noreturn)
#endif

#ifndef YNORETURN_B
#define YNORETURN_B
#endif

#ifdef __GNUC__
#define YUNUSED __attribute__((unused))
#endif

#ifndef YUNUSED
#define YUNUSED
#endif

#if (defined(__GNUC__) && (__GNUC__ > 2))
#define EXPECT_TRUE(Cond) __builtin_expect(!!(Cond), 1)
#define EXPECT_FALSE(Cond) __builtin_expect(!!(Cond), 0)
#define FORCED_INLINE inline __attribute__ ((always_inline))
#define PREFETCH_READ(Pointer, Priority) __builtin_prefetch((const void *)(Pointer), 0, Priority)
#define PREFETCH_WRITE(Pointer, Priority) __builtin_prefetch((const void *)(Pointer), 1, Priority)
#endif

#ifndef PREFETCH_READ
#define PREFETCH_READ(Pointer, Priority) (void)(const void *)(Pointer), (void)Priority
#endif

#ifndef PREFETCH_WRITE
#define PREFETCH_WRITE(Pointer, Priority) (void)(const void *)(Pointer), (void)Priority
#endif


#ifndef EXPECT_TRUE
#define EXPECT_TRUE(Cond) (Cond)
#define EXPECT_FALSE(Cond) (Cond)
#endif

#if (!defined(NDEBUG) && __GNUC__==3 && __GNUC_MINOR__==4) || defined(_sun_)
#undef FORCED_INLINE
#define FORCED_INLINE inline
#endif

#ifdef _MSC_VER
#define FORCED_INLINE __forceinline
#endif

#ifndef FORCED_INLINE
#define FORCED_INLINE inline
#endif

#ifdef __GNUC__
#   define _packed __attribute__((packed))
#else
#   define _packed
#endif

#if defined(__GNUC__) && (__GNUC__> 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define _warn_unused_result __attribute__((warn_unused_result))
#endif

#ifndef _warn_unused_result
#define _warn_unused_result
#endif

#if defined(__GNUC__)
    #define HIDDEN_VISIBILITY __attribute__ ((visibility("hidden")))
#endif

#if !defined(HIDDEN_VISIBILITY)
    #define HIDDEN_VISIBILITY
#endif

#ifdef NDEBUG
#define IF_DEBUG(X)
#else
#define IF_DEBUG(X) X
#endif

#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#undef ARRAY_BEGIN
#define ARRAY_BEGIN(arr) (arr)

#undef ARRAY_END
#define ARRAY_END(arr) ((arr) + ARRAY_SIZE(arr))

// Конкатенация двух символов, даже при условии, что один из аргументов является макросом
// (макрос с совпадающим именем и совсем другим поведением может быть определен также в perl5/[0-9\.]+/mach/CORE/thread.h)
#ifdef JOIN
#undef JOIN
#endif

#define JOIN(X, Y)     UTIL_PRIVATE_DO_JOIN(X, Y)
#define UTIL_PRIVATE_DO_JOIN(X, Y)  UTIL_PRIVATE_DO_JOIN2(X, Y)
#define UTIL_PRIVATE_DO_JOIN2(X, Y) X##Y

#define STRINGIZE(X) UTIL_PRIVATE_STRINGIZE_AUX(X)
#define UTIL_PRIVATE_STRINGIZE_AUX(X) #X

#ifndef UNUSED
#define UNUSED(var) (void)(var)
#endif

#if defined(__has_feature)
    #if __has_feature(thread_sanitizer)
        #define _tsan_enabled_
    #endif
    #if __has_feature(memory_sanitizer)
        #define _msan_enabled_
    #endif
    #if __has_feature(address_sanitizer)
        #define _asan_enabled_
    #endif
#endif

#if defined(__COUNTER__)
    #define GENERATE_UNIQUE_ID(N) JOIN(N, __COUNTER__)
#endif

#if !defined(GENERATE_UNIQUE_ID)
    #define GENERATE_UNIQUE_ID(N) JOIN(N, __LINE__)
#endif
