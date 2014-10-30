#include "util.h"
#include "encodexml.h"

#include <util/generic/stroka.h>
#include <util/charset/codepage.h>

static const bool safe_chars[256] = {
    0, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 0, 1,  1, 1, 0, 0,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 0, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
};

Stroka EncodeXMLString(const char* str)  {
    Stroka strout;
    if (!str)
        return Stroka();
    char num[15];
    num[14] = 0;
    num[13] = ';';
    for (const char* s = str, *s1; *s;) {
        for (s1 = s; safe_chars[(unsigned char)(*s1)]; s1++)
            ;
        if (s1 != s)
            strout.replace(+strout, 0, s, 0, s1 - s, s1 - s);
        s = s1;
        if (*s == 0)
            break;
        strout.replace(+strout, 0, "&#", 0, 2, 2);
        char *p = &num[13];
        for (int code = csYandex.unicode[(unsigned char)(*s++)]; code > 0; code /= 10)
            *(--p) = '0' + (code % 10);
        strout.append(p);
    }
    return strout;
}

Stroka EncodeXML(const char* str, int qEncodeEntities) {
    Stroka strout, tmp;
    if (!str)
        return Stroka();
    const char* s;
    if (qEncodeEntities) {
        tmp = EncodeHtmlPcdata(str, 1);
        s = ~tmp;
    } else {
        s = str;
    }
    int prev = 0;
    int i;
    for (i = 0; s[i]; i++) {
        // remove all "irregular" chars
        // XML 1.0 specification, section 2.2:
        // Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
        if (s[i] > 0 && s[i] < 32
            && s[i] != 0x09 && s[i] != 0x0A && s[i] != 0x0D) {
            strout.append(s + prev, 0, i-prev, i-prev);
            prev = i + 1;
        }
    }
    strout.append(s + prev, 0, i-prev, i-prev);
    return strout;
}
