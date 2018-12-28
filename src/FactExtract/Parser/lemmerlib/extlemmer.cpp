#include <bindings/c/mystem/iface.h>
#include <util/system/progname.h>
#include <util/folder/dirut.h>

#include "extlemmer.h"

typedef MystemAnalysesHandle* (*FUNC_MystemAnalyze)(TSymbol* word, int len);
typedef MystemLemmaHandle*    (*FUNC_MystemLemma)(MystemAnalysesHandle* analyses, int i);
typedef int     (*FUNC_MystemAnalysesCount)(MystemAnalysesHandle* analyses);
typedef void    (*FUNC_MystemDeleteAnalyses)(MystemAnalysesHandle* analyses);
typedef TSymbol* (*FUNC_MystemLemmaText)(MystemLemmaHandle* lemma);
typedef int     (*FUNC_MystemLemmaTextLen)(MystemLemmaHandle* lemma);
typedef int     (*FUNC_MystemLemmaQuality)(MystemLemmaHandle* lemma);
typedef char*   (*FUNC_MystemLemmaStemGram)(MystemLemmaHandle* lemma);
typedef char**  (*FUNC_MystemLemmaFlexGram)(MystemLemmaHandle* lemma);
typedef int     (*FUNC_MystemLemmaFlexGramNum)(MystemLemmaHandle* lemma);

typedef TSymbol* (*FUNC_MystemLemmaForm)(MystemLemmaHandle* lemma);
typedef int     (*FUNC_MystemLemmaFormLen)(MystemLemmaHandle* lemma);

typedef int     (*FUNC_MystemLemmaFlexLen)(MystemLemmaHandle* lemma);
typedef int     (*FUNC_MystemLemmaRuleId)(MystemLemmaHandle* lemma);


typedef MystemFormsHandle* (*FUNC_MystemGenerate)(MystemLemmaHandle* lemma);
typedef MystemFormHandle*  (*FUNC_MystemForm)(MystemFormsHandle* forms, int i);
typedef int     (*FUNC_MystemFormsCount)(MystemFormsHandle* forms);
typedef void    (*FUNC_MystemDeleteForms)(MystemFormsHandle* forms);
typedef TSymbol* (*FUNC_MystemFormText)(MystemFormHandle* form);
typedef int     (*FUNC_MystemFormTextLen)(MystemFormHandle* form);
typedef char*   (*FUNC_MystemFormStemGram)(MystemFormHandle* form);
typedef char**  (*FUNC_MystemFormFlexGram)(MystemFormHandle* form);
typedef int     (*FUNC_MystemFormFlexGramNum)(MystemFormHandle* form);

namespace NLemmer {

class TGrammarFiltr;
TGrammarFiltr* GetNewDummyGrammarFiltr(const char* grammar);

TSimpleLemmer::TSimpleLemmer(const Stroka& path) {
    Stroka libPath = path;
    if (path.length() == 0) {
        SlashFolderLocal(libPath);
        libPath += Stroka("libmystem_c_binding.so");
    }

    Lib.Open(~libPath, RTLD_NOW | RTLD_DEEPBIND | RTLD_NODELETE);

    if (!IsInitialized())
        yexception() << "Can't load lemmer from \"" << path << "\"";
}

bool TSimpleLemmer::IsInitialized() const {
    return Lib.IsLoaded();
}

TSimpleLemmer::~TSimpleLemmer() {
}

size_t TSimpleLemmer::AnalyzeWord(const TChar* word, size_t len, TWLemmaArray& out, TLangMask langmask, const docLanguage* doclangs, const TAnalyzeWordOpt& opt) {
    UNUSED(langmask);
    UNUSED(doclangs);
    UNUSED(opt);

    //Cerr << "AnalyzeWord: " << word << " " << len << Endl;

    TSymbol* w = const_cast<TSymbol*>(word);
    MystemAnalysesHandle* analyses = GET_FUNC(Lib, MystemAnalyze)(w, len);
    Wtroka src = Wtroka(word).substr(0, len);

    return CopyFromMystem(analyses, src, out);
}

size_t TSimpleLemmer::CopyFromMystem(MystemAnalysesHandle* analyses, const Wtroka& src, TWLemmaArray& out) {
    size_t count = GET_FUNC(Lib, MystemAnalysesCount)(analyses);

    out.clear();
    out.resize(count);

    for (size_t i = 0; i != count; ++i) {
         MystemLemmaHandle* lemma = GET_FUNC(Lib, MystemLemma)(analyses, i);
         NLemmerAux::TSimpleLemmaSetter setter(out[i]);
         setter.SetLemma(GET_FUNC(Lib, MystemLemmaText)(lemma),
                         GET_FUNC(Lib, MystemLemmaTextLen)(lemma),
                         GET_FUNC(Lib, MystemLemmaForm)(lemma),
                         GET_FUNC(Lib, MystemLemmaFormLen)(lemma),
                         GET_FUNC(Lib, MystemLemmaStemGram)(lemma),
                         GET_FUNC(Lib, MystemLemmaFlexGram)(lemma),
                         GET_FUNC(Lib, MystemLemmaFlexGramNum)(lemma),
                         GET_FUNC(Lib, MystemLemmaFlexLen)(lemma),
                         GET_FUNC(Lib, MystemLemmaRuleId)(lemma),
                         GET_FUNC(Lib, MystemLemmaQuality)(lemma),
                         src);
    }

    DeleteAnalyses(analyses);
    return count;
}

MystemFormsHandle* TSimpleLemmer::GetFormsHandleInner(TSymbol* text, int len, ui32 requiredRuleId) {
    MystemFormsHandle* f = NULL;
    MystemAnalysesHandle* analyses = GET_FUNC(Lib, MystemAnalyze)(text, len);
    size_t count = GET_FUNC(Lib, MystemAnalysesCount)(analyses);

    for (size_t i = 0; i != count; ++i) {
         MystemLemmaHandle* h = GET_FUNC(Lib, MystemLemma)(analyses, i);

         ui32 ruleId = GET_FUNC(Lib, MystemLemmaRuleId)(h);

         if (ruleId == requiredRuleId) {
             f = GET_FUNC(Lib, MystemGenerate)(h);
             ++i;
             if (i != count) {
                 for(; i != count; ++i) {
                     MystemLemmaHandle* h2 = GET_FUNC(Lib, MystemLemma)(analyses, i);
                     ui32 ruleId = GET_FUNC(Lib, MystemLemmaRuleId)(h2);
                 }
             }

             break;
         }
    }

    DeleteAnalyses(analyses);
    return f;
}

MystemFormsHandle* TSimpleLemmer::GetFormsHandle(const TSimpleLemma& lemma) {
    MystemFormsHandle* f = GetFormsHandleInner((TSymbol*)lemma.SourceText.c_str(), lemma.SourceText.length(), lemma.GetRuleId());
    if (f == NULL)
        f = GetFormsHandleInner((TSymbol*)lemma.GetForma(), lemma.GetFormLength(), lemma.GetRuleId());

    YASSERT(NULL != f);
    return f;
}

int TSimpleLemmer::GetLemmaQuality(MystemLemmaHandle* h) {
    return GET_FUNC(Lib, MystemLemmaQuality)(h);
}

int TSimpleLemmer::GetFormsCount(MystemFormsHandle* h) {
    return GET_FUNC(Lib, MystemFormsCount)(h);
}

MystemFormHandle* TSimpleLemmer::GetFormHandle(MystemFormsHandle* h, int i) {
    return GET_FUNC(Lib, MystemForm)(h, i);
}

TChar* TSimpleLemmer::GetFormText(MystemFormHandle* h) {
    return GET_FUNC(Lib, MystemFormText)(h);
}

int TSimpleLemmer::GetFormTextLen(MystemFormHandle* h) {
    return GET_FUNC(Lib, MystemFormTextLen)(h);
}

char** TSimpleLemmer::GetFormFlexGram(MystemFormHandle* h) {
    return GET_FUNC(Lib, MystemFormFlexGram)(h);
}

int TSimpleLemmer::GetFormFlexGramNum(MystemFormHandle* h) {
    return GET_FUNC(Lib, MystemFormFlexGramNum)(h);
}


void TSimpleLemmer::DeleteAnalyses(MystemAnalysesHandle *analyses) {
    GET_FUNC(Lib, MystemDeleteAnalyses)(analyses);
}

void TSimpleLemmer::DeleteForms(MystemFormsHandle *forms) {
    GET_FUNC(Lib, MystemDeleteForms)(forms);  
}

size_t AnalyzeWord(const TChar* word, size_t len, TWLemmaArray& out, TLangMask langmask, const docLanguage* doclangs, const TAnalyzeWordOpt& opt) {
    return Singleton<TSimpleLemmer>()->AnalyzeWord(word, len, out, langmask, doclangs, opt);
}

size_t Generate(const TSimpleLemma &lemma, TWordformArray& out, const char* needed_grammar) {
    TClueGrammarFiltr grm_filtr(needed_grammar);
    TAutoPtr<TFormGenerator> gen = lemma.Generator(&grm_filtr);
    TSimpleWordform form;
    while (gen->GenerateNext(form))
        out.push_back(form);

    return out.size();
}


}
