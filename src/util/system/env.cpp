#include "env.h"

#include <util/generic/stroka.h>
#include <util/generic/utility.h>

#include <stdlib.h>
#ifdef _win_
#include <winint.h>
#endif

Stroka GetEnv(const Stroka& key) {
#ifdef _win_
    char buf[1001];
    Zero(buf);
    int len = GetEnvironmentVariable(~key, &buf[0], 1000);
    return Stroka(&buf[0], len);
#else
    const char* env = getenv(~key);
    return env ? Stroka(env) : Stroka();
#endif
}

void SetEnv(const Stroka& key, const Stroka& value) {
#ifdef _win_
    SetEnvironmentVariable(~key, ~value);
#else
    setenv(~key, ~value, true /*replace*/);
#endif
}
