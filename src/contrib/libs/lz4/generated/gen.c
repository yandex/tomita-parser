#define YVERSION  10
#define MEMORY_USAGE  10
#include "inc.c"
#define YVERSION  11
#define MEMORY_USAGE  11
#include "inc.c"
#define YVERSION  12
#define MEMORY_USAGE  12
#include "inc.c"
#define YVERSION  13
#define MEMORY_USAGE  13
#include "inc.c"
#define YVERSION  14
#define MEMORY_USAGE  14
#include "inc.c"
#define YVERSION  15
#define MEMORY_USAGE  15
#include "inc.c"
#define YVERSION  16
#define MEMORY_USAGE  16
#include "inc.c"
#define YVERSION  17
#define MEMORY_USAGE  17
#include "inc.c"
#define YVERSION  18
#define MEMORY_USAGE  18
#include "inc.c"
#define YVERSION  19
#define MEMORY_USAGE  19
#include "inc.c"
#define YVERSION  20
#define MEMORY_USAGE  20
#include "inc.c"

struct TLZ4Methods* LZ4Methods(int memory) {
    switch (memory) {
        case 10: return &y_10_ytbl;
        case 11: return &y_11_ytbl;
        case 12: return &y_12_ytbl;
        case 13: return &y_13_ytbl;
        case 14: return &y_14_ytbl;
        case 15: return &y_15_ytbl;
        case 16: return &y_16_ytbl;
        case 17: return &y_17_ytbl;
        case 18: return &y_18_ytbl;
        case 19: return &y_19_ytbl;
        case 20: return &y_20_ytbl;
    }

    return 0;
}
