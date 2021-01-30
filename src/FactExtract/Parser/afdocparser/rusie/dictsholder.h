#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/ptr.h>
#include <util/generic/cast.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>

#include <kernel/gazetteer/gazetteer.h>

#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/common/facttypeholder.h>
#include <FactExtract/Parser/afdocparser/rus/morph.h>
#include <FactExtract/Parser/afglrparserlib/simplegrammar.h>
#include <FactExtract/Parser/auxdictlib/articles_parser.h>
#include <FactExtract/Parser/auxdictlib/dictindex.h>


class CHomonym;
class CWord;
class CWordVector;

class TBuiltinKWTypes;
class TGztArticle;

#ifndef TOMITA_EXTERNAL
class TTagger;
#endif


class CDictsHolder: public CGramInfo
{
private:
     // can only be created as singleton
    DECLARE_SINGLETON_FRIEND(CDictsHolder)

public:
  CDictsHolder();

  virtual void Init(const Stroka& strDicPath, const Stroka& strBinPath = "");
  void InitFragmDics();
  void InitGazetteerTitleIndex();

  virtual void InitNames(const Stroka& strNameFile, ECharset encoding);
  virtual void InitNamesFromProtobuf();
  virtual TSurnamePredictor* LoadSurnamePrediction() const;
  virtual yset<Wtroka> LoadRusExtension() const;
  virtual TMapAbbrev LoadRusAbbrev() const;

  virtual ~CDictsHolder();

  inline const TGazetteer* Gazetteer() const {
      return m_Gazetteer.Get();
  }

  inline const TBuiltinKWTypes& BuiltinKWTypes() const {
      const TBuiltinKWTypes* ptr = m_BuiltinKWTypes.Get();
      YASSERT(ptr != NULL);
      return *ptr;
  }

  TKeyWordType FindKWType(const Stroka& kwtypeName) const {
      return m_Gazetteer->ProtoPool().FindMessageTypeByName(kwtypeName);
  }

  static bool IsTitleMatch(const TWtringBuf& artTitle, const TWtringBuf& searchTitle, bool exactMatch = false);
  static bool IsArticleMatch(const TGztArticle& art, const SArtPointer& searchArt, bool exactMatch = false);
  static bool IsArticleMatch(const article_t& art, const SArtPointer& searchArt, bool exactMatch = false);

  bool GetGztArticleByTitle(const Wtroka& title, TGztArticle& article) const;
  bool GetGztArticleByTitlePrefix(const Wtroka& titlePrefix, TGztArticle& article) const;

  //for CHomonym

  // returns true if @h has assigned article which art-pointer is contained in @artPointers
  bool HasArtPointer(const CHomonym& h, EDicType dicType, const yset<SArtPointer>& artPointers) const;

  // match article by kwtype, or name or name prefix (for article folders)
  bool MatchArticlePtr(const CHomonym& h, EDicType dic, const SArtPointer& art, bool exactMatch = false) const;
  // Return first gzt-article, matched by MatchArticlePtr method
  bool GetMatchingArticle(const CHomonym& h, const SArtPointer& art, TGztArticle& article, bool exactMatch = false) const;

  const article_t* GetAuxArticle(const CHomonym& h, EDicType dicType) const;
  virtual Stroka GetStrKWType(const CHomonym& h, ECharset encoding)const;

  TKeyWordType GetKWType(const CHomonym& h, EDicType dic) const;
  Wtroka GetArticleTitle(const CHomonym& h, EDicType dic) const;

  virtual or_valencie_list_t GetSubjValVars(const CHomonym& h) const;

  bool IsSimConjWithCorr(const CHomonym& h) const;
  Wtroka GetUnquotedDictionaryLemma(const CHomonym& h, EDicType dic) const;

  virtual bool HasArticle(const CHomonym& h, const SArtPointer& artP, EDicType) const;

  inline bool HasKWType(const CHomonym& h, TKeyWordType kwType) const {
      return HasArticle(h, SArtPointer(kwType), KW_DICT);
  }

  //for CWord
  virtual bool  HasArticle(const CWord& w, yset<TKeyWordType>&  kw_types) const;

  virtual bool  HasArticle(const CWord& w, const SArtPointer& artP) const;
  virtual int   HasArticle_i(const CWord& w, const SArtPointer& artP) const;

  virtual TKeyWordType GetAuxKWType(const SDictIndex& di) const;
  virtual Wtroka GetAuxArticleTitle(const SDictIndex& di) const;

  const article_t* GetAuxArticle (const SDictIndex& DictIndex) const;
  const article_t* GetAuxArticle(const Wtroka& strTitle, EDicType dicType) const;
  int GetAuxArticleIndex(const Wtroka& strTitle, EDicType dicType) const;
  const article_t* GetEmptyAuxArticle() const;

  const CIndexedDictionary& GetDict(EDicType DicType) const;

  virtual bool  HasSimConjNotWithCommaObligatory(const CWord& w) const;

  const CWorkGrammar& GetGrammarOrRegister(const Stroka& path) const;

#ifndef TOMITA_EXTERNAL
  const TTagger& GetTaggerOrRegister(const Stroka& path, const TGztArticle& taggerArticle) const;
#endif

  inline CContentsCluster* FindCluster(const Wtroka& word, EDicType DicType, ECompType CompType) const {
      YASSERT(DicType < DICTYPE_COUNT);
      return m_Dicts[DicType].m_Contents.FindCluster(word, CompType);
  }

  long FindTemplateArticle(const CHomonym& homonym) const;      //grammems here should include both POS and flex grammems

  const CFilterStore& GetFilter(TKeyWordType type) const;

  size_t GetLemmaCorpusFreq(Stroka Lemma) const;

  const TFactTypeHolder& FactTypes() const {
      return *m_FactTypes;
  }

  const fact_type_t& RequireFactType(const TStringBuf& factTypeName) const {
      const fact_type_t* ft = m_FactTypes->Find(factTypeName);
      if (EXPECT_FALSE(ft == NULL))
          ythrow yexception() << "Unknown fact type \"" << factTypeName << "\" is requested.";
      return *ft;
  }

  const NGzt::TArticlePool::TTitleMap* GetGztArticleIndex() const;

public: // data
  CWorkGrammar  m_SynAnGrammar;

private:
  bool InitSynAnGrammar(const Stroka& dicsPath);
  void InitLemmaFreq();

  void InitAuxKwDict();
  void InitGazetteer();
  void InitGazetteerTitleIndexImpl();
  void InitFactTypes();
  void InitRoot(const Stroka& strDicPath, const Stroka& strBinPath = "");

  void InitAuxDic(const Stroka& strDicName, const Stroka& text, SDefaultFieldValues& DefaultFieldValues, EDicType dic, bool bInitGrammemsTemplate = false);
  bool ReadDict(dictionary_t& dict, SDefaultFieldValues& DefaultFieldValues, const Stroka& path, const Stroka &text = "");

  const SArticleLemmaInfo& GetLemmaInfo(const CHomonym& h, EDicType dic, Wtroka& lemma_text, SArticleLemmaInfo& lemma_info_scratch) const;

  //для всех грамматик для всех терминалов записанных как леммы делаем искусственные статьи
  void MakeArtificialArticlesForLemmasFromAllGrammars();
  void MakeArtificialArticlesForLemmasFromGrammar(CWorkGrammar& grammar);

  void AddArtIDToKWTypes(CWorkGrammar& grammar);

  const CWorkGrammar* GetGrammarOrRegisterInt(const Stroka& path) const;

private:
    Stroka DicPath;
    Stroka DicFdoPath;
    Stroka BinPath;
    Stroka RootFile;
    bool PathCompatibilityMode; // "true" means looking for predefined "fdo" folder

  CIndexedDictionary m_Dicts[DICTYPE_COUNT];

  TRWMutex GrammarLock, TaggerLock;
  typedef ymap<Stroka, CWorkGrammar*> TGrammarIndex;
  mutable TGrammarIndex RegisteredGrammars;   // access to this member is protected with GrammarLock

#ifndef TOMITA_EXTERNAL
  typedef ymap<Stroka, TTagger*> TTaggerIndex;
  mutable TTaggerIndex RegisteredTaggers;   // access to this member is protected with TaggerLock
#endif

  const CWorkGrammar EmptyGrammar;

  THolder<NGzt::TArticlePool::TTitleMap> GztArticleIndex;
  ymap<TKeyWordType,CFilterStore> m_Filters; //фильтры для ключевых слов для ситуаций

  bool m_bFramDictsLoaded;
  ymap<Stroka, size_t> m_LemmaFreq;

  THolder<TGazetteer> m_Gazetteer;
  yhash<TKeyWordType, yset<TGztArticle> > m_CustomKWTypes;

  THolder<TBuiltinKWTypes> m_BuiltinKWTypes;
  THolder<TFactTypeHolder> m_FactTypes;

  DECLARE_NOCOPY(CDictsHolder)
};

inline CDictsHolder* GetMutableGlobalDictsHolder() {
    return CheckedCast<CDictsHolder*>(TMorph::GlobalMutableGramInfo());
}

inline const CDictsHolder* GetGlobalDictsHolder() {
    return CheckedCast<const CDictsHolder*>(GetGlobalGramInfo());
}

#define GlobalDictsHolder (GetGlobalDictsHolder())


