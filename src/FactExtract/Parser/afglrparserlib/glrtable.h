#pragma once

#include <util/generic/vector.h>
#include <library/iter/iter.h>

#include <FactExtract/Parser/common/serializers.h>

#include <kernel/gazetteer/common/serialize.h>
#include <kernel/gazetteer/common/compactstorage.h>


#include "commontypes.h"
#include "agreement.h"

struct CWorkGrammar;

struct CGLRRuleInfo
{
    ui32 m_OriginalRuleNo;
    ui32 m_RuleLength;
    ui32 m_LeftPart;
    ui32 m_SynMainItemNo;

    CRuleAgreement m_RuleAgreement; //agreement vector for the rule with agreement function pointers;

    void Init(const CWorkRule*);

    bool operator < (const CGLRRuleInfo& _X) const {
        return m_OriginalRuleNo < _X.m_OriginalRuleNo;
    }

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    void Swap(CGLRRuleInfo& a) {
        if (&a != this) {
            ::DoSwap(m_OriginalRuleNo, a.m_OriginalRuleNo);
            ::DoSwap(m_RuleLength, a.m_RuleLength);
            ::DoSwap(m_LeftPart, a.m_LeftPart);
            ::DoSwap(m_SynMainItemNo, a.m_SynMainItemNo);
            ::DoSwap(m_RuleAgreement, a.m_RuleAgreement);
        }
    }
};

// used temporarily to construct raw GLR-table
struct CSLRCell
{
    bool m_bShift;
    int m_GotoLine;
    yvector<const CGLRRuleInfo*> m_ReduceRules;

    CSLRCell()
        : m_bShift(false)
        , m_GotoLine(-1)
    {
    }

    bool IsEmpty() const {
        return m_GotoLine == -1 && !m_bShift && m_ReduceRules.empty();
    }
};

class TCompactSLRCell {

    static const ui32 Empty = ~static_cast<ui32>(1);    // 111...110
public:

    // for serialization
    TCompactSLRCell()
        : ShiftGoto(Empty), RulesIndex(0)
    {
    }

    TCompactSLRCell(bool shift, int gotoLine, ui32 rulesIndex)
        : ShiftGoto((gotoLine >= 0) ? (gotoLine << 1) : Empty)
        , RulesIndex(rulesIndex)
    {
        YASSERT(!shift || gotoLine >= 0);       // (shift && gotoLine == -1) is not allowed
        if (shift)
            ShiftGoto |= 1;
    }

    inline bool HasGotoLine() const {
        return (ShiftGoto & Empty) != Empty;
    }

    inline ui32 GetGotoLine() const {
        return ShiftGoto >> 1;
    }

    inline ui32 GetRulesIndex() const {
        return RulesIndex;
    }

    void Save(TOutputStream* output) const
    {
        ::Save(output, ShiftGoto);
        ::Save(output, RulesIndex);
    }

    void Load(TInputStream* input)
    {
        ::Load(input, ShiftGoto);
        ::Load(input, RulesIndex);
    }

private:
    ui32 ShiftGoto;     // (GotoLine << 1) | Shift
    ui32 RulesIndex;
};

template <class T>
class TSparseMatrix {
    // Implementation of Yale Sparse Matrix Format (see http://en.wikipedia.org/wiki/Sparse_matrix for details)
    // Elements can be added only in accending order of (row, column) pair
public:
    TSparseMatrix() {
        RowIndex.push_back(0);
    }

    bool IsEmpty() const {
        return Values.empty();
    }

    void Add(ui32 row, ui32 column, T value) {
        size_t newRows = row + 1;
        YASSERT(newRows >= RowIndex.size() || (newRows == RowIndex.size() - 1 && (ColumnIndex.empty() || column > ColumnIndex.back())));
        Values.push_back(value);
        ColumnIndex.push_back(column);
        while (RowIndex.size() <= newRows)
            RowIndex.push_back(RowIndex.back());
        RowIndex.back() = Values.size();
    }
/*
    bool Has(ui32 row, ui32 column) const {
        ui32 beg = RowIndex[row];
        ui32 end = RowIndex[row + 1];
        return std::binary_search(ColumnIndex.begin() + beg, ColumnIndex.begin() + end, column);
    }
*/
    const T& Get(ui32 row, ui32 column) const {
        ui32 beg = RowIndex[row];
        ui32 end = RowIndex[row + 1];
        yvector<ui32>::const_iterator it = std::lower_bound(ColumnIndex.begin() + beg, ColumnIndex.begin() + end, column);
        size_t index = it - ColumnIndex.begin();
        YASSERT(index < ColumnIndex.size() && ColumnIndex[index] == column);
        return Values[index];
    }

    void Save(TOutputStream* output) const
    {
        ::SaveProtected(output, Values);
        ::SaveProtected(output, RowIndex);
        ::SaveProtected(output, ColumnIndex);
    }

    void Load(TInputStream* input)
    {
        ::LoadProtected(input, Values);
        ::LoadProtected(input, RowIndex);
        ::LoadProtected(input, ColumnIndex);
    }

private:
    yvector<T> Values;
    yvector<ui32> RowIndex;
    yvector<ui32> ColumnIndex;
};

class CGLRTable {
public:
    CGLRTable()
        : m_pWorkGrammar(NULL)
        , Rows(0), Columns(0)
    {
    }

    bool BuildGLRTable(Stroka GrammarFileName = "");
    const CGLRRuleInfo* GetRuleInfo (const CWorkRule*) const;
    void SwapRuleAgreements(yvector<CRuleAgreement>& agreements);

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);

    bool IsEmpty() const {
        return Rows == 0;
    }

    bool IsEmptyCell(size_t row, size_t column) const {
        YASSERT(row < Rows && column < Columns);
        size_t idx = row*Columns + column;
        return (Mask[idx >> 3] & (1 << (idx & 7))) == 0;
    }

    ui32 GetEncodedCell(size_t row, size_t column) const {
        //const ui32 zero = 0;
        //yvector<TPair< TPair<ui32, ui32>, ui32> >::const_iterator it =
        //    std::lower_bound(SimpleCells.begin(), SimpleCells.end(), MakePair(MakePair(row, column), zero));
        //YASSERT(it != SimpleCells.end());
        //return it->second;
        return SimpleCells.Get(row, column);
    }

    bool IsSimpleCell(ui32 encodedCell) const {
        return (encodedCell & 1) == 0;
    }

    bool GetGotoLine(ui32 encodedCell, ui32& gotoLine) const {
        if (!IsSimpleCell(encodedCell)) {
            const TCompactSLRCell& complex = ComplexCells[DecodeIndex(encodedCell)];
            if (complex.HasGotoLine()) {
                gotoLine = complex.GetGotoLine();
                return true;
            }
        }
        return false;
    }

    const CGLRRuleInfo& GetSimpleCellRule(ui32 encodedCell) const {
        YASSERT(IsSimpleCell(encodedCell));
        return GetRuleByIndex(DecodeIndex(encodedCell));
    }

    // iterator over rule indexes from m_RuleInfos
    typedef NGzt::TCompactStorage<ui32>::TIter TRuleIndexIter;
    // mapping from rule indexes to rule pointers
    friend class NIter::TMappedIterator<CGLRRuleInfo, TRuleIndexIter, CGLRTable>;    // to access operator()
    typedef NIter::TMappedIterator<CGLRRuleInfo, TRuleIndexIter, CGLRTable> TRuleIter;

    TRuleIter IterComplexCellRules(ui32 encodedCell) const {
        YASSERT(!IsSimpleCell(encodedCell));
        const TCompactSLRCell& complex = ComplexCells[DecodeIndex(encodedCell)];
        return TRuleIter(RuleIndexStorage[complex.GetRulesIndex()], this);
    }

private:
    void Compact(const yvector< yvector<CSLRCell> >& table);
    bool Verify(const yvector< yvector<CSLRCell> >& table) const;
    ui32 GetRuleIndex(const CGLRRuleInfo* rule) const;

    const CGLRRuleInfo& GetRuleByIndex(ui32 index) const {
        return m_RuleInfos[index];
    }

    // for TRuleIter to use @this as mapper from rule index to rule pointer
    inline const CGLRRuleInfo& operator()(ui32 index) const {
        return GetRuleByIndex(index);
    }

    ui32 DecodeIndex(ui32 encodedCell) const {
        // for simple cell it is an index from m_RuleInfos,
        // otherwise it is an index from RuleIndexStorage.
        return encodedCell >> 1;
    }

public:
    const CWorkGrammar* m_pWorkGrammar;
    yvector<CGLRRuleInfo> m_RuleInfos;

private:
    size_t Rows, Columns;

    yvector<ui8> Mask;                          // bit mask for all cells: empty and non-empty
    TSparseMatrix<ui32> SimpleCells;            // non-empty cells: (row, column) -> index from m_RuleInfos or index from ComplexCells (defined by highest bit)
    yvector<TCompactSLRCell> ComplexCells;      // non-empty cells with custom content: either several reduce rules or (Shift, GotoLine) != (false, -1)
    NGzt::TCompactStorage<ui32> RuleIndexStorage;

};
