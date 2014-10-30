#pragma once
#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afglrparserlib/grammaritem.h>

struct CWorkGrammar;

//приписывает терминал грамматики очередному слову
class CTerminalsAssigner
{
public:
    CTerminalsAssigner(const CWorkGrammar& grammar, const CWordsPair& wpSent);
    void BuildTerminalSymbols(CWord& w);

private:
    void BuildTerminalSymbolsLoop(CWord& w, int iTermID, eTerminals Symb);
    bool CheckWordPostfix(CWord& w, const CGrammarItem& terminalSymbol) const;

    bool CheckLabelPostfix(const CHomonym& hom, const CGrammarItem& grammarItem) const;
    bool CheckKWPostfix(const CHomonym& hom, const CGrammarItem& grammarItem) const;

    inline bool CheckGrammemsPostfix(const CHomonym& h, const CGrammarItem& grammarItem) const {
        return h.Grammems.HasAll(grammarItem.m_ItemGrammems) &&
              !h.Grammems.HasAny(grammarItem.m_NegativeItemGrammems);
    }

    bool CheckTerminalAdj(CWord::SHomIt pHom, eTerminals eTermExcept);
    bool CheckTerminalWord(CWord& w) const;

    // minimal checks
    inline void AssignHomonymTerminalWeak(const CHomonym& homonym, const CGrammarItem& grammarItem, int termId) const {
        if (CheckKWPostfix(homonym, grammarItem) && CheckLabelPostfix(homonym, grammarItem))
            homonym.AddTerminalSymbol(termId);
    }

    // full checks
    inline void AssignHomonymTerminal(const CHomonym& homonym, const CGrammarItem& grammarItem, int termId) const {
        if (CheckGrammemsPostfix(homonym, grammarItem))
            AssignHomonymTerminalWeak(homonym, grammarItem, termId);
    }

    void AssignWordTerminal(CWord& w, const CGrammarItem& grammarItem, int termId) const;
    void AssignNounTerminal(CWord& w, const CGrammarItem& grammarItem, int termId) const;

    inline void AssignPOSTerminal(CWord& w, const CGrammarItem& grammarItem, int termId, TGrammar partOfSpeech) const {
        if (CheckWordPostfix(w, grammarItem))
            for (CWord::SHomIt it = w.IterHomonyms(); it.Ok32(); ++it)
                if (it->HasPOS(partOfSpeech))
                    AssignHomonymTerminal(*it, grammarItem, termId);
    }

private:
    const CWorkGrammar& m_Grammar;
    CWordsPair m_wpSent;    // отрезок слов, на котором запускается грамматика
};
