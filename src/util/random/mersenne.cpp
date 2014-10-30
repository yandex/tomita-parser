#include "mersenne.h"
#include <util/generic/yexception.h>

namespace NPrivate {

TTempBuf ReadRandData(TInputStream* pool, size_t len) {
    TTempBuf tmp(len);

    if (pool->Load(tmp.Data(), len) != len) {
        ythrow yexception() << "Unexpected end of stream";
    }

    return tmp;
}

}
