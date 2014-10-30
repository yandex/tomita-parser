#pragma once

#include <util/system/defaults.h>

enum TFormType {
    fGeneral,
    fExactWord,  //!
    fExactLemma, //!!
    fWeirdExactWord
};

class Stroka;
const Stroka& ToString(TFormType);
bool FromString(const Stroka& name, TFormType& ret);

