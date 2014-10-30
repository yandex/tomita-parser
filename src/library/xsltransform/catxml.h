#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/stroka.h>

class TInputStream;

typedef yvector<TSharedPtr<TInputStream>> TStreamVector;

struct TXmlConcatTask {
    Stroka Element;
    TStreamVector Streams;
    yvector<TXmlConcatTask> Children;

    TXmlConcatTask(const Stroka& element)
        : Element(element)
    {
    }
};
