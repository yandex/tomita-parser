#pragma once

#include <util/generic/stroka.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/system/mutex.h>
#include <util/generic/strbuf.h>

#include <library/token/enumbitset.h>
#include <contrib/libs/protobuf/descriptor.h>


#include "common_constants.h"
#include "serializers.h"

enum eTerminals {T_FILE, T_NOUN, T_ADJ, T_ADJPARTORDNUM, T_ORDNUM, T_UNPOS, T_PREP, T_CONJAND,
    T_QUOTEDBL, T_QUOTESNG, T_WORD, T_BRACKET, T_FIO, T_COMMA,
    T_HYPHEN, T_PUNCT, T_COLON, T_ANY, T_0, T_GEO, T_QOUTEDWORDS,
    T_EOSENT, T_ARTIFICIAL_EOS, T_ANYWORD, T_ADV, T_PARTICIPLE, T_VERB, T_INFINITIVE, T_DATE,
    T_PERCENT, T_DOLLAR, T_LBRACKET, T_RBRACKET, T_NUMSIGN, T_AMPERSAND, T_PLUSSIGN, TermCount};

//declare as simple type for auto-serialization
DECLARE_PODTYPE(eTerminals)

const Stroka TerminalValues[TermCount] = {
    "FILE",
    "Noun",
    "Adj",
    "AdjPartOrdNum",
    "OrdinalNumeral",
    "UnknownPOS",
    "Prep",
    "SimConjAnd",
    "QuoteDbl",
    "QuoteSng",
    "Word",
    "Bracket",
    "Fio",
    "Comma",
    "Hyphen",
    "Punct",
    "Colon",
    "?",
    "0",
    "Geo",
    "QuotedWords",
    "EOSent",
    "$",
    "AnyWord",
    "Adv",
    "Participle",
    "Verb",
    "Infinitive",
    "YYDate",
    "Percent",
    "Dollar",
    "LBracket",
    "RBracket",
    "NumSign",
    "Ampersand",
    "PlusSign"
};

// Abstract interface for getting terminal IDs by text name.
class ITerminalDictionary {
public:
    virtual eTerminals FindTerminal(const Stroka& name) const = 0;
    virtual bool IsTerminal(const Stroka& name) const = 0;
    virtual ~ITerminalDictionary() {}
};

template <class TDeriv>
class TSingleton {
public:
    static TDeriv* Singleton() {
        return ::Singleton<TDeriv>();
    }
};

// uses NSpike::GetTerminalByName(), depends on linkage (old behaviour)
class TDefaultTerminalDictionary: public ITerminalDictionary, public TSingleton<TDefaultTerminalDictionary> {
public:
    virtual eTerminals FindTerminal(const Stroka& name) const;
    virtual bool IsTerminal(const Stroka& name) const;
};

// Always uses TerminalValues from current header
class TFactexTerminalDictionary: public ITerminalDictionary, public TSingleton<TFactexTerminalDictionary> {
public:
    TFactexTerminalDictionary();
    virtual eTerminals FindTerminal(const Stroka& name) const;
    virtual bool IsTerminal(const Stroka& name) const;
private:
    typedef yhash<Stroka, eTerminals> TIndex;
    TIndex Index;
};



  typedef enum
  {
    FioField,
    TextField,
    DateField,
    BoolField,
    FactFieldTypesCount
  } EFactFieldType;

  //declare as simple type for auto-serialization
  DECLARE_PODTYPE(EFactFieldType)

  const char FactFieldTypeStr[FactFieldTypesCount][iMaxWordLen] =
  {
    "fio",
    "text",
    "date",
    "bool"
  };

  enum EFactAlgorithm
  {
    eShName,
    eUnStatus,
    eDlPronoun,
    eQuotedOrgNorm,
    eNotNorm,
    ePronominalAnaphora,
    eAbbrevAnaphora,
    eCompDescAnaphora,
    ePronCompAnaphora,
    ePossCompAnaphora,
    eNormalizeWithGrammems,
    eOnlyFromFact,

    eFactAlgCount
  };

  typedef  TEnumBitSet<EFactAlgorithm, eShName, eFactAlgCount> TFactAlgorithmBitSet;

  const char FactAlg [eFactAlgCount][iMaxWordLen] =
  {
    "sh_name",
    "un_status",
    "dl_pronoun",
    "quoted_org_norm",
    "not_norm",
    "pronominal_anaphora",
    "abbrev_anaphora",
    "comp_desc_anaphora",
    "pron_comp_anaphora",
    "poss_comp_anaphora",
    "normalize_with_grammems",
    "only_from_fact"
  };

inline TStringBuf GetTomitaPrefix() {
    return STRINGBUF("tomita:");
}

inline TStringBuf GetAlgPrefix() {
    return STRINGBUF("alg:");
}

inline TStringBuf GetTaggerPrefix() {
    return STRINGBUF("tagger:");
}

bool SkipPrefix(TStringBuf& keytext, const TStringBuf& prefix);
bool SkipKnownPrefix(TStringBuf& keytext);
bool SkipTomitaPrefix(TStringBuf& keytext);
bool SkipAlgPrefix(TStringBuf& keytext);


inline bool IsNoneKWType(const Stroka& name)
{
    static const Stroka NONE = "none";
    return name == NONE;
}

// Fixed kw-types names (known in compile-time). Used as singleton.
// To get corresponding kwtypes (i.e. proto-descriptors), use NGzt::ProtoPool::RequireMessageType()
// or CDictsHolder::FixedKWTypes() (the last should be more handy and efficient).
class TFixedKWTypeNames
{
public:
    Stroka Number, Geo, Date, CompanyAbbrev, CompanyInQuotes,
           Fio, FioWithoutSurname, FioChain, FdoChain,
           TlVariant, DateChain, CompanyChain, StockCompany,
           Parenthesis, CommunicVerb,
           GeoAdm, GeoCity, GeoHauptstadt, GeoSmallCity, GeoVill,
           AntiStatus, LightStatus;

    TFixedKWTypeNames();

    inline const yvector<Stroka>& GetList() const
    {
        return List;
    }

private:
    void Add(Stroka& field, const char* name);

    yvector<Stroka> List;
};

inline const TFixedKWTypeNames& GetFixedKWTypeNames()
{
    return *Singleton<TFixedKWTypeNames>();
}

// A table of used kw-types (pointers to gzt-article descriptors)
// Used for serialization/deserialization compatible with old classes (i.e. ones designed to use EKWType)
// If used for serialization of kw-types then it should be serialized itself (to make correct de-serialization possible)
// Used as singleton in TKeyWordType::Save/Load and GetKWTypePool()
class TKWTypePool
{
public:
    typedef google::protobuf::Descriptor TDescriptor;

    TKWTypePool() {
        Reset();
    }

    template <typename TProtoPool>
    void RegisterPool(const TProtoPool& pool);

    // NGzt::TPritoPool-like methods
    const TDescriptor* FindMessageTypeByName(const Stroka& name) const;
    const TDescriptor* RequireMessageType(const Stroka& name) const;

    // save/load specified kw-types
    void SaveKWType(TOutputStream* buffer, const TDescriptor* type) const;
    const google::protobuf::Descriptor* LoadKWType(TInputStream* buffer) const;

    // save/load table of kwtype names
    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

private:
    void Reset() {
        mRegisteredTypes.clear();
        mSaveIndex.clear();
        mLoadIndex.clear();

        // NULL kw-type has index 0 (by default)
        static const TDescriptor* null_descriptor = NULL;
        mSaveIndex[null_descriptor] = 0;
        mLoadIndex.push_back(null_descriptor);
    }

    yhash<Stroka, const TDescriptor*> mRegisteredTypes;

    // Used for serializing kwtypes (it is always has same set of descriptors as mRegisteredTypes)
    yhash<const TDescriptor*, ui32> mSaveIndex;

    // Used for de-serializing kwtypes (it is always a subset of mRegisteredTypes)
    // It could be either descriptor table from last loaded grammar file (in this case an number and order of descriptors here
    // is different from mSaveIndex), or it could be same as mSaveIndex (after registering pool).
    yvector<const TDescriptor*> mLoadIndex;

};

inline TKWTypePool& GetMutableKWTypePool()
{
    return *Singleton<TKWTypePool>();
}

inline const TKWTypePool& GetKWTypePool()
{
    return GetMutableKWTypePool();
}

// substitution for EKWType
class TKeyWordType
{
public:
    inline TKeyWordType()
        : ArticleDescriptor(NULL)
    {
    }

    // intentionally non-explicit!
    inline TKeyWordType(const google::protobuf::Descriptor* descriptor)
        : ArticleDescriptor(descriptor)
    {
    }

    inline const google::protobuf::Descriptor& operator*() const
    {
        return *ArticleDescriptor;
    }

    inline const google::protobuf::Descriptor* operator->() const
    {
        return ArticleDescriptor;
    }

    inline const google::protobuf::Descriptor* GetDescriptor() const
    {
        return ArticleDescriptor;
    }

    friend inline bool operator==(const TKeyWordType& a, const TKeyWordType& b)
    {
        return a.ArticleDescriptor == b.ArticleDescriptor;
    }

    friend inline bool operator!=(const TKeyWordType& a, const TKeyWordType& b)
    {
        return a.ArticleDescriptor != b.ArticleDescriptor;
    }

    inline bool operator<(const TKeyWordType& x) const
    {
        // we cannot use direct pointer address comparison
        // as it is unstable from run to run
        //return ArticleDescriptor < x.ArticleDescriptor;

        if (ArticleDescriptor == x.ArticleDescriptor)
            return false;

        if (ArticleDescriptor == NULL)      // NULL < not NULL
            return true;
        if (x.ArticleDescriptor == NULL)
            return false;

        // most oftenly descriptors will be from same file (at global scope)
        // so comparing indices will be sufficient
        if (ArticleDescriptor->index() < x.ArticleDescriptor->index())
            return true;
        if (ArticleDescriptor->index() > x.ArticleDescriptor->index())
            return false;

        // otherwise compare by full type names (slow)
        return ArticleDescriptor->full_name() < x.ArticleDescriptor->full_name();
    }

    inline void Save(TOutputStream* buffer) const
    {
        Singleton<TKWTypePool>()->SaveKWType(buffer, ArticleDescriptor);
    }

    inline void Load(TInputStream* buffer)
    {
        ArticleDescriptor = Singleton<TKWTypePool>()->LoadKWType(buffer);
    }

private:
    friend struct hash<TKeyWordType>;
    const google::protobuf::Descriptor* ArticleDescriptor;

};

//to enable hashing of TKeyWordType
template <>
struct hash<TKeyWordType>
{
    inline size_t operator()(const TKeyWordType& s) const
    {
        // we cannot use direct pointer address as hash-value
        // as it is unstable from run to run
        //return reinterpret_cast<size_t>(s.ArticleDescriptor);

        const size_t NULL_HASH = 229634309;
        return s.ArticleDescriptor != NULL ? s.ArticleDescriptor->full_name().hash() : NULL_HASH;
    }
};

// Special descriptor pool - creates fake protobuf descriptors on request
// Singleton (via GetFakeProtoPool).
class TFakeProtoPool
{
public:
    const google::protobuf::Descriptor* FindMessageTypeByName(const Stroka& name);
private:
    google::protobuf::DescriptorPool Pool;
    TMutex Mutex;
};

inline TFakeProtoPool* GetFakeProtoPool()
{
    return Singleton<TFakeProtoPool>();
}

// Same as SArtPointer but without resolved kw-type (only with its title)
// These objects are created on parser-parameters xml parsing when gazetteer proto-pool is not yet loaded.
struct TUnresolvedArtPointer
{
    Wtroka ArticleTitle;        // could be both specified, in this case try resolve by title first, then by kwtype.
    Stroka KWTypeTitle;
    bool ForLink;
    bool ForLinkFindOnly;
    bool ForMainPage;
    bool ForUrl;

    inline TUnresolvedArtPointer()
        : ForLink(false)
        , ForLinkFindOnly(false)
        , ForMainPage(false)
        , ForUrl(false)
    {
    }

    static inline TUnresolvedArtPointer AsKWType(const Stroka& kwtype) {
        TUnresolvedArtPointer art;
        art.KWTypeTitle = kwtype;
        return art;
    }

    static inline TUnresolvedArtPointer AsTitle(const Wtroka& title) {
        TUnresolvedArtPointer art;
        art.ArticleTitle = title;
        return art;
    }

    static inline TUnresolvedArtPointer Any(const Wtroka& name) {
        TUnresolvedArtPointer art;
        art.ArticleTitle = name;
        // kwtypes can be only ascii identifiers
        if (IsStringASCII(name.begin(), name.end()))
            art.KWTypeTitle = WideToASCII(name);
        return art;
    }

    inline bool operator < (const TUnresolvedArtPointer& p) const
    {
        if (KWTypeTitle != p.KWTypeTitle)
            return KWTypeTitle < p.KWTypeTitle;
        return ArticleTitle < p.ArticleTitle;
    }

    inline bool operator == (const TUnresolvedArtPointer& p) const
    {
        return (!KWTypeTitle.empty() && KWTypeTitle == p.KWTypeTitle) || ArticleTitle == p.ArticleTitle;
    }
};

struct SArtPointer
{
    SArtPointer()
        : m_bForLink(false)
        , m_bForLinkFindOnly(false)
        , m_bForMainPage(false)
        , m_bForUrl(false)
    {
    }

    explicit SArtPointer(TKeyWordType t)
        : m_KWType(t)
        , m_bForLink(false)
        , m_bForLinkFindOnly(false)
        , m_bForMainPage(false)
        , m_bForUrl(false)
    {
    }

    explicit SArtPointer(const Wtroka& s)
        : m_strArt(s)
        , m_bForLink(false)
        , m_bForLinkFindOnly(false)
        , m_bForMainPage(false)
        , m_bForUrl(false)
    {
    }

    SArtPointer(TKeyWordType t, const Wtroka& s)
        : m_strArt(s)
        , m_KWType(t)
        , m_bForLink(false)
        , m_bForLinkFindOnly(false)
        , m_bForMainPage(false)
        , m_bForUrl(false)
    {
    }


    // turning TUnresolvedArtPointer into resolved SArtPointer
    template <typename TProtoPool>
    SArtPointer(TUnresolvedArtPointer unresolved_art, const TProtoPool& descriptors);

    void Reset()
    {
        m_KWType = NULL;
        m_bForLink = false;
        m_bForMainPage = false;
        m_bForLinkFindOnly = false;
        m_bForUrl = false;
        m_strArt.clear();
    }

    bool HasKWType() const;
    bool HasStrType() const;

    TKeyWordType GetKWType() const;
    const Wtroka& GetStrType() const;
    void PutKWType(TKeyWordType kw);
    void PutStrType(const Wtroka& s);
    void PutType(TKeyWordType kw, const Wtroka& s);
    bool IsValid() const;
    void Clear();

    void PutIsForUrl(bool b)
    {
        m_bForUrl = b;
    }

    bool IsForUrl() const
    {
        return m_bForUrl;
    }

    void PutIsForLink(bool b)
    {
        m_bForLink = b;
    }

       void PutIsForLinkFindOnly(bool b)
    {
        m_bForLinkFindOnly = b;
        if (b)
            m_bForLink = b;
    }

    bool IsForLink() const
    {
        return m_bForLink;
    }

    bool IsForLinkFindOnly() const
    {
        return m_bForLinkFindOnly;
    }

    void PutIsForMainPage(bool b)
    {
        m_bForMainPage = b;
    }

    bool IsForMainPage() const
    {
        return m_bForMainPage;
    }

    bool operator<(const SArtPointer& p) const;
    bool operator==(const SArtPointer& _X) const;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

protected:
    Wtroka m_strArt;
    TKeyWordType m_KWType;
    bool m_bForLink;
    bool m_bForLinkFindOnly;
    bool m_bForMainPage;
    bool m_bForUrl;
};

EFactAlgorithm Str2FactAlgorithm(const char* str);
const char* Str2FactAlgorithm(EFactAlgorithm alg);

template <typename TProtoPool>
void TKWTypePool::RegisterPool(const TProtoPool& pool) {
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    Reset();

    for (size_t i = 0; i < pool.Size(); ++i) {
        const TDescriptor* descriptor = pool[i];
        if (!mRegisteredTypes.has(descriptor->full_name())) {
            mRegisteredTypes[descriptor->full_name()] = descriptor;
            if (mSaveIndex.find(descriptor) == mSaveIndex.end())
                mSaveIndex.insert(TPair<const TDescriptor*, ui32>(descriptor, mSaveIndex.size()));
            mLoadIndex.push_back(descriptor);
        }
    }
}

template <typename TProtoPool>
SArtPointer::SArtPointer(TUnresolvedArtPointer art, const TProtoPool& descriptors)
    : m_strArt(art.ArticleTitle)
    , m_KWType(NULL)
    , m_bForLink(art.ForLink)
    , m_bForLinkFindOnly(art.ForLinkFindOnly)
    , m_bForMainPage(art.ForMainPage)
    , m_bForUrl(art.ForUrl)
{
    if (!art.KWTypeTitle.empty())
        m_KWType = descriptors.RequireMessageType(art.KWTypeTitle);
}

