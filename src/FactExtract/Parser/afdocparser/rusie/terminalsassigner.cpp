#include "terminalsassigner.h"
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include "dictsholder.h"

CTerminalsAssigner::CTerminalsAssigner(const CWorkGrammar& grammar, const CWordsPair& wpSent):
    m_Grammar(grammar),
    m_wpSent(wpSent)
{
}

void CTerminalsAssigner::BuildTerminalSymbols(CWord& w)
{
    if (w.IsEndOfStreamWord()) {
        if (m_Grammar.i_EOSentNum > -1) {
            w.m_AutomatSymbolInterpetationUnion.insert(m_Grammar.i_EOSentNum);
            if (w.GetRusHomonymsCount() != 1)
                ythrow yexception() << "EOS word has no homonyms";
            w.IterHomonyms()->AddTerminalSymbol(m_Grammar.i_EOSentNum);
        };

        w.m_AutomatSymbolInterpetationUnion.insert(m_Grammar.GetEndOfStreamSymbol());
        return;
    }

    //если это слово не из того куска предложения, который мы хотим обрабатвать
    if (!m_wpSent.Includes(w.GetSourcePair()))
        return;

    for (ymap<int, eTerminals>::const_iterator it_term = m_Grammar.m_UniqueGrammarTerminalIDs.begin();
          it_term != m_Grammar.m_UniqueGrammarTerminalIDs.end(); it_term++)
        BuildTerminalSymbolsLoop(w, it_term->first, it_term->second);

    w.UniteHomonymsTerminalSymbols();
}

void CTerminalsAssigner::AssignWordTerminal(CWord& w, const CGrammarItem& grammarItem, int termId) const {

    if (!CheckWordPostfix(w, grammarItem))
        return;

    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it) {
        const CHomonym& h = *it;
        if (CheckKWPostfix(h, grammarItem)) {
            if (CheckGrammemsPostfix(h, grammarItem) && CheckLabelPostfix(h, grammarItem))
                h.AddTerminalSymbol(termId);
        } else if (grammarItem.IsNonePostfixKWType()) {
            // clear this terminal in all previous homonyms
            for (CWord::SHomIt it_del = w.IterHomonyms(); it_del != it; ++it_del)
                it_del->EraseTerminalSymbol(termId);
            return;
        }
    }
}

void CTerminalsAssigner::AssignNounTerminal(CWord& w, const CGrammarItem& grammarItem, int termId) const {
    if (!CheckWordPostfix(w, grammarItem))
        return;

    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it) {
        if (it->HasGrammem(gAbbreviation) && !w.m_bUp && !w.IgnoreUpperCase())
            return;
        // not allow names to be Nouns (except lowercased names)
        if (it->HasPOS(gSubstantive) && (!it->IsName() || !w.m_bUp))
            AssignHomonymTerminal(*it, grammarItem, termId);
    }
}

void CTerminalsAssigner::BuildTerminalSymbolsLoop(CWord& w, int iTermID, eTerminals Symb)
{
    const CGrammarItem& I = m_Grammar.m_UniqueGrammarItems[iTermID];

    switch (Symb) {
    case TermCount:
        for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
            if (it->Lemma == I.m_Lemma)
                if (CheckGrammemsPostfix(*it, I) && CheckWordPostfix(w, I) && CheckKWPostfix(*it, I))
                    it->AddTerminalSymbol(iTermID);
        return;

    case T_ANYWORD:
        // same as T_WORD, without text form check
        AssignWordTerminal(w, I, iTermID);
        return;

    case T_WORD:
        if (CheckTerminalWord(w))
            AssignWordTerminal(w, I, iTermID);
        return;

    case T_NOUN:
        AssignNounTerminal(w, I, iTermID);
        return;

    case T_ORDNUM:
        AssignPOSTerminal(w, I, iTermID, gAdjNumeral);
        return;

    case T_ADV:
        AssignPOSTerminal(w, I, iTermID, gAdverb);
        return;

    case T_PARTICIPLE:
        AssignPOSTerminal(w, I, iTermID, gParticiple);     // full participle
        return;

    case T_VERB:
        AssignPOSTerminal(w, I, iTermID, gVerb);   // verb in personal (finite) form
        return;

    case T_INFINITIVE:
        AssignPOSTerminal(w, I, iTermID, gInfinitive);
        return;

    case T_ADJ:
    case T_ADJPARTORDNUM:
        if (CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                if (CheckTerminalAdj(it, Symb))
                    AssignHomonymTerminal(*it, I, iTermID);
        return;

    case T_UNPOS:
        if ((w.m_typ == Word || w.m_typ == Hyphen) && CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                if (it->HasUnknownPOS())
                    AssignHomonymTerminal(*it, I, iTermID);
        return;

    case T_PREP:
        if (CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                if (it->HasPOS(gPreposition))
                    AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    case T_CONJAND:
        if (w.IsAndConj())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_QUOTEDBL:
        if (w.IsDoubleQuote())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_QUOTESNG:
        if (w.IsSingleQuote())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_LBRACKET:
        if (w.IsOpenBracket())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_RBRACKET:
        if (w.IsCloseBracket())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_BRACKET:
        if (w.IsOpenBracket() || w.IsCloseBracket())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_HYPHEN:
        if (w.IsDash())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_PUNCT:
        if (w.IsPunct())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_COMMA:
        if (w.IsComma())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_COLON:
        if (w.IsColon())
            w.AddTerminalSymbolToAllHomonyms(iTermID);
        return;

    case T_DATE:
        if (CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    case T_PERCENT:
        if (w.m_txt.find('%') != Stroka::npos && CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    case T_DOLLAR:
        if (w.m_txt.find('$') != Stroka::npos && CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    case T_NUMSIGN:
        if (Symbol == w.m_typ && w.m_txt.find('#') != Stroka::npos && CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    case T_PLUSSIGN:
        if (Symbol == w.m_typ && w.m_txt.find('+') != Stroka::npos && CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    case T_AMPERSAND:
        if (Symbol == w.m_typ && w.m_txt.find('&') != Stroka::npos && CheckWordPostfix(w, I))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                AssignHomonymTerminalWeak(*it, I, iTermID);
        return;

    default:
        return;
    }
}

bool CTerminalsAssigner::CheckWordPostfix(CWord& w, const CGrammarItem& TerminalSymbol) const
{
    if (!w.IgnoreUpperCase()) {
        if (FIRST_LET == TerminalSymbol.m_PostfixStore.m_hreg1)
            if (!w.m_bUp && w.m_typ != Digit)   // allow digits for hreg1 (e.g. "3-ya"<h-reg1> "ulitsa" "stroiteley")
                return false;

        if (SECOND_LET == TerminalSymbol.m_PostfixStore.m_hreg2 || ALL_LET == TerminalSymbol.m_PostfixStore.m_hreg)
            if (!w.HasAtLeastTwoUpper())
                return false;

        if (NOt == TerminalSymbol.m_PostfixStore.m_hreg2)
            if (w.HasAtLeastTwoUpper())
                return false;

        if (ALL_LET == TerminalSymbol.m_PostfixStore.m_lreg)
            if (w.m_bUp)
                return false;
    }

    if (NO_PREP == TerminalSymbol.m_PostfixStore.m_hom) {
        if (w.HasPOS(gPreposition))
            return false;
    }
    if (NO_HOMONYMS == TerminalSymbol.m_PostfixStore.m_nohomonyms) {
        if (w.GetHomonymsCount() > 1 && !TerminalSymbol.IsNonePostfixKWType()) {
            TKeyWordType kwType = TerminalSymbol.GetPostfixKWType(DICT_KWTYPE).GetKWType();
            const CHomonym* hom_with_pos = NULL;
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it) {
                if (hom_with_pos  == NULL)
                    hom_with_pos = &(*it);
                // если терминал - географический и у слова есть омоним без пометы geo
                else if (GlobalDictsHolder->BuiltinKWTypes().IsGeo(kwType)) {
                    if (!it->IsGeo() && !it->HasKWType(kwType, KW_DICT))
                        return false;
                }
                // если есть омонимы разных частей речи
                else if (!hom_with_pos->HasSamePOS(*it))
                        return false;

            }
        }
    }
    if (NOt == TerminalSymbol.m_PostfixStore.m_dict) {
        if (!w.HasUnknownPOS()
            //for 'Гута-банк' case, which is not in the dictionary, but predicted by the second part.
            && !(w.m_variant.size() > 0 && w.m_typ == Hyphen))
            return false;
    }
    if (LATIN == TerminalSymbol.m_PostfixStore.m_lat) {
        if ((w.m_typ != AltWord) && !w.IsLatComplexWordAsCompName() &&
            (w.m_typ != AbbEosAlt) && (w.m_typ != AbbDigAlt) && (w.m_typ != AbbAlt))
            return false;
    }

    if (NOt == TerminalSymbol.m_PostfixStore.m_lat) {
        if (!((w.m_typ != AltWord) && !w.IsLatComplexWordAsCompName() &&
             (w.m_typ != AbbEosAlt) && (w.m_typ != AbbDigAlt) && (w.m_typ != AbbAlt)))
            return false;
    }

    if (DICT_ORG == TerminalSymbol.m_PostfixStore.m_org_morph_dict) {
        // if(!w.HasHomFromMorphOrgDic())  -- always true
            return false;
    }

    if (NOt == TerminalSymbol.m_PostfixStore.m_org_morph_dict) {
        // if(w.HasHomFromMorphOrgDic())
        //    return false;
    }
    if (DICT_GEO == TerminalSymbol.m_PostfixStore.m_geo_morph_dict) {
        if (!w.HasGeoHom())
            return false;
    }
    if (NOt == TerminalSymbol.m_PostfixStore.m_geo_morph_dict) {
        if (w.HasGeoHom())
            return false;
    }
    if (DICT_NAME == TerminalSymbol.m_PostfixStore.m_name_morph_dict) {
        if (!w.HasNameHom())
            return false;
    }
    if (NOt == TerminalSymbol.m_PostfixStore.m_name_morph_dict) {
        if (w.HasNameHom())
            return false;
    }
    if (NOt == TerminalSymbol.m_PostfixStore.m_fw) {
        if (0 == (w.GetSourcePair().FirstWord() - m_wpSent.FirstWord()))
            return false;
    }
    if (YEs == TerminalSymbol.m_PostfixStore.m_fw) {
        if (0 != (w.GetSourcePair().FirstWord() - m_wpSent.FirstWord()))
            return false;
    }
    if (NOt == TerminalSymbol.m_PostfixStore.m_is_multiword) {
        if (w.IsMultiWord())
            return false;
    }
    if (YEs == TerminalSymbol.m_PostfixStore.m_is_multiword) {
        if (!w.IsMultiWord())
            return false;
    }

    return true;
}

bool CTerminalsAssigner::CheckKWPostfix(const CHomonym& h, const CGrammarItem& grammarItem) const
{
    // special case <kwtype=none>: deny if pHom has any kwtype from rKwTypes
    if (grammarItem.IsNonePostfixKWType() && GlobalDictsHolder->HasArtPointer(h, KW_DICT, m_Grammar.m_kwArticlePointer))
        return false;

    const SArtPointer& kwTypeYes = grammarItem.GetPostfixKWType(DICT_KWTYPE);
    if (!GlobalDictsHolder->MatchArticlePtr(h, KW_DICT, kwTypeYes))
        return false;

    const SArtPointer& kwTypeNo = grammarItem.GetPostfixKWType(DICT_NOT_KWTYPE);
    if (kwTypeNo.IsValid() && GlobalDictsHolder->MatchArticlePtr(h, KW_DICT, kwTypeNo))
        return false;

    return true;
}

bool CTerminalsAssigner::CheckTerminalAdj(CWord::SHomIt pHom, eTerminals eTermExcept)
{
    return (pHom->IsMorphAdj() && T_ADJ == eTermExcept) ||
           ((pHom->IsFullAdjective() || pHom->IsFullParticiple() || pHom->HasPOS(gAdjNumeral)) && T_ADJPARTORDNUM == eTermExcept);
}

bool CTerminalsAssigner::CheckTerminalWord(CWord& w) const
{
    return ((w.m_typ != UnknownPrim
               && w.m_typ != NewLine
               && w.m_typ != Space
               && w.m_typ != Punct
               //&& w.m_typ != Initial  // see FACTEX-2335
               && w.m_typ != Symbol
               && w.m_typ != Complex)
             || w.IsComplexWordAsCompName()
             || w.IsComplexWordAsDate()
             || w.IsMultiWord()
            );
}

bool CTerminalsAssigner::CheckLabelPostfix(const CHomonym& h, const CGrammarItem& grammarItem) const
{
    const Wtroka& label = grammarItem.GetPostfixKWType(DICT_LABEL).GetStrType();
    if (!label.empty())
        return h.HasLabel(label);

    const Wtroka& notLabel = grammarItem.GetPostfixKWType(DICT_NOT_LABEL).GetStrType();
    if (!notLabel.empty())
        return !h.HasLabel(notLabel);

    return true;
}
