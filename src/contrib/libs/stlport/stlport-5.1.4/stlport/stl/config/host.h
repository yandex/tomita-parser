#if defined(__WINE__)
    #undef __linux__
    #undef linux
    #undef i386
    #undef  _STLP_NATIVE_INCLUDE_PATH
    #undef __unix
    #undef __unix__
    #undef  _PTHREADS

    #define _NOTHREADS 1
    #define _STLP_NO_THREADS 1
    #define _STLP_STD_NAME NStl
    #define _STLP_IN_USE 1
    #define _STLP_USE_STATIC_LIB 1
    #define _STLP_USE_NEWALLOC 1
    #define _STLP_USE_DEFAULT_FILE_OFFSET
#define _STLP_NO_WCHAR_T
    #define _STLP_NO_VENDOR_MATH_L
    #define _STLP_NO_VENDOR_STDLIB_L
    #define _STLP_MSVC_LIB 1300
#define _STLP_NO_NATIVE_MBSTATE_T
#define _STLP_NO_NATIVE_WIDE_FUNCTIONS
    #define _STLP_USE_WIN32_IO
//#define _STLP_WIN32
#else
    #include "../../../../config.h"
#endif

#define _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#define _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT
