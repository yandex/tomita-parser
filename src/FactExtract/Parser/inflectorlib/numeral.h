#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <library/lemmer/dictlib/grammarhelper.h>
#include <FactExtract/Parser/lemmerlib/lemmerlib.h>
#include <library/token/token_flags.h>


namespace NInfl {

class TNumFlexParadigm;

class TNumFlexByk {
public:
    TNumFlexByk()
        : Language(LANG_UNK)
    {
    }

    virtual ~TNumFlexByk() {
    }

    virtual bool Split(const TWtringBuf& text, TWtringBuf& numeral, TWtringBuf& delim, TWtringBuf& flexion) const;

    virtual const TNumFlexParadigm* Recognize(const TWtringBuf& /*numeral*/, const TWtringBuf& /*flex*/) const {
        return NULL;
    }

protected:
    virtual void AddForms(const Wtroka& numeral, TNumFlexParadigm* dst, const TGramBitSet& filter);
    inline void AddNumeralForms(const Wtroka& numeral, TNumFlexParadigm* dst) {
        AddForms(numeral, dst, TGramBitSet(gAdjNumeral));
    }

    docLanguage Language;
};

class TNumFlexLemmer {
public:
    // should be created via Singleton
    TNumFlexLemmer();

    static const TNumFlexByk& Default() {
        return *(Singleton<TNumFlexLemmer>()->DefaultByk);
    }

    static const TNumFlexByk& Byk(const TLangMask& langMask) {
        const TNumFlexLemmer* lemmer = Singleton<TNumFlexLemmer>();
        for (docLanguage lang = langMask.FindFirst(); lang != LANG_MAX; lang = langMask.FindNext(lang)) {
            ymap<docLanguage, TSharedPtr<TNumFlexByk> >::const_iterator it = lemmer->Byks.find(lang);
            if (it != lemmer->Byks.end())
                return *(it->second);
        }
        return Default();
    }

private:
    TSharedPtr<TNumFlexByk> DefaultByk;
    ymap<docLanguage, TSharedPtr<TNumFlexByk> > Byks;
};

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
