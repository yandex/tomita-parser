#pragma once

#if !defined (_STLP_OUTERMOST_HEADER_ID)
    #define _STLP_OUTERMOST_HEADER_ID 0x2790
    #include <stlport/stl/_prolog.h>
#elif (_STLP_OUTERMOST_HEADER_ID == 0x2790) && !defined (_STLP_DONT_POP_HEADER_ID)
    #define _STLP_DONT_POP_HEADER_ID
#endif

#if defined(__WINE__)
#include_next <inttypes.h>
#else
#if (defined _MSC_VER) && (_MSC_VER < 1800)
    #include "inttypes_msvc.h"
#else
    #include _STLP_NATIVE_C_HEADER(inttypes.h)
#endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x2790)
    #if ! defined (_STLP_DONT_POP_HEADER_ID)
        #include <stlport/stl/_epilog.h>
        #undef  _STLP_OUTERMOST_HEADER_ID
    #else
        #undef  _STLP_DONT_POP_HEADER_ID
    #endif
#endif
