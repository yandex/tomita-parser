#pragma once

namespace NIter {

// TFilter must have method "bool operator()(const T&)"
template <class TSourceIter, class TFilter>
class TFilteringIterator {
public:
    typedef typename TSourceIter::TValue TValue;

    TFilteringIterator()
        : Source()
        , Filter()
    {
    }

    TFilteringIterator(TSourceIter iter, TFilter filter)
        : Source(iter)
        , Filter(filter)
    {
        FindNext();
    }

    inline bool Ok() const {
        return Source.Ok();
    }

    inline void operator++() {
        ++Source;
        FindNext();
    }

    inline const TValue* operator->() const {
        return Source.operator->();
    }

    inline const TValue& operator*() const {
        return *Source;
    }
private:
    inline void FindNext() {
        while (Source.Ok() && !Filter(*Source))
            ++Source;
    }

    TSourceIter Source;
    TFilter Filter;
};

} // namespace NIter
