#pragma once

#include "stack.h"

#include <util/generic/strbuf.h>

namespace Nydx {
namespace Nio {

template<typename TPStream, typename TPDerived> class TStreamStackFactoryT
{
public:
    typedef TStreamStackT<TPStream> TDStack;
    typedef TStreamStackPtrT<TPStream> TDStackPtr;

    static void Open(TDStack& stream, const TStringBuf& name) {
        if (!stream.Empty())
            stream.Reset();
        DoOpen(stream, name);
    }

    static bool TryOpen(TDStack& stream, const TStringBuf& name) {
        if (!stream.Empty())
            return false;
        DoOpen(stream, name);
        return true;
    }

    static TDStackPtr Create(const TStringBuf& name) {
        TDStackPtr stream;
        DoOpen(*stream, name);
        return stream;
    }

private:
    static void DoOpen(TDStack& stream, const TStringBuf& name) {
        TPDerived::DoOpen(stream, name);
    }
};

// a stack object produced by a factory
template<typename TPStream, typename TPFactory> class TStreamStackProductT
    : public TStreamStackT<TPStream>
{
public:
    TStreamStackProductT()
    {}

    TStreamStackProductT(const TStringBuf& name) {
        DoOpen(name);
    }

    void DoOpen(const TStringBuf& name) {
        TPFactory::DoOpen(*this, name);
    }

    void Open(const TStringBuf& name) {
        TPFactory::Open(*this, name);
    }

    bool TryOpen(const TStringBuf& name) {
        return TPFactory::TryOpen(*this, name);
    }
};

}
}
