#pragma once

//#include "liblemmer/lemmer.h"

#include <library/lemmer/dictlib/grammarhelper.h>

#include <util/string/vector.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/stroka.h>

using namespace NSpike;

TGramBitSet HumanGrammemsToGramBitSet(const TWtringBuf& strGrammems);

class TSurnamePredictor
{

public:

    struct TFormInfo
    {
        int SurnameLemmaFlexNo;
        TGramBitSet StemGrammar;
        yvector<TGramBitSet> FlexGrammars;
    };

    struct TSurnameLemma
    {
        TSurnameLemma() { Weight = 0; }
        void Clear() { SurnameFlex.clear(); Weight = 0; };
        Wtroka SurnameFlex;
        double Weight;
    };

    struct TPredictedSurname
    {
        TPredictedSurname() {Weight = 0;}
        Wtroka Lemma;
        TGramBitSet StemGrammar;
        yvector<TGramBitSet> FlexGrammars;
        double Weight;
    };

public:

    TSurnamePredictor();
    ~TSurnamePredictor();

    void Load(const Stroka& path, ECharset encoding/* = CODES_WIN*/);

    /*
    As TSurnameLemmaModelProto following message type is expected:

    message TSurnameLemmaModel {
        required string suffix ;
        required float  weight ;

        message TSurnameForm {
            required string suffix ;
            required string gram ;
        }
        repeated TSurnameForm form;
    }
    */
    template <class TSurnameLemmaModelProto>
    void AddFromProto(const TSurnameLemmaModelProto& model) {
        TSurnameLemma lemma;
        yvector<TTempForm> paradigma;

        lemma.SurnameFlex = UTF8ToWide(model.Getsuffix());
        lemma.Weight = model.Getweight();

        for (size_t i = 0; i < model.formSize(); i++) {
            const typename TSurnameLemmaModelProto::TSurnameForm &form = model.Getform(i);
            TTempForm t;

            t.Flex = UTF8ToWide(form.Getsuffix());
            t.Grammars = HumanGrammemsToGramBitSet(UTF8ToWide(form.Getgram()));
            paradigma.push_back(t);

            if (MaxSurnameSuffixLength < t.Flex.size())
                MaxSurnameSuffixLength = t.Flex.size();
        }

        SaveParadigma(lemma,paradigma);
    }

    bool Predict(const TWtringBuf& w, yvector<TPredictedSurname>& res) const;

protected:

    struct TTempForm
    {
        Wtroka Flex;
        TGramBitSet Grammars;
    };

    void SaveParadigma(TSurnameLemma& surnameLemma, yvector<TTempForm>& paradigma);

    size_t MaxSurnameSuffixLength;
    yvector<TSurnameLemma> SurnameLemmasFlex;

    typedef ymap<Wtroka, yvector<TFormInfo>, ::TLess<TWtringBuf> > TSurnameFlexMap;
    TSurnameFlexMap SurnameFlex2Paradigmas;

};
