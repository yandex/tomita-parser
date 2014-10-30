#pragma once

#include "defaults.h"

//on some systems (not win, freebd, linux, but darwin (Mac OS X)
//multiple mlock calls on the same address range
//require the corresponding number of munlock calls to actually unlock the pages

//on some systems you must have privilege and resource limit

void LockMemory(const void* addr, size_t len);
void UnlockMemory(const void* addr, size_t len);

//does nothing on win because i don't know how to implement it
void UnlockAllMemory();
