#pragma once

#include "non_temp.h"

namespace NIter {

// Iterators over base sub-iterator values, joined as a single iterator
// following should be compilable: TExternalIt(*TInternalIt)
template <typename TExternalIt, typename TInternalIt>
class TSuperIterator: public TExternalIt {
public:
    TSuperIterator() {
    }

    TSuperIterator(TInternalIt it)
        : Int(it)
    {
        if (RestartExternal())
            FindNext();
    }

    void operator++() {
        TExternalIt::operator++();
        FindNext();
    }

private:
    bool RestartExternal() {
        if (Int.Ok()) {
            static_cast<TExternalIt&>(*this) = TExternalIt(*Int);    // @Int always returns T&
            return true;
        } else
            return false;
    }

    void FindNext() {
        while (!TExternalIt::Ok()) {
            ++Int;
            if (!RestartExternal())
                break;
        }
    }

private:
    TNonTempIterator<TInternalIt> Int;
};

} // namespace NIter
