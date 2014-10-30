#pragma once

// A temporary auxillary file for the migration to C++11

#define _ALLOW_KEYWORD_MACROS

#if __cplusplus < 201103 || defined (_MSC_VER)
    #define constexpr const
    #define noexcept throw ()
#else
    #define ARCADIA_CPP_11
#endif
