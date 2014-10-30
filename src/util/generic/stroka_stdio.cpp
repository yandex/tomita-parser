#include "cast.h"

#include <util/string/cast.h>
#include <util/fgood.h>

#ifdef _freebsd_ // fgetln
#define getline getline_alt_4test
#endif // _freebsd_

bool getline(TFILEPtr &f, Stroka &s) {
    char buf[4096];
    char *buf_ptr;
    if (s.capacity() > sizeof(buf)) {
        s.resize(s.capacity());
        if ((buf_ptr = fgets(s.begin(), IntegerCast<int>(s.capacity()), f)) == 0)
            return false;
    } else {
        if ((buf_ptr = fgets(buf, sizeof(buf), f)) == 0)
            return false;
    }
    size_t buf_len = strlen(buf_ptr);
    bool line_complete = buf_len && buf_ptr[buf_len - 1] == '\n';
    if (line_complete)
        buf_len--;
    if (buf_ptr == s.begin())
        s.resize(buf_len);
    else
        s.AssignNoAlias(buf, buf_len);
    if (line_complete)
        return true;
    while (fgets(buf, sizeof(buf), f)) {
        size_t buf_len = strlen(buf);
        if (buf_len && buf[buf_len - 1] == '\n') {
            buf[buf_len - 1] = 0;
            s.append(buf, buf_len - 1);
            return true;
        }
        s.append(buf, buf_len);
    }
    return true;
}
