#pragma once

#include <util/generic/vector.h>

#include "groupsubtree.h"
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include <FactExtract/Parser/afglrparserlib/inputsequence.h>
#include "primitivegroup.h"
#include <FactExtract/Parser/afglrparserlib/inputitemfactory.h>
#include "syngroup.h"
#include "tomitaitemsholder.h"

class CSynChainVariant
{
public:
    CSynChainVariant(const CWordVector& Words);
    CSynChainVariant(const CSynChainVariant& SynChainVariant);
    CSynChainVariant& operator=(const CSynChainVariant& SynChainVariant);
    virtual ~CSynChainVariant() {
    }

    void AddPrimitiveGroup(CSymbolNode& childNode);//, CGLRParser& parser);
    void AddGroup(CSymbolNode& childNode, CGLRParser& m_Parser);
    void AddSubGroups(CGroupSubTree* pSubTree, CSymbolNode& node, CGLRParser& parser, CGroup* pParentGroup, CGroup* pParentClonedGroup);
    virtual void Free();

//protected:
    yvector<CGroupSubTree*> m_SynItems;
    const CWordVector& m_Words;
};

class CSynChains : public CTomitaItemsHolder
{
public:
    CSynChains(const CWordVector& Words, const CWorkGrammar& gram, yvector<SWordHomonymNum>& clauseWords);
    virtual ~CSynChains() {
    }

    virtual void Run();
    virtual void Free();
    void CopyFrom(const CSynChains& SynChains);

    yvector<CSynChainVariant> m_SynVariants;
    CGLRParser  m_Parser;
    int m_iSENTNonTerminal;
    int m_iGROUPNonTerminal;
    int m_iWORDTerminal;

protected:
    void GetGrammarSymbols();
    void DeleteUnusefulNonTerminal(yvector<int>& roots);
    void DeleteUnusefulNonTerminal(int iNode);
    int  DeleteGroupsWithEqualAdresses(int iNode);
    void AddSynVariants(yvector<int>& roots);
    void AddSynVariant(CSymbolNode& node);
    int  GetCoverage(int iRoot);
    int  GetGroupsCount(int iNode);
    bool IsRoot(int iNode);

//protected:

};
