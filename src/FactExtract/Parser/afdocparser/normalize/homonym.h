#pragma once


#include <FactExtract/Parser/inflectorlib/complexword.h>

#include <library/lemmer/dictlib/grammarhelper.h>

#include <util/generic/stroka.h>


class CWord;
class CHomonym;
class CWordVector;
class THomonymGrammems;


class THomonymInflector {
public:
    // @hom could be NULL if unknown
    THomonymInflector(const CWord& word, const CHomonym* hom, const CWordVector& allWords)
        : Word(word), Hom(hom), AllWords(allWords)
    {
    }

    bool Inflect(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram);
    //bool Normalize(Wtroka& res);

    NInfl::TComplexWord MakeComplexWord(const TGramBitSet& grammems, bool known) const;

    static bool FindInForms(const THomonymGrammems& forms, const TGramBitSet& grammems, TGramBitSet& resgram);
    static const CHomonym* SelectBestHomonym(const CWord& word, const TGramBitSet& desired);
    static TGramBitSet SelectBestHomoform(const CHomonym& hom, const TGramBitSet& desired);

private:
    bool FindInForms(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) const;
    bool AsForeign(const TGramBitSet& grammems, Wtroka& restext, TGramBitSet& resgram) const;

private:
    const CWord& Word;
    const CHomonym* Hom;
    const CWordVector& AllWords;
};

