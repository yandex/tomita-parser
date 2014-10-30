#pragma once

#if !defined (_STLP_OUTERMOST_HEADER_ID)
    #define _STLP_OUTERMOST_HEADER_ID 0x2791
    #include <stlport/stl/_prolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x2791) && !defined (_STLP_DONT_POP_HEADER_ID)
    #define _STLP_DONT_POP_HEADER_ID
#endif

#if defined(__WINE__)
#include_next <stdint.h>
#else
#ifdef _MSC_VER
    #include "stdint_msvc.h"
#else
    #include _STLP_NATIVE_C_HEADER(stdint.h)
#endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x2791)
    #if ! defined (_STLP_DONT_POP_HEADER_ID)
        #include <stlport/stl/_epilog.h>
        #undef  _STLP_OUTERMOST_HEADER_ID
    #else
        #undef  _STLP_DONT_POP_HEADER_ID
    #endif
#endif
