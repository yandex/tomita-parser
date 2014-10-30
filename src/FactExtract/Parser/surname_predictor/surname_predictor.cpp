#include "surname_predictor.h"

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/charset/wide.h>


TGramBitSet HumanGrammemsToGramBitSet(const TWtringBuf& strGrammems)
{
    Stroka str = WideToChar(~strGrammems, +strGrammems, CODES_WIN);
    VectorStrok gramTokens = splitStrokuBySet(str.c_str(), ",");

    TGramBitSet grammars;
    for (size_t j = 0; j < gramTokens.size(); j++) {
        if (gramTokens[j].empty())
            continue;
        TGrammar gr = TGrammarIndex::GetCode(gramTokens[j]);
        if (gr != gInvalid)
            grammars.Set(gr);
    }
    return grammars;
}

TSurnamePredictor::TSurnamePredictor()
    : MaxSurnameSuffixLength(0)
{
}

TSurnamePredictor::~TSurnamePredictor()
{
}

bool TSurnamePredictor::Predict(const TWtringBuf& w, yvector<TPredictedSurname>& res) const
{
    int MinLemmaSize = 1;
    TGrammarBunch usedFlexGrammars;
    for (int i = Min<int>((int)w.length() - MinLemmaSize, MaxSurnameSuffixLength); i > 0; i--) {
        TWtringBuf Suffix = w.SubStr(w.size() - i);
        TSurnameFlexMap::const_iterator it = SurnameFlex2Paradigmas.find(Suffix);

        //храним те граммемы, которые мы уже приписывали, для того, чтобы когда мы предсказываем
        //по более короткому хвосту, не приписывать граммемы уже приписанного более длинного
        //например, "Балуевского" может предсказаться по АВДУЕВСКИЙ и ТОЛСТОЙ, у АВДУЕВСКИЙ
        //хвост длиннее, и все граммемы хвоста по ТОЛСТОЙ вкладываются в граммемы по АВДУЕВСКИЙ
        //значит вообще не добавляем омонима, предсказанного ТОЛСТОЙ

        yvector<TGramBitSet> grammemsForThisSuffix;
        if (it != SurnameFlex2Paradigmas.end())
        {
            const yvector<TFormInfo>& formInfos = it->second;
            for (int j=0; j < formInfos.ysize(); j++)
            {
                TPredictedSurname predictedSurname;
                for (int k = 0 ; k < formInfos[j].FlexGrammars.ysize() ; k ++)
                {
                    TGramBitSet gramBitSet = formInfos[j].FlexGrammars[k];
                    gramBitSet |= formInfos[j].StemGrammar & AllGenders;
                    if (usedFlexGrammars.find(gramBitSet) == usedFlexGrammars.end())
                        predictedSurname.FlexGrammars.push_back(formInfos[j].FlexGrammars[k]);
                }

                if (predictedSurname.FlexGrammars.size() == 0)
                    continue;
                predictedSurname.StemGrammar = formInfos[j].StemGrammar;
                grammemsForThisSuffix.insert(grammemsForThisSuffix.begin(), predictedSurname.FlexGrammars.begin(),
                                                                            predictedSurname.FlexGrammars.end());

                const TSurnameLemma& L = SurnameLemmasFlex[formInfos[j].SurnameLemmaFlexNo];

                predictedSurname.Lemma = ::ToWtring(w.SubStr(0, w.size() - i)) + L.SurnameFlex;
                predictedSurname.Weight = L.Weight;
                res.push_back(predictedSurname);
            }
        }
        for (int k = 0 ; k < grammemsForThisSuffix.ysize() ; k++)
            usedFlexGrammars.insert(grammemsForThisSuffix[k]);

    }
    return res.size() > 0;
}

void TSurnamePredictor::SaveParadigma(TSurnameLemma& surnameLemma, yvector<TTempForm>& paradigma)
{
    if (surnameLemma.SurnameFlex.empty() ||
        (surnameLemma.Weight == 0))
        return;

    TGramBitSet stemGram;

    SurnameLemmasFlex.push_back(surnameLemma);

    unsigned char c = gBefore + 1;
    for (; c < gMax ; c++)
    {
        int i;
        for (i = 0 ; i < paradigma.ysize() ; i++)
        {
            if (!paradigma[i].Grammars.Test(NTGrammarProcessing::ch2tg(c)))
                break;
        }
        if (i >= paradigma.ysize())
            stemGram.Set(NTGrammarProcessing::ch2tg(c));
    }

    Stroka ss = stemGram.ToString(",");

    for (int i = 0 ; i < paradigma.ysize() ; i++)
    {
        TSurnameFlexMap::iterator it = SurnameFlex2Paradigmas.find(paradigma[i].Flex);

        if (it == SurnameFlex2Paradigmas.end())
        {
            TFormInfo formInfo;
            formInfo.SurnameLemmaFlexNo = SurnameLemmasFlex.ysize() - 1;
            formInfo.StemGrammar = stemGram;
            ss = TGramBitSet(paradigma[i].Grammars & ~stemGram).ToString(",");
            formInfo.FlexGrammars.push_back(paradigma[i].Grammars & ~stemGram);
            SurnameFlex2Paradigmas[paradigma[i].Flex] = yvector<TFormInfo>(1,formInfo);
        } else {
            yvector<TFormInfo>& infos = it->second;
            int j;
            for (j = 0; j < infos.ysize(); j++) {
                if (infos[j].SurnameLemmaFlexNo == (SurnameLemmasFlex.ysize() - 1)) {
                    ss = TGramBitSet(paradigma[i].Grammars & ~stemGram).ToString(",");
                    infos[j].FlexGrammars.push_back(paradigma[i].Grammars & ~stemGram);
                    break;
                }
            }
            if (j >= infos.ysize()) {
                TFormInfo formInfo;
                formInfo.SurnameLemmaFlexNo = SurnameLemmasFlex.ysize() - 1;
                formInfo.StemGrammar = stemGram;
                ss = TGramBitSet(paradigma[i].Grammars & ~stemGram).ToString(",");
                formInfo.FlexGrammars.push_back(paradigma[i].Grammars & ~stemGram);
                infos.push_back(formInfo);
            }
        }
    }
}

static inline void CustomSplit(const TStringBuf& str, char sep, yvector<TStringBuf>& result)
{
    result.clear();

    typedef TContainerConsumer< yvector<TStringBuf> > TConsumer;
    TConsumer consumer(&result);
    TSkipEmptyTokens<TConsumer> filter(&consumer);
    SplitString(str.data(), str.data() + str.size(), TCharDelimiter<const char>(sep), filter);
}

void TSurnamePredictor::Load(const Stroka& path, ECharset encoding)
{
    TIFStream is(path);

    TSurnameLemma lemma;
    yvector<TTempForm> paradigma;
    Stroka line;
    yvector<TStringBuf> fields;
    while (is.ReadLine(line)) {
        CustomSplit(line, ' ', fields);
        if (fields.ysize() < 2)
            continue;

        if (fields[0][0] == '-')
            fields[0] = fields[0].SubStr(1);

        if (fields[1][0] == '-')
            fields[1] = fields[1].SubStr(1);

        if (line[0] == '$') {
           SaveParadigma(lemma, paradigma);
           paradigma.clear();
           lemma.SurnameFlex = CharToWide(fields[1], encoding);
           lemma.Weight = FromString<double>(~fields[2], +fields[2]);
           continue;
        };

        if (lemma.SurnameFlex.empty())
            continue;
        if (MaxSurnameSuffixLength < fields[0].size())
            MaxSurnameSuffixLength = fields[0].size();

        TTempForm tempForm;
        tempForm.Flex = CharToWide(fields[0], encoding);
        tempForm.Grammars = HumanGrammemsToGramBitSet(CharToWide(fields[1], encoding));
        paradigma.push_back(tempForm);
    }
}

