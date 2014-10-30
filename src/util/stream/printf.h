#pragma once

#include <util/system/compat.h>

class TOutputStream;

size_t printf_format(2,3) Printf(TOutputStream& out, const char* fmt, ...);
size_t printf_format(2,0) Printf(TOutputStream& out, const char* fmt, va_list params);
