#pragma once

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <kernel/gazetteer/gazetteer.h>

#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/auxdictlib/dictionary.h>


// Defined in this file:
class TGztArticle;
class TArticleRef;
class TBuiltinKWTypes;

class TGztArticle: public NGzt::TArticlePtr
{
public:
    inline TGztArticle()
        : NGzt::TArticlePtr()
    {
    }

    TGztArticle(ui32 offset, const NGzt::TArticlePool& articles)
        : NGzt::TArticlePtr(offset, articles)
    {
    }

    TGztArticle(ui32 offset, const TGztArticle& article)
        : NGzt::TArticlePtr(offset, article)
    {
    }

    TGztArticle(const NGzt::TArticlePtr& article)
        : NGzt::TArticlePtr(article)
    {
    }

    inline bool Empty() const {
        return IsNull();
    }

    static const NGzt::TDescriptor* AuxDicArticleDescriptor();
    bool IsAuxDicArticle() const;

    size_t GetMainWord() const;

    const NGzt::TMessage* GetLemmaInfo() const;
    Wtroka GetLemmaText(const NGzt::TMessage& lemma_info) const;
    size_t GetLemmaMainWord(const NGzt::TMessage& lemma_info) const;
    bool GetLemmaAlways(const NGzt::TMessage& lemma_info) const;
    bool GetLemmaIndeclinable(const NGzt::TMessage& lemma_info) const;
    TGramBitSet GetLemmaOutputGrammems(const NGzt::TMessage& lemma_info) const;
    bool GetLightKW() const;

    bool FillSArticleLemmaInfo(SArticleLemmaInfo& lemma_info, Wtroka& lemma_text) const;

private:

    // check that specified @field is not repeated and has specified @type
    static inline void AssertField(const NGzt::TFieldDescriptor* field, NGzt::TFieldDescriptor::CppType type)
    {
        YASSERT(CheckField(field, type));
    }

    static inline bool CheckField(const NGzt::TFieldDescriptor* field, NGzt::TFieldDescriptor::CppType type)
    {
        return (field != NULL &&
                field->is_repeated() == false &&
                field->cpp_type() == type);
    }

    template <typename T>
    static inline T GetGenericField(const NGzt::TMessage& object, int field_number) {
        NGzt::TProtoFieldIterator<T> iter(object, field_number);
        YASSERT(iter.Ok());
        return *iter;
    }

    static inline const NGzt::TMessage* GetMessageField(const NGzt::TMessage& object, int field_number) {
        NGzt::TProtoFieldIterator<const NGzt::TMessage*> iter(object, field_number);
        // for optional message fields an default constructed message is returned, but we return NULL in this case.
        return iter.Ok() && !iter.Default() ? *iter : NULL;
    }
};

// Contains either aux-dic article index or gzt-article (not both)
// Should be replaced with TGztArticle when SDictIndex is removed from code.
class TArticleRef
{
public:
    inline explicit TArticleRef(SDictIndex index)
        : AuxDicIndex(index)
    {
        YASSERT(index.IsValid());
    }

    inline TArticleRef(const TGztArticle& gztArticle)
        : GztArticle(gztArticle)
    {
        YASSERT(!GztArticle.Empty());
    }

    inline SDictIndex AuxDic() const
    {
        return AuxDicIndex;
    }

    inline const TGztArticle& Gzt() const
    {
        return GztArticle;
    }

private:
    SDictIndex AuxDicIndex;
    TGztArticle GztArticle;
};

class TBuiltinKWTypes
{
public:
    TKeyWordType Number, Geo, Date, CompanyAbbrev, CompanyInQuotes,
                 Fio, FioChain, FioWithoutSurname,
                 FdoChain,
                 TlVariant, DateChain, CompanyChain, StockCompany,
                 Parenthesis, CommunicVerb,
                 GeoAdm, GeoCity, GeoHauptstadt, GeoSmallCity, GeoVill,
                 AntiStatus, LightStatus;

    TBuiltinKWTypes(const NGzt::TProtoPool& descriptors);

    inline bool IsGeo(TKeyWordType type) const
    {
        YASSERT(Descriptors != NULL);
        return type != NULL && Descriptors->IsSubType(type.GetDescriptor(), Geo.GetDescriptor());
    }

    inline bool IsGeoUpperCase(TKeyWordType type) const
    {
        // TODO: remove this method with corresponding proto-option
        return IsGeo(type) && type != GeoAdj && type != National;
    }

    inline bool IsGeoCity(TKeyWordType type) const
    {
        YASSERT(Descriptors != NULL);
        return type == GeoCity || type == GeoHauptstadt || type == GeoSmallCity;
    }

    inline bool IsGeoVill(TKeyWordType type) const
    {
        YASSERT(Descriptors != NULL);
        return type == GeoVill || type == GeoSmallCity;
    }

private:
    void Init(TKeyWordType& kwtype, const Stroka& kwtype_name);
    void VerifyFixedNames() const;

    const NGzt::TProtoPool* Descriptors;
    ymap<Stroka, TKeyWordType> NameIndex;

    // optional kwtypes (could be NULLs)
    TKeyWordType GeoAdj, National;
};

