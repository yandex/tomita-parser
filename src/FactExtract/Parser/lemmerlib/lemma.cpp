#include <library/lemmer/dictlib/grammarhelper.h>

#include <util/charset/wide.h>
#include <util/generic/chartraits.h>
#include "lemma.h"
#include "form.h"
#include "extlemmer.h"


namespace NLemmer {

TGrammarFiltr* GetNewDummyGrammarFiltr(const char* grammar) {
    Stroka gr[2];
    gr[0] = Stroka(grammar);
    gr[1] = Stroka();
    return new TClueGrammarFiltr(gr);
}

bool TFormGenerator::GenerateNext(TSimpleWordform& form) {
    if (ItPtr.Get() == NULL)
        return false;

    ISimpleWordformIterator& it = *ItPtr;
    for (; it.IsValid(); operator++()) {
        it->ConstructForm(form);
        operator++();
        return true;
    }

    return false;
}

const TLanguage* GetLanguageById(docLanguage id) {
    UNUSED(id);
    return NULL;
}

static bool InitFlexGrammar(const char* stemGr, const char* needGram, Stroka& buf) {
    assert(needGram);
    if (!stemGr)
        stemGr = "";
    buf = "";
    for (;*needGram; needGram++) {
        const TGramClass clNeed = GetClass(*needGram);
        const char *gr = stemGr;
        bool bAdd = true;
        for (; *gr; gr++) {
            const TGramClass cl = GetClass(*gr);
            if (cl == clNeed) {
                if (*gr == *needGram) {
                    bAdd = false;
                    break;
                } else if (cl)
                    return false;
            }
        }
        if (bAdd)
            buf += *needGram;
    }
    return true;
}

TClueGrammarFiltr::TClueGrammarFiltr(const Stroka neededGr[])
{
    for (const Stroka* ps = neededGr; !ps->empty(); ++ps)
        Grammar.push_back(*ps);
    if (Grammar.empty())
        Grammar.push_back("");
};

void TClueGrammarFiltr::SetLemma(const TSimpleLemma& lemma) {
    FlexGram.clear();
    for (size_t i = 0; i < Grammar.size(); ++i) {
        Stroka buf;
        if (InitFlexGrammar(lemma.GetStemGram(), Grammar[i].c_str(), buf))
            FlexGram.push_back(buf);
    }
}

bool TClueGrammarFiltr::CheckFlexGr(const char* flexGram) const {
    if (!IsProperStemGr())
        return false;
    for (size_t i = 0; i < FlexGram.size(); ++i) {
        if (!flexGram || !*flexGram) {
            if (FlexGram[i].empty())
                return true;
        } else if (NTGrammarProcessing::HasAllGrams(flexGram, FlexGram[i].c_str()))
            return true;
    }
    return false;
}

}

TAutoPtr<NLemmer::TFormGenerator> TSimpleLemma::Generator(const NLemmer::TGrammarFiltr* filter) const {
    return new NLemmer::TDefaultFormGenerator(*this, filter);
}

TSimpleWordformIterator::TSimpleWordformIterator(const TSimpleLemma& lemma)
    : Lemma(lemma)
    , Form(lemma)
    , Current(NULL)
    , FlexPos(0) {
    Lemma.SetDefaultText(lemma, TWtringBuf(lemma.GetText(), lemma.GetTextLength()));
    Form.SetDefaultText(lemma, TWtringBuf(lemma.GetForma(), lemma.GetFormLength()));

    FormsHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormsHandle(lemma);
    FormsCount = Singleton<NLemmer::TSimpleLemmer>()->GetFormsCount(FormsHandle);
    FormsPos = 0;

    Lemma.SetForm(Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, FormsPos));
    Form.SetForm(Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, FormsPos));
    Current = &Form;
}

void TSimpleWordformIterator::operator++() {
    MystemFormHandle *fh = Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, FormsPos);
    size_t flexCount = Singleton<NLemmer::TSimpleLemmer>()->GetFormFlexGramNum(fh);
    if (flexCount > 0 && FlexPos < flexCount - 1) {
        FlexPos++;
        Form.SetForm(Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, FormsPos), FlexPos);
        return;
    }

    FormsPos++;
    FlexPos = 0;
    if (FormsPos < FormsCount) {
        Form.SetForm(Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, FormsPos));
    } else {
        Current = NULL;
    }
}

void TSimpleWordformKit::SetDefaultText(const TSimpleLemma& lemma, const TWtringBuf& text) {
    Lemma = &lemma;

    MystemFormsHandle* FormsHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormsHandle(lemma);
    YASSERT(NULL != FormsHandle);
    FormHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, 0);
    YASSERT(NULL != FormHandle);
}

TWtringBuf TSimpleWordformKit::ConstructText(TChar* buffer, size_t bufferSize) const {
    const TChar* formText = Singleton<NLemmer::TSimpleLemmer>()->GetFormText(FormHandle);
    size_t formLen = Singleton<NLemmer::TSimpleLemmer>()->GetFormTextLen(FormHandle);

    size_t charsToCopy = (formLen + 1) > bufferSize ? bufferSize - 1 : formLen;
    TCharTraits<TChar>::Copy(buffer, formText, charsToCopy);
    buffer[charsToCopy] = TChar(0);

    return TWtringBuf(buffer, charsToCopy);
}

void TSimpleWordformKit::ConstructText(Wtroka& text) const {
    text = DefaultText;
}

void TSimpleWordformKit::ConstructForm(TSimpleWordform& form) const {
    ConstructText(form.w);
    form.StemGram = Lemma->GetStemGram();
    char** flexGr = Singleton<NLemmer::TSimpleLemmer>()->GetFormFlexGram(FormHandle);
    for (int i = 0; i < Singleton<NLemmer::TSimpleLemmer>()->GetFormFlexGramNum(FormHandle); i++) {
        form.FlexGram.push_back(flexGr[i]);
    }
}

TSimpleWordformKit::TSimpleWordformKit(const TSimpleLemma& lemma, size_t flexIndex)
    : Lemma(&lemma)
    , FlexGrammar(NULL)
{
    if (flexIndex < lemma.FlexGramNum())
        FlexGrammar = lemma.GetFlexGram()[flexIndex];

    Wtroka lcLemma = lemma.GetSourceText();
    lcLemma.to_lower();
    DefaultText = lcLemma;

    MystemFormsHandle* FormsHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormsHandle(lemma);
    YASSERT(NULL != FormsHandle);
    FormHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, 0);
    YASSERT(NULL != FormHandle);
}

TSimpleWordformKit::TSimpleWordformKit(const TSimpleLemma& lemma)
    : Lemma(&lemma)
    , FormHandle(NULL)
    , FlexGrammar(NULL) {

    if (NULL != lemma.GetFlexGram())
        FlexGrammar = lemma.GetFlexGram()[0];

    Wtroka lcLemma = lemma.GetSourceText();
    lcLemma.to_lower();
    DefaultText = lcLemma;

    MystemFormsHandle* FormsHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormsHandle(lemma);
    YASSERT(NULL != FormsHandle);
    FormHandle = Singleton<NLemmer::TSimpleLemmer>()->GetFormHandle(FormsHandle, 0);
    YASSERT(NULL != FormHandle);
}


const char* TSimpleWordformKit::GetFlexGram() const {
    return FlexGrammar;
}

void TSimpleWordformKit::SetForm(MystemFormHandle* form, size_t flexPos) {
    FormHandle = form;
    char **pp = Singleton<NLemmer::TSimpleLemmer>()->GetFormFlexGram(FormHandle);
    if (NULL != pp)
        FlexGrammar = pp[flexPos];

    const TChar* formText = Singleton<NLemmer::TSimpleLemmer>()->GetFormText(FormHandle);
    DefaultText = formText;
}

namespace NLemmerAux {
void TSimpleLemmaSetter::SetNormalizedForma(const TChar* form, size_t len) {
    Lemma.FormNormalizedLen = Min(len, (size_t) MAXWORD_BUF - 1);
    memcpy(Lemma.FormNormalized, form, Lemma.FormNormalizedLen * sizeof(TChar));
    Lemma.FormNormalized[Lemma.FormNormalizedLen] = 0;
}

void TSimpleLemmaSetter::SetLemma(const TChar* lemmaText, size_t lemmaLen, const TChar* formText, size_t formLen, const char* stemGram, const char* const* grs, size_t count, size_t flexLen, ui32 ruleId, int quality, const Wtroka& src) {
    Lemma.LemmaLen = Min(lemmaLen, (size_t)MAXWORD_BUF - 1);
    memcpy(Lemma.LemmaText, lemmaText, Lemma.LemmaLen * sizeof(TChar));
    Lemma.LemmaText[Lemma.LemmaLen] = 0;

    Lemma.FormNormalizedLen = Min(formLen, (size_t)MAXWORD_BUF - 1);
    memcpy(Lemma.FormNormalized, formText, Lemma.FormNormalizedLen * sizeof(TChar));
    Lemma.FormNormalized[Lemma.FormNormalizedLen] = 0;

    Lemma.StemGram = stemGram;
    SetFlexGrs(grs, count);

    memcpy(Lemma.FormNormalized, lemmaText, Lemma.LemmaLen * sizeof(TChar));
    Lemma.FormNormalized[Lemma.LemmaLen] = 0;
    Lemma.FormNormalizedLen = Lemma.LemmaLen;

    Lemma.FlexLen = flexLen;
    Lemma.RuleId = ruleId;
    Lemma.SourceText = src;
    Lemma.Quality = quality;
}

size_t TSimpleLemmaSetter::AddFlexGr(const char* flexGr) {
    if (Lemma.GramNum < MAX_GRAM_PER_WORD) {
        Lemma.FlexGram[Lemma.GramNum] = flexGr;
        ++Lemma.GramNum;
    }
    return Lemma.GramNum;
}

void TSimpleLemmaSetter::SetFlexGrs(const char* const* grs, size_t count) {
    Lemma.GramNum = Min(count, MAX_GRAM_PER_WORD);
    memcpy(Lemma.FlexGram, grs, Lemma.GramNum * sizeof(char*));
}

void TSimpleLemmaSetter::SetQuality(ui32 quality) {
    Lemma.Quality = quality;
}
}
