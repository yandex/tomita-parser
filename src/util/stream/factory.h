#pragma once

#include <util/generic/ptr.h>

class Stroka;
class TInputStream;
class TOutputStream;

TAutoPtr<TInputStream> OpenInput(const Stroka& url);
TAutoPtr<TOutputStream> OpenOutput(const Stroka& url);

// If @input is a compressed data stream return proper decompressing stream
// Otherwise return a copy of @input.
TAutoPtr<TInputStream> OpenMaybeCompressedInput(TInputStream* input);
