#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <library/lemmer/dictlib/grammarhelper.h>
#include <FactExtract/Parser/lemmerlib/lemmerlib.h>
#include <library/token/token_flags.h>


namespace NInfl {

class TNumFlexParadigm;

class TNumeralAbbr {
public:
    TNumeralAbbr(const TLangMask& langMask, const TWtringBuf& wordText);

    bool IsRecognized() const {
        return Paradigm != NULL;
    }

    bool Inflect(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet* resgram = NULL) const;

    void ConstructText(const TWtringBuf& newflex, Wtroka& text) const;
private:

    TWtringBuf Prefix, Numeral, Delim, Flexion;
    const TNumFlexParadigm* Paradigm;   // without ownership
};

}   // namespace NInfl
