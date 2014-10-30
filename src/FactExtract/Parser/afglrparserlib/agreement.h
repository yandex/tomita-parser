#pragma once

#include <util/generic/vector.h>
#include <util/system/defaults.h>

#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/common/fact-types.h>
#include <FactExtract/Parser/common/toma_constants.h>
#include <FactExtract/Parser/common/serializers.h>

enum ePrefix {HOM, NO_HOMS, EXCLUDE_FROM_NORM, HREG, HREG1, HREG2, DICT, LAT, ORGAN, GEO, NAME_DIC, LREG, DICT_KWTYPE,
              DICT_NOT_KWTYPE, REPLACE_INTERP, REG_EXP, NOT_REG_EXP,
              FIRST_LET_IN_SENT, IS_MULTIWORD, DICT_LABEL, DICT_NOT_LABEL, PrefixCount};

const Stroka AlphabetPrefix[PrefixCount] = {
"Hom",
"N0Homonym",
"ExcludeFromNorm",
"HReg",
"HReg1",
"HReg2",
"Dict",
"Lat",
"Org",
"Geo",
"Name",
"LReg",
"KWType",
"NotKWType",
"ReplaceInterp",
"RegExp",
"NotRegExp",
"FirstLetInSent",
"IsMultiword",
"Label",
"NotLabel"
};

enum ePostfix {NO_PREP, NO_HOMONYMS, ALL_LET, FIRST_LET, SECOND_LET, NOt, LATIN, DICT_ORG, DICT_GEO, DICT_NAME, YEs, PostfixCount};

//declare ePostfix as simple type for auto-serialization
DECLARE_PODTYPE(ePostfix)

const Stroka AttributeValues[PostfixCount] = {
"not_prep",
"no_homs",
"all",
"one",
"two",
"not",
"latin",
"org",
"geo",
"name",
"yes"
};

enum eAgrAttributes { LET, ATLET, GNC_AGR, NC_AGR, C_AGR, GN_AGR, GC_AGR, SP_AGR, COORDAGR,
                      NOT_GNC_AGR, NOT_NC_AGR, NOT_C_AGR, NOT_GN_AGR, NOT_GC_AGR, NOT_SP_AGR, QUOTED, NOT_QUOTED,
                      LQUOTED, NOT_LQUOTED, RQUOTED, NOT_RQUOTED, FEMIN_C_AGR, NOT_FEMIN_C_AGR,
                      FIO_AGR, NOT_FIO_AGR, AgrAttributesCount };

inline bool is_unary_reduce_check (const eAgrAttributes c)
{
    return            c == QUOTED
                ||    c == NOT_QUOTED
                ||  c == LQUOTED
                ||  c == RQUOTED
                ||  c == NOT_LQUOTED
                ||  c == NOT_RQUOTED;

};
inline bool is_unary_check (const eAgrAttributes c)
{
    return            c == LET
                ||    c == ATLET
                ||    is_unary_reduce_check(c);
};

const Stroka AgrAttributeValues[AgrAttributesCount] = {
"let",
"@let",
"gnc-agr",
"nc-agr",
"c-agr",
"gn-agr",
"gc-agr",
"sp-agr",
"coordagr",
"~gnc-agr",
"~nc-agr",
"~c-agr",
"~gn-agr",
"~gc-agr",
"~sp-agr",
"quoted",
"~quoted",
"l-quoted",
"~l-quoted",
"r-quoted",
"~r-quoted",
"fem-c-agr",
"~fem-c-agr",
"fio-agr",
"~fio-agr"
};

enum EInterpSymbs {
e_FIO,
e_CompanyName,
e_CompanyNameQ,
e_CompanyNameBank,
e_CompLongName,
e_CompLongNameSub,
e_GEOOrgAsCompName,
e_MinOrgName,
e_GEOOrg,
e_GEO,
e_ShortName,
e_CompanyAbbrevW,
e_CompDescP,
e_CompDescPEllipse,
e_PossPronounCompDescrP,
e_SubCompP,
e_SubCompGeo,
e_SubCompPEllipse,
e_SubCompP_LReg,
e_ClearPost,
e_PostGeoBlock,
e_PostGeoBlock2,
e_PostMinBlock,
e_PostChiefVerbAcc,
e_PostChiefVerbIns,
e_NPStatus,
e_NPStatus_Adj,
e_NPStatus_postposition,
e_DateGr,
e_PostDescr,
e_PreFioNP,
e_FdoPRes,
e_FIOChain,
e_FdoCompAbbrevEllipse,
e_FdoCompDescrEllipse,
e_FdoEmptyDescrEllipse,
e_FdoPossessiveEllipse,
e_LOrderFdoP,
e_ROrderFdoP,
InterpSymbsCount
};

const Stroka InterpSymbs[InterpSymbsCount] = {
"FIO",
"CompanyName",
"CompanyNameQ",
"CompanyNameBank",
"CompLongName",
"CompLongNameSub",
"GEOOrgAsCompName",
"MinOrgName",
"GEOOrg",
"GEO",
"ShortName",
"CompanyAbbrevW",
"CompDescP",
"CompDescPEllipse",
"PossPronounCompDescrP",
"SubCompP",
"SubCompGeo",
"SubCompPEllipse",
"SubCompP_LReg",
"ClearPost",
"PostGeoBlock",
"PostGeoBlock2",
"PostMinBlock",
"PostChiefVerbAcc",
"PostChiefVerbIns",
"NPStatus",
"NPStatus_Adj",
"NPStatus_postposition",
"DateGr",
"PostDescr",
"PreFioNP",
"FdoPRes",
"FIOChain",
"FdoCompAbbrevEllipse",
"FdoCompDescrEllipse",
"FdoEmptyDescrEllipse",
"FdoPossessiveEllipse",
"LOrderFdoP",
"ROrderFdoP"
};

//rule sample: A -> B<agr[1]> D<agr[2]> E<agr[2]> C<agr[1]>
struct SAgr {
    yvector<int> m_AgreeItems; //numbers of the agree right items
    TGramBitSet m_Grammems; //unary agreement: grammems for nonterminal
    TGramBitSet m_NegativeGrammems; //unary agreement: negative grammems for nonterminal
    ECoordFunc  e_AgrProcedure; //id of the agreement procedures in the function pointers container
    bool m_bNegativeAgreement;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    SAgr() {
        e_AgrProcedure = CoordFuncCount;
        m_Grammems.Reset();
        m_NegativeGrammems.Reset();
        m_bNegativeAgreement = false;
    };
    SAgr(int iItemNum, const TGramBitSet& iGrammems, bool bNegative) {
        m_Grammems.Reset();
        m_NegativeGrammems.Reset();
        m_bNegativeAgreement = bNegative;
        m_AgreeItems.push_back(iItemNum);
        if (!bNegative)
            m_Grammems = iGrammems;
        else
            m_NegativeGrammems = iGrammems;

        e_AgrProcedure = CoordFuncCount;
    };

    bool IsUnary() const
    {
        return    (m_AgreeItems.size() == 1) &&
                (m_Grammems.any());
    }
};

struct SKWSet
{
    SKWSet() { m_bNegative = false; m_bApplyToTheFirstWord=false;};
    bool operator==(const SKWSet& _X) const
    {
        return (m_bNegative == _X.m_bNegative && m_KWset == _X.m_KWset);
    };
    bool     m_bNegative;
    bool     m_bApplyToTheFirstWord;
    yset<SArtPointer> m_KWset;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_bNegative);
        ::Save(buffer, m_bApplyToTheFirstWord);
        ::Save(buffer, m_KWset);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_bNegative);
        ::Load(buffer, m_bApplyToTheFirstWord);
        ::Load(buffer, m_KWset);
    }

};

struct SGrammemUnion
{
    SGrammemUnion() {  };
    yvector<TGramBitSet> m_GramUnion;
    yvector<TGramBitSet> m_NegGramUnion;
    yvector< bool > m_CheckFormGrammem;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_GramUnion);
        ::Save(buffer, m_NegGramUnion);
        ::Save(buffer, m_CheckFormGrammem);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_GramUnion);
        ::Load(buffer, m_NegGramUnion);
        ::Load(buffer, m_CheckFormGrammem);
    }

    bool operator ==(const SGrammemUnion& _X) const
    {
        return      (m_CheckFormGrammem == _X.m_CheckFormGrammem)
                &&  (m_NegGramUnion == _X.m_NegGramUnion)
                &&  (m_GramUnion == _X.m_GramUnion);
    }

};

class TRegexMatcher {
public:
    TRegexMatcher(const TWtringBuf& pattern);
    ~TRegexMatcher();

    bool MatchesUtf8(const TStringBuf& text) const;
    static bool IsCompatible(const TWtringBuf& pattern);

private:
    class TScanner;
    THolder<TScanner> Impl;
};

// класс для хранения множества регулярных выражений, которые описывают входную словоформу
class CWordFormRegExp {
public:
    CWordFormRegExp()
        : Place(ApplyToMainWord)
        , IsNegative(false)
    {
    }

    const Stroka DebugString(int itemNo) const;

    bool operator == (const CWordFormRegExp& rFormRegExp) const {
        return RegexPattern == rFormRegExp.RegexPattern && IsNegative == rFormRegExp.IsNegative;
    }

    bool operator < (const CWordFormRegExp& rFormRegExp) const {
        if (IsNegative != rFormRegExp.IsNegative)
            return IsNegative < rFormRegExp.IsNegative;
        else
            return RegexPattern < rFormRegExp.RegexPattern;
    }

    bool IsEmpty() const {
        return RegexPattern.empty();
    }

    void SetRegexUtf8(const Stroka& pattern);

    bool Matches(const TWtringBuf& text) const;
    bool Accepted(const TWtringBuf& text) const {
        return Matches(text) != IsNegative;
    }

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

public:
    enum EApplyPlace {ApplyToFirstWord, ApplyToLastWord, ApplyToMainWord};
    EApplyPlace Place;

    bool IsNegative;

private:
    Stroka RegexPattern;  // utf-8
    TSharedPtr<TRegexMatcher> Matcher;
};

DECLARE_PODTYPE(CWordFormRegExp::EApplyPlace)

//игнорируя покрытие, выбирает из двух построенных графов A и B, пересечение слов которых не пусто,
//граф, левая граница которого находится ближе к началу предложения
//(т.е. нужен для эмпирического правила выбора левостороннего ветвления ФДО фактов);
//для этого в языке вводится помета при правилах:
//A -> X {left_dominant};
//B -> Y {right_recessive};
//из примера: "...президента Юкос М. Ходорковского, корреспондента аналитического журнала "Эксперт"..."
//будет выбран ФДО факт "президента Юкос М. Ходорковского"
struct SDominantRecessive {
    SDominantRecessive() {
        m_bDominant = false;
        m_bRecessive = false;
    };

    bool operator==(const SDominantRecessive& _X) const
    {
        return (m_bDominant == _X.IsDominant()
                && m_bRecessive == _X.IsRecessive()
                );
    };

    void SetDominant(bool bDom = true)
    {
        m_bDominant = bDom;
        if (m_bDominant && m_bRecessive)
            ythrow yexception() << "Error in \"SDominantRecessive::SetDominant\"";
    };

    void SetRecessive(bool bReces = true)
    {
        m_bRecessive = bReces;
        if (m_bDominant && m_bRecessive)
            ythrow yexception() << "Error in \"SDominantRecessive::SetRecessive\"";
    };

    bool IsDominant() const
    {
        return m_bDominant;
    };

    bool IsRecessive() const
    {
        return m_bRecessive;
    };

    bool HasValue() const
    {
        return (m_bRecessive || m_bDominant);
    };

protected:
    bool m_bDominant;
    bool m_bRecessive;
};

const size_t MaxOutCount = 1000;
struct SExtraRuleInfo {

    SExtraRuleInfo() {
        clear();
    };

    void clear() {
        m_OutGrammems.Reset();
        m_OutCount = MaxOutCount;
        m_OutWeight = 1;
        m_bTrim = m_bNotHRegFact = false;
    }

    bool operator==(const SExtraRuleInfo& _X) const
    {
        return (m_OutGrammems == _X.m_OutGrammems
                 && m_OutCount == _X.m_OutCount
                 && m_OutWeight == _X.m_OutWeight
                 && m_bTrim == _X.m_bTrim
                 && m_bNotHRegFact == _X.m_bNotHRegFact
                 && m_DomReces == _X.m_DomReces
                );
    };

    bool operator!=(const SExtraRuleInfo& _X) const
    {
        return (!(*this == _X));
    };

    TGramBitSet m_OutGrammems; //specified grammems assigned to a group (to a nonterminal in the left part of the rule on the reduce)
    size_t m_OutCount;
    double m_OutWeight;
    bool m_bTrim;
    bool m_bNotHRegFact;
    SDominantRecessive m_DomReces;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct TGztArticleField {
    SArtPointer Article;
    Stroka Field;

    TGztArticleField() {
    }

    TGztArticleField(const SArtPointer& article, const Stroka& field)
        : Article(article)
        , Field(field)
    {
    }

    void Save(TOutputStream* buffer) const {
        ::Save(buffer, Article);
        ::Save(buffer, Field);
    }

    void Load(TInputStream* buffer) {
        ::Load(buffer, Article);
        ::Load(buffer, Field);
    }
};

struct SRuleExternalInformationAndAgreements
{
    yvector< SAgr > m_RulesAgrs; //CompDescPEllipse ->NPAdjConj<gnc-agr[1], nc-agr[2]>*  CompanyDescrW<gnc-agr[1], rt> Word<nc-agr[2]>;
    ymap< int, SKWSet > m_KWSets; //S -> NP<kwset=~[post, company_in_quotes, _министр]>;
    ymap< int, yvector<fact_field_reference_t> > m_FactsInterpretation; //common interpretation: fact_types.cxx
    ymap< int, CWordFormRegExp> m_WFRegExps;
    ymap< int, SGrammemUnion > m_RuleGrammemUnion;
    ymap< int, bool> m_FirstWordConstraints;

    ymap<int, TGztArticleField> m_GztWeight;

    SExtraRuleInfo m_ExtraRuleInfo;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

class CExternalGrammarMacros
{
public:
    CExternalGrammarMacros() {
        m_bNoInterpretation = false;
        m_b10FiosLimit = false;
    };

    bool m_bNoInterpretation;
    bool m_b10FiosLimit;
    yset<SArtPointer> m_KWsNotInGrammar;

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_bNoInterpretation);
        ::Save(buffer, m_b10FiosLimit);
        ::Save(buffer, m_KWsNotInGrammar);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_bNoInterpretation);
        ::Load(buffer, m_b10FiosLimit);
        ::Load(buffer, m_KWsNotInGrammar);
    }

};

typedef yvector<SRuleExternalInformationAndAgreements> CRuleAgreement;

//1(base)00(min)00(hour)0(day of week)000(days)00(weeks)00(month)000(year)
#define ITSELF_INTERP LL(1000000000000000)

typedef ymap< Wtroka, TPair< int, Stroka > > SGrammarArticleTitles;

inline i64 ten_pow(i64 inum, int ipower)
{
    if (0 > ipower) return inum;

    for (int i = 1; i <= ipower; i++)
        inum *= 10;

    return inum;
};

inline int get_rank(i64 inum)
{
    int i_rank = 0;
    for (; inum > 0; i_rank++)
        inum /= 10;

    return i_rank;
}

inline i64 __abs64(i64 n)
{
    return (0 > n) ? ((-1)*n) : n;
}

inline ePostfix Str2Postfix(const Stroka& str)
{
    for (int i = 0; i < PostfixCount; i++)
        if (AttributeValues[i] == str)
            return (ePostfix)i;

    return PostfixCount;
}
