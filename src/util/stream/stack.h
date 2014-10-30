#pragma once

#include <util/generic/ref.h>
#include <util/generic/stack.h>
#include <util/generic/noncopyable.h>

namespace Nydx {
namespace Nio {

template<typename TPStream> class TStreamStackT
    : public TPointerBase<TStreamStackT<TPStream>, TPStream>
    , public TNonCopyable
{
public:
    typedef TPStream TDStream;
    typedef TSharedRef<TPStream> TDStreamRef;

private:
    typedef ystack<TDStreamRef> TDStack;
    TDStack Stack_;

public:
    virtual ~TStreamStackT() {
        Reset();
    }

    // stack ops

    bool Empty() const {
        return Stack_.empty();
    }

    size_t Size() const {
        return Stack_.size();
    }

    void Push(const TSharedRef<TPStream>& ref) {
        if (ref)
            Stack_.push(ref);
    }

    void Pop() {
        Stack_.pop();
    }

    const TDStreamRef& Top() const {
        return Stack_.top();
    }

    // TPointerCommon
    TDStream *Get() const {
        return Empty() ? NULL : Top().Get();
    }

    explicit operator bool () const {
        return !Empty();
    }

    operator TDStream *() const {
        return Get();
    }

    // other

    void Swap(TStreamStackT& ref) {
        Stack_.Swap(ref.Stack_);
    }

    void Reset() {
        // order is important
        while (!Empty())
            Pop();
    }
};

template<typename TPStream> struct TStreamStackPtrT
    : public TSharedPtr<TStreamStackT<TPStream> >
{
    typedef TStreamStackT<TPStream> TDStack;
    TStreamStackPtrT()
        : TSharedPtr<TDStack>(new TDStack)
    {}
};

}
}
