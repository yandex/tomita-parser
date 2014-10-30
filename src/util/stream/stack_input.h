#pragma once

#include "input.h"
#include "stack.h"

namespace Nydx {
namespace Nio {

typedef TStreamStackT<TInputStream> TStreamStackInput;
typedef TSharedRef<TStreamStackInput> TStreamStackInputRef;

}
}
