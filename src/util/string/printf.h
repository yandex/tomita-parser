#pragma once

#include <util/generic/stroka.h>

/// formatted print. return printed length:
int printf_format(2,0) vsprintf(Stroka& s, const char* c, va_list params);
/// formatted print. return printed length:
int printf_format(2,3) sprintf(Stroka& s, const char* c, ...);
Stroka printf_format(1,2) Sprintf(const char* c, ...);
int printf_format(2,3) fcat(Stroka& s, const char* c, ...);
