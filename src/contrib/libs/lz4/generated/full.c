#include "iface.h"

#define ONLY_COMPRESS
#define RENAME(a)        DO_RENAME(YVERSION, a)
#define DO_RENAME(a, b)  DO_RENAME2(a, b)
#define DO_RENAME2(a, b) y_ ## a ## _ ## b

#include "gen.c"
