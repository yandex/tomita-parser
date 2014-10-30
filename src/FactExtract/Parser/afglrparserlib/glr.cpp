#include "glr.h"
#include "simplegrammar.h"
#include "inputitemfactory.h"


void CSymbolNode::AddInputItem(CInputItem* pItem)
{
    if (m_pItem == NULL)
        m_pItem = pItem;
    else {
        const CInputItem::TWeight newWeight = pItem->GetWeight();
        const CInputItem::TWeight curWeight = m_pItem->GetWeight();
        if (curWeight < newWeight) {
            m_OtherItemVariants.push_back(m_pItem);
            m_pItem = pItem;
        } else
            m_OtherItemVariants.push_back(pItem);
    }
}

CGLRParser::CGLRParser(CInputItemFactory* pInputItemFactory)
    : m_bDisableWORDSStateNodes(false)
    , m_bSynAnOptions(false)
    , m_SENTNonTerminal(-1)
    , m_WORDTerminal(-1)
    , m_GROUPNonTerminal(-1)
    , m_MaxLastGroup(-1, -1)
    , m_iCreatedGroups(0)
    , m_bRobust(false)
    , m_bBreakAfterMaxSymbolNodesCount(true)
    , m_pInputItemFactory(pInputItemFactory)
{
}

CGLRParser::CGLRParser(CInputItemFactory* factory, CGLRTable* table, bool breakaftermax, bool isrobust, int sent, int word, int group)
    : m_bDisableWORDSStateNodes(false)
    , m_bSynAnOptions(sent > -1 && word > -1 && group > -1 ? true : false)
    , m_InputLevels(1, 1)
    , m_StateNodes(1, CStateNode())
    , m_SENTNonTerminal(sent)
    , m_WORDTerminal(word)
    , m_GROUPNonTerminal(group)
    , m_MaxLastGroup(-1, -1)
    , m_iCreatedGroups(0)
    , m_bRobust(isrobust)
    , m_bBreakAfterMaxSymbolNodesCount(breakaftermax)
    , m_pTable(table)
    , m_pInputItemFactory(factory)
{

}

void CGLRParser::PutSynAnOptions(int SENTNonTerminal, int WORDTerminal, int GROUPNonTerminal)
{
    m_SENTNonTerminal = SENTNonTerminal;
    m_GROUPNonTerminal = GROUPNonTerminal;
    m_WORDTerminal = WORDTerminal;
    m_bSynAnOptions = true;
}

void    CGLRRuleInfo::Init(const CWorkRule* p)
{
    m_OriginalRuleNo = p->m_OriginalRuleNo;
    m_LeftPart = p->m_LeftPart;
    m_RuleLength = p->m_RightPart.m_Items.size();
    m_SynMainItemNo = p->m_RightPart.m_SynMainItemNo;
};

/*
  create a state node v0 labelled with the start state 0 of DFA
  set the first input related closure set be  {v0},
      m_ActiveNodes = 0
      m_PendingShifts = 0
      m_PendingReductions = 0

*/
void CGLRParser :: InitGLRParser(const CGLRTable*   pTable)
{
    m_StateNodes.clear();
    m_SymbolNodes.clear();
    CStateNode Node;
    m_pTable = pTable;
    YASSERT(m_pTable != NULL);
    YASSERT(!m_pTable->IsEmpty());
    Node.m_StateNo = 0;
    m_StateNodes.push_back(Node);

    m_InputLevels.resize(1);
    m_InputLevels[0] = 1;

    m_PendingReductions.clear();
    m_PendingShifts.clear();
    m_ActiveNodes.clear();

    //m_iLastGroupRightBorder = -1;
    m_MaxLastGroup = MakePair(-1, -1);
    m_BuiltGROUPs.clear();
};

void CGLRParser :: CopyFrom(const CGLRParser& C)
{
    m_pTable = C.m_pTable;
    m_SymbolNodes = C.m_SymbolNodes;
    m_StateNodes = C.m_StateNodes;
    m_ActiveNodes = C.m_ActiveNodes;
    m_PendingShifts = C.m_PendingShifts;
    m_PendingReductions = C.m_PendingReductions;
    m_InputLevels = C.m_InputLevels;
    m_pInputItemFactory = C.m_pInputItemFactory;

};

bool CGLRParser::ParseSymbol(const yset<int>& Symbols, bool bStartPoint)
{
    m_ActiveNodes.clear();
    m_bDisableWORDSStateNodes = false;
    m_BuiltGROUPs.clear();

    {
        //  copying from previous input-related closure set to the current actve nodes set
        size_t u = m_InputLevels.back();
        (void)u;
        assert (m_StateNodes.size() == u);
        size_t start = 0;
        if (m_InputLevels.size() > 1)
            start = m_InputLevels[m_InputLevels.size() - 2];

        for (size_t i=start; i<m_InputLevels.back(); i++)
            m_ActiveNodes.insert(i);
    }

    if (bStartPoint)
        m_ActiveNodes.insert(0);

    size_t iFirstReducedStateNode = m_StateNodes.size();

    while (!m_ActiveNodes.empty() || !m_PendingReductions.empty()) {
        if (!m_ActiveNodes.empty())
            Actor(Symbols);
        else
            Reducer(Symbols);

        if (((m_SymbolNodes.size() >=  MaxSymbolNodesCount) || (m_iCreatedGroups >=  (int)MaxSymbolNodesCount*5)) && m_bBreakAfterMaxSymbolNodesCount)
            return false;
    };

    //  finish current input related closure set
    m_InputLevels.back() = m_StateNodes.size();

    if (m_bDisableWORDSStateNodes) {
        size_t start = 0;

        if (m_InputLevels.size() > 1) {
            //start = m_InputLevels[m_InputLevels.size() - 2];
            start = iFirstReducedStateNode;
        }

        for (size_t i=start; i<m_InputLevels.back(); i++)
            m_ActiveNodes.insert(i);

        DisableWORDSSrtateNodes();
    }

    // add a new one input related closure set
    m_InputLevels.push_back(m_StateNodes.size());

    Shifter(Symbols);

    bool bResult = m_StateNodes.size() - m_InputLevels.back() > 0;
    m_InputLevels.back() = m_StateNodes.size();
    m_bDisableWORDSStateNodes = false;

    if (m_bRobust)
        return true;
    else
        return bResult;

};

/*
 ACTOR(i)
   remove v  from m_ActiveNodes, and let h be the label of v
   if 'shift k' is an action in position (h, SymbolNo) of Parsing Table, add (v,k) to  m_PendingShifts
   for each entry 'reduce j' in position (h,SymbolNo) of Parsing Table
   {
        for  each successor node u  of v add (u,j) to m_PendingReductions
   }
*/
//void CGLRParser::Actor(size_t SymbolNo)
void CGLRParser::Actor(const yset<int>& Symbols)
{
    assert (!m_ActiveNodes.empty());
    size_t StateNodeNo = *m_ActiveNodes.begin();
    m_ActiveNodes.erase(m_ActiveNodes.begin());
    const CStateNode& Node = m_StateNodes[StateNodeNo];

    for (yset<int>::const_iterator it = Symbols.begin(); it != Symbols.end(); it++) {
        size_t row = Node.m_StateNo;
        size_t column = *it;
        if (m_pTable->IsEmptyCell(row, column))
            continue;

        ui32 encoded = m_pTable->GetEncodedCell(row, column);
        ui32 gotoLine;
        if (m_pTable->GetGotoLine(encoded, gotoLine))
            m_PendingShifts.insert(CPendingShift(StateNodeNo, gotoLine, column));

        if (m_pTable->IsSimpleCell(encoded)) {
            for (size_t j = 0; j < Node.m_SymbolChildren.size(); ++j)
                m_PendingReductions.insert(CPendingReduction(&m_pTable->GetSimpleCellRule(encoded), Node.m_SymbolChildren[j], StateNodeNo));
        } else for (CGLRTable::TRuleIter ruleIt = m_pTable->IterComplexCellRules(encoded); ruleIt.Ok(); ++ruleIt)
            for (size_t j = 0; j < Node.m_SymbolChildren.size(); ++j)
                m_PendingReductions.insert(CPendingReduction(&*ruleIt, Node.m_SymbolChildren[j], StateNodeNo));
    }
}

bool CGLRParser::HasPathOfLengthTwo(CInputItem* pNewItem, int SourceNodeNo, int TargetNodeNo, size_t& SymbolNodeNo, const CSymbolNode& BetweenNode) const
{
    if ((m_SymbolNodes.size() >= MaxSymbolNodesCount) && m_bBreakAfterMaxSymbolNodesCount)
        return false;
    //return false; /**/
    assert ((size_t)SourceNodeNo < m_StateNodes.size());
    assert ((size_t)TargetNodeNo < m_StateNodes.size());

    const CStateNode& Node = m_StateNodes[SourceNodeNo];
    // going to the depth
    for (size_t j=0; j < Node.m_SymbolChildren.size(); j++) {
        SymbolNodeNo = Node.m_SymbolChildren[j];
        const CSymbolNode& SymbolNode = m_SymbolNodes[SymbolNodeNo];
                bool bEqualNodes = m_pInputItemFactory->EqualNodes(pNewItem, SymbolNode.m_pItem);
                bool bChildFound = (SymbolNode.FindChild(TargetNodeNo) != -1);
                bool bEqualWeight1 = (pNewItem->GetWeight() == SymbolNode.m_pItem->GetWeight());
                bool bRes = (bChildFound && bEqualNodes && bEqualWeight1 && (BetweenNode.m_InputStart == SymbolNode.m_InputStart));
                if (bRes)
                   return true;

    };
    return false;
};

size_t CGLRParser::GetFirstNodeNoOfCurrentClosure() const
{
    return  (m_InputLevels.size () == 1) ? 0:m_InputLevels.back();
}

/*
 this function creates or re-uses a  node which should be in the current input related closure set
 and which should have StateNo associated with it.
 Return Value:
    NewNodeNo - the number of the result node
 if such a node was already  created then the fucntion return false, otherwise it returns true.
*/
bool CGLRParser::CreateOrReuseNode(int StateNo, size_t& NewNodeNo)
{
    assert (!m_InputLevels.empty());

    if (m_bRobust)
        // if m_bRobust, then the root is included into the active  nodes set,
        //  if the root (m_StateNodes[0]) is included, we should first check whether
        // we can reuse it, we should do it specially, because
        // the cycle which goes through the current input related closure set  doesn't include the root
        if (m_StateNodes[0].m_StateNo == StateNo) {
            NewNodeNo = 0;
            return false;
        };

    /**/
    if (!m_bSynAnOptions) {
        for (NewNodeNo=GetFirstNodeNoOfCurrentClosure(); NewNodeNo < m_StateNodes.size(); NewNodeNo++)
            if (m_StateNodes[NewNodeNo].m_StateNo == StateNo)
                return false;
    }

    CStateNode Node;
    YASSERT(!m_pTable->IsEmpty());
    Node.m_StateNo = StateNo;
    m_StateNodes.push_back(Node);
    NewNodeNo = m_StateNodes.size() - 1;

    return true;
};

void CGLRParser::Free()
{
    for (int i = 0; i < (int)m_SymbolNodes.size(); i++) {
        CSymbolNode& symbolNode = m_SymbolNodes[i];
        delete symbolNode.m_pItem;
        for (int j = 0; j < (int)symbolNode.m_OtherItemVariants.size(); j++)
            delete symbolNode.m_OtherItemVariants[j];
    }
    m_iCreatedGroups = 0;
}

/*
  REDUCER (i)
    remove (u,j) from R, where  u is a symbol node, and j is a rule no
    let RuleLength be the length of the right hand side od rule j
    let LeftPart be the symbol on the left hand side of rule j
    for each  state node w which can be reached from u along a path of length (2*RuleLength-1) do
    {
        let k be the label of the w( = "ChildNode.m_StateNo")
        let "goto l" be the entry in position (k, LeftPart) in the parsing table (="Cell.m_GotoLine")
        if there is no node in the last input related closure set labeled l then
          create a new state node in the GSS labelled l and  add it to  the last input related closure set
                                        and to m_ActiveNodes
        let  v be the node in the last input related closure labelled l (==NewNodeNo)
        if there is a path of length 2  in the GSS from v to w then do nothing
        else
        {
            create a new symbol node u' in the GSS labelled LeftPart
            make u' successor of v and a predecessor of w
            if v  is not in A
            {
                for all reductions rk in position  (l, SymbolNo) of the table add (u,k) to  R
            }
        };

    };

*/

void CGLRParser::FillRightPartItems(yvector<CInputItem*>& rightPartItems, const yvector<size_t>& Path)
{
    for (int i = Path.ysize() - 2; i > 0; i -= 2) {
        YASSERT(Path[i] < m_SymbolNodes.size());
        rightPartItems.push_back(m_SymbolNodes[Path[i]].m_pItem);
    }
}

void CGLRParser::DisableWORDSSrtateNodes()
{
    //return;
    yset<size_t> disabledStateNode;

    TPair<size_t,size_t> maxGroup;

    maxGroup  = *m_BuiltGROUPs.begin();
    yset< TPair<size_t,size_t> >::iterator itGROUPS = m_BuiltGROUPs.begin();
    for (; itGROUPS != m_BuiltGROUPs.end(); itGROUPS++) {
        int leftPos = itGROUPS->first;

        if ((int)maxGroup.first > leftPos)
            maxGroup.first = leftPos;
    }

    //bool bCanDisable = !( ((int)maxGroup.first) <= m_iLastGroupRightBorder - 1 );
    //bool bCanDisable = true;
    bool bCanDisable = !((m_MaxLastGroup.first < (int)maxGroup.first) && ((int)maxGroup.first <= m_MaxLastGroup.second - 1)) ||
                            (m_MaxLastGroup.first == -1);

    //m_iLastGroupRightBorder = maxGroup.second;
    m_MaxLastGroup = maxGroup;

    yset<CPendingShift>::iterator it_shifts = m_PendingShifts.begin();
    for (; it_shifts != m_PendingShifts.end(); it_shifts++) {
        const CPendingShift& pendingShift = *it_shifts;
        CStateNode& stateNode = m_StateNodes[pendingShift.m_StateNodeNo];

        int iDeletedCount = 0;

        if (stateNode.m_SymbolChildren.size() > 0) {
            for (int i = stateNode.m_SymbolChildren.size() - 1; i >= 0; i--) {
                int iSymbolNode = stateNode.m_SymbolChildren[i];
                //if( m_SymbolNodes[iSymbolNode].m_bDisabled )
                //  iDeletedCount++;
                CSymbolNode& symbolNode = m_SymbolNodes[iSymbolNode];
                if ((symbolNode.m_SymbolNo == m_WORDTerminal) && bCanDisable) {
                    symbolNode.m_bDisabled = true;
                    iDeletedCount++;
                }

                if ((symbolNode.m_SymbolNo == m_GROUPNonTerminal) && bCanDisable) {
                    if (symbolNode.m_InputStart > maxGroup.first) {
                        symbolNode.m_bDisabled = true;
                        iDeletedCount++;
                    }
                }

            }
            if ((int)stateNode.m_SymbolChildren.size() == iDeletedCount)
                stateNode.m_SymbolChildren.clear();
        }
    }

}

void CGLRParser::DisableWORDSSrtateNodesOld()
{
    //return;
    yset<size_t> disabledStateNode;

    TPair<size_t,size_t> maxGroup;

    maxGroup  = *m_BuiltGROUPs.begin();
    yset< TPair<size_t,size_t> >::iterator itGROUPS = m_BuiltGROUPs.begin();
    for (; itGROUPS != m_BuiltGROUPs.end(); itGROUPS++) {
        int leftPos = itGROUPS->first;

        if ((int)maxGroup.first > leftPos)
            maxGroup.first = leftPos;
    }

    //bool bCanDisable = !( ((int)maxGroup.first) <= m_iLastGroupRightBorder - 1 );
    bool bCanDisable = true;

    //for( ; it != m_ActiveNodes.end() ; it++ )
    //{
    //  int iStatetNode = *it;
    //  CStateNode& stateNode = m_StateNodes[iStatetNode];
    //  for( int i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
    //  {
    //      int iChildSymbolNode = stateNode.m_SymbolChildren[i];
    //      CSymbolNode& symboleNode = m_SymbolNodes[iChildSymbolNode];
    //      if( (symboleNode.m_SymbolNo == m_SENTNonTerminal ) &&
    //          ( m_BuiltGROUPs.find( MakePair(symboleNode.m_InputStart, symboleNode.m_InputEnd) ) != m_BuiltGROUPs.end() ) )
    //      {
    //          if( ( symboleNode.m_Terminals.find(m_WORDTerminal) != symboleNode.m_Terminals.end()) &&
    //              ( ( symboleNode.m_InputStart == maxGroup.first ) && (symboleNode.m_InputEnd == maxGroup.second) ) )
    //              disabledStateNode.insert(*it);

    //          if( (symboleNode.m_Terminals.size() == 1) && (m_WORDTerminal == *symboleNode.m_Terminals.begin() ) )
    //              disabledStateNode.insert(*it);
    //      }

    //      if(  symboleNode.m_SymbolNo == m_GROUPNonTerminal &&
    //           bCanDisable )
    //      {
    //          //yset< TPair<size_t,size_t> >::iterator itGROUPS = m_BuiltGROUPs.begin();
    //          //maxGroup  = *m_BuiltGROUPs.begin();
    //          if( symboleNode.m_InputStart > maxGroup.first )
    //              disabledStateNode.insert(*it);
    //          //for( ; itGROUPS != m_BuiltGROUPs.end() ; itGROUPS++ )
    //          //{
    //          //  int leftPos = itGROUPS->first;

    //          //  if( maxGroup.first > leftPos)
    //          //      maxGroup.first = leftPos;

    //          //  if( symboleNode.m_InputStart > itGROUPS->first )
    //          //      break;
    //          //}

    //          //if( itGROUPS != m_BuiltGROUPs.end() )
    //          //{
    //          //  disabledStateNode.insert(*it);
    //          //}
    //      }
    //  }
    //}

    ////if( disabledStateNode.size() != 1)
    ////    return;

    //while( !disabledStateNode.empty() )
    //{
    //  yset<size_t>::iterator it = disabledStateNode.begin();
    //  int iStateNode = *it;
    //  disabledStateNode.erase(it);
    //  m_StateNodes[iStateNode].m_bDisabled = true;

    //  for( int j = 0 ; j < m_StateNodes[iStateNode].m_SymbolChildren.size() ; j++ )
    //  {
    //      int iSymbolNode = m_StateNodes[iStateNode].m_SymbolChildren[j];
    //      m_SymbolNodes[iSymbolNode].m_bDisabled = true;
    //  }
    //  yset<size_t>::iterator itRed = m_StateNodes[iStateNode].m_ReducedTo.begin();
    //  for( ; itRed != m_StateNodes[iStateNode].m_ReducedTo.end() ; itRed++ )
    //  {
    //      disabledStateNode.insert(*itRed);
    //  }
    //}

    //m_iLastGroupRightBorder = maxGroup.second;

    //int iPrevLevel =  m_InputLevels[m_InputLevels.size() - 2];

    //CSymbolNode& prevLevelNode = m_StateNodes[iPrevLevel].m_SymbolChildren[0];

    //if( !bCanDisable )
    //  return;

    yset<CPendingShift>::iterator it_shifts = m_PendingShifts.begin();
    for (; it_shifts != m_PendingShifts.end(); it_shifts++) {
        const CPendingShift& pendingShift = *it_shifts;
        CStateNode& stateNode = m_StateNodes[pendingShift.m_StateNodeNo];

        int iDeletedCount = 0;

        if (stateNode.m_SymbolChildren.size() > 0) {
            for (int i = stateNode.m_SymbolChildren.size() - 1; i >= 0; i--) {
                int iSymbolNode = stateNode.m_SymbolChildren[i];
                //if( m_SymbolNodes[iSymbolNode].m_bDisabled )
                //  iDeletedCount++;
                CSymbolNode& symbolNode = m_SymbolNodes[iSymbolNode];
                if ((symbolNode.m_SymbolNo == m_WORDTerminal) && bCanDisable) {
                    symbolNode.m_bDisabled = true;
                    iDeletedCount++;
                }

                if ((symbolNode.m_SymbolNo == m_GROUPNonTerminal) && bCanDisable) {
                    if (symbolNode.m_InputStart > maxGroup.first) {
                        symbolNode.m_bDisabled = true;
                        iDeletedCount++;
                    }
                }

            }
            if ((int)stateNode.m_SymbolChildren.size() == iDeletedCount)
                stateNode.m_SymbolChildren.clear();
        }
    }

    //for( int j = m_SymbolNodes.size() - 1 ; j >= 0 ; j-- )
    //{

    //  if( m_SymbolNodes[j].m_InputEnd < maxGroup.second )
    //      break;

    //  yset< TPair<size_t,size_t> >::iterator itGROUPS = m_BuiltGROUPs.begin();
    //  for( ; itGROUPS != m_BuiltGROUPs.end() ; itGROUPS++ )
    //  {
    //      if( TryToDisableSymbolNode(m_SymbolNodes[j], itGROUPS->first) )
    //          m_SymbolNodes[j].m_bDisabled = true;
    //  }
    //}
}

bool CGLRParser::TryToDisableSymbolNode(CSymbolNode& symboleNode, int iStart)
{
    if (symboleNode.m_SymbolNo != m_WORDTerminal)
        return false;

    if (((int)symboleNode.m_InputStart == iStart)) {
        return true;
    } else {
        for (int i = 0; i < (int)symboleNode.m_NodeChildren.size(); i++) {
            CStateNode& stateNode = m_StateNodes[symboleNode.m_NodeChildren[i]];
            for (int j = 0; j < (int)stateNode.m_SymbolChildren.size(); j++) {
                if (TryToDisableSymbolNode(m_SymbolNodes[stateNode.m_SymbolChildren[j]], iStart))
                    return true;
            }
        }
    }

    return false;
}

//void CGLRParser::DisableWORDSSrtateNodesOld()
//{
//  //return;
//  yset<size_t>::iterator it = m_ActiveNodes.begin();
//  //yset<size_t> disabledStateNode;
//  yset< TPair<int, int> > disabledStateNode2Children;
//  yset< TPair<int, int> > notDisabledStateNode2Children;
//  yset<size_t> groupStateNodes;

//  for( ; it != m_ActiveNodes.end() ; it++ )
//  {
//      int iStatetNode = *it;
//      CStateNode& stateNode = m_StateNodes[iStatetNode];
//      for( int i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
//      {
//          int iChildSymbolNode = stateNode.m_SymbolChildren[i];
//          CSymbolNode& symboleNode = m_SymbolNodes[iChildSymbolNode];
//          if( ( symboleNode.m_SymbolNo == m_WORDTerminal ) &&
//              ( m_BuiltGROUPs.find( MakePair(symboleNode.m_InputStart, symboleNode.m_InputEnd) ) != m_BuiltGROUPs.end() ) )
//          {
//              break;
//              //disabledStateNode2Children.insert( MakePair(*it,iChildSymbolNode) );
//              //symboleNode.m_bDisabled = true;
//          }

//          if(  symboleNode.m_SymbolNo == m_GROUPNonTerminal )
//          {
//              groupStateNodes.insert(*it);
//              notDisabledStateNode2Children.insert( MakePair(*it,iChildSymbolNode) );
//          }
//          if(  symboleNode.m_SymbolNo == m_GROUPNonTerminal )
//          {
//              yset< TPair<size_t,size_t> >::iterator itGROUPS = m_BuiltGROUPs.begin();
//              for( ; itGROUPS != m_BuiltGROUPs.end() ; itGROUPS++ )
//              {
//                  int leftPos = itGROUPS->first;
//                  if( symboleNode.m_InputStart > itGROUPS->first )
//                      break;
//              }
//              if( itGROUPS != m_BuiltGROUPs.end() )
//              {
//                  disabledStateNode2Children.insert( MakePair(*it,iChildSymbolNode) );
//                  symboleNode.m_bDisabled = true;
//              }
//          }

//      }

//      if( i < stateNode.m_SymbolChildren.size() )
//      {
//          for( i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
//          {
//              int iChildSymbolNode = stateNode.m_SymbolChildren[i];
//              CSymbolNode& symboleNode = m_SymbolNodes[iChildSymbolNode];
//              disabledStateNode2Children.insert( MakePair(*it,iChildSymbolNode) );
//              symboleNode.m_bDisabled = true;
//          }
//          //disabledStateNode.insert(stateNode.m_ReducedTo.begin(), stateNode.m_ReducedTo.end());
//      }

//  }

//  while( !groupStateNodes.empty() )
//  {
//      yset<size_t>::iterator it = groupStateNodes.begin();
//      size_t iStateNode = *it;
//      groupStateNodes.erase(it);
//      CStateNode& stateNode = m_StateNodes[iStateNode];
//      bool bAdd = false;
//      for( int i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
//      {
//          int iChildSymbolNode = stateNode.m_SymbolChildren[i];
//          CSymbolNode& symboleNode = m_SymbolNodes[iChildSymbolNode];
//          TPair<int,int> s2c = MakePair(iStateNode,iChildSymbolNode);
//          if(  notDisabledStateNode2Children.find(s2c) == notDisabledStateNode2Children.end() )
//          {
//              notDisabledStateNode2Children.insert( s2c);
//              bAdd = true;
//          }
//      }
//      if( bAdd )
//          groupStateNodes.insert(stateNode.m_ReducedTo.begin(), stateNode.m_ReducedTo.end());
//  }

//  notDisabledStateNode2Children.clear();

//  //while( !disabledStateNode.empty() )
//  //{
//  //  yset<size_t>::iterator it = disabledStateNode.begin();
//  //  size_t iStateNode = *it;
//  //  disabledStateNode.erase(it);
//  //  CStateNode& stateNode = m_StateNodes[iStateNode];
//  //  bool bAdd = false;
//  //  for( int i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
//  //  {
//  //      int iChildSymbolNode = stateNode.m_SymbolChildren[i];
//  //      CSymbolNode& symboleNode = m_SymbolNodes[iChildSymbolNode];
//  //      if( disabledStateNode2Children.find(symboleNode.m_PathTop) != disabledStateNode2Children.end() )
//  //      {
//  //          if( !symboleNode.m_bDisabled )
//  //          {
//  //              disabledStateNode2Children.insert( MakePair(iStateNode,iChildSymbolNode) );
//  //              symboleNode.m_bDisabled = true;
//  //              bAdd = true;
//  //          }
//  //      }
//  //  }
//  //  if( bAdd )
//  //      disabledStateNode.insert(stateNode.m_ReducedTo.begin(), stateNode.m_ReducedTo.end());
//  //}

//  while( !disabledStateNode2Children.empty() )
//  {
//      yset< TPair<int, int> >::iterator itDisabled = disabledStateNode2Children.begin();
//      TPair<int, int> pathTop = *itDisabled;
//      disabledStateNode2Children.erase(itDisabled);

//      it = m_ActiveNodes.begin();
//      for( ; it != m_ActiveNodes.end() ; it++ )
//      {
//          CStateNode& stateNode = m_StateNodes[*it];
//          for( int i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
//          {
//              int iChildSymbolNode = stateNode.m_SymbolChildren[i];
//              CSymbolNode& symboleNode = m_SymbolNodes[iChildSymbolNode];
//              if( (symboleNode.m_PathTop == pathTop) &&
//                  (notDisabledStateNode2Children.find(pathTop) == notDisabledStateNode2Children.end())  )
//              {
//                  if( !symboleNode.m_bDisabled )
//                  {
//                      disabledStateNode2Children.insert( MakePair(*it,iChildSymbolNode) );
//                      symboleNode.m_bDisabled = true;
//                  }
//              }
//          }
//      }
//  }
//}
/*
void CGLRParser::DisableStateNode(CStateNode& stateNode)
{
    for( int i = 0 ; i < stateNode.m_SymbolChildren.size() ; i++ )
    {
        CSymbolNode& symboleNode = m_SymbolNodes[stateNode.m_SymbolChildren[i]];
        if( symboleNode.m_SymbolNo == m_WORDSTerminal )
        {
            if( m_BuiltGROUPs.find( MakePair(symboleNode.m_InputStart, symboleNode.m_InputEnd) ) != m_BuiltGROUPs.end() )
            {
                //stateNode.m_bDisabled = true;
                break;
            }
        }
    }
}*/

void CGLRParser::ReduceOnePath(const yset<int>& Symbols, const CPendingReduction& Reduction, const yvector<size_t>& Path)
{
    size_t LeftPart = Reduction.m_pReduceRule->m_LeftPart;
    //YASSERT(2 * RuleLength + 1 == Path.size());

    // ChildNodeNo  is the first node which should be reduced
    size_t ChildNodeNo = Path.back();
    const CStateNode& ChildNode = m_StateNodes[ChildNodeNo];

    //const CSLRCell& Cell = m_pTable->m_Table[ChildNode.m_StateNo][LeftPart];
    YASSERT(!m_pTable->IsEmptyCell(ChildNode.m_StateNo, LeftPart));
    ui32 encoded = m_pTable->GetEncodedCell(ChildNode.m_StateNo, LeftPart);

    ui32 gotoLine = 0;
    bool hasGotoLine = m_pTable->GetGotoLine(encoded, gotoLine);
    YASSERT(hasGotoLine);

    size_t NewNodeNo;
    yvector<CInputItem*> rightPartItems;
    FillRightPartItems(rightPartItems, Path);
    size_t iInputStart = m_SymbolNodes[Path[Path.size()-2]].m_InputStart;
    size_t iInputEnd = m_SymbolNodes[Path[1]].m_InputEnd;
    CInputItem* pNewItem = m_pInputItemFactory->CreateNewItem(LeftPart, Reduction.m_pReduceRule->m_RuleAgreement,
        iInputStart,  iInputEnd, Reduction.m_pReduceRule->m_SynMainItemNo, rightPartItems);

    // не прошло согласование
    if (pNewItem == NULL)
        return;

    m_iCreatedGroups += pNewItem->GetFactsCount();
    if (LeftPart == (size_t)m_GROUPNonTerminal) {
        m_bDisableWORDSStateNodes = true;
        m_BuiltGROUPs.insert(MakePair(iInputStart, iInputEnd));
    }

    if (CreateOrReuseNode(gotoLine, NewNodeNo))
        //  if a node was created then we should add it to ActiveNodes in order to call Actor
        //  for it on the next iteration (and to add next pending reductions  and shifts)6
        m_ActiveNodes.insert(NewNodeNo);

    int iReducedNode = *Path.begin();

    size_t NewSymbolNode;

    CSymbolNode SN;
    SN.m_InputStart = m_SymbolNodes[Path[Path.size()-2]].m_InputStart;
    SN.m_InputEnd = m_SymbolNodes[Path[1]].m_InputEnd;
    AddTerminals(Path, SN);
    if (!HasPathOfLengthTwo(pNewItem,  NewNodeNo, ChildNodeNo, NewSymbolNode, SN)) {
        SN.AddInputItem(pNewItem);
        SN.m_SymbolNo = LeftPart;
        SN.m_NodeChildren.push_back(ChildNodeNo);
        SN.m_PathTop = MakePair(iReducedNode, Path[1]);
        m_SymbolNodes.push_back(SN);

        NewSymbolNode = m_SymbolNodes.size() - 1;
        m_StateNodes[NewNodeNo].m_SymbolChildren.push_back(NewSymbolNode);
        //int childSymbolNode = (int)m_StateNodes[NewNodeNo].m_SymbolChildren.size() - 1;

        m_StateNodes[iReducedNode].m_ReducedTo.insert(NewNodeNo);
        //m_StateNodes[iReducedNode].m_ReducedTo.insert( MakePair(NewNodeNo, childSymbolNode) );

        /*
            if NewNodeNo is not in m_ActiveNodes, it means that it was already created before
            we have got in this  procedure. And it means that for this node
            on the next iteration the Actor would be called.
            Since we have just created a new path from NewNode we should reprocess all reductions
            from this node, since some reductions can use this new path.
        */
        if (m_ActiveNodes.find(NewNodeNo) == m_ActiveNodes.end())
            for (yset<int>::const_iterator it = Symbols.begin(); it != Symbols.end(); it++) {
                if (!m_pTable->IsEmptyCell(gotoLine, *it)) {
                    ui32 encodedNext = m_pTable->GetEncodedCell(gotoLine, *it);
                    if (m_pTable->IsSimpleCell(encodedNext))
                        m_PendingReductions.insert(CPendingReduction(&m_pTable->GetSimpleCellRule(encodedNext), NewSymbolNode, NewNodeNo));
                    else for (CGLRTable::TRuleIter ruleIter = m_pTable->IterComplexCellRules(encodedNext); ruleIter.Ok(); ++ruleIter)
                        m_PendingReductions.insert(CPendingReduction(&*ruleIter, NewSymbolNode, NewNodeNo));
                }
            }
    } else
        m_SymbolNodes[NewSymbolNode].AddInputItem(pNewItem);

    //  storing parse tree
    {
        m_SymbolNodes[NewSymbolNode].m_ParseChildren.push_back();
        CParsingChildrenSet& S = m_SymbolNodes[NewSymbolNode].m_ParseChildren.back();
        S.m_ReduceRule = Reduction.m_pReduceRule;
        for (int i = Path.size() - 2; i > 0; i-=2)
            S.m_Items.push_back(Path[i]);
    }
}

// This  function is written mostly for non-recursive depth-first search in GSS (see Internet for documentation).
// when a path of  length 2*RuleLength-1 is found the reduction action (procedure ReduceOnePath)
// is called.
// Here is description of this procedure.
// We use DFS-stack which is a sequence of pairs
// <first[1],end[1]>, <first[2],end[2]>,...,<first[n],end[n]>,  where  for each 1<i<=n,
//  <first[i],end[i]> are all unobserved children of node first[i-1]. Each iteration we made
// the following:
// 1. the current  path is not long enough  and  the current top of the DFS stack has children
//  push to DFS stack all children of current top.
// 2. otherwise  we go to the neighbour or to the parent's neighbour of the current top, if a neighbour doesn't exist

//void CGLRParser::Reducer(size_t SymbolNo)
void CGLRParser::Reducer(const yset<int>& Symbols)
{

    assert (!m_PendingReductions.empty());
    assert (m_ActiveNodes.empty());

    CPendingReduction Reduction = *m_PendingReductions.begin();
    m_PendingReductions.erase(m_PendingReductions.begin());
    size_t RuleLength = Reduction.m_pReduceRule->m_RuleLength;

    yvector< size_t > Path;
    yvector< size_t > Starts;
    {
        // adding the first item of the DFS-stack
        Path.push_back(Reduction.m_LastStateNodeNo);
        Starts.push_back(0); // the first value of Starts[0] is unused
        Path.push_back(Reduction.m_SymbolNodeNo);
        Starts.push_back(0);
    }

    do
    {
        assert (Path.size() > 1);
        size_t d = Path.back();
        /*
            if the length of path is odd then the last element is a symbol node
            else it is a state node.
        */

        if (Path.size() == 2*RuleLength+1) {
            ReduceOnePath(Symbols, Reduction, Path);
        };

        const yvector<size_t>& children = (Path.size()%2==0) ? m_SymbolNodes[d].m_NodeChildren : m_StateNodes[d].m_SymbolChildren;
        if  ((Path.size() <= 2*RuleLength)
                &&  !children.empty()
            ) {
            Path.push_back(*(children.begin()));
            Starts.push_back(0);
        } else {
            while (Path.size () > 1) {
                d = Path.back();
                const yvector<size_t>& curr_children = (Path.size()%2==0) ? m_SymbolNodes[d].m_NodeChildren : m_StateNodes[d].m_SymbolChildren;

                Starts.back()++;
                if (Starts.back() < curr_children.size()) {
                    Path.push_back(curr_children[Starts.back()]);
                    Starts.push_back(0);
                    break;
                };

                Path.erase(Path.end()-1);
                Starts.erase(Starts.end()-1);

            }

        };
    }
    while (Path.size () != 1);
};

/*
    SHIFTER(i)
    while (m_PendingShifts != 0) {
        remove an element (v,k) from m_PendingShifts
        if there is no node, w,  labelled k  in the last input related closure create one
        if w does not have a successor mode,u, labelled SymbolNo create one
        if u is not already a predecessor of v, make it one
    };

*/

void CGLRParser::Shifter(const yset<int>& Symbols)
{
    (void)Symbols;
    while (!m_PendingShifts.empty()) {
        // == remove an element (m_StateNodeNo, m_NextStateNo) from m_PendingShifts

        CPendingShift Shift = *m_PendingShifts.begin();

        m_PendingShifts.erase(m_PendingShifts.begin());

        CStateNode& stateNode = m_StateNodes[Shift.m_StateNodeNo];
        if (stateNode.m_SymbolChildren.size() == 0 && m_bDisableWORDSStateNodes &&
            m_InputLevels.size() > 2)
            continue;

        // == if there is no node, w,  labelled m_NextStateNo  in m_InputLevels.back() create one

        size_t NewNode;
        CreateOrReuseNode(Shift.m_NextStateNo, NewNode);
        CStateNode& Node = m_StateNodes[NewNode];

        // == if w does not have a successor mode,u, labelled SymbolNo create one
        size_t SymbolNo = Shift.m_SymbolNo;

        //yvector<size_t>::const_iterator it = Node.m_SymbolChildren.end();

        yvector<size_t>::const_iterator it = Node.m_SymbolChildren.begin();
        for (; it != Node.m_SymbolChildren.end(); it++)
            if ((size_t)m_SymbolNodes[*it].m_SymbolNo == SymbolNo)
                break;

        int SymbolNode;
        if (it == Node.m_SymbolChildren.end()) {
            //  creating new symbol node
            CSymbolNode SN;
            SN.m_SymbolNo = SymbolNo;
            SN.m_InputStart = m_InputLevels.size() - 2;
            SN.m_InputEnd = m_InputLevels.size() - 1;
            SN.AddInputItem(m_pInputItemFactory->CreateNewItem(SymbolNo, SN.m_InputStart));
            if (NULL == SN.m_pItem) continue; //nim : 21.10.04
            m_SymbolNodes.push_back(SN);
            SymbolNode = m_SymbolNodes.size() - 1;
            Node.m_SymbolChildren.push_back(SymbolNode);
        } else
            //  reusing old symbol node
            SymbolNode = *it;

        // == if u is not already a predecessor of v, make it one
        if (m_SymbolNodes[SymbolNode].FindChild(Shift.m_StateNodeNo) == -1)
            m_SymbolNodes[SymbolNode].m_NodeChildren.push_back(Shift.m_StateNodeNo);
    }
}

size_t CGLRParser::GetNumberOfClosureSet(size_t StateNodeNo) const
{
    for (size_t i=0; i<m_InputLevels.size(); i++) {
        size_t start = 0;
        if (i >0)
            start = m_InputLevels[i-1];
        size_t  end = m_InputLevels[i];
        if ((StateNodeNo >= start)
                &&  (StateNodeNo < end)
            )
            return i;
    };
    return m_InputLevels.size();
}

/*
return true if the main symbol of the grammar was recognized from the
start of the input
*/
bool    CGLRParser::HasGrammarRootAtStart() const
{
    for (size_t i=0; i< m_SymbolNodes.size();i++)
        if  (m_pTable->m_pWorkGrammar->m_UniqueGrammarItems[m_SymbolNodes[i].m_SymbolNo].m_bGrammarRoot
                &&  m_SymbolNodes[i].m_InputStart == 0
            )
        return true;

    return false;
};

void CGLRParser::AddTerminals(const yvector<size_t>& Path, CSymbolNode& newNode)
{
    newNode.m_Terminals.clear();
    for (int i=Path.size()-2; i>0; i-=2) {
        const CSymbolNode& symbolNode = m_SymbolNodes[Path[i]];
        if (symbolNode.IsLeaf())
            newNode.m_Terminals.push_back(symbolNode.m_SymbolNo);
        else
            newNode.m_Terminals.insert(newNode.m_Terminals.end(), symbolNode.m_Terminals.begin(), symbolNode.m_Terminals.end());
    }
}

void CSymbolNode::Free()
{
    delete m_pItem;
    for (int i = 0; i < (int)m_OtherItemVariants.size(); i++)
        delete m_OtherItemVariants[i];
}
