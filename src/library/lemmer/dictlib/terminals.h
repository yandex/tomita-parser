#pragma once

#include "grammarhelper.h"

#include <util/generic/stroka.h>

#include <library/token/nlptypes.h>

namespace NSpike {

struct TTerminal {
    const char* Name;
    NLP_TYPE Type;
    const char* Gram;
    const char* Lemma;
    int Code;
};

const TTerminal* TerminalByName(const Stroka& name);
Stroka BitsetToString(const yvector<TGramBitSet>& bitset, const Stroka& delim = ", ", const Stroka& groupdelim = "; ");
TGramBitSet StringToBitset(const Stroka& gram, const Stroka& delim = ",; ");

bool Agree(const TGramBitSet& controller, const TGramBitSet& target, bool negative = false);
bool Agree(const TGramBitSet& controller, const yvector<TGramBitSet>& target, bool negative = false);
bool Agree(const TGramBitSet& controller, const TGramBitSet& target, const yvector<TGramBitSet>& categories/*, TGramBitSet& result*/);

} //end of namespace
