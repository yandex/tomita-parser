// This module contains all functions which should parse and process
// source of the grammar. The main function   of this module is BuildWorkGrammar,
// which contains all necssary calls to build a WorkGrammar from a source file.

#include "sourcegrammar.h"
#include "simplegrammar.h"

//==========================================================================
//============================  CParseRule     ============================
//==========================================================================

Stroka CParseRule::toString() const
{
    Stroka Result;
    for (size_t i=0; i<m_RightParts.size(); i++) {
        Result += m_LeftPart.GetDumpString() + " -> ";
        for (size_t j=0; j<m_RightParts[i].size(); j++) {
            Result +=  m_RightParts[i][j].GetDumpString();

            if (j+1 != m_RightParts[i].size())
                Result += ", ";
        };
        Result += "\n";
    };
    return     Result;
};

void CParseRule::Print() const
{
    printf ("%s",toString().c_str());
};

// ========================================================================
// ===========================   CParseGrammar ============================
// ========================================================================

CParseGrammar::CParseGrammar()
{
    m_Language = morphGerman;
};

/*
this function  goes through all rules, if a rule has no right main item, then
it declares that the first item is the first.
*/
bool CParseGrammar::AssignSynMainItemIfNotDefined()
{
    Stroka ErrorMsg;
    for (size_t i =0; i<m_Rules.size(); i++) {
        for (size_t j =0; j<m_Rules[i].m_RightParts.size(); j++) {
            yvector<CGrammarItem>& RightPart = m_Rules[i].m_RightParts[j];

            size_t CountOfSynMainNodes =0;

            for (size_t k=0; k<RightPart.size(); k++)
                if (RightPart[k].m_bSynMain)
                    CountOfSynMainNodes++;

            if (CountOfSynMainNodes  == 0) {
                assert (!RightPart.empty());
                RightPart[0].m_bSynMain = true;
            } else if (CountOfSynMainNodes > 1) {
                    Cerr << "Rule " << m_Rules[i].toString() << " has more than one syntax roots" << Endl;
                    return false;
                };
        };
    };
    return true;
};

void CParseGrammar::Print()
{
    for (size_t i =0; i<m_Rules.size(); i++)
        m_Rules[i].Print();
};

void CParseGrammar::Clear()
{
    m_Rules.clear();
    m_Roots.clear();
}

/*
in this procedure we use RuleItemPartialLess and RuleItemPartialEqual in order to search
*/

bool CParseGrammar::CheckCoherence() const
{
    yset<CGrammarItem> AllLeftParts;
    size_t i =0;

    for (; i<m_Rules.size(); i++)
        AllLeftParts.insert(m_Rules[i].m_LeftPart);

    for (i =0; i<m_Rules.size(); i++)
        for (size_t k =0; k<m_Rules[i].m_RightParts.size(); k++)
            for (size_t j =0; j<m_Rules[i].m_RightParts[k].size(); j++) {
                const CGrammarItem& I = m_Rules[i].m_RightParts[k][j];
                if (I.m_Type == siMeta) {
                    if    (AllLeftParts.find(I) == AllLeftParts.end()) {
                        Cerr << Substitute("\"$0\" is used, but not defined (line $1; file $2)\n",
                                           I.m_ItemStrId, m_Rules[i].m_RightParts[k].m_SourceLineNo,
                                           m_Rules[i].m_RightParts[k].m_SourceLine) << Endl;
                        return  false;
                    };
                };
            };

    return true;
};

void CParseGrammar::EncodeGrammar(CWorkGrammar& Result) const
{
    Result.m_EncodedRules.clear();
    Result.m_UniqueGrammarItems.clear();
    int RuleId = 0;

    for (size_t i =0; i<m_Rules.size(); i++) {
        CWorkRule Rule;
        Rule.m_LeftPart  = Result.GetItemIdOrRegister(m_Rules[i].m_LeftPart);
        for (size_t k =0; k<m_Rules[i].m_RightParts.size(); k++) {
            Rule.m_RightPart.m_Items.clear();
            for (size_t j =0; j<m_Rules[i].m_RightParts[k].size(); j++) {
                const CGrammarItem& Item = m_Rules[i].m_RightParts[k][j];
                size_t  id = Result.GetItemIdOrRegister(Item);
                Rule.m_RightPart.m_Items.push_back(id);

                if (Item.m_bSynMain)
                    Rule.m_RightPart.m_SynMainItemNo = Rule.m_RightPart.m_Items.size() - 1;

                Result.m_UniqueGrammarItems[id].m_bGrammarRoot = false;

            };
            Rule.m_OriginalRuleNo = RuleId;
            if (!Result.m_EncodedRules.insert(Rule).second)
                ythrow yexception() << "CParseGrammar.EncodeGrammar : a duplicate rule is found";
            RuleId++;
        };

    };

    if (!m_Roots.empty()) {
        for (size_t i=0; i< Result.m_UniqueGrammarItems.size(); i++)
            Result.m_UniqueGrammarItems[i].m_bGrammarRoot =
                m_Roots.end() != std::find(m_Roots.begin(), m_Roots.end(), Result.m_UniqueGrammarItems[i]);
    };
};

