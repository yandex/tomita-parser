#include "strbuf.h"

#include <util/stream/output.h>

template <>
void Out<TStringBuf>(TOutputStream& os, const TStringBuf& obj) {
    os.Write(obj.data(), obj.length());
}

template <>
void Out<TWtringBuf>(TOutputStream& os, const TWtringBuf& obj) {
    WriteString(os, obj.data(), obj.length());
}
