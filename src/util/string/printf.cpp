#include "printf.h"

#include <util/stream/ios.h>
#include <util/stream/printf.h>

int vsprintf(Stroka& s, const char* c, va_list params) {
    TStringOutput so(s.remove());

    return Printf(so, c, params);
}

int sprintf(Stroka& s, const char *c, ...) {
    va_list params;
    va_start(params, c);
    const int k = vsprintf(s, c, params);
    va_end(params);
    return k;
}

Stroka Sprintf(const char* c, ...) {
    Stroka s;
    va_list params;
    va_start(params, c);
    vsprintf(s, c, params);
    va_end(params);
    return s;
}

int fcat(Stroka& s, const char* c, ...) {
    TStringOutput so(s);

    va_list params;
    va_start(params, c);
    const size_t ret = Printf(so, c, params);
    va_end(params);

    return ret;
}
