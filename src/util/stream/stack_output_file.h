#pragma once

#include "stack_output.h"
#include "stack_factory.h"

namespace Nydx {
namespace Nio {

class TStreamStackOutputFileFactory
    : public TStreamStackFactoryT<TOutputStream, TStreamStackOutputFileFactory>
{
public:
    static void DoOpen(TDStack& stream, const TStringBuf& file);
};

typedef
    TStreamStackProductT<TOutputStream, TStreamStackOutputFileFactory>
    TStreamStackOutputFile;

}
}
