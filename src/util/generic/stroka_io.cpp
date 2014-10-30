#include "stroka.h"

#include <util/fput.h>

#include <stlport/iostream>

#include <cctype>

bool Stroka::read_token_part(TIStream& is, char* buf, size_t bufsize, char) {
    is.width(bufsize);
    buf[0] = 0;
    is >> buf;
    if (!is.good())
        return true;
    if (isspace(is.peek()))
        return true;
    return false;
}

bool Stroka::read_line_part(TIStream& is, char* buf, size_t bufsize, char delim) {
#if defined(_STLP_IN_USE)
    while (bufsize > 1) {
        const int c = is.get();

        if (c == EOF || c == delim || !is) {
            *buf = 0;

            return true;
        }

        *buf++ = (char)c;
        --bufsize;
    }

    *buf = 0;

    return false;
#else
    is.get(buf, bufsize, delim);

    if (is.rdstate() & std::ios::failbit && !is.eof()) {
        is.clear();
    }

    if (!is.good()) {
        return true;
    }

    if (is.peek() == delim) {
        is.get();

        return true;
    }

    return false;
#endif
}

// from Doug Schmidt
// modified by iseg
#define CHUNK_SIZE 512
TIStream& Stroka::build(TIStream& is, bool(*f)(TIStream&, char*, size_t, char), size_t& total, char delim) {
    char buf[CHUNK_SIZE];
    bool enough = (*f)(is, buf, CHUNK_SIZE, delim);
    size_t count = (size_t) strlen(buf);
    size_t chunk_start = total;
    total += count;
    if (!enough)
        build(is, f, total, delim);
    else
        Relink(Allocate(total));
    memcpy(p + chunk_start, buf, count);
    return is;
}

TOStream& operator<< (TOStream& os, const Stroka& s) {
    return os.write(~s,+s);
}
