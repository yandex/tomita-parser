#pragma once

#include <util/generic/ptr.h>

/**
 * Ownership gaining stream wrappers.
 */

class TInputStream;
class TOutputStream;

template<typename TResultInput>
class TInputHolder : private THolder<TInputStream>, public TResultInput {
public:
    inline TInputHolder(TAutoPtr<TInputStream> input)
        : THolder<TInputStream>(input)
        , TResultInput(Get())
    {
    }

    template<typename TParam1>
    inline TInputHolder(TAutoPtr<TInputStream> input, const TParam1& param1)
        : THolder<TInputStream>(input.Release())
        , TResultInput(Get(), param1)
    {
    }

    virtual ~TInputHolder() throw () {
    }
};

template<typename TResultOutput>
class TOutputHolder : private THolder<TOutputStream>, public TResultOutput {
public:
    inline TOutputHolder(TAutoPtr<TOutputStream> output)
        : THolder<TOutputStream>(output.Release())
        , TResultOutput(Get())
    {
    }

    template<typename TParam1>
    inline TOutputHolder(TAutoPtr<TOutputStream> output, const TParam1& param1)
        : THolder<TOutputStream>(output.Release())
        , TResultOutput(Get(), param1)
    {
    }

    virtual ~TOutputHolder() throw () {
    }
};
