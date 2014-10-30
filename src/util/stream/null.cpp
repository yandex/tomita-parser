#include "null.h"

#include <util/generic/singleton.h>

TNullIO& StdNullStream() throw () {
    return *Singleton<TNullIO>();
}

TNullInput::TNullInput() throw () {
}

TNullInput::~TNullInput() throw () {
}

size_t TNullInput::DoRead(void* /*buf*/, size_t /*len*/) {
    return 0;
}

bool TNullInput::DoNext(const void** /*ptr*/, size_t* /*len*/) {
    return false;
}

TNullOutput::TNullOutput() throw () {
}

TNullOutput::~TNullOutput() throw () {
}

void TNullOutput::DoWrite(const void* /*buf*/, size_t /*len*/) {
}

TNullIO::TNullIO() throw () {
}

TNullIO::~TNullIO() throw () {
}
