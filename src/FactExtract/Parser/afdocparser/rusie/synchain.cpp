#include "synchain.h"
#include "primitivegroup.h"


CSynChainVariant::CSynChainVariant(const CWordVector& Words)
    : m_Words(Words)
{
}

CSynChainVariant::CSynChainVariant(const CSynChainVariant& SynChains)
    : m_SynItems(SynChains.m_SynItems)
    , m_Words(SynChains.m_Words)
{
}

CSynChainVariant& CSynChainVariant::operator=(const CSynChainVariant& SynChainVariant)
{
    m_SynItems = SynChainVariant.m_SynItems;
    return *this;
}

void CSynChainVariant::Free()
{
    for (size_t i = 0; i < m_SynItems.size(); i++) {
        CGroupSubTree* pTree = m_SynItems[i];
        pTree->Free();
        delete pTree;
    }
    m_SynItems.clear();
}

void CSynChainVariant::AddSubGroups(CGroupSubTree* pSubTree, CSymbolNode& node, CGLRParser& parser, CGroup* pParentGroup, CGroup* pParentClonedGroup)
{
    assert(node.m_ParseChildren.size() <= 1);
    if (node.m_ParseChildren.size() == 0)
        return;
    const CGroup* pMainGroup = pParentGroup->GetMainGroup();
    for (size_t i = 0; i < node.m_ParseChildren[0].m_Items.size(); i++) {
        CSymbolNode& childNode = parser.m_SymbolNodes[node.m_ParseChildren[0].m_Items[i]];
        CGroup* pGroup = (CGroup*)childNode.m_pItem;

        CGroup* pClonedGroup = pGroup->Clone();
        if ((*pMainGroup) == (*pGroup))
            pParentClonedGroup->SetMainGroup(pClonedGroup);
        pSubTree->AddGroup(pClonedGroup);
        AddSubGroups(pSubTree, childNode, parser, pGroup, pClonedGroup);
    }
}

void CSynChainVariant::AddGroup(CSymbolNode& childNode, CGLRParser& parser)
{
    CGroup* pGroup = (CGroup*)childNode.m_pItem;
    CGroup* pClonedGroup = pGroup->Clone();
    CGroupSubTree* pSubTree = new CGroupSubTree(pClonedGroup);
    pSubTree->SetPair(*pGroup);
    AddSubGroups(pSubTree, childNode, parser, pGroup, pClonedGroup);
    m_SynItems.push_back(pSubTree);
}

void CSynChainVariant::AddPrimitiveGroup(CSymbolNode& childNode)//, CGLRParser& parser)
{
    CPrimitiveGroup* pGroup = (CPrimitiveGroup*)((CPrimitiveGroup*)childNode.m_pItem)->Clone();
    CGroupSubTree* pSubTree = new CGroupSubTree(pGroup);
    pSubTree->SetPair(*pGroup);
    m_SynItems.push_back(pSubTree);
}

//---------------------CSynChains------------------------------------

CSynChains::CSynChains(const CWordVector& Words, const CWorkGrammar& gram, yvector<SWordHomonymNum>& clauseWords)
    : CTomitaItemsHolder(Words, gram, clauseWords)
    , m_Parser(this)
{
    GetGrammarSymbols();
    m_Parser.PutSynAnOptions(m_iSENTNonTerminal, m_iWORDTerminal, m_iGROUPNonTerminal);
    m_Parser.m_bBreakAfterMaxSymbolNodesCount = false;

}

int CSynChains::DeleteGroupsWithEqualAdresses(int iNode)
{
    yvector<CSymbolNode>& Output = m_Parser.m_SymbolNodes;
    CSymbolNode& symboleNode = Output[iNode];

    if (symboleNode.m_ParseChildren.size() == 0)
        return iNode;

    yvector<size_t>& v = symboleNode.m_ParseChildren[0].m_Items;
    if (v.size() == 1) {
        return DeleteGroupsWithEqualAdresses(v[0]);
    } else {
        for (size_t i = 0; i < v.size(); i++) {
            v[i] = DeleteGroupsWithEqualAdresses(v[i]);
        }
        return iNode;
    }
}

void CSynChains::DeleteUnusefulNonTerminal(int iNode)
{
    yvector<CSymbolNode>& Output = m_Parser.m_SymbolNodes;
    CSymbolNode& symboleNode = Output[iNode];
    assert(symboleNode.m_ParseChildren.size() == 1);

    yvector<size_t>& v = symboleNode.m_ParseChildren[0].m_Items;
    for (size_t j=0; j < v.size(); j++) {
        CSymbolNode& childNode = Output[v[j]];
        if ((childNode.m_SymbolNo == m_iSENTNonTerminal) ||
            (childNode.m_SymbolNo == m_iGROUPNonTerminal)) {
            DeleteUnusefulNonTerminal(v[j]);
            v.erase(v.begin() + j);
            v.insert(v.begin() + j, childNode.m_ParseChildren[0].m_Items.begin(),childNode.m_ParseChildren[0].m_Items.end());
            j+= childNode.m_ParseChildren[0].m_Items.size() - 1;
        }
    }
}

int CSynChains::GetCoverage(int iRoot)
{
    yvector<CSymbolNode>& Output = m_Parser.m_SymbolNodes;
    CSymbolNode& root = Output[iRoot];
    yvector<size_t>& v = root.m_ParseChildren[0].m_Items;
    int iCoverage = 0;
    for (size_t j=0; j < v.size(); j++) {
        CSymbolNode& childNode = Output[v[j]];
        if (childNode.m_SymbolNo != m_iWORDTerminal)
            iCoverage += childNode.m_InputEnd - childNode.m_InputStart;
    }
    return iCoverage;
}

int CSynChains::GetGroupsCount(int iNode)
{
    yvector<CSymbolNode>& Output = m_Parser.m_SymbolNodes;
    CSymbolNode& symboleNode = Output[iNode];
    if (symboleNode.m_ParseChildren.size() == 0)
        return 0;

    yvector<size_t>& v = symboleNode.m_ParseChildren[0].m_Items;
    if (v.size() == 1)
        return GetGroupsCount(v[0]);

    int iGroupsCount = 0;
    if (!IsRoot(iNode))
        iGroupsCount++;
    for (size_t j=0; j < v.size(); j++) {
        CSymbolNode& childNode = Output[v[j]];
        if ((childNode.m_SymbolNo != m_iWORDTerminal) &&
            ((childNode.m_InputEnd - childNode.m_InputStart) > 1)) {
            //iGroupsCount++;
            iGroupsCount += GetGroupsCount(v[j]);
        }
    }
    return iGroupsCount;
}

bool CSynChains::IsRoot(int iNode)
{
    return m_GLRgrammar.m_UniqueGrammarItems[m_Parser.m_SymbolNodes[iNode].m_SymbolNo].m_bGrammarRoot;
}

void CSynChains::DeleteUnusefulNonTerminal(yvector<int>& roots)
{
    yvector<int> all_roots;
    yvector<CSymbolNode>& Output = m_Parser.m_SymbolNodes;
    size_t iMinLength = m_FDOChainWords.size();//m_InputWords.size();
    int iMaxCoverage = 0;

    size_t i;
    for (i = 0; i < Output.size(); i++) {
        if    (m_GLRgrammar.m_UniqueGrammarItems[Output[i].m_SymbolNo].m_bGrammarRoot
                &&    (Output[i].m_InputStart == 0)
                &&    !Output[i].m_bDisabled
                &&    (Output[i].m_SymbolNo != (int)m_GLRgrammar.GetEndOfStreamSymbol())
            ) {
            assert(Output[i].m_ParseChildren.size() == 1);
            DeleteUnusefulNonTerminal(i);
            if (Output[i].m_ParseChildren[0].m_Items.size() < iMinLength)
                iMinLength = Output[i].m_ParseChildren[0].m_Items.size();
            int iCoverage = GetCoverage(i);
            if (iCoverage > iMaxCoverage)
                iMaxCoverage = iCoverage;
            all_roots.push_back(i);
        }
    }

    //for( i = 0 ; i < all_roots.size() ; i++ )
    //{
    //    if( GetCoverage(all_roots[i]) == iMaxCoverage )
    //        roots.push_back(all_roots[i]);
    //}

    int iMinGroupsCount = 0;

    for (i = 0; i < all_roots.size(); i++) {
        if (Output[all_roots[i]].m_ParseChildren[0].m_Items.size() == iMinLength) {
            int iGroupCount = GetGroupsCount(all_roots[i]);
            if ((iMinGroupsCount == 0) || (iGroupCount < iMinGroupsCount))
                iMinGroupsCount = iGroupCount;
        }
    }

    for (i = 0; i < all_roots.size(); i++) {
        if (GetCoverage(all_roots[i]) == iMaxCoverage) {
            if (Output[all_roots[i]].m_ParseChildren[0].m_Items.size() == iMinLength)
                roots.push_back(all_roots[i]);
            else if (GetGroupsCount(all_roots[i]) >= iMinGroupsCount)
                    roots.push_back(all_roots[i]);
        }
    }

    for (i = 0; i < roots.size(); i++) {
        CSymbolNode& node = Output[roots[i]];
        for (size_t j = 0; j < node.m_ParseChildren[0].m_Items.size(); j++) {
            node.m_ParseChildren[0].m_Items[j] = DeleteGroupsWithEqualAdresses(node.m_ParseChildren[0].m_Items[j]);
        }
    }
}

void CSynChains::AddSynVariant(CSymbolNode& node)
{
    CSynChainVariant synChainVariant(m_Words);
    assert(node.m_ParseChildren.size() == 1);
    for (size_t i = 0; i < node.m_ParseChildren[0].m_Items.size(); i++) {
        CSymbolNode& childNode = m_Parser.m_SymbolNodes[node.m_ParseChildren[0].m_Items[i]];
        if (childNode.m_SymbolNo == m_iWORDTerminal)
            synChainVariant.AddPrimitiveGroup(childNode);
        else
            synChainVariant.AddGroup(childNode, m_Parser);
    }
    m_SynVariants.push_back(synChainVariant);
}

void CSynChains::AddSynVariants(yvector<int>& roots)
{
    for (size_t i = 0; i < roots.size(); i++) {
        AddSynVariant(m_Parser.m_SymbolNodes[roots[i]]);
    }
}

void CSynChains::GetGrammarSymbols()
{
    for (size_t i = 0; i < m_GLRgrammar.m_UniqueGrammarItems.size(); i++) {
        if (m_GLRgrammar.m_UniqueGrammarItems[i].m_ItemStrId == "SENT")
            m_iSENTNonTerminal = i;
        if (m_GLRgrammar.m_UniqueGrammarItems[i].m_ItemStrId == "Group")
            m_iGROUPNonTerminal = i;
        if (m_GLRgrammar.m_UniqueGrammarItems[i].m_ItemStrId == "AnyWord")
            m_iWORDTerminal = i;
    }
}

void CSynChains::Run()
{
    if (m_FDOChainWords.size() == 0) {
        CSynChainVariant synChainVariant(m_Words);
        m_SynVariants.push_back(synChainVariant);
        return;
    }

    m_Parser.InitGLRParser(&m_GLRgrammar.m_GLRTable);
    yset<int> EndOfSentenceSet;
    EndOfSentenceSet.insert(m_GLRgrammar.GetEndOfStreamSymbol());
    yset<int> AnyWordSet;
    AnyWordSet.insert(m_iWORDTerminal);
    for (size_t i = 0; i < m_FDOChainWords.size(); i++) {
        //const yset<int>& Symbols = m_InputWords[i]->m_AutomatSymbolInterpetationUnion;
        const yset<int>& Symbols = m_Words.GetWord(m_FDOChainWords[i]).m_AutomatSymbolInterpetationUnion;
        if (Symbols.size() == 0) {
            if (!m_Parser.ParseSymbol(AnyWordSet, false))
                ythrow yexception() << "Can't parse symbol!";
        } else if (!m_Parser.ParseSymbol(Symbols, false))
                ythrow yexception() << "Can't parse symbol!";
        if    (i+1 == m_FDOChainWords.size()) {
            m_Parser.ParseSymbol(EndOfSentenceSet, false);
        };
    };

    yvector<int> roots;
    DeleteUnusefulNonTerminal(roots);
    AddSynVariants(roots);
    m_Parser.Free();
}

void CSynChains::CopyFrom(const CSynChains& SynChains)
{
    for (size_t i = 0; i < SynChains.m_SynVariants.size(); i++) {
        CSynChainVariant newVariant(m_Words);
        for (size_t j = 0; j < SynChains.m_SynVariants[i].m_SynItems.size(); ++j)
            newVariant.m_SynItems.push_back(SynChains.m_SynVariants[i].m_SynItems[j]->Clone());
        m_SynVariants.push_back(newVariant);
    }
}

void CSynChains::Free()
{
    for (size_t i = 0; i < m_SynVariants.size(); i++)
        m_SynVariants[i].Free();
    m_SynVariants.clear();
}
