#pragma once

#include <util/system/defaults.h>

size_t wcslen(const wchar16* str);
const wchar16* wcschr(const wchar16* str, wchar16 ch);
int wcscmp(const wchar16* str1, const wchar16* str2);
int wcsncmp(const wchar16* str1, const wchar16* str2, size_t n);
long wcstol(const wchar16* ptr, wchar16** endptr, int base);
double wcstod(const wchar16* ptr, wchar16** endptr);
