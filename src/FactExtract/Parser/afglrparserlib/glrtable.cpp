#include <util/system/defaults.h>
#include <util/stream/buffer.h>

#include "glrtable.h"
#include "simplegrammar.h"
#include "lr-items.h"

#include <FactExtract/Parser/common/pathhelper.h>


void CGLRRuleInfo::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_OriginalRuleNo);
    ::Save(buffer, m_RuleLength);
    ::Save(buffer, m_LeftPart);
    ::Save(buffer, m_SynMainItemNo);
    ::Save(buffer, m_RuleAgreement);
}

void CGLRRuleInfo::Load(TInputStream* buffer)
{
    ::Load(buffer, m_OriginalRuleNo);
    ::Load(buffer, m_RuleLength);
    ::Load(buffer, m_LeftPart);
    ::Load(buffer, m_SynMainItemNo);
    ::Load(buffer, m_RuleAgreement);
}

/*--------------------------------class CGLRTable------------------------------------------*/

const CGLRRuleInfo* CGLRTable::GetRuleInfo (const CWorkRule* R) const
{
    CGLRRuleInfo I;
    I.m_OriginalRuleNo = R->m_OriginalRuleNo;
    yvector<CGLRRuleInfo>::const_iterator it = std::lower_bound(m_RuleInfos.begin(), m_RuleInfos.end(), I);
    YASSERT(it != m_RuleInfos.end());
    YASSERT(it->m_OriginalRuleNo == R->m_OriginalRuleNo);
    return &*it;
}

static void BuildRuleInfo(const yset<CWorkRule>& rules, yvector<CGLRRuleInfo>& res)
{
    res.clear();
    for (yset<CWorkRule>::const_iterator it = rules.begin(); it != rules.end(); ++it) {
        res.push_back();
        res.back().Init(&*it);
    }
    std::sort(res.begin(), res.end());
}

/*
Constructing the SLR parsing table
 * Compute the canonical LR collection set of LR(0) items for grammr G
   let it be C = {I0, I1,...,In}
 * For terminal a, if A->X.aY in Ii and goto(Ii,a) = Ij, then set action[i,a] to shift j
 * if A->X. is in Ii and A != S',  for all terminals  a in Follow(A) set action [i,a]
   to  reduce A->X (S' is a new specially  inserted root of the grammar)
 * if S'->S. is in Ii, then set action[i,$]=accept (this is not implemented because we use
   this grammar for longest match search)
 * For non-terminal symbol A, if goto(Ii,A) = Ij, then set goto(i,A)=j
 * set all other table entries to "error"

 Since we should use this table for GLR, we should not stop when
 a conflict occurrs, to the contrary we should proceed collection all possible
 actions in one table  cell.
*/

bool CGLRTable::BuildGLRTable(Stroka /*GrammarFileName*/)
{
    YASSERT(m_pWorkGrammar != NULL);

    // initializing m_RuleInfos, in order to be independent
    // from class CWorkGrammar while  parsing
    BuildRuleInfo(m_pWorkGrammar->m_EncodedRules, m_RuleInfos);

    // creating a canonical LR(0) index item set collection
    CLRCollectionSet S;
    S.Compute(m_pWorkGrammar);

    yvector< yvector<CSLRCell> > table(S.m_ItemSets.size());

    // building @table by CLRCollectionSet
    for (size_t i = 0; i < S.m_ItemSets.size(); ++i) {
        const yset<CLRItem>& items = S.m_ItemSets[i];

        // tableLine is a new table line in SLR table, which should  be added to the end of the table
        // we resize it to contain all symbols
        yvector<CSLRCell>& tableLine = table[i];
        tableLine.resize(m_pWorkGrammar->m_UniqueGrammarItems.size());

        // initializing shift actions and reduce actions
        for (yset<CLRItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
            // "For terminal a, if A->X.aY in Ii and goto(Ii,a) = Ij, then set action[i,a] to shift j"
            if  (!it->IsFinished() && m_pWorkGrammar->m_UniqueGrammarItems[it->GetSymbolNo()].m_Type != siMeta) {
                int SymbolNo = it->GetSymbolNo();
                size_t NextState;
                if (S.GetGotoValue(SymbolNo, i, NextState) && (size_t)tableLine[SymbolNo].m_GotoLine != NextState) {
                    YASSERT(tableLine[SymbolNo].m_GotoLine == -1);
                    YASSERT(!tableLine[SymbolNo].m_bShift);
                    tableLine[SymbolNo].m_bShift = true;
                    tableLine[SymbolNo].m_GotoLine = NextState;
                }
            }
            // if A->X. is in Ii and A != S',  for all terminals  a in Follow(A) set action [i,a]
            // to  reduce A->X
            if  (it->IsFinished() && it->m_pRule->m_LeftPart != m_pWorkGrammar->GetNewRoot()) {
                const yset<size_t>& follow = m_pWorkGrammar->m_FollowSets[it->m_pRule->m_LeftPart];
                for (yset<size_t>::const_iterator it1 = follow.begin(); it1 != follow.end(); ++it1)
                    tableLine[*it1].m_ReduceRules.push_back(GetRuleInfo(it->m_pRule));
            }
        }
        //  going  through non-terminal symbols, initializing GOTO table  for non terminals
        for (size_t symbolNo = 0; symbolNo < m_pWorkGrammar->m_UniqueGrammarItems.size(); ++symbolNo)
            if (m_pWorkGrammar->m_UniqueGrammarItems[symbolNo].m_Type == siMeta) {
                size_t nextState;
                if (S.GetGotoValue(symbolNo, i, nextState)) {
                    YASSERT(tableLine[symbolNo].m_GotoLine == -1);
                    tableLine[symbolNo].m_GotoLine = nextState;
                }
            }
    }

    Compact(table);
    return true;
};

void CGLRTable::SwapRuleAgreements(yvector<CRuleAgreement>& agreements)
{
    for (size_t i = 0; i < m_RuleInfos.size(); ++i) {
        CRuleAgreement& A = agreements[m_RuleInfos[i].m_OriginalRuleNo];
        if (!A.empty())
            m_RuleInfos[i].m_RuleAgreement.swap(A);
    }
}

void CGLRTable::Save(TOutputStream* buffer) const
{
    ::SaveProtectedSize(buffer, Rows);
    ::SaveProtectedSize(buffer, Columns);

    ::SaveProtected(buffer, m_RuleInfos);
    ::SaveProtected(buffer, Mask);
    ::Save(buffer, SimpleCells);    // already protected
    ::SaveProtected(buffer, ComplexCells);
    ::Save(buffer, RuleIndexStorage);
}

void CGLRTable::Load(TInputStream* buffer)
{
    Rows = ::LoadProtectedSize(buffer);
    Columns = ::LoadProtectedSize(buffer);

    ::LoadProtected(buffer, m_RuleInfos);
    ::LoadProtected(buffer, Mask);
    ::Load(buffer, SimpleCells);    // already protected
    ::LoadProtected(buffer, ComplexCells);
    ::Load(buffer, RuleIndexStorage);
}

static inline void CheckIndexOverflow(size_t index) {
    const size_t MAX_INDEX = Max<ui32>() >> 1;     // one bit is reserved for compacting
    if (index > MAX_INDEX)
        ythrow yexception() << "GLR-Table compacting: maximum index reached (" << index << ").";
}

inline ui32 CGLRTable::GetRuleIndex(const CGLRRuleInfo* rule) const {
    size_t index = rule - m_RuleInfos.begin();
    CheckIndexOverflow(index);
    return index;
}

void CGLRTable::Compact(const yvector< yvector<CSLRCell> >& table)
{
    Mask.clear();
    if (table.empty())
        return;

    Rows = table.size();
    Columns = table[0].size();
    YASSERT(Columns != 0);

    NGzt::TCompactStorageBuilder<ui32> ruleIndexStorageBuilder;
    yvector<ui32> tmp;

    Mask.resize((Rows * Columns - 1)/8 + 1);
    for (size_t row = 0; row < table.size(); ++row) {
        for (size_t column = 0; column < table[row].size(); ++column) {
            const CSLRCell& cell = table[row][column];
            if (cell.IsEmpty())
                continue;

            size_t index = row*Columns + column;
            Mask[index >> 3] |= 1 << (index & 7);
            if (cell.m_ReduceRules.size() == 1 && !cell.m_bShift && cell.m_GotoLine == -1)
                // lowest bit 0 means the rest value is already rule index (without encoding in RuleIndexStorage)
                //SimpleCells.push_back(MakePair(MakePair(row, column), GetRuleIndex(cell.m_ReduceRules[0]) << 1));
                SimpleCells.Add(row, column, GetRuleIndex(cell.m_ReduceRules[0]) << 1);
            else {
                tmp.clear();
                for (size_t i = 0; i < cell.m_ReduceRules.size(); ++i)
                    tmp.push_back(GetRuleIndex(cell.m_ReduceRules[i]));
                ui32 rulesIndex = ruleIndexStorageBuilder.AddRange(tmp.begin(), tmp.end());

                // lowest bit 1 means the rest value is already rule index
                //SimpleCells.push_back(MakePair(MakePair(row, column), (ComplexCells.size() << 1) + 1));    // overflow check will be done later
                SimpleCells.Add(row, column, (ComplexCells.size() << 1) + 1);
                ComplexCells.push_back(TCompactSLRCell(cell.m_bShift, cell.m_GotoLine, rulesIndex));
            }
        }
    }
    TBufferStream tmpbuf;
    ruleIndexStorageBuilder.Save(&tmpbuf);
    RuleIndexStorage.Load(&tmpbuf);

    // make sure we are not out of index space:
    CheckIndexOverflow(ComplexCells.size());

    //assert that reverse conversion produces original data
    YASSERT(Verify(table));
}

bool CGLRTable::Verify(const yvector< yvector<CSLRCell> >& table) const {

    yset<const CGLRRuleInfo*> rules1, rules2;
    for (size_t row = 0; row < Rows; ++row)
        for (size_t column = 0; column < Columns; ++column) {
            const CSLRCell& cell = table[row][column];
            YASSERT(cell.IsEmpty() == IsEmptyCell(row, column));
            if (cell.IsEmpty())
                continue;
            ui32 encoded = GetEncodedCell(row, column);
            if (IsSimpleCell(encoded)) {
                YASSERT(cell.m_bShift == false);
                YASSERT(cell.m_GotoLine == -1);
                YASSERT(cell.m_ReduceRules.size() == 1);
                YASSERT(&GetSimpleCellRule(encoded) == cell.m_ReduceRules[0]);
            } else {
                ui32 gotoLine;
                if (GetGotoLine(encoded, gotoLine))
                    YASSERT(cell.m_GotoLine == (int)gotoLine);
                else
                    YASSERT(!cell.m_bShift);
                //YASSERT(cell.m_bShift == ComplexCells[DecodeIndex(encoded)].HasGotoLine());

                // compare reduce rules without ordering
                rules1.clear();
                for (TRuleIter ruleIter = IterComplexCellRules(encoded); ruleIter.Ok(); ++ruleIter)
                    rules1.insert(&*ruleIter);
                rules2.clear();
                rules2.insert(cell.m_ReduceRules.begin(), cell.m_ReduceRules.end());
                YASSERT(rules1 == rules2);
            }
        }
    return true;
}

