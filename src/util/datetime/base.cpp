#include "base.h"

#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/stream/output.h>
#include <util/stream/printf.h>
#include <util/system/compat.h>
#include <util/memory/tempbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

Stroka Strftime(const char* format, const struct tm* tm) {
    size_t size = Max<size_t>(strlen(format) * 2 + 1, 107);
    for (;;) {
        TTempBuf buf(size);
        int r = strftime(buf.Data(), buf.Size(), format, tm);
        if (r != 0)
            return Stroka(buf.Data(), r);
        size *= 2;
    }
}

template <>
TDuration FromStringImpl<TDuration, char>(const char* s, size_t len) {
    return TDuration::Parse(TStringBuf(s, len));
}

template <>
bool TryFromStringImpl<TDuration, char>(const char* s, size_t len, TDuration& result) {
    return TDuration::TryParse(TStringBuf(s, len), result);
}

template <>
void Out<TDuration>(TOutputStream& os, TTypeTraits<TDuration>::TFuncParam duration) {
    os << duration.Seconds() << '.';
    Printf(os, "%06d", (int)duration.MicroSecondsOfSecond());
    os << 's';
}

template <>
void Out<TInstant>(TOutputStream& os, TTypeTraits<TInstant>::TFuncParam instant) {
    char buf[DATE_8601_LEN];
    sprint_date8601(buf, instant.TimeT());
    if (!*buf)  // shouldn't happen due to current implementation of sprint_date8601() and GmTimeR()
        ythrow yexception() << "Out<TInstant>: year does not fit into an integer";
    os.Write(buf, DATE_8601_LEN - 2 /* 'Z' + '\0' */);
    os << '.';
    Printf(os, "%06d", (int)instant.MicroSecondsOfSecond());
    os << 'Z';
}


Stroka TDuration::ToString() const {
    return ::ToString(*this);
}


Stroka TInstant::ToString() const {
    return ::ToString(*this);
}

Stroka TInstant::ToStringUpToSeconds() const {
    char buf[DATE_8601_LEN];
    sprint_date8601(buf, TimeT());
    if (!*buf)
        ythrow yexception() << "TInstant::ToStringUpToSeconds: year does not fit into an integer";
    return Stroka(buf, DATE_8601_LEN - 1 /* '\0' */);
}

void Sleep(TDuration duration) {
    NanoSleep(duration.NanoSeconds());
}
