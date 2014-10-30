#include "gztarticle.h"
#include <FactExtract/Parser/afdocparser/builtins/articles_base.pb.h>


const NGzt::TDescriptor* TGztArticle::AuxDicArticleDescriptor() // static
{
    return TAuxDicArticle::descriptor();
}

bool TGztArticle::IsAuxDicArticle() const
{
    return IsInstance(TAuxDicArticle::descriptor());
}

size_t TGztArticle::GetMainWord() const
{
    size_t res = GetField<ui32>(TAuxDicArticle::kMainwordFieldNumber);
    return (res >= 1)? res - 1 : 0;
}

const NGzt::TMessage* TGztArticle::GetLemmaInfo() const
{
    if (!IsNull() && IsAuxDicArticle())
        return GetMessageField(*Get(), TAuxDicArticle::kLemmaFieldNumber);
    else
        return NULL;
}

const TStringBuf INDECLINABLE_PREFIX = STRINGBUF("!");

Wtroka TGztArticle::GetLemmaText(const NGzt::TMessage& lemma_info) const
{
    // gazetteer stores all articles strings as utf8
    Stroka text = GetGenericField<Stroka>(lemma_info, TLemmaInfo::kTextFieldNumber);
    if (text.has_prefix(INDECLINABLE_PREFIX))
        return Wtroka::FromUtf8(TStringBuf(text).SubStr(INDECLINABLE_PREFIX.size()));
    else
        return Wtroka::FromUtf8(text);
}

size_t TGztArticle::GetLemmaMainWord(const NGzt::TMessage& lemma_info) const
{
    size_t res = GetGenericField<ui32>(lemma_info, TLemmaInfo::kMainwordFieldNumber);
    return (res >= 1)? res - 1 : 0;
}

bool TGztArticle::GetLemmaAlways(const NGzt::TMessage& lemma_info) const
{
    return GetGenericField<bool>(lemma_info, TLemmaInfo::kAlwaysFieldNumber);
}

bool TGztArticle::GetLemmaIndeclinable(const NGzt::TMessage& lemma_info) const
{
    return GetGenericField<bool>(lemma_info, TLemmaInfo::kIndeclinableFieldNumber) ||
           // FACTEX-2258: syntactic sugar for "indeclinable = true"
           GetGenericField<Stroka>(lemma_info, TLemmaInfo::kTextFieldNumber).has_prefix(INDECLINABLE_PREFIX);
}

TGramBitSet TGztArticle::GetLemmaOutputGrammems(const NGzt::TMessage& lemma_info) const
{
    TGramBitSet res;
    const NGzt::TMessage* outgrams = GetMessageField(lemma_info, TLemmaInfo::kOutgramFieldNumber);
    if (outgrams != NULL)
        for (NGzt::TProtoFieldIterator<Stroka> it(*outgrams, TGrammemSet::kGFieldNumber); it.Ok(); ++it)
            NGzt::ParseSingleGrammemSet(*it, res);
    return res;
}

bool TGztArticle::GetLightKW() const
{
    if (!IsNull() && IsAuxDicArticle())
        return GetField<bool>(TAuxDicArticle::kLightKwFieldNumber);
    else
        return false;
}

static void SplitToWords(const Wtroka& phrase, yvector<Wtroka>& words)
{
    TWtringBuf tmp = phrase;
    tmp = StripString(tmp);
    while (!tmp.empty()) {
        words.push_back(ToWtring(tmp.NextTok(' ')));
        tmp = StripString(tmp);
    }
}

bool TGztArticle::FillSArticleLemmaInfo(SArticleLemmaInfo& lemma_info, Wtroka& lemma_text) const
{
    const NGzt::TMessage* gzt_lemma_info = GetLemmaInfo();
    if (gzt_lemma_info == NULL)
        return false;
    lemma_info.Reset();
    lemma_info.m_iMainWord = GetLemmaMainWord(*gzt_lemma_info);
    lemma_info.m_bIndeclinable = GetLemmaIndeclinable(*gzt_lemma_info);
    lemma_info.m_bAlwaysReplace = GetLemmaAlways(*gzt_lemma_info);
    lemma_text = GetLemmaText(*gzt_lemma_info);
    SplitToWords(lemma_text, lemma_info.m_Lemmas);
    if (lemma_info.m_iMainWord >= (int)lemma_info.m_Lemmas.size())
        lemma_info.m_iMainWord = 0;

    return true;
}



TBuiltinKWTypes::TBuiltinKWTypes(const NGzt::TProtoPool& descriptors)
    : Descriptors(&descriptors)
{
    const TFixedKWTypeNames& fixed_names = GetFixedKWTypeNames();

    Init(Number, fixed_names.Number);
    Init(Date, fixed_names.Date);
    Init(DateChain, fixed_names.DateChain);

    Init(CompanyAbbrev, fixed_names.CompanyAbbrev);
    Init(CompanyInQuotes, fixed_names.CompanyInQuotes);
    Init(CompanyChain, fixed_names.CompanyChain);
    Init(StockCompany, fixed_names.StockCompany);

    Init(FdoChain, fixed_names.FdoChain);

    Init(Fio, fixed_names.Fio);
    Init(FioWithoutSurname, fixed_names.FioWithoutSurname);
    Init(FioChain, fixed_names.FioChain);
    Init(TlVariant, fixed_names.TlVariant);

    Init(Parenthesis, fixed_names.Parenthesis);
    Init(CommunicVerb, fixed_names.CommunicVerb);

    Init(Geo, fixed_names.Geo);
    //Init(GeoContinent, fixed_names.GeoContinent);
    //Init(GeoCountry, fixed_names.GeoCountry);
    Init(GeoAdm, fixed_names.GeoAdm);
    //Init(GeoDistrict, fixed_names.GeoDistrict);
    Init(GeoCity, fixed_names.GeoCity);
    Init(GeoHauptstadt, fixed_names.GeoHauptstadt);
    Init(GeoSmallCity, fixed_names.GeoSmallCity);
    Init(GeoVill, fixed_names.GeoVill);

    //Init(GeoAdj, fixed_names.GeoAdj);
    //Init(National, fixed_names.National);

    Init(AntiStatus, fixed_names.AntiStatus);
    Init(LightStatus, fixed_names.LightStatus);

    // check that all fixed kw-types are initialized
    // (mostly to make sure we have not forgot anything)
    VerifyFixedNames();


    // initialize optional kwtypes
    GeoAdj = Descriptors->FindMessageTypeByName("geo_adj");
    National = Descriptors->FindMessageTypeByName("national");
}

void TBuiltinKWTypes::VerifyFixedNames() const
{
    const yvector<Stroka>& fixed_names = GetFixedKWTypeNames().GetList();
    for (size_t i = 0; i < fixed_names.size(); ++i)
        if (NameIndex.find(fixed_names[i]) == NameIndex.end())
            ythrow yexception() << "Fixed kw-type " << fixed_names[i] << " is not initialized!";
}

void TBuiltinKWTypes::Init(TKeyWordType& kwtype, const Stroka& kwtype_name)
{
    if (kwtype_name.empty())
        ythrow yexception() << "Fixed kw-type without name found (i.e. the name is not set in FixedKWTypeNames).";
    kwtype = Descriptors->FindMessageTypeByName(kwtype_name);
    NameIndex[kwtype_name] = kwtype;
}

