#include <time.h>

#include <util/stream/file.h>
#include <util/stream/lz.h>
#include <util/stream/buffered.h>

#include "simplegrammar.h"
#include <sys/stat.h>

#include <kernel/gazetteer/common/serialize.h>
#include <FactExtract/Parser/lemmerlib/lemmerlib.h>


static const Stroka NEW_ROOT = "$NewRoot";
static const Stroka END_OF_INPUT = "$";

void ConvertRuleIntoIntVector(const CWorkRule& r_Rule, yvector<int>& v_Rule)
{
    v_Rule.push_back(r_Rule.m_OriginalRuleNo);
    v_Rule.push_back(r_Rule.m_LeftPart);
    v_Rule.push_back(r_Rule.m_RightPart.m_SynMainItemNo);
    for (int i = 0; i < (int)r_Rule.m_RightPart.m_Items.size(); i++)
        v_Rule.push_back(r_Rule.m_RightPart.m_Items[i]);
}

void ConvertIntVectorIntoRule(const yvector<int>& v_Rule, CWorkRule& r_Rule)
{
    assert(v_Rule.size() > 3);

    r_Rule.m_OriginalRuleNo = v_Rule[0];
    r_Rule.m_LeftPart = v_Rule[1];
    r_Rule.m_RightPart.m_SynMainItemNo = v_Rule[2];
    for (int i = 3; i < (int)v_Rule.size(); i++)
        r_Rule.m_RightPart.m_Items.push_back(v_Rule[i]);
}

void ConvertFilterIntoIntVector(const yvector<CFilterPair>& r_Filter, yvector<int>& v_Filter)
{
    for (int i = 0; i < (int)r_Filter.size(); i++) {
        v_Filter.push_back(r_Filter[i].m_iItemID);
        v_Filter.push_back(r_Filter[i].m_iWordDistance);
    }
}

void ConvertIntVectorIntoFilter(const yvector<int>& v_Filter, yvector<CFilterPair>& r_Filter)
{
    for (int i = 1; i < (int)v_Filter.size(); i=i+2)
        r_Filter.push_back(CFilterPair(v_Filter[i-1], v_Filter[i]));
}

void SAgr::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_AgreeItems);
    ::Save(buffer, m_Grammems);
    ::Save(buffer, m_NegativeGrammems);
    ::Save(buffer, e_AgrProcedure);
    ::Save(buffer, m_bNegativeAgreement);
}

void SAgr::Load(TInputStream* buffer)
{
    ::Load(buffer, m_AgreeItems);
    ::Load(buffer, m_Grammems);
    ::Load(buffer, m_NegativeGrammems);
    ::Load(buffer, e_AgrProcedure);
    ::Load(buffer, m_bNegativeAgreement);
}

void SExtraRuleInfo::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_OutGrammems);
    ::Save(buffer, m_OutCount);
    ::Save(buffer, m_OutWeight);
    ::Save(buffer, m_bTrim);
    ::Save(buffer, m_bNotHRegFact);
    ::Save(buffer, m_DomReces.IsDominant());
    ::Save(buffer, m_DomReces.IsRecessive());
}

void SExtraRuleInfo::Load(TInputStream* buffer)
{
    ::Load(buffer, m_OutGrammems);
    ::Load(buffer, m_OutCount);
    ::Load(buffer, m_OutWeight);
    ::Load(buffer, m_bTrim);
    ::Load(buffer, m_bNotHRegFact);

    bool b;
    ::Load(buffer, b);
    m_DomReces.SetDominant(b);

    ::Load(buffer, b);
    m_DomReces.SetRecessive(b);
}

void SRuleExternalInformationAndAgreements::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_RulesAgrs);
    ::Save(buffer, m_KWSets);
    ::Save(buffer, m_FactsInterpretation);
    ::Save(buffer, m_WFRegExps);
    ::Save(buffer, m_RuleGrammemUnion);
    ::Save(buffer, m_ExtraRuleInfo);
    ::Save(buffer, m_FirstWordConstraints);
    ::Save(buffer, m_GztWeight);
}

void SRuleExternalInformationAndAgreements::Load(TInputStream* buffer)
{
    ::Load(buffer, m_RulesAgrs);
    ::Load(buffer, m_KWSets);
    ::Load(buffer, m_FactsInterpretation);
    ::Load(buffer, m_WFRegExps);
    ::Load(buffer, m_RuleGrammemUnion);
    ::Load(buffer, m_ExtraRuleInfo);
    ::Load(buffer, m_FirstWordConstraints);
    ::Load(buffer, m_GztWeight);
}

//========================================================================
//=====================   CWorkGrammar      ==============================
//========================================================================
CWorkGrammar::CWorkGrammar()
{
    m_GLRTable.m_pWorkGrammar  = this;

    i_EOSentNum = -1;
    m_ExternalGrammarMacros.m_bNoInterpretation = false;
    m_ExternalGrammarMacros.m_b10FiosLimit = false;
};
CWorkGrammar::~CWorkGrammar()
{

};

size_t      CWorkGrammar::GetItemIdOrRegister(const CGrammarItem& I)
{
    yvector<CGrammarItem>::const_iterator it = std::find(m_UniqueGrammarItems.begin(),m_UniqueGrammarItems.end(),I);

    if (it != m_UniqueGrammarItems.end())
        return it - m_UniqueGrammarItems.begin();
    else {
        m_UniqueGrammarItems.push_back(I);
        // slot m_bSynMain is no used in  WorkGrammar it is used only in class CParseGrammar
        m_UniqueGrammarItems.back().m_bSynMain = false;
        return m_UniqueGrammarItems.size() - 1;

    };
};

Stroka CWorkGrammar::GetLeftPartStr(yset<CWorkRule>::const_iterator it) const
{
    return m_UniqueGrammarItems[it->m_LeftPart].GetDumpString();
};

Stroka  CWorkGrammar::GetRuleStr(const CWorkRule& R, int AsteriskNo) const
{
    Stroka Result = m_UniqueGrammarItems[R.m_LeftPart].GetDumpString() + "->";
    size_t l=0;
    for (; l<R.m_RightPart.m_Items.size(); l++) {
        if ((size_t)AsteriskNo == l)
            Result += "*";

        Result += m_UniqueGrammarItems[R.m_RightPart.m_Items[l]].GetDumpString();

        if (l+1 !=  R.m_RightPart.m_Items.size())
            Result += ", ";
    };
    if ((size_t)AsteriskNo == l)
            Result += "*";

    return Result;
};

Stroka  CWorkGrammar::GetRuleStr(const CWorkRule& R) const
{
    return  GetRuleStr(R, R.m_RightPart.m_SynMainItemNo);
};

Stroka  CWorkGrammar::GetRuleStr(CWRI it) const
{
    return GetRuleStr(*it);

};

void CWorkGrammar::Print() const
{
    printf ("===========\n");
    int RuleNo = 0;
    for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
        RuleNo++;
        printf ("%s\n", GetRuleStr(it).c_str());
    };
};

size_t CWorkGrammar::GetCountOfRoots()  const
{
    size_t Result  = 0;
    Stroka Dump;
    for (size_t i=0; i<m_UniqueGrammarItems.size(); i++)
        if (m_UniqueGrammarItems[i].m_bGrammarRoot) {
            Result++;
            Dump += m_UniqueGrammarItems[i].GetDumpString();
            Dump += ",";
        }
    if (Result > 1)
        fprintf (stderr, "Roots : %s\n",Dump.c_str());

    return Result;
};

size_t  CWorkGrammar::GetRulesCount() const
{
    return  m_EncodedRules.size();
};

void  CWorkGrammar::CreateAutomatSymbolInformation()
{
    m_AutomatSymbolInformation.clear();

    for (TerminalSymbolType i=0; i<(int)m_UniqueGrammarItems.size(); i++)
        m_AutomatSymbolInformation[i] = m_UniqueGrammarItems[i].GetDumpString();

};

void  CWorkGrammar::BuildAutomat(yset<CWorkRule>& EncodedRules)
{
    CreateAutomatSymbolInformation();

    m_TrieHolder.Create(EncodedRules, &m_AutomatSymbolInformation);
}

bool    CWorkGrammar::IsValid() const
{
    for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
        const CWorkRightRulePart& R = it->m_RightPart;
        if (R.m_SynMainItemNo >= R.m_Items.size()) {
            Cerr << Substitute("$0 has no syntax root", GetRuleStr(it)) << Endl;
            return false;
        };
    };

    return true;
};

void CWorkGrammar::IndexEncodedRules(TEncodedRulesIndex& index) const
{
    index.clear();
    index.resize(m_UniqueGrammarItems.size());
    for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); ++it)
        index[it->m_LeftPart].push_back(&*it);
}

void  UpdateOld2NewMap(ymap<int, int>& Old2New, size_t& Index, int& MaxNewIndex)
{
    ymap<int, int>::const_iterator it =  Old2New.find(Index);
    if (it == Old2New.end()) {
        Old2New[Index] = MaxNewIndex;
        Index = MaxNewIndex;
        MaxNewIndex++;
    } else
        Index = it->second;
};

void    CWorkGrammar::DeleteUnusedSymbols()
{
    yset<CWorkRule> NewEncodedRules;
    ymap<int, int> Old2New;
    int CountOfUsedSymbols = 0;
    for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
        CWorkRule R = *it;
        UpdateOld2NewMap(Old2New, R.m_LeftPart, CountOfUsedSymbols);
        for (int l = 0; l < (int)R.m_RightPart.m_Items.size(); ++l)
            UpdateOld2NewMap(Old2New, (size_t&)R.m_RightPart.m_Items[l], CountOfUsedSymbols);

        NewEncodedRules.insert(R);
    }
    m_EncodedRules.swap(NewEncodedRules);

    yvector<CGrammarItem> NewUniqueRuleItems(CountOfUsedSymbols);
    ymap<int, int>::const_iterator it1 = Old2New.begin();
    for (; it1 !=  Old2New.end(); it1++)
        NewUniqueRuleItems[it1->second] = m_UniqueGrammarItems[it1->first];

    m_UniqueGrammarItems.swap(NewUniqueRuleItems);
};

// building m_FirstSets.
// m_FirstSets[i] is a set of all terminals which can start a terminal sequence,
// which can be produced from  nonterminal symbol  i
//==============
//  1. We first initialize the FIRST sets to the empty set
// 2. Then we process each grammar rule in the following way: if the right-hand side
// starts with a teminal  symbol. we add this symbol to the FIRST set  of the
//  left-hand side, since it  can be the firts symbol of a sentential form
// derived from the left side.  If the right-hand side starts with a non-terminal symbol
// we add all symbols of the present FIRST set of this non-terminal to the FIRST set
// of the left-hand side. These are all symbols that can be the first  terminal of
//  a sentential for derived from teh left-hand side.
//  3. The previuos  step is repeated until no more new symbols are added to any of the
// FIRST sets.
void CWorkGrammar::Build_FIRST_Set()
{
    m_FirstSets.clear();
    m_FirstSets.resize(m_UniqueGrammarItems.size());
    bool bChanged = true;
    while (bChanged) {
        bChanged = false;
        for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
            const CWorkRule& R = (*it);
            int SymbolNo = R.m_RightPart.m_Items[0];
            if (m_UniqueGrammarItems[SymbolNo].m_Type == siMeta) {
                const yset<size_t>& s = m_FirstSets[SymbolNo];
                for (yset<size_t>::const_iterator it = s.begin(); it != s.end(); it++)
                    if (m_FirstSets[R.m_LeftPart].insert(*it).second)
                        bChanged  = true;
            } else if (m_FirstSets[R.m_LeftPart].insert(SymbolNo).second)
                    bChanged  = true;
        };
    };

};

/*
    This functions builds the slot CWorkGrammar::m_RootPrefixes.
    This slot contains  all terminal sequences (length < PrefixLength)
    which can start the root symbol of the grammar. First we should compute
    First_k() function where k=PrefixLength for each non-terminal symbol, then
    the union of First_k() of all root  symbols will produce CWorkGrammar::m_RootPrefixes.
    The process of computing First_k() is described in many sources about Parsing and is similar
    to function CWorkGrammar::Build_FIRST_Set.
*/

template <class Type, size_t Size>
struct CTinyVector  {
    Type     m_Items[Size];
    int  m_ItemsCount;
    CTinyVector ()
    {
        m_ItemsCount =  0;
    }
    void push_back (Type    Item)
    {
        assert (m_ItemsCount < (int)Size);
        m_Items[m_ItemsCount++] = Item;
    };
    size_t size () const    {
        return  m_ItemsCount;
    };
    Type& operator[](int No) {  return m_Items[No]; };
    const Type& operator[](int  No) const { return m_Items[No]; };
    const Type* begin() const { return m_Items; };
    const Type* end()   const { return m_Items+m_ItemsCount;};
    bool        empty() const { return m_ItemsCount == 0; };
    bool operator <  (const CTinyVector<Type,Size>& _X) const
    {
        return std::lexicographical_compare(m_Items,m_Items+m_ItemsCount,
            _X.m_Items,_X.m_Items+_X.m_ItemsCount);

    }
    bool operator ==  (const CTinyVector<Type,Size>& _X) const
    {
        return      m_ItemsCount == _X.m_ItemsCount
                &&  !memcmp(m_Items, _X.m_Items, sizeof(Type)*m_ItemsCount);;

    }

};

void CWorkGrammar::Build_FIRST_Set_k(size_t PrefixLength)
{
    //typedef yvector<TerminalSymbolType > CPrefix;
    typedef CTinyVector<TerminalSymbolType,3> CPrefix;
    typedef yset<CPrefix> CPrefixSet;
    ymap<size_t, CPrefixSet > First_k;

    bool bChanged = true;
    size_t PrefixNumber = 0;
    size_t CycleNo = 0;

    while (bChanged) {
        CycleNo++;
        bChanged = false;
        for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
            const CWorkRule& R = (*it);
            //Stroka DumpRule = GetRuleStr(R);
            CPrefixSet ThisRuleResult;
            //  we should add an empty prefix
            ThisRuleResult.insert(CPrefix());

            for (size_t i=0; i < std::min (PrefixLength,R.m_RightPart.m_Items.size()); i++) {
                CPrefixSet Additions;
                int SymbolNo = R.m_RightPart.m_Items[i];
                if (m_UniqueGrammarItems[SymbolNo].m_Type == siMeta)
                    Additions = First_k[SymbolNo];
                else {
                    CPrefix P;
                    P.push_back(SymbolNo);
                    Additions.insert(P);
                }

                if (Additions.empty()) {
                    ThisRuleResult.clear();
                    break;
                };

                CPrefixSet NewRuleResult;

                for (CPrefixSet::const_iterator it =  Additions.begin(); it != Additions.end(); it++) {
                    const CPrefix& Addition = *it;
                    CPrefixSet Set;
                    for (CPrefixSet::const_iterator it1 =  ThisRuleResult.begin(); it1 != ThisRuleResult.end(); it1++) {
                        const CPrefix& Prefix = *it1;
                        if  (Prefix.size() < PrefixLength) {
                            CPrefix UpdatedPrefix = Prefix;

                            int AddLen = std::min(Addition.size(), PrefixLength - Prefix.size());

                            for (size_t k=0;  k < (size_t)AddLen; k++)
                                UpdatedPrefix.push_back(Addition[k]);
                            //UpdatedPrefix.insert (UpdatedPrefix.end(), Addition.begin(), Addition.begin()+AddLen);

                            assert (!UpdatedPrefix.empty());

                            // if P.size() < PrefixLength then we can add to P some new
                            //  projections since P is a finished projection
                            assert (UpdatedPrefix.size() <= PrefixLength);

                            Set.insert(UpdatedPrefix);
                        } else
                            Set.insert(Prefix);

                    };
                    NewRuleResult.insert(Set.begin(), Set.end());
                };
                ThisRuleResult.swap(NewRuleResult);

            }; // end of the cycle through the right-hand part

            //  updating Firstk() for this rule

            {
                CPrefixSet& s =  First_k[R.m_LeftPart];
                size_t SaveSize = s.size();
                s.insert (ThisRuleResult.begin(), ThisRuleResult.end());
                if (SaveSize != s.size()) {
                    bChanged = true;
                }
                PrefixNumber -= SaveSize;
                PrefixNumber += s.size();
            }

        }
    };

    //  Building CWorkGrammar::m_RootPrefixes
    size_t RuleId = 0;
    for  (ymap<size_t, CPrefixSet >::const_iterator it=First_k.begin(); it != First_k.end(); it++)
    if (m_UniqueGrammarItems[it->first].m_bGrammarRoot) {
        for (CPrefixSet::const_iterator it1 =  it->second.begin(); it1 != it->second.end(); it1++) {
            CWorkRule R;
            R.m_LeftPart = it->first;
            R.m_OriginalRuleNo = RuleId++;
            //R.m_RightPart.m_Items = *it1;
            R.m_RightPart.m_Items.insert(R.m_RightPart.m_Items.begin(), it1->begin(),it1->end());
            R.m_RightPart.m_SynMainItemNo = 0;
            m_RootPrefixes.insert(R);
        };
    };
};

// building m_FollowSets.
// m_FollowSets[i] is a set of all terminals which can be after a terminal sequence,
// which can be produced from  nonterminal symbol  i
void CWorkGrammar::Build_FOLLOW_Set()
{
    //  FIRST sets should  be already generated
    assert (m_FirstSets.size () == m_UniqueGrammarItems.size());
    m_FollowSets.clear();
    m_FollowSets.resize(m_UniqueGrammarItems.size());

    bool bChanged = true;
    while (bChanged) {
        bChanged = false;
        for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
            const CWorkRule& R = (*it);
            for (size_t i=0; i+1 < R.m_RightPart.m_Items.size(); i++) {
                int SymbolNo = R.m_RightPart.m_Items[i+1];

                if (m_UniqueGrammarItems[SymbolNo].m_Type == siMeta) {
                    //  adding FIRST(SymbolNo) to  FOLLOW(R.m_RightPart[i])
                    const yset<size_t>& s = m_FirstSets[SymbolNo];
                    for (yset<size_t>::const_iterator it = s.begin(); it != s.end(); it++)
                        if (m_FollowSets[R.m_RightPart.m_Items[i]].insert(*it).second)
                            bChanged  = true;
                } else
                    //  adding SymbolNo itself to  FOLLOW(SymbolNo)
                    if (m_FollowSets[R.m_RightPart.m_Items[i]].insert(SymbolNo).second)
                        bChanged  = true;
            };

            int SymbolNo = R.m_RightPart.m_Items.back();
            if (m_UniqueGrammarItems[SymbolNo].m_Type == siMeta) {
                //  adding FOLLOW(R.m_LeftPart) to  FOLLOW(SymbolNo)
                const yset<size_t>& s = m_FollowSets[R.m_LeftPart];
                for (yset<size_t>::const_iterator it = s.begin(); it != s.end(); it++)
                    if (m_FollowSets[SymbolNo].insert(*it).second)
                        bChanged  = true;
            };
        };
    };

 };

bool CWorkGrammar::TransferOutCountFromSymbolToItsDependencies(yvector<CRuleAgreement>& RuleInfos)
{
    // if i is number of meta symbol than
    // DependenciesOfMetaSymbols[i] is  a set of all meta symbols which occurs in its right part (recursively)
    yvector< yset<size_t> > DependenciesOfMetaSymbols (m_UniqueGrammarItems.size());

    // строим непосредственные зависомости     по правилам
    for (size_t ParentSymbolNo=0; ParentSymbolNo<m_UniqueGrammarItems.size(); ParentSymbolNo++) {
        if (m_UniqueGrammarItems[ParentSymbolNo].m_Type == siMeta) {
            for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++)
            if (it->m_LeftPart == ParentSymbolNo) {
                const CWorkRule& R = *it;
                for (int l=0; l<(int)R.m_RightPart.m_Items.size(); l++) {
                    size_t  DepSymbolNo  = R.m_RightPart.m_Items[l];
                    if (m_UniqueGrammarItems[DepSymbolNo ].m_Type == siMeta) {
                        DependenciesOfMetaSymbols[ParentSymbolNo].insert(DepSymbolNo);
                    }
                };

            };
        }
    };

    // транзитивное замыкание DependenciesOfMetaSymbols
    bool bOneMoreTime = true;
    while (bOneMoreTime) {
        bOneMoreTime = false;
        for (size_t ParentSymbolNo=0; ParentSymbolNo<m_UniqueGrammarItems.size(); ParentSymbolNo++) {
            yset<size_t>& s = DependenciesOfMetaSymbols[ParentSymbolNo];
            int SaveSize = s.size();
            for (yset<size_t>::const_iterator it = s.begin(); it != s.end(); it++)
                s.insert(DependenciesOfMetaSymbols[*it].begin(),DependenciesOfMetaSymbols[*it].end());
            bOneMoreTime |= (s.size() > (size_t)SaveSize);
        }
    }

    // MaxOutCountForSymbols[i] - сколько слов может занимать символ i
    yvector< size_t >  MaxOutCountForSymbols(m_UniqueGrammarItems.size(),0);

    // вычисляем максимальное число слов, которое может занимать каждый символ
    for (size_t ParentSymbolNo=0; ParentSymbolNo<m_UniqueGrammarItems.size(); ParentSymbolNo++)
    if (m_UniqueGrammarItems[ParentSymbolNo].m_Type == siMeta) {
        size_t& SymbolMaxOutCount = MaxOutCountForSymbols[ParentSymbolNo];
        assert (SymbolMaxOutCount == 0);

        // идем по всем правилам, ищем такое правило, по которому  ParentSymbolNo занимает максимальное число входных слов
        for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++)
        if (it->m_LeftPart == ParentSymbolNo) {
            const CWorkRule& R = *it;
            assert (R.m_OriginalRuleNo < RuleInfos.size());
            const CRuleAgreement& A = RuleInfos[R.m_OriginalRuleNo];

            size_t RuleMaxOutCount = 0;
            for (size_t i=0; i < A.size(); i++)
                if (A[i].m_ExtraRuleInfo.m_OutCount < MaxOutCount) {
                    RuleMaxOutCount = std::max(RuleMaxOutCount, A[i].m_ExtraRuleInfo.m_OutCount);
                    break;
                }
            // если RuleMaxOutCount == 0, значит при правиле в грамматике не было упоминания {Outcount= n},
            // и это означает, что это правило не накладывает ограничения на длину левой части, т.е.
            // левая часть может быть занимать аж MaxOutCount слов
            if (RuleMaxOutCount == 0)
                RuleMaxOutCount = MaxOutCount;
            SymbolMaxOutCount = std::max (SymbolMaxOutCount, RuleMaxOutCount);
        }
        assert (SymbolMaxOutCount > 0);
        if (SymbolMaxOutCount == 0)
            ythrow yexception() << "Unused meta-symbol in TransferOutCountFromSymbolToItsDependencies";

    }

    // если у главный символ может занимать только N слов, то и зависимый не более N слов
    for (size_t ParentSymbolNo=0; ParentSymbolNo<m_UniqueGrammarItems.size(); ParentSymbolNo++)
    if (m_UniqueGrammarItems[ParentSymbolNo].m_Type == siMeta) {
        yset<size_t>& s = DependenciesOfMetaSymbols[ParentSymbolNo];

        for (yset<size_t>::const_iterator it = s.begin(); it != s.end(); it++)
            MaxOutCountForSymbols[*it] = std::min(MaxOutCountForSymbols[*it], MaxOutCountForSymbols[ParentSymbolNo]);
    }

    // пишем в правила
    for (CWRI it = m_EncodedRules.begin(); it!= m_EncodedRules.end(); it++) {
        const CWorkRule& R = *it;
        if (MaxOutCountForSymbols[R.m_LeftPart] != MaxOutCount) {
            assert (R.m_OriginalRuleNo < RuleInfos.size());
            CRuleAgreement& A = RuleInfos[R.m_OriginalRuleNo];
            if (A.empty())
                A.push_back(SRuleExternalInformationAndAgreements());
            A.back().m_ExtraRuleInfo.m_OutCount = MaxOutCountForSymbols[R.m_LeftPart];
        }
    }
    return  true;
}

void CFilterStore::Save(TOutputStream* buffer) const
{
    ::SaveProtected(buffer, m_UniqueFilterItems);
    ::SaveProtected(buffer, m_FilterSet);
    ::Save(buffer, m_UniqueFilterTerminalIDs);
}

void CFilterStore::Load(TInputStream* buffer, const ITerminalDictionary& terminals)
{
    ::LoadProtected(buffer, m_UniqueFilterItems);
    ::LoadProtected(buffer, m_FilterSet);
    ::Load(buffer, m_UniqueFilterTerminalIDs);
    LoadTerminalIDsFromFilterItems(terminals);
}

void CFilterStore::Load(TInputStream* buffer) {
    TDefaultTerminalDictionary terminals;
    this->Load(buffer, terminals);
}

bool    CWorkGrammar::SaveFirstAndFollowSets(Stroka GrammarFileName) const
{
    FILE * fp = fopen(GrammarFileName.c_str(), "wb");

    fprintf(fp, "FIRST sets:\n\n");

    for (int i = 0; i < (int)m_FirstSets.size(); i++) {
        fprintf(fp, "%s = { ", m_UniqueGrammarItems[i].m_ItemStrId.c_str());
        for (yset<size_t>::const_iterator it_term = m_FirstSets[i].begin(); it_term != m_FirstSets[i].end(); it_term++)
            fprintf(fp, "%s ", m_UniqueGrammarItems[*it_term].m_ItemStrId.c_str());
        fprintf(fp, "}\n");
    }

    fprintf(fp, "\n\nFOLLOW sets:\n\n");
    for (int i = 0; i < (int)m_FollowSets.size(); i++) {
        fprintf(fp, "%s = { ", m_UniqueGrammarItems[i].m_ItemStrId.c_str());
        for (yset<size_t>::const_iterator it_term = m_FollowSets[i].begin(); it_term != m_FollowSets[i].end(); it_term++)
            fprintf(fp, "%s ", m_UniqueGrammarItems[*it_term].m_ItemStrId.c_str());
        fprintf(fp, "}\n");
    }

    fclose(fp);

    return true;
};

// adding a new root non-terminal "$NewRoot" and a special
// symbol for end of input ("$")
bool CWorkGrammar::AugmentGrammar(yvector<CRuleAgreement>& Agreements)
{
    if (GetCountOfRoots() != 1)
        ythrow yexception() << "A simple grammar should have only one root.";

    for (size_t i = 0; i < m_UniqueGrammarItems.size(); ++i)
        if (m_UniqueGrammarItems[i].m_ItemStrId == NEW_ROOT || m_UniqueGrammarItems[i].m_ItemStrId == END_OF_INPUT) {
            YASSERT(false);
            return false;
        }

    size_t rootIndex = 0;
    for (size_t i = 0; i < m_UniqueGrammarItems.size(); ++i)
        if (m_UniqueGrammarItems[i].m_bGrammarRoot) {
            rootIndex = i;
            break;
        }

    CGrammarItem I;

    //  adding a special symbol (end of input)
    I.m_ItemStrId = END_OF_INPUT;
    I.m_Lemma = CharToWide(END_OF_INPUT);
    I.m_Type = siString;
    m_UniqueGrammarItems.push_back(I);

    //  adding a new root
    I.m_ItemStrId = NEW_ROOT;
    I.m_Type = siMeta;
    m_UniqueGrammarItems.push_back(I);

    CWorkRule R;
    R.m_OriginalRuleNo = Agreements.size();
    Agreements.push_back(CRuleAgreement());

    R.m_LeftPart = m_UniqueGrammarItems.size() - 1;
    R.m_RightPart.m_SynMainItemNo = 0;
    R.m_RightPart.m_Items.push_back(rootIndex);
    R.m_RightPart.m_Items.push_back(m_UniqueGrammarItems.size() - 2);
    m_EncodedRules.insert(R);
    return true;
};

size_t  CWorkGrammar::GetNewRoot() const
{
    YASSERT(!m_UniqueGrammarItems.empty());
    YASSERT(m_UniqueGrammarItems.back().m_ItemStrId == NEW_ROOT);
    return m_UniqueGrammarItems.size() - 1;
};

size_t  CWorkGrammar::GetEndOfStreamSymbol() const
{
    YASSERT(m_UniqueGrammarItems.size() >= 2);
    YASSERT(m_UniqueGrammarItems.back().m_ItemStrId == NEW_ROOT);
    YASSERT(m_UniqueGrammarItems[m_UniqueGrammarItems.size() - 2].m_ItemStrId == END_OF_INPUT);
    return m_UniqueGrammarItems.size() - 2;

};

bool BuildGLRTableForGrammar(Stroka GrammarFileName, CWorkGrammar& WorkGrammar, yvector<CRuleAgreement>& Agreements)
{

    WorkGrammar.Build_FIRST_Set_k(1); //изменил 3 на 5 : nim : 19.05.04
    WorkGrammar.BuildAutomat(WorkGrammar.m_RootPrefixes);
    if (!WorkGrammar.AugmentGrammar(Agreements)) {
        return false;
    };

    WorkGrammar.Build_FIRST_Set();
    WorkGrammar.Build_FOLLOW_Set();

    WorkGrammar.TransferOutCountFromSymbolToItsDependencies(Agreements);

    if (!WorkGrammar.m_GLRTable.BuildGLRTable(GrammarFileName))/***/
    {
        return false;
    };
    return true;
};

void    CWorkGrammar::LoadEOFSentTerminal()
{
    for (int i = 0; i < (int)m_UniqueGrammarItems.size(); i++)
        if (m_UniqueGrammarItems[i].m_ItemStrId == TerminalValues[T_EOSENT]) {
            i_EOSentNum = i;
            break;
        }
}

static void BuildTerminalIndex(const yvector<CGrammarItem> grammarItems, const ITerminalDictionary& terminals,
                               ymap<int, eTerminals>& index) {
    for (size_t i = 0; i < grammarItems.size(); ++i) {
        const CGrammarItem& item = grammarItems[i];
        if (siString == item.m_Type || siAnyWord == item.m_Type || siFileList == item.m_Type)
            index[i] = terminals.FindTerminal(item.m_ItemStrId);
    }
}

void CWorkGrammar::LoadTerminalIDsFromGrammarItems(const ITerminalDictionary& terminals) {
    BuildTerminalIndex(m_UniqueGrammarItems, terminals, m_UniqueGrammarTerminalIDs);
}

void CFilterStore::LoadTerminalIDsFromFilterItems(const ITerminalDictionary& terminals) {
    BuildTerminalIndex(m_UniqueFilterItems, terminals, m_UniqueFilterTerminalIDs);
}

bool    CFilterStore::IsEmpty() const
{
    return (0 == m_FilterSet.size());
}

void    CWorkGrammar::LoadSharedKwTypesForGrammar()
{
    for (int i = 0; i < (int)m_UniqueGrammarItems.size(); i++) {
        LoadSharedKwTypesForGrammar(m_UniqueGrammarItems.at(i).GetPostfixKWType(DICT_KWTYPE));
        LoadSharedKwTypesForGrammar(m_UniqueGrammarItems.at(i).GetPostfixKWType(DICT_NOT_KWTYPE));
    }
    for (int i = 0; i < (int)m_FilterStore.m_UniqueFilterItems.size(); i++) {
        LoadSharedKwTypesForGrammar(m_FilterStore.m_UniqueFilterItems.at(i).GetPostfixKWType(DICT_KWTYPE));
        LoadSharedKwTypesForGrammar(m_FilterStore.m_UniqueFilterItems.at(i).GetPostfixKWType(DICT_NOT_KWTYPE));
    }
    for (int i = 0; i < (int)m_GLRTable.m_RuleInfos.size(); i++)
        for (int j = 0; j < (int)m_GLRTable.m_RuleInfos.at(i).m_RuleAgreement.size(); j++) {
            ymap< int, SKWSet >::const_iterator it = m_GLRTable.m_RuleInfos.at(i).m_RuleAgreement.at(j).m_KWSets.begin();

            for (; it != m_GLRTable.m_RuleInfos.at(i).m_RuleAgreement.at(j).m_KWSets.end(); it++) {
                yset<SArtPointer>::const_iterator it_art = it->second.m_KWset.begin();
                for (; it_art != it->second.m_KWset.end(); it_art++)
                    LoadSharedKwTypesForGrammar(*it_art);
            }
        }

    yset<SArtPointer>::const_iterator it = m_ExternalGrammarMacros.m_KWsNotInGrammar.begin();
    for (; it != m_ExternalGrammarMacros.m_KWsNotInGrammar.end(); it++)
        LoadSharedKwTypesForGrammar(*it);
}

void CWorkGrammar::LoadSharedKwTypesForGrammar(const SArtPointer& rArt)
{
    if (rArt.IsValid())
        m_kwArticlePointer.insert(rArt);
}

void    CWorkGrammar::LoadInterpSymbForGrammar()
{
    m_InterpSymbols.resize(InterpSymbsCount);
    for (int i = 0; i < (int)m_UniqueGrammarItems.size(); i++)
        for (int j = 0; j < InterpSymbsCount; j++)
            if (InterpSymbs[j] == m_UniqueGrammarItems.at(i).m_ItemStrId) {
                m_InterpSymbols.at(j).push_back(i); break;
            }
}

bool    CWorkGrammar::IsCertainInterpSymbol(EInterpSymbs eInterp, int iSymbID) const
{
    for (int i = 0; i < (int)m_InterpSymbols.at((int)eInterp).size(); i++)
        if (iSymbID == m_InterpSymbols.at((int)eInterp).at(i))
            return true;

    return false;
}

void    CWorkGrammar::CheckInterpretationFields()
{
    if (m_ExternalGrammarMacros.m_bNoInterpretation) return;
    for (int i = 0; i < (int)m_GLRTable.m_RuleInfos.size(); i++)
        for (int j = 0; j <  (int)m_GLRTable.m_RuleInfos.at(i).m_RuleAgreement.size(); j++)
            if (m_GLRTable.m_RuleInfos.at(i).m_RuleAgreement.at(j).m_FactsInterpretation.size() > 0)
                return;

    m_ExternalGrammarMacros.m_bNoInterpretation = true;
}

void CWorkGrammar::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_Language);
    ::SaveProtected(buffer, m_UniqueGrammarItems);
    ::Save(buffer, m_UniqueGrammarTerminalIDs);

    ::SaveProtected(buffer, m_EncodedRules);
    ::SaveProtected(buffer, m_FirstSets);
    ::SaveProtected(buffer, m_FollowSets);

    ::Save(buffer, m_RootPrefixes);
    ::Save(buffer, m_AutomatSymbolInformation);
    ::Save(buffer, m_TrieHolder);

    ::Save(buffer, m_GLRTable);     // protected

    ::Save(buffer, i_EOSentNum);
    ::Save(buffer, m_ExternalGrammarMacros);
    ::SaveProtected(buffer, m_kwArticlePointer);

    ::Save(buffer, m_FilterStore);  // protected
    ::SaveProtected(buffer, m_InterpSymbols);
}

void CWorkGrammar::Load(TInputStream* buffer) {
    TDefaultTerminalDictionary terminals;
    this->Load(buffer, terminals);
}

void CWorkGrammar::Load(TInputStream* buffer, const ITerminalDictionary& terminals)
{
    ::Load(buffer, m_Language);
    ::LoadProtected(buffer, m_UniqueGrammarItems);
    ::Load(buffer, m_UniqueGrammarTerminalIDs);

    ::LoadProtected(buffer, m_EncodedRules);
    ::LoadProtected(buffer, m_FirstSets);
    ::LoadProtected(buffer, m_FollowSets);

    ::Load(buffer, m_RootPrefixes);
    ::Load(buffer, m_AutomatSymbolInformation);
    ::Load(buffer, m_TrieHolder);
    m_TrieHolder.SetSymbolInformation(&m_AutomatSymbolInformation);

    ::Load(buffer, m_GLRTable);     // protected
    m_GLRTable.m_pWorkGrammar = this;

    ::Load(buffer, i_EOSentNum);
    ::Load(buffer, m_ExternalGrammarMacros);
    ::LoadProtected(buffer, m_kwArticlePointer);

    m_FilterStore.Load(buffer, terminals);  // protected
    ::LoadProtected(buffer, m_InterpSymbols);

    LoadSharedKwTypesForGrammar();
    LoadEOFSentTerminal();
    LoadTerminalIDsFromGrammarItems(terminals);
    m_FilterStore.LoadTerminalIDsFromFilterItems(terminals);
    CheckInterpretationFields();
}

bool CWorkGrammar::SaveToFile(const Stroka& file_name)
{
    TOFStream f(file_name);

    // binary protection
    SaveHeader(&f);

    // compress the rest of grammar
    const ui16 block_size = 1 << 15;
    TLz4Compress z(&f, block_size);
    TBufferedOutput b(&z, block_size);

    // in order to collect kw-types table (to save it before all other content), serialize to tmp-buffer first
    TBufferOutput tmpbuf;
    this->Save(&tmpbuf);

    // now save collected kw-types
    GetKWTypePool().Save(&b);
    // and write already serialized grammar content after
    ::SaveProtectedSize(&b, tmpbuf.Buffer().Size());
    ::SaveArray(&b, tmpbuf.Buffer().Data(), tmpbuf.Buffer().Size());

    return true;
}

bool CWorkGrammar::LoadFromFile(const Stroka& file_name, const ITerminalDictionary& terminals)
{
    TIFStream f(file_name);

    // binary protection
    VerifyHeader(&f);

    TLz4Decompress z(&f);
    // first load kw-types table
    GetMutableKWTypePool().Load(&z);
    // then load the grammar itself
    ::LoadProtectedSize(&z);
    this->Load(&z, terminals);
    return true;
}

bool CWorkGrammar::LoadFromFile(const Stroka& file_name) {
    TDefaultTerminalDictionary terminals;
    return LoadFromFile(file_name, terminals);
}

// A version number for tomita grammar binary format.
// Should be increased when the format is modified and
// old binaries become incompatible with new compiler.
//static const ui32 GRAMMAR_BINARY_VERSION = 1;
//static const ui32 GRAMMAR_BINARY_VERSION = 2;       // Izafet agreement
//static const ui32 GRAMMAR_BINARY_VERSION = 3;       // Using PIRE instead of PCRE for regex matches, all regexps stored as UTF8 now (was cp1251)
//static const ui32 GRAMMAR_BINARY_VERSION = 4;       // Geo-agreement
//static const ui32 GRAMMAR_BINARY_VERSION = 5;       // Gzt-weight
//static const ui32 GRAMMAR_BINARY_VERSION = 6;       // TCompactStorage format is changed during gazetteer refactoring (first itemset fixed to be an empty set)
//static const ui32 GRAMMAR_BINARY_VERSION = 7;       // MiniLzo -> Lz4 for binaries compression (better license)
  static const ui32 GRAMMAR_BINARY_VERSION = 8;       // Lemmer version added to header


// Same for TTomitaCompiler logic, when it changed to
// an extent of requiring to re-compile all grammars.
//static const ui32 TOMITA_COMPILER_VERSION = 2;
//static const ui32 TOMITA_COMPILER_VERSION = 3;      // Tomita encodes all parsed non-ASCII strings to UTF8
  static const ui32 TOMITA_COMPILER_VERSION = 4;      // Substitution variables implemented (e.g. #define rootS <gram=S,rt>)


static ui32 GetLemmerVersion() {
    return 0; // TODO: Add version info to .so
}

void CWorkGrammar::SaveHeader(TOutputStream* output) const
{
    SaveProtectedSize(output, GRAMMAR_BINARY_VERSION);
    SaveProtectedSize(output, TOMITA_COMPILER_VERSION);
    SaveProtectedSize(output, GetLemmerVersion());
    SaveProtected(output, SourceFiles);
}

bool CWorkGrammar::LoadHeader(TInputStream* input)
{
    try {
        size_t binaryVersion = 0;
        if (!LoadProtectedSize(input, binaryVersion) || binaryVersion != GRAMMAR_BINARY_VERSION)
            return false;
        size_t compilerVersion = 0;
        if (!LoadProtectedSize(input, compilerVersion) || compilerVersion != TOMITA_COMPILER_VERSION)
            return false;
        size_t lemmerVersion = 0;
        if (!LoadProtectedSize(input, lemmerVersion) || lemmerVersion != GetLemmerVersion())
            return false;
        LoadProtected(input, SourceFiles);
        return true;
    } catch (yexception&) {
        return false;
    }
}

void CWorkGrammar::VerifyHeader(TInputStream* input)
{
    if (!LoadHeader(input)) {
        Stroka msg = "Failed to load tomita grammar: the binary is incompatible with your program. "
                     "You should either re-compiler your .bin, or rebuild your program from corresponding version of sources.\n";
        Cerr << msg << Endl;
        ythrow yexception() << msg;
    }
}
