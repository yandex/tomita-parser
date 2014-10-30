#include "format.h"

#include <util/string/cast.h>
#include <util/datetime/base.h>

void Time(TOutputStream& l) {
    l << millisec();
}

void TimeHumanReadable(TOutputStream& l) {
    char timeStr[30];
    const time_t t = time(0);

    l << ctime_r(&t, timeStr);
}
