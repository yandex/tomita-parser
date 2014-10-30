#pragma once

#include "stack_input.h"
#include "stack_factory.h"

namespace Nydx {
namespace Nio {

class TStreamStackInputFileFactory
    : public TStreamStackFactoryT<TInputStream, TStreamStackInputFileFactory>
{
public:
    static void DoOpen(TDStack& stream, const TStringBuf& file);
};

typedef
    TStreamStackProductT<TInputStream, TStreamStackInputFileFactory>
    TStreamStackInputFile;

}
}
