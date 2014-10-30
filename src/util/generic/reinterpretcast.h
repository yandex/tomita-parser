#pragma once

template <class TTo, class TFrom>
static inline TTo ReinterpretCast(TFrom from) {
    union HIDDEN_VISIBILITY {
        TFrom from;
        TTo to;
    } a;

    a.from = from;

    return a.to;
}
