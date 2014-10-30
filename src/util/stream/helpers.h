#pragma once

#include <util/system/defaults.h>

class TBuffer;
class TInputStream;
class TOutputStream;

/// @addtogroup Streams
/// @{

ui64 TransferData(TInputStream* in, TOutputStream* out, void* buf, size_t sz);
ui64 TransferData(TInputStream* in, TOutputStream* out, TBuffer&);
ui64 TransferData(TInputStream* in, TOutputStream* out, size_t sz);
ui64 TransferData(TInputStream* in, TOutputStream* out);

/// @}
