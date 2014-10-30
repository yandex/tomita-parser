#pragma once

#include "defaults.h"

#if defined(_win_)
    typedef void* FHANDLE;
    #define INVALID_FHANDLE ((FHANDLE)(long)-1)
#elif defined(_unix_)
    typedef int FHANDLE;
    #define INVALID_FHANDLE -1
#else
#   error
#endif
