#pragma once

#include "lemma.h"
#include "form.h"

namespace NLemmer {

class TSimpleLemmer {
    DECLARE_NOCOPY(TSimpleLemmer);
    TDynamicLibrary Lib;

public:
    TSimpleLemmer(const Stroka& path = "");
    ~TSimpleLemmer();

    bool IsInitialized() const;
    inline TDynamicLibrary& GetLibrary() {
        return Lib;
    }

    size_t AnalyzeWord(const TChar* word, size_t len, TWLemmaArray& out, TLangMask langmask, const docLanguage* doclangs = NULL, const TAnalyzeWordOpt& opt = TAnalyzeWordOpt());

    MystemFormsHandle* GetFormsHandle(const TSimpleLemma& lemma);
    MystemFormHandle* GetFormHandle(MystemFormsHandle* h, int i);

    void DeleteAnalyses(MystemAnalysesHandle *analyses);

    int GetLemmaQuality(MystemLemmaHandle* h);
    int GetFormsCount(MystemFormsHandle* h);
    TChar* GetFormText(MystemFormHandle* h);
    int GetFormTextLen(MystemFormHandle* h);
    char** GetFormFlexGram(MystemFormHandle* h);
    int GetFormFlexGramNum(MystemFormHandle* h);

protected:

    MystemFormsHandle* GetFormsHandleInner(TSymbol* text, int len, ui32 requiredRuleId);
    size_t CopyFromMystem(MystemAnalysesHandle* analyses, const Wtroka& src, TWLemmaArray& out);
};

}
