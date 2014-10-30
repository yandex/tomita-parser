#pragma once

#include <util/system/platform.h>

#if defined(_win_)

#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <windows.h>

#   undef GetFreeSpace
#   undef LoadImage
#   undef GetMessage
#   undef SendMessage
#   undef DeleteFile
#   undef OPTIONAL
#   undef S_OK
#   undef S_FALSE
#   undef GetUserName
#   undef CreateMutex
#   undef GetObject

#   undef ERROR

#endif
