#include "strip.h"
#include "ascii.h"

bool Collapse(const Stroka& from, Stroka& to, size_t maxLen) {
    size_t len;
    bool changed = false;
    to = from;
    if (maxLen == 0)
        maxLen = to.length();
    else
        maxLen = Min(maxLen, to.length());
    for (size_t start = 0; start < maxLen; ++start) {
        for (len = 0; start + len < maxLen; ++len) {
            if (!IsAsciiSpace(to[start + len]))
                break;
        }
        if (len > 1 || (len == 1 && to[start] != ' ')) {
            maxLen -= (len - 1);
            changed = true;
            to.replace(start, len, " ");
        }
    }
    return changed;
}

void CollapseText(const Stroka& from, Stroka& to, size_t maxLen) {
    Collapse(from, to, maxLen);
    Strip(to);
    if (+to >= maxLen) {
        to.remove(maxLen - 5); // " ..."
        to.reverse();
        size_t pos = to.find_first_of(" .,;");
        if (pos != Stroka::npos && pos < 32)
            to.remove(0, pos + 1);
        to.reverse();
        to.append(" ...");
    }
}
