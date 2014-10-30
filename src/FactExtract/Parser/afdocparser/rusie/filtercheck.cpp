#include "filtercheck.h"

bool CFilterCheck::CheckFiltersWithoutOrder(const CFilterStore& filters)
{
    if (filters.IsEmpty()) return true;

    for (size_t i = 0; i < filters.m_FilterSet.size(); i++) {
        size_t j;
        for (j = 0; j < filters.m_FilterSet[i].size(); j++) {
            if (!AssignFilterItems(NULL, filters, filters.m_FilterSet[i][j].m_iItemID, false))
                break;
        }
        if (j == filters.m_FilterSet[i].size()) return true;
    }

    return false;
}

bool CFilterCheck::CheckFilters(const CFilterStore& filters)
{
    //if ( 0 == filters.m_FilterSet.size() ) return true;
    if (filters.IsEmpty()) return true;

    ymap< int, yset<int> > FilterItemToWords;

    for (size_t i = 0; i < filters.m_UniqueFilterItems.size(); i++)
        AssignFilterItems(&FilterItemToWords, filters, i);

    yvector<int> TemplateStack;
    for (size_t i = 0; i < filters.m_FilterSet.size(); i++) {
        TemplateStack.clear();
        int j = 0;
        for (; j < (int)filters.m_FilterSet[i].size();) {
            ymap< int, yset<int> >::iterator it_item = FilterItemToWords.find(filters.m_FilterSet[i][j].m_iItemID);
            if (FilterItemToWords.end() == it_item) break;
            if (0 == it_item->second.size()) break;
            yset<int>::const_iterator it_wrd_id = it_item->second.begin();
            if ((int)TemplateStack.size() == j+1) {
                it_wrd_id = it_item->second.find(TemplateStack[(int)TemplateStack.size()-1]);
                if (it_wrd_id != it_item->second.end()) it_wrd_id++;
                TemplateStack.erase(TemplateStack.end()-1);
            }
            for (; it_wrd_id != it_item->second.end(); it_wrd_id++) {
                assert((int)TemplateStack.size() == j);
                if ((j < 1)
                   || (filters.m_FilterSet[i][j-1].m_iWordDistance >= (*it_wrd_id - TemplateStack[j-1])-1
                        && (*it_wrd_id - TemplateStack[j-1]) > 0)) {
                    TemplateStack.push_back(*it_wrd_id); break;
                }
            }
            if (it_wrd_id != it_item->second.end()) j++;
            else j--;
            if (j < 0) break;
        }
        if ((int)filters.m_FilterSet[i].size() == j)
            return true;
    }

    return false;
}

bool CFilterCheck::AssignFilterItems(ymap< int, yset<int> >* FilterItemToWords, const CFilterStore& filters, int iID, bool bFull)
{
    const CGrammarItem& gramitem = filters.m_UniqueFilterItems[iID];
    SArtPointer artP = gramitem.GetPostfixKWType(DICT_NOT_KWTYPE);
    const yvector<SWordHomonymNum>* pItemsKW = NULL;

    if (artP.IsValid()) {
        pItemsKW = m_MultiWordCreator.GetFoundWords(artP, KW_DICT);
        if (pItemsKW != NULL)
            return false;
    }

    artP = gramitem.GetPostfixKWType(DICT_KWTYPE);

    bool bKwWrd = false;
    if (!gramitem.IsNonePostfixKWType() && artP.IsValid()) {
        pItemsKW = m_MultiWordCreator.GetFoundWords(artP, KW_DICT);
        if (pItemsKW == NULL)
            return false;
        bKwWrd = true;
    }

    bool bHreg1, bHreg2, bLreg, bNotFromMorph;
    char cFW, cNameDic, cOrgDic, cGeoDic;
    cFW = cNameDic = cOrgDic = cGeoDic = 0;
    bHreg1 = bHreg2 = bLreg = bNotFromMorph = false;
    const CWordFormRegExp* pWFRegExp = gramitem.GetRegExpPostfix(REG_EXP);

    if (NULL == pWFRegExp)
        pWFRegExp = gramitem.GetRegExpPostfix(NOT_REG_EXP);
    if (FIRST_LET == gramitem.HasPostfix(HREG1))
        bHreg1 = true;
    if (SECOND_LET == gramitem.HasPostfix(HREG2)
        || ALL_LET == gramitem.HasPostfix(HREG))
        bHreg2 = true;
    if (ALL_LET == gramitem.HasPostfix(LREG))
        bLreg = true;
    if (NOt == gramitem.HasPostfix(DICT))
        bNotFromMorph = true;
    if (YEs == gramitem.HasPostfix(FIRST_LET_IN_SENT))
        cFW = 1;
    if (NOt == gramitem.HasPostfix(FIRST_LET_IN_SENT))
        cFW = 2;

    if (DICT_NAME == gramitem.HasPostfix(NAME_DIC))
        cNameDic = 1;
    if (NOt == gramitem.HasPostfix(NAME_DIC))
        cNameDic = 2;

    if (DICT_ORG == gramitem.HasPostfix(ORGAN))
        cOrgDic = 1;
    if (NOt == gramitem.HasPostfix(ORGAN))
        cOrgDic = 2;

    if (DICT_GEO == gramitem.HasPostfix(GEO))
        cGeoDic = 1;
    if (NOt == gramitem.HasPostfix(GEO))
        cGeoDic = 2;

    eTerminals eTerm = filters.m_UniqueFilterTerminalIDs.find(iID)->second;

    if (!bHreg1 && !bHreg2 && !bLreg && !bNotFromMorph &&
         !cFW && !cNameDic && !cOrgDic && !cGeoDic && NULL == pWFRegExp && bKwWrd && eTerm == T_WORD) {
        if (bFull) ConvertToOriginalWords(*FilterItemToWords, *pItemsKW, iID);
        return true;
    }

    yvector<SWordHomonymNum> ItemWrds;
    size_t iCount = m_MultiWordCreator.m_Words.OriginalWordsCount();
    if (bKwWrd) {
        ItemWrds = *pItemsKW;
        iCount = pItemsKW->size();
    }

    for (int j = 0;; j++) {
        if (j == (int)iCount) break;
        bool bDel = false;

        const CWord* pW = NULL;
        if (bKwWrd)
            pW = &(m_MultiWordCreator.m_Words.GetWord(ItemWrds[j]));
        else
            pW = &(m_MultiWordCreator.m_Words.GetWord(SWordHomonymNum(j, -1)));

        if ((bHreg1 && !pW->m_bUp) || (bHreg2 && !pW->HasAtLeastTwoUpper()) || (bLreg && pW->m_bUp) ||
            (bNotFromMorph && !pW->HasUnknownPOS()) || (cFW && (cFW == 1) != (pW->GetSourcePair().FirstWord() == 0)) ||
            (cGeoDic && (cGeoDic == 1) != pW->HasGeoHom()) || (cNameDic && (cNameDic == 1) != pW->HasNameHom()))
            bDel = true;

        if (NULL != pWFRegExp && !pWFRegExp->Matches(pW->GetOriginalText()))
            bDel = true;

        if (!gramitem.m_Lemma.empty()) {
            bDel = true;
            for (CWord::SHomIt it_hom = pW->IterHomonyms(); it_hom.Ok(); ++it_hom)
                if (it_hom->Lemma == /*NStr::Decode(gramitem.m_ItemStrId)*/ gramitem.m_Lemma) {
                    bDel = false;
                    break;
                }
        } else if (!CompareTermTypeWithWord(pW, eTerm))
            bDel = true;

        if (bDel && bKwWrd) {
            ItemWrds.erase(ItemWrds.begin()+j);
            j--;
            iCount--;
        } else if (!bDel && !bKwWrd)
            ItemWrds.push_back(SWordHomonymNum(j, -1));
    }

    if (bFull) ConvertToOriginalWords(*FilterItemToWords, ItemWrds, iID);
    if (0 == ItemWrds.size()) return false;
    return true;
}

bool CFilterCheck::CompareTermTypeWithWord(const CWord* pW, eTerminals eTerm)
{
    switch (eTerm) {
        case T_ANYWORD:
            return true;

        case T_WORD:
            return  ((pW->m_typ != UnknownPrim
                          && pW->m_typ != NewLine
                          && pW->m_typ != Space
                          && pW->m_typ != Punct
                          && pW->m_typ != Symbol
                          && pW->m_typ != Complex)
                          || pW->IsComplexWordAsCompName()
                          || pW->IsMultiWord()
                    );
        case T_NOUN:
            return pW->HasPOS(gSubstantive);
        case T_ADJPARTORDNUM:
        case T_ADJ:
            return pW->HasPOS(gAdjective) || pW->HasPOS(gParticiple) || pW->HasPOS(gAdjNumeral);
        case T_ORDNUM:
            return pW->HasPOS(gAdjNumeral);
        case T_ADV:
            return pW->HasPOS(gAdverb);
        case T_PARTICIPLE:
            return pW->HasPOS(gParticiple);
        case T_VERB:
            return pW->HasPOS(gVerb);
        case T_UNPOS:
            return pW->HasUnknownPOS() && (pW->m_typ == Word || pW->m_typ == Hyphen);
        case T_PREP:
            return pW->HasPOS(gPreposition);
        case T_CONJAND:
            return pW->IsAndConj();
        case T_QUOTEDBL:
            return pW->IsDoubleQuote();
        case T_QUOTESNG:
            return pW->IsSingleQuote();
        case T_LBRACKET:
            return pW->IsOpenBracket();
        case T_RBRACKET:
            return pW->IsCloseBracket();
        case T_BRACKET:
            return pW->IsOpenBracket() || pW->IsCloseBracket();
        case T_HYPHEN:
            return pW->IsDash();
        case T_PUNCT:
            return pW->IsPunct();
        case T_COMMA:
            return pW->IsComma();
        case T_PERCENT:
            return pW->GetText().find('%') != Stroka::npos;
        case T_DOLLAR:
            return pW->GetText().find('$') != Stroka::npos;
        case T_COLON:
            return pW->IsColon();
        case T_NUMSIGN:
            return Symbol == pW->m_typ && pW->GetText().find('#') != Stroka::npos;
        case T_AMPERSAND:
            return Symbol == pW->m_typ && pW->GetText().find('&') != Stroka::npos;
        case T_PLUSSIGN:
            return Symbol == pW->m_typ && pW->GetText().find('+') != Stroka::npos;

        default:
            return false;
    }
}

void CFilterCheck::ConvertToOriginalWords(ymap< int, yset<int> >& FilterItemToWords,
                                          const yvector<SWordHomonymNum>& ItemWrds, int iID)
{
    if (0 == ItemWrds.size()) return;
    yset<int>& rWrdIds = FilterItemToWords[iID];
    for (size_t i = 0; i < ItemWrds.size(); i++)
        rWrdIds.insert(m_MultiWordCreator.m_Words.GetWord(ItemWrds.at(i)).GetSourcePair().LastWord());
}

