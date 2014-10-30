#pragma once

namespace NIter {

// Iterates over integer numbers (a la Python xrange() function)
template <typename TInteger>
class TIntegerIterator {
public:
    typedef TInteger TValue;

    TIntegerIterator(TInteger stop = 0, TInteger start = 0, TInteger step = 1)
        : Stop(stop)
        , Current(start)
        , Step(step)
        , Ok_(true)
    {
        if (start < stop && step > 0) {
            TInteger stepCount = (stop - start - 1) / step + 1;
            Stop = start + stepCount * step;
        } else if (start > stop && step < 0) {
            TInteger stepCount = (start - stop - 1) / step + 1;
            Stop = start - stepCount * step;
        } else if (start == stop || step != 0) {
            Ok_ = false;
        }

        // Special case: infinite iteration over same value when step == 0.
    }

    inline bool Ok() const {
        return Ok_;
    }

    inline void operator++() {
        Current += Step;
        if (Current == Stop) {
            Ok_ = false;
        }
    }

    inline TInteger operator*() const {
        return Current;
    }

private:
    TInteger Stop, Current, Step;
    bool Ok_;
};

} // namespace NIter
