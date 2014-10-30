#pragma once

#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include <FactExtract/Parser/afdocparser/rusie/textwordsequence.h>
#include <FactExtract/Parser/afdocparser/rusie/factgroup.h>

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/set.h>


struct CWorkGrammar;
class CWord;
class CHomonym;

// A helper for converting absolute word coordinates to tomita group coordinates
class TTomitaWords {
public:
    TTomitaWords(const CWordVector& words, const yvector<SWordHomonymNum>& fdoChainWords)
        : Words(words), TomitaWords(fdoChainWords)
    {}

    const CWord& operator[](size_t tomitaIndex) const {
        return Words.GetWord(TomitaWords[tomitaIndex]);
    }

    size_t Size() const {
        return TomitaWords.size();
    }

    CWordsPair TomitaCoords(const CWordsPair& group) const;
    const CFactSynGroup* GetChildGroup(const CFactSynGroup& parent, const CWordSequence& sequence) const;

    SWordHomonymNum GetFirstHomonymNum(const CSynGroup* group, size_t wordIndex, yvector<int>& alternativeHomIDs) const;
    SWordHomonymNum GetFirstHomonymNum(const CSynGroup* group, size_t wordIndex) const;
    const CHomonym& GetFirstHomonym(const CSynGroup* group, size_t wordIndex, yvector<const CHomonym*>& alternativeHomonyms) const {
        yvector<int> alternativeHomIDs;
        const CHomonym& resHom = Words[GetFirstHomonymNum(group, wordIndex, alternativeHomIDs)];
        for (int i = 0; i < alternativeHomIDs.ysize(); i++)
        {
            const CWord& w = (*this)[wordIndex];
            alternativeHomonyms.push_back(&w.GetRusHomonym(alternativeHomIDs[i]));
        }
        return resHom;
    }
private:
    const CWordVector& Words;
    const yvector<SWordHomonymNum>& TomitaWords;
};

class TSequenceInflector {
public:
    TSequenceInflector(CTextWS& sequence, const CFactSynGroup& parent,
                       const CWordVector& words, const yvector<SWordHomonymNum>& fdoChainWords)
        : Words(words), TomitaWords(words, fdoChainWords)
        , WordSequence(sequence)
        , ParentGroup(&parent)
        , Group(NULL)
        , MainWordIndex(0)
        , RestrictGztLemma(false)
    {
        if (WordSequence.HasLemmas())
            return;     // already normalized

        Group = TomitaWords.GetChildGroup(*ParentGroup, WordSequence);
        YASSERT(Group != NULL);

        Init();
    }

    void SetGrammar(const CWorkGrammar* grammar) {
        Grammar = grammar;
    }

    void ExcludeWord(size_t tomitaIndex) {
        Items[tomitaIndex].Excluded = true;
    }

    void Inflect(const TGramBitSet& grammems);
    void Normalize(const TGramBitSet& grammems);
    void Normalize();


    // only replace words with 'always-replace' gzt-lemmas if any.
    void ReplaceLemmaAlways();

private:
    void Init();
    void ResetMainWord(const CFactSynGroup* group);
    void ExtractAgreements(const CFactSynGroup& group);
    void ExtractBinaryAgreement(const CCheckedAgreement& agr);
    void AddBinaryAgreement(size_t w1, size_t w2, const TGramBitSet& agree);

    void CollectExcludedWords(const CGroup& group);
    void ExcludeOdinIz();
    void DelegateExcludedDescriptors();

    TGramBitSet SelectBestHomoform(size_t wordIndex, const CHomonym& hom, const TGramBitSet& desired) const;
    const CHomonym* SelectBestHomonym(size_t wordIndex, yvector<const CHomonym*>& alternativeHomonyms) const;

    TGramBitSet SelectNormGrammems() const;

    bool ShouldUseGztLemma(size_t wordIndex) const;
    bool WasCheckedByKWType(size_t wordIndex) const;
    bool WasCheckedByKWType(size_t wordIndex, const CGroup* curgroup) const;
    bool WasCheckedByKWSet(size_t wordIndex, const CGroup* curgroup) const;
    bool WasCheckedByKWSet(size_t wordIndex, const CGroup* curgroup, const SKWSet& kwset) const;



    bool InflectItems(const TGramBitSet& grammems);
    bool InflectWord(size_t wordIndex, const TGramBitSet& grammems);
    bool InflectWord(size_t wordIndex, const TGramBitSet& grammems, bool isnew, Wtroka& restext, TGramBitSet& resgram) const;
    Wtroka GetOriginalText(size_t wordIndex) const;       // a fallback if InflectWord fails

    bool NormalizeAsNumber(const CHomonym& hom, Wtroka& restext) const;
    bool InflectGztLemma(size_t wordIndex, const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) const;
    const CHomonym& GetHomonymWithBestGztLemmaByLevenshteinDist(size_t wordIndex) const;
    const SArticleLemmaInfo& GetBestGztLemmaByLevenshteinDist(const CWord& w, const CHomonym& hom, Wtroka& lemmaText, SArticleLemmaInfo& lemmInfo) const;
    bool InflectArtificialLemma(size_t wordIndex, const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) const;
    bool InflectSimpleSequence(size_t wordIndex, const CWordSequence& seq, const TGramBitSet& grammems,
                               yvector<Wtroka>& restext, TGramBitSet& resgram) const;



private:
    const CWordVector& Words;
    TTomitaWords TomitaWords;
    const CWorkGrammar* Grammar;

    CTextWS& WordSequence;
    const CFactSynGroup *ParentGroup, *Group;
    size_t MainWordIndex;

    bool RestrictGztLemma;      // if true, gzt-lemmas are used only when lemma.always is set

    yvector<size_t> ExcludedWords;     // in tomita coords (indices from @TomitaWords)

    struct TItem {
        const CWord* Word;
        const CHomonym* Homonym;
        TGramBitSet Keep, Enforce;
        yvector<const CHomonym*> AlternativeHomonyms;

        TGramBitSet ResultGrammems;
        Wtroka ResultText;
        bool AgreedNoun;        // true if the word is a noun agreed with the main word.
        bool Excluded;
        bool UseQuotes;

        TItem(): Word(NULL), Homonym(NULL), AgreedNoun(false), Excluded(false), UseQuotes(false) {}
    };

    struct TBinAgr {
        size_t Word1, Word2;
        TGramBitSet Grammems;
    };

    yvector<TItem> Items;       // only Items[Group->FirstWord():Group->LastWord()] are modified
    yvector<TBinAgr> BinAgrs;

    class TImpl;    // static impl methods
};
