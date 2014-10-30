#pragma once

namespace NIter {

template <class T>
class TBaseIterator {
public:
    typedef T TValue;

    virtual ~TBaseIterator() {       // destructor is virtual!
    }

    //  basic iterator interface
    virtual bool Ok() const = 0;
    virtual void operator++() = 0;
    virtual const T* operator->() const = 0;

    inline const T& operator*() const {
        return *(operator->());
    }
};

} // namespace NIter
