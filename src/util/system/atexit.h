#pragma once

#include "defaults.h"

typedef void (*TAtExitFunc)(void*);
typedef void (*TTraditionalAtExitFunc)();

void AtExit(TAtExitFunc func, void* ctx);
void AtExit(TAtExitFunc func, void* ctx, size_t priority);

void AtExit(TTraditionalAtExitFunc func);
void AtExit(TTraditionalAtExitFunc func, size_t priority);
