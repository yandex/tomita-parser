#pragma once

#include "stack.h"
#include "output.h"

namespace Nydx {
namespace Nio {

typedef TStreamStackT<TOutputStream> TStreamStackOutput;
typedef TSharedRef<TStreamStackOutput> TStreamStackOutputRef;

}
}
