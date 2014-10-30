#include <util/system/defaults.h>

#include  "lr-items.h"
#include  "simplegrammar.h"


void AddToItemSet(const CWorkGrammar::TEncodedRulesIndex& rulesIndex, yset<CLRItem>& ItemSet, int SymbolNo)
{
    const yvector<const CWorkRule*>& rules = rulesIndex[SymbolNo];
    for (size_t i = 0; i < rules.size(); ++i)
        ItemSet.insert(CLRItem(rules[i], 0));
}

/*
 this function closes an item set:
 if there is an item of the form A->v*Bw in an item set and  in the grammar there is a rule
 of form B->w' then  the item B->*w' should also be in the item set.
*/
void CloseItemSet(const CWorkGrammar* pWorkGrammar, const CWorkGrammar::TEncodedRulesIndex& rulesIndex, yset<CLRItem>& ItemSet)
{
    size_t SaveSize;
    yset <int> AlreadyAdded;
    do
    {
        SaveSize = ItemSet.size();
        for (yset<CLRItem>::const_iterator it = ItemSet.begin(); it != ItemSet.end(); it++)
            if (!it->IsFinished()) {
                int SymbolNo = it->GetSymbolNo();
                if (pWorkGrammar->m_UniqueGrammarItems[SymbolNo].m_Type == siMeta && AlreadyAdded.find(SymbolNo) == AlreadyAdded.end()) {
                    AddToItemSet(rulesIndex, ItemSet, SymbolNo);
                    AlreadyAdded.insert(SymbolNo);
                }
            }
    }
    while (SaveSize < ItemSet.size());
};

void PrintItemSet(const CWorkGrammar* pWorkGrammar, const yset<CLRItem>& ItemSet)
{
    for (yset<CLRItem>::const_iterator it = ItemSet.begin(); it != ItemSet.end(); it++) {
        Stroka s = pWorkGrammar->GetRuleStr(*it->m_pRule, it->m_DotSymbolNo);
        printf ("%s\n", s.c_str());
    };

};

void CLRCollectionSet::Compute(const CWorkGrammar* pWorkGrammar)
{
    YASSERT(pWorkGrammar != NULL);

    m_GotoFunction.clear();
    m_ItemSets.clear();

    CWorkGrammar::TEncodedRulesIndex rulesIndex;
    pWorkGrammar->IndexEncodedRules(rulesIndex);

    // constructing Item set0
    m_ItemSets.push_back(yset<CLRItem>());
    yset<CLRItem>& ItemSet0 = m_ItemSets.back();
    AddToItemSet(rulesIndex, ItemSet0, pWorkGrammar->GetNewRoot());
    CloseItemSet(pWorkGrammar, rulesIndex, ItemSet0);

    int EOSSymbol = pWorkGrammar->GetEndOfStreamSymbol();

    ymap<size_t, yset<CLRItem> > nextItemSets;

    for (size_t i = 0; i < m_ItemSets.size(); ++i) {
        nextItemSets.clear();
        const yset<CLRItem>& currentItemSet = m_ItemSets[i];
        for (yset<CLRItem>::const_iterator it = currentItemSet.begin(); it != currentItemSet.end(); ++it)
            if (!it->IsFinished() && it->GetSymbolNo() != EOSSymbol)
                nextItemSets[it->GetSymbolNo()].insert(CLRItem(it->m_pRule, it->m_DotSymbolNo + 1));

        for (ymap<size_t, yset<CLRItem> >::iterator it = nextItemSets.begin(); it != nextItemSets.end(); ++it) {
            yset<CLRItem>& newItemSet = it->second;
            CloseItemSet(pWorkGrammar, rulesIndex, newItemSet);
            yvector<yset<CLRItem> >::iterator existing = std::find(m_ItemSets.begin(), m_ItemSets.end(), newItemSet);
            int newStateNo;
            if (existing != m_ItemSets.end())
                newStateNo = existing - m_ItemSets.begin();
            else {
                newStateNo = m_ItemSets.size();
                m_ItemSets.push_back(yset<CLRItem>());
                newItemSet.swap(m_ItemSets.back());
            }
            m_GotoFunction[CStateAndSymbol(it->first, i)] = newStateNo;
        }
    }
}

void CLRCollectionSet::SaveStateCollection(Stroka GrammarFileName, const CWorkGrammar* p_WorkGrammar) const
{
    if (GrammarFileName.empty()) return;
    FILE * fp = fopen(GrammarFileName.c_str(), "wb");

    for (int i = 0; i < (int)m_ItemSets.size(); i++) {
        fprintf(fp, "I%i =\n", i);

        for (yset<CLRItem>::const_iterator it = m_ItemSets[i].begin(); it != m_ItemSets[i].end(); it++) {
            fprintf(fp, "\t%s -> ", p_WorkGrammar->m_UniqueGrammarItems[it->m_pRule->m_LeftPart].m_ItemStrId.c_str());

            for (int j = 0; j < (int)it->m_pRule->m_RightPart.m_Items.size(); j++) {
                if (j == (int)it->m_DotSymbolNo)
                    fprintf(fp, "; ");
                fprintf(fp, "%s ", p_WorkGrammar->m_UniqueGrammarItems[it->m_pRule->m_RightPart.m_Items[j]].m_ItemStrId.c_str());
            }

            if (it->m_DotSymbolNo == it->m_pRule->m_RightPart.m_Items.size())
                fprintf(fp, "; ");
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "\n");
    for (ymap< CStateAndSymbol, size_t>::const_iterator it_goto = m_GotoFunction.begin(); it_goto != m_GotoFunction.end(); it_goto++)
        fprintf(fp, "GOTO( I%" PRISZT ", %s ) = I%" PRISZT "\n", it_goto->first.m_StateNo, p_WorkGrammar->m_UniqueGrammarItems[it_goto->first.m_SymbolNo].m_ItemStrId.c_str(), it_goto->second);

    fclose(fp);
}

