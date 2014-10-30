#include "type.h"
#include "ascii.h"

bool IsSpace(const char* s, size_t len) throw () {
    if (len == 0)
        return false;
    for (const char* p = s; p < s + len; ++p)
        if (!IsAsciiSpace(*p))
            return false;
    return true;
}

bool IsNumber(const TStringBuf& s) throw () {
    if (s.empty())
        return false;

    for (size_t i = 0; i < s.length(); ++i) {
        if (!isdigit((unsigned char)s[i]))
            return false;
    }
    return true;
}

bool istrue(const TStringBuf& v) {
    if (!v)
        return false;

    return !strnicmp(~v, "da", v.length()) || !strnicmp(~v, "yes", v.length())
        || !strnicmp(~v, "on", v.length()) || !strnicmp(~v, "1", v.length())
        || !strnicmp(~v, "true", v.length());
}

bool isfalse(const TStringBuf& v) {
    if (!v)
        return false;

    return !strnicmp(~v, "net", v.length()) || !strnicmp(~v, "no", v.length())
        || !strnicmp(~v, "off", v.length()) || !strnicmp(~v, "0", v.length())
        || !strnicmp(~v, "false", v.length());
}
