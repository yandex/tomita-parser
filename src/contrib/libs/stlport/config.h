#pragma once

#if defined(NATIVE_INCLUDE_PATH)
    #include <util/system/platform.h>
#else
    #include "../../../util/system/platform.h"
#endif

#if defined(_unix_) && !defined(_PTHREADS)
    #define _PTHREADS
#endif

#define _STLP_STD_NAME NStl
#define _STLP_IN_USE
#define _STLP_USE_STATIC_LIB
#define _STLP_USE_NEWALLOC 1

#if !defined(USE_INTERNAL_STL)
    #define _STLP_DONT_REDEFINE_STD
#endif

#undef _STLP_NATIVE_INCLUDE_PATH

#undef i386
#undef linux

/*
 * we need some magick here...
 */
#if defined(NATIVE_INCLUDE_PATH)
    #define _STLP_NATIVE_INCLUDE_PATH NATIVE_INCLUDE_PATH
#else
    #if defined(__GNUC__)
        #if defined(_freebsd_)
            #define _STLP_NATIVE_INCLUDE_PATH /usr/local/lib/gcc-__GNUC__.__GNUC_MINOR__.__GNUC_PATCHLEVEL__/include/c++/

        #elif defined(_linux_) || defined(_darwin_)
            #if __GNUC__ < 4
                #define _STLP_NATIVE_INCLUDE_PATH /usr/include/c++/__GNUC__.__GNUC_MINOR__/
            #else
                #define _STLP_NATIVE_INCLUDE_PATH /usr/include/c++/__GNUC__.__GNUC_MINOR__.__GNUC_PATCHLEVEL__/
            #endif
        #endif
    #endif
#endif
