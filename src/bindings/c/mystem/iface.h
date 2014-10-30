#pragma once

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void MystemAnalysesHandle;
typedef void MystemLemmaHandle;
typedef void MystemFormsHandle;
typedef void MystemFormHandle;
typedef unsigned short int TSymbol;

MystemAnalysesHandle* MystemAnalyze(TSymbol* word, int len);
void MystemDeleteAnalyses(MystemAnalysesHandle* analyses);
int MystemAnalysesCount(MystemAnalysesHandle* analyses);

MystemLemmaHandle* MystemLemma(MystemAnalysesHandle* analyses, int i);

TSymbol* MystemLemmaText(MystemLemmaHandle* lemma);
int     MystemLemmaTextLen(MystemLemmaHandle* lemma);
TSymbol* MystemLemmaForm(MystemLemmaHandle* lemma);
int     MystemLemmaFormLen(MystemLemmaHandle* lemma);
int     MystemLemmaQuality(MystemLemmaHandle* lemma);
char*   MystemLemmaStemGram(MystemLemmaHandle* lemma);
char**  MystemLemmaFlexGram(MystemLemmaHandle* lemma);
int     MystemLemmaFlexGramNum(MystemLemmaHandle* lemma);
int     MystemLemmaFlexLen(MystemLemmaHandle* lemma);
int     MystemLemmaRuleId(MystemLemmaHandle* lemma);

MystemFormsHandle* MystemGenerate(MystemLemmaHandle* lemma);
void MystemDeleteForms(MystemFormsHandle* forms);
int MystemFormsCount(MystemFormsHandle* forms);

MystemFormHandle* MystemForm(MystemFormsHandle* forms, int i);

TSymbol* MystemFormText(MystemFormHandle* form);
int     MystemFormTextLen(MystemFormHandle* form);
char*   MystemFormStemGram(MystemFormHandle* form);
char**  MystemFormFlexGram(MystemFormHandle* form);
int     MystemFormFlexGramNum(MystemFormHandle* form);

#if defined(__cplusplus)
}
#endif
