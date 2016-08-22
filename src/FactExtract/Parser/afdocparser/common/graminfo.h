#pragma once

#include <fstream>
#include <FactExtract/Parser/common/wordhomonymnum.h>
#include <FactExtract/Parser/common/toma_constants.h>
#include <FactExtract/Parser/common/gramstr.h>

#include <FactExtract/Parser/afdocparser/common/langdata.h>

#include <util/system/guard.h>
#include <util/generic/stroka.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

enum EBastardMode { No = 0, OutOfDict = 1, Always = 2 };

const char g_strNameDic[100] = "name_thes.txt";
const char g_strFragRes[100] = "fr_res.xml";
const char g_strSynRes[100] = "syn_res.xml";

enum ENameType { FirstName = 0, Surname, Patronomyc,  InitialName, InitialPatronomyc, NameTypeCount, NoName };
enum EFIOType {FIOname = 0, FIname, Fname, IOname, Iname,  FIinName, FIinOinName, FIOinName, FIOTypeCount, FInameIn, IFnameIn, IOnameIn, IOnameInIn, InameIn};

enum EWordSequenceType {CompanyAbbrevWS = 0, SubCompanyDescrWS, CompanyDescrWS, CompanyWS, CompanyShortNameWS, PostWS, SubPostWS,  FioWS, DateTimeWS, NumberWS, GeoWS, StatusWS, KeyWord, FpcWS, WSCount, NoneWS };

const char g_strPatronomycStub[100]="#Patronomyc#";
const char g_strNameStub[100]="#Name#";
const char g_strInitialPatronomycStub[100]="#InitialPatronomyc#";
const char g_strInitialNameStub[100]="#InitialName#";
const char g_strSurnameMark[100]="#SurnameMark#";
extern const Wtroka g_strFIONonTerminal;                // "@фио"
extern const Wtroka g_strDateNonTerminal;               // "@дата"
const char g_strNumberNonTerminal[100]="@число";

extern const Wtroka g_strFioFormPrefix;                 // "$$$ff"
extern const Wtroka g_strFioLemmaPrefix;                // "$$$fl"

extern const Wtroka g_strPatronomycStubShort;           // "#p#"
extern const Wtroka g_strNameStubShort;                 // "#n#"
extern const Wtroka g_strInitialPatronomycStubShort;    // "#ip#"
extern const Wtroka g_strInitialNameStubShort;          // "#in#"

const char g_strFIONonTerminalShort[100]="@фио";
const char g_strDateNonTerminalShort[100]="@дата";
const char g_strNumberNonTerminalShort[100]="@число";

const char g_strConjDic[100]="aux_dic_conj.txt";
const char g_strPredicDic[100]="aux_dic_pred.txt";
const char g_strTemplateDic[100]="aux_dic_template.txt";
const char g_strPrepDic[100]="aux_dic_prep.txt";
const char g_strKWDicSrc[100]="aux_dic_kw.cxx";
const char g_strKWDicBin[100]="aux_dic_kw.bin";

const char g_strArticlesGzt[] = "articles.gzt";
const char g_strArticlesGztBin[] = "articles.gzt.bin";

struct SName
{
  SName(bool bMainVar = true)
      : m_bMainVar(bMainVar)
  {
  }

  yvector<Wtroka> m_NameVariants; //familiar variants of first name
  bool m_bMainVar;
};

enum EDicType {CONJ_DICT = 0, PREDIC_DICT, TEMPLATE_DICT, PREP_DICT, KW_DICT, DICTYPE_COUNT };

class SDictIndex
{
friend class CGramInfo;

public:
  SDictIndex()
      : m_DicType(DICTYPE_COUNT)
      , m_iArt(-1)
  {
  }

  SDictIndex(EDicType DicType, i32 iArt)
      : m_DicType(DicType)
      , m_iArt(iArt)
  {
  }

  bool IsValid() const
  {
      return m_DicType >= 0 && m_DicType < DICTYPE_COUNT && m_iArt >= 0;
  }

  bool operator==(const SDictIndex& index) const
  {
        return m_DicType == index.m_DicType
               && m_iArt == index.m_iArt;
  }

  EDicType m_DicType;
  i32  m_iArt;

};

class CHomonym;
class CWord;
class TSurnamePredictor;

class CGramInfo {
public:
    // Abstract, CDictsHolder (derived from CGramInfo) is used
    CGramInfo() {
    }

    virtual ~CGramInfo() {
    }

    virtual void Init(const Stroka& /*strDicPath*/, const Stroka& strBinPath = "") = 0;

    virtual void InitNames(const Stroka& strNameFile, ECharset encoding);
    virtual TSurnamePredictor* LoadSurnamePrediction() const = 0;
    virtual yset<Wtroka> LoadRusExtension() const = 0;
    virtual TMapAbbrev LoadRusAbbrev() const = 0;

    /////////////////////////////////////////////////////////////////////////////////////
    //***All this methods throw exception, their real implementation is in CDictsIndex
    //***preserved for compatibility

    //virtual functions for accessing articles from CHomonym and CWord

    //for CHomonym
    virtual Stroka GetStrKWType(const CHomonym& h, ECharset encoding) const = 0;
    virtual bool IsSimConjWithCorr(const CHomonym& h) const = 0;
    virtual bool HasArticle(const CHomonym& h, const SArtPointer& artP, EDicType dic) const = 0;

    //for CWord
    virtual bool  HasArticle(const CWord& w, yset<TKeyWordType>&  kw_types) const = 0;
    virtual bool  HasArticle(const CWord& w, const SArtPointer& artP) const = 0;
    virtual int   HasArticle_i(const CWord& w, const SArtPointer& artP) const = 0;

    virtual TKeyWordType GetAuxKWType(const SDictIndex& di) const = 0;
    virtual Wtroka GetAuxArticleTitle(const SDictIndex& di) const = 0;

    virtual bool  HasSimConjNotWithCommaObligatory(const CWord& w) const = 0;

    static void ParseNamesString(const Wtroka& str, Wtroka& field, yvector<Wtroka>& names);

    const yvector<Wtroka>* GetFirstNameVariants(Wtroka strName) const;
    Wtroka GetFullFirstName(Wtroka strName) const;
    bool FindCardinalNumber(const Wtroka& strNumber, ui64& integer, int& fractional) const;
    bool FindOrdinalNumber(const Wtroka& strNumber, ui64& integer, int& fractional) const;

    Wtroka Modality2Str(EModality modal) const;
    Wtroka ClauseType2Str(EClauseType type) const;

    static bool IsNounGroup(int iTerminal);
    static bool IsVerbGroup(int iTerminal);
    static bool IsNounGroupForSubject(int iTerminal);

    static bool s_bRunFragmentationWithoutXML;
    static bool s_bLoadLemmaFreq;
    static bool s_bRunSituationSearcher;
    static bool s_bRunCompanyAndPostAnalyzer;
    static bool s_bRunResignationAppointment;
    static bool s_bForceRecompile;
    static size_t s_maxFactsCount;

    static EBastardMode s_BastardMode;

    static TOutputStream* s_PrintRulesStream;
    static TOutputStream* s_PrintGLRLogStream;
    static THolder<TOutputStream> s_PrintRulesStreamHolder;
    static THolder<TOutputStream> s_PrintGLRLogStreamHolder;
    static bool s_bDebugProtobuf;           //for debugging, off by default, switched on with param "--print-protobuf" or simply "--protobuf"

    static bool s_bNeedAuxKwDict;

    static ECharset s_DebugEncoding;       // encoding of debug output

protected:

    void InsertNames(const Wtroka& field, yvector<Wtroka>& names, Wtroka& curArticle);

protected:
    typedef ymap<Wtroka, SName, ::TLess<TWtringBuf> > TNameIndex;
    THolder<TNameIndex> m_FirstNames;

    DECLARE_NOCOPY(CGramInfo);
};
