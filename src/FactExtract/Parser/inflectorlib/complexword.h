#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <library/lemmer/dictlib/grammarhelper.h>
#include <library/token/token_flags.h>

#include <FactExtract/Parser/lemmerlib/lemmerlib.h>

namespace NInfl {

// Declared in this file:
class TComplexWord;
class TSimpleAutoColloc;


// forward
struct TYemmaSelector;

class TComplexWord {
public:
    TComplexWord(const TLangMask& langMask, const Wtroka& wordText, const Wtroka& lemmaText = Wtroka(),
                 const TGramBitSet& knownGrammems = TGramBitSet(), const TYandexLemma* knownLemma = NULL,
                 bool UseKnownGrammemsInLemmer = false);

    // constructor for non-temporary strings (owned by someone else)
    TComplexWord(const TLangMask& langMask, const TWtringBuf& wordText, const TWtringBuf& lemmaText = TWtringBuf(),
                 const TGramBitSet& knownGrammems = TGramBitSet(), const TYandexLemma* knownLemma = NULL,
                 bool useKnownGrammemsInLemmer = false);

    TComplexWord(const Wtroka& wordText, const yvector<TYandexLemma>& yemmas);
    TComplexWord(const TWtringBuf& wordText, const yvector<TYandexLemma>& yemmas);

    bool Inflect(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram = NULL) const;
    bool Normalize(Wtroka& res, TGramBitSet* resgrammems = NULL) const;


    // simply copy capitalization masks from @src to @dst, word by word
    static void CopyCapitalization(TWtringBuf src, Wtroka& dst);

    // more complex behaviour: look at the number of words in @src and @dst and find some common patterns
    static void MimicCapitalization(TWtringBuf src, Wtroka& dst);

    // case-insensitive word text comparison
    static bool IsEqualText(const TWtringBuf& a, const TWtringBuf& b);


    static inline TGramBitSet ExtractFormGrammems(const TYandexLemma& word, size_t flexIndex) {
        TGramBitSet grammems;
        YASSERT(flexIndex < word.FlexGramNum());
        NSpike::ToGramBitset(word.GetStemGram(), word.GetFlexGram()[flexIndex], grammems);
        return grammems;
    }

    inline TGramBitSet Grammems() const {
        return HasYemma ? SelectedYemmaGrammems() : KnownGrammems;
    }

    Stroka DebugString() const;


private:
    void ResetYemma(const TYandexLemma* knownLemma);
    void Analyze();
    bool SelectBestAgreedYemma(const TGramBitSet& agree, TYemmaSelector& res);

    bool InflectInt(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram) const;
    bool InflectYemma(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram) const;
    bool SplitAndModify(size_t wordDelim, const TGramBitSet& grammems,
                        Wtroka& res, TGramBitSet* resgrammems) const;

    // selected yemma grammems
    inline TGramBitSet SelectedYemmaGrammems() const {
        YASSERT(HasYemma);
        return ExtractFormGrammems(Yemma, 0);
    }


private:
    TLangMask LangMask;
    Wtroka WordTextBuffer, LemmaTextBuffer;
    TWtringBuf WordText, LemmaText;
    TGramBitSet KnownGrammems;
    bool UseKnownGrammemsInLemmer;

    yvector<TYandexLemma> CandidateYemmas;

    TYandexLemma Yemma;
    bool HasYemma;
    bool Analyzed;      // CandidateYemmas were already found using NLemmer::AnalyzeWord

    friend class TSimpleAutoColloc;
};


// A collocation with one main word and all other words dependent from the main one.
// The type of dependency (dependent features) are figured automatically.
class TSimpleAutoColloc {
public:
    TSimpleAutoColloc()
        : Main(0)
    {
    }

    void AddWord(const TComplexWord& word, bool isMain = false) {
        Words.push_back(word);
        if (isMain)
            Main = Words.size() - 1;
    }

    void AddMainWord(const TComplexWord& word) {
        AddWord(word, true);
    }

    // Find first noun in the sequence and set it as main
    void GuessMainWord();

    // Re-analyze dependent words to select lemmas best agreed with the main word
    void ReAgree();

    bool Inflect(const TGramBitSet& grammems, yvector<Wtroka>& res, TGramBitSet* resgram = NULL) const;
    // TODO: bool Modify(const TGramBitSet& grammems, Wtroka& res, TGramBitSet* resgram = NULL) const;
    bool Normalize(yvector<Wtroka>& res) const;

    Stroka DebugString() const;

private:
    void AgreeWithMain(const TGramBitSet& gram, TYemmaSelector* best = NULL);

private:
    yvector<TComplexWord> Words;
    size_t Main;
};



}   // NInfl
