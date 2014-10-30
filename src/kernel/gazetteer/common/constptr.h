#pragma once

#include <util/generic/ptr.h>


// allows TIntrusivePtr<const T>

template <class T, class D = TDelete>
class TMutableRefCounted {
public:
    inline TMutableRefCounted()
        : Counter_(0)
    {
    }

    inline TMutableRefCounted(const TMutableRefCounted&)
        : Counter_(0)
    {
    }

    inline void operator=(const TMutableRefCounted&) {
    }

    inline void Ref() const {
        Counter_.Inc();
    }

    inline void UnRef() const {
        if (!Counter_.Dec()) {
            D::Destroy(static_cast<const T*>(this));
        }
    }

    inline void DecRef() const {
        Counter_.Dec();
    }

private:
    mutable TAtomicCounter Counter_;
};
