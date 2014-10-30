#pragma once

#include "winint.h"
#include "defaults.h"

#include <util/generic/strbuf.h>

bool WinSymLink(const char* targetName, const char* linkName);
Stroka WinReadLink(const char* path);
HANDLE CreateFileWithUtf8Name(const TStringBuf& fName, ui32 accessMode, ui32 shareMode, ui32 createMode, ui32 attributes);
int WinRemove(const char* name);
