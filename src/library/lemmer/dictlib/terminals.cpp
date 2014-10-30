#include <util/generic/hash.h>
#include <util/string/vector.h>
#include "terminals.h"

namespace NSpike {

static TTerminal YxTerminals[] = {
    {"AnyWord", NLP_END, "", "", 1},
    {"Adj", NLP_END, "A", "", 2},
    {"Adv", NLP_END, "ADV", "", 3},
    {"Com", NLP_END, "COM", "", 4},
    {"Conj", NLP_END, "CONJ", "", 5},
    {"Intj", NLP_END, "INTJ", "", 6},
    {"Part", NLP_END, "PART", "", 7},
    {"Prep", NLP_END, "PR", "", 8},
    {"Noun", NLP_END, "S", "", 9},
    {"Verb", NLP_END, "V", "", 10},
    {"AdjNum", NLP_END, "ANUM", "", 11},
    {"AdjPron", NLP_END, "APRO", "", 12},
    {"AdvPron", NLP_END, "ADVPRO", "", 13},
    {"Pron", NLP_END, "SPRO", "", 14},
    {"Det", NLP_END, "ART", "", 15},
    {"Idiom", NLP_END, "idiom", "", 16},
    {"Word", NLP_WORD, "", "", 17},
    {"NumI", NLP_INTEGER, "", "", 18},
    {"NumF", NLP_FLOAT, "", "", 19},
    {0, NLP_END, "", "", 20}
};

static const struct TYxTerminalIndex:
public yhash<Stroka, TTerminal, THash<Stroka>, TEqualTo<Stroka> > {
    TYxTerminalIndex() {
        for (TTerminal *i = YxTerminals; i->Name; ++i) {
            insert(value_type(i->Name, *i));
        }
    }
} YxTerminalIndex;

const TTerminal* YxTerminalByName(const Stroka& name) {
    TYxTerminalIndex::const_iterator it = YxTerminalIndex.find(name);
    if (it == YxTerminalIndex.end())
        return NULL;
    return &(it->second);
}

const TTerminal* TerminalByName(const Stroka& name) {
    return YxTerminalByName(name);
}

Stroka BitsetToString(const yvector<TGramBitSet>& bitset, const Stroka& delim /*= ", "*/, const Stroka& groupdelim /*= ", "*/) {
    Stroka s;
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (!s.empty())
            s += groupdelim;
        s += bitset[i].ToString(~delim);
    }
    return s;
}

TGramBitSet StringToBitset(const Stroka& gram, const Stroka& delim /*= ",; "*/) {
    if (gram.empty())
        return TGramBitSet();
    TGramBitSet res;
    VectorStrok tokens;
    SplitStrokuBySet(&tokens, ~gram, ~delim);
    for (size_t i = 0; i < tokens.size(); ++i) {
        TGrammar code = TGrammarIndex::GetCode(tokens[i]);
        if (code != gInvalid)
            res.Set(code);
    }
    return res;
}

bool Agree(const TGramBitSet& controller, const TGramBitSet& target, bool negative) {
    if (controller.none())
        return true;
    if (negative)
        return !((controller & target) == controller);
    return ((controller & target) == controller);
}

bool Agree(const TGramBitSet& controller, const yvector<TGramBitSet>& target, bool negative) {
    for (size_t i = 0; i < target.size(); ++i)
        if (Agree(controller, target[i], negative))
            return true;
    return false;
}

bool Agree(const TGramBitSet& controller, const TGramBitSet& target, const yvector<TGramBitSet>& categories/*, TGramBitSet& result*/) {
    TGramBitSet result;
    for (size_t i = 0; i < categories.size(); ++i) {
        if (((controller & categories[i]).any()) &&
           ((target & categories[i]).any()) &&
           ((categories[i] & controller & target).none()))
            return false;

        result |= controller & target & categories[i];
    }
    return result.any();
}

} //end of namespace
