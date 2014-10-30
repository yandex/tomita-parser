#pragma once

#include <util/generic/vector.h>

#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/common/serializers.h>

#include "commontypes.h"
#include "ahokorasick.h"
#include "glr.h"
#include "grammaritem.h"


enum ParseMethodEnum {TrieParsingMethod, LR0ParsingMethod, GLRParsingMethod};

struct CFilterPair
{
    CFilterPair():  m_iItemID(-1),
                    m_iWordDistance(-1)
    {}

    int m_iItemID;
    int m_iWordDistance;

    CFilterPair(int i_id, int i_dist)
    {
        m_iItemID = i_id;
        m_iWordDistance = i_dist;
    }
    bool operator==(const CFilterPair& _X) const
    {
        return (m_iItemID == _X.m_iItemID && m_iWordDistance == _X.m_iWordDistance);
    }

    void Save(TOutputStream* buffer) const
    {
        ::Save(buffer, m_iItemID);
        ::Save(buffer, m_iWordDistance);
    }

    void Load(TInputStream* buffer)
    {
        ::Load(buffer, m_iItemID);
        ::Load(buffer, m_iWordDistance);
    }

};

typedef yset<CWorkRule>::const_iterator CWRI;
typedef yset<CWorkRule>::iterator WRI;
typedef TPair< Stroka, yvector<int> > tFileTerminalSynonyms;
typedef ymap<Stroka, tFileTerminalSynonyms > CFileTerminalSynonyms;

class CFilterStore
{
public:
    CFilterStore() {};
    ~CFilterStore() {};

    void    LoadTerminalIDsFromFilterItems(const ITerminalDictionary& terminals);
    bool    IsEmpty() const;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer, const ITerminalDictionary& terminals);
    void Load(TInputStream* buffer);    // uses TDefaultTerminalDictionary by default

public:
    yvector<CGrammarItem>   m_UniqueFilterItems; //nim : set of items belonged to grammar filter header
    yvector< yvector<CFilterPair> >     m_FilterSet; //nim : filter set
    ymap<int, eTerminals>               m_UniqueFilterTerminalIDs; //nim : терминалы использующиеся в фильтрах
};

struct CKWGrammarTypes
{
    void insert(TKeyWordType e_type) { m_ekwTypes.insert(e_type); };
    void insert(Stroka s_type) { m_skwTypes.insert(s_type); };

    yset<TKeyWordType> m_ekwTypes; //nim : key word types used in the current grammar
    yset<Stroka> m_skwTypes;  //nim : key word article titles used in the current grammar
};

//declare MorphLanguageEnum as simple type for auto-serialization
DECLARE_PODTYPE(MorphLanguageEnum)

struct CWorkGrammar
{
    MorphLanguageEnum                   m_Language;
    yvector<CGrammarItem>               m_UniqueGrammarItems;
    //nim : 17.05.04 : ключ -  номер элемента в m_UniqueGrammarItems, значение - идентификатор терминала.
    ymap<int, eTerminals>               m_UniqueGrammarTerminalIDs;
    //nim : 17.05.04 : ключ -  имя словаря с KW-словами,
    //значение - вектор номеров элементов в m_UniqueGrammarItems (вектор синонимов), которые являются синонимами одного терминала-файла (FILE[company_kw.txt]~FILE[h-reg1,company_kw.txt]~FILE[h-reg2,company_kw.txt] ).

    yset<CWorkRule>                     m_EncodedRules;

    yvector< yset<size_t> >             m_FirstSets;
    yvector< yset<size_t> >             m_FollowSets;

    //  we use here CWorkRule in order to be compatible with the functions
    //  which build Trie-Automat, though we use only CWorkRule::m_RightPart::m_Items
    //  all other slots are unused
    yset<CWorkRule>                     m_RootPrefixes;

    SymbolInformationType               m_AutomatSymbolInformation;

    CTrieHolder                         m_TrieHolder;

    CGLRTable                           m_GLRTable;

    int                                 i_EOSentNum; //nim: order number of the EOSent terminal in the m_UniqueGrammarItems vector

    //bool                              m_bHasFactInterpretation;
    CExternalGrammarMacros              m_ExternalGrammarMacros;

    yset<SArtPointer>                   m_kwArticlePointer;

    CFilterStore                        m_FilterStore;
    yvector< yvector<int> >             m_InterpSymbols;

    // source files (root grammar file and all included via #include macro)  ->  their checksums
    ymap<Stroka, Stroka>                SourceFiles;

    CWorkGrammar();
    ~CWorkGrammar();

    void SaveHeader(TOutputStream* buffer) const;
    bool LoadHeader(TInputStream* buffer);
    void VerifyHeader(TInputStream* buffer);    // load and fail if header is bad

    // main content of grammar (without header info)
    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer, const ITerminalDictionary& terminals);
    void Load(TInputStream* buffer);     // uses TDefaultTerminalDictionary by default

    // header + compressed content of grammar
    bool SaveToFile(const Stroka& file_name);
    bool LoadFromFile(const Stroka& file_name);
    bool LoadFromFile(const Stroka& file_name, const ITerminalDictionary& terminals);

    size_t  GetItemIdOrRegister(const CGrammarItem& I);
    size_t  GetRulesCount() const;
    bool    DeleteMetaSymbols();
    void    Print() const;
    Stroka  GetLeftPartStr(CWRI it) const;
    Stroka  GetRuleStr(CWRI it) const;
    Stroka  GetRuleStr(const CWorkRule& R) const;
    Stroka  GetRuleStr(const CWorkRule& R, int AsteriskNo) const;
    bool    GetPossibleTerminalStrings(size_t NonTerminalSymbolNo, ymap<size_t, yvector< CWorkRightRulePart > >& MetaSymbolToTerminalRules) const;
    size_t  GetCountOfRoots()  const;
    void    BuildAutomat(yset<CWorkRule>& EncodedRules);
    bool    IsValid() const;
    bool    SaveFirstAndFollowSets(Stroka GrammarFileName) const;
    void    DeleteUnusedSymbols();
    void    Build_FIRST_Set();
    void    Build_FOLLOW_Set();
    //  adding a new root is necessary  for building LR(0) item sets
    bool    AugmentGrammar(yvector<CRuleAgreement>& Agreements);
    size_t  GetNewRoot() const;
    size_t  GetEndOfStreamSymbol() const;
    void    Build_FIRST_Set_k(size_t PrefixLength);
    void    LoadEOFSentTerminal();
    void    LoadTerminalIDsFromGrammarItems(const ITerminalDictionary& terminals);
    void    LoadSharedKwTypesForGrammar();
    void    LoadSharedKwTypesForGrammar(const SArtPointer& rArt);
    void    CheckInterpretationFields();
    void    LoadInterpSymbForGrammar();
    bool    IsCertainInterpSymbol(EInterpSymbs eInterp, int iSymbID) const;
    void    PutGrammarID(int iGramID);
    int     GetGrammarID() const;
    bool    TransferOutCountFromSymbolToItsDependencies(yvector<CRuleAgreement>& RuleInfos);

    Stroka GetSymbolName(size_t symbol) const {
        return m_UniqueGrammarItems[symbol].m_ItemStrId;
    }
    bool IsRootSymbol(size_t symbol) {
        return m_UniqueGrammarItems[symbol].m_bGrammarRoot;
    }
    const CGrammarItem& GetItem(size_t symbol) const {
        return m_UniqueGrammarItems[symbol];
    }
    const yvector<CGrammarItem>& GetItems() const {
        return m_UniqueGrammarItems;
    }

    typedef yvector< yvector<const CWorkRule*> > TEncodedRulesIndex;
    void IndexEncodedRules(TEncodedRulesIndex& index) const;

protected:
    void    CreateAutomatSymbolInformation();

};

extern bool BuildGLRTableForGrammar(Stroka GrammarFileName, CWorkGrammar& WorkGrammar, yvector<CRuleAgreement>&);
