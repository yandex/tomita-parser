
#include <util/string/strip.h>
#include "wordsequence.h"
#include "../rus/homonym.h"
#include <FactExtract/Parser/common/string_tokenizer.h>
#include <util/charset/unidata.h>


bool SWordSequenceLemma::HasLemma() const
{
    return !m_LemmaToShow.empty() || !m_CapitalizedLemma.empty();
}

Wtroka SWordSequenceLemma::GetLemma(bool bCapitalized) const
{
    if (bCapitalized && !m_CapitalizedLemma.empty())
        return m_CapitalizedLemma;
    return m_LemmaToShow;
}

CWordSequence::CWordSequence()
{
    m_WSType = NoneWS;
    Grammems.Reset();
    IsArtificialPair_ = false;
    IsConcatenationLemma_ = false;
    IsNormalizedLemma_ = false;
}

void CWordSequence::SetArtificialPair(const CWordsPair& wp)
{
    IsArtificialPair_ = true;
    SetPair(wp);
}

void CWordSequence::SetArtificialPair(bool bArtificial)
{
    IsArtificialPair_ = bArtificial;
}

void CWordSequence::SetConcatenationLemma(bool bConcat)
{
    IsConcatenationLemma_ = bConcat;
}

TKeyWordType CWordSequence::GetKWType() const
{
    if (!m_GztArticle.Empty())
        return m_GztArticle.GetType();
    else if (m_DicIndex.IsValid())
        return GlobalGramInfo->GetAuxKWType(m_DicIndex);
    else
        return NULL;
}

Wtroka CWordSequence::GetArticleTitle() const
{
    if (!m_GztArticle.Empty())
        return m_GztArticle.GetTitle();
    else if (m_DicIndex.IsValid())
        return GlobalGramInfo->GetAuxArticleTitle(m_DicIndex);
    else
        return Wtroka();
}

void CWordSequence::PutWSType(EWordSequenceType type)
{
    m_WSType = type;
}

const SWordHomonymNum& CWordSequence::GetMainWord() const
{
    return m_MainWord;
}

void CWordSequence::SetMainWord(int iW, int iH)
{
    m_MainWord.m_HomNum = iH;
    m_MainWord.m_WordNum = iW;
}

void CWordSequence::SetMainWord(const SWordHomonymNum& wh)
{
    m_MainWord = wh;
}

void CWordSequence::PutArticleOfHomonym(const CHomonym& homonym, EDicType dicType)
{
    if (dicType == KW_DICT && homonym.HasGztArticle())
        PutGztArticle(homonym.GetGztArticle());
    else if (homonym.HasAuxArticle(dicType))
        PutAuxArticle(SDictIndex(dicType, homonym.GetAuxArticleIndex(dicType)));
    else {
        m_DicIndex = SDictIndex();
        m_GztArticle = TGztArticle();
    }
}

void CWordSequence::PutArticle(const TArticleRef& article)
{
    if (article.AuxDic().IsValid())
        PutAuxArticle(article.AuxDic());
    else
        PutGztArticle(article.Gzt());
}

void CWordSequence::PutAuxArticle(const SDictIndex& dicIndex)
{
    m_DicIndex = dicIndex;
    m_GztArticle = TGztArticle();
}

const SDictIndex& CWordSequence::GetAuxArticleIndex() const
{
    return m_DicIndex;
}

void CWordSequence::PutGztArticle(const TGztArticle& gztArticle)
{
    m_DicIndex = SDictIndex();
    m_GztArticle = gztArticle;
}

const TGztArticle& CWordSequence::GetGztArticle() const
{
    return m_GztArticle;
}

void CWordSequence::ClearLemmas()
{
    Lemmas.clear();
    LemmaText.clear();
    CapitalizedLemmaText.clear();
    IsNormalizedLemma_ = false;
}

void CWordSequence::AddLemmas(const yvector<SWordSequenceLemma>& lemmas, bool normalized)
{
    for (size_t i = 0; i < lemmas.size(); ++i)
        AddLemma(lemmas[i], normalized);
}

void CWordSequence::ResetLemmas(const yvector<SWordSequenceLemma>& lemmas, bool normalized)
{
    ClearLemmas();
    AddLemmas(lemmas, normalized);
}

void CWordSequence::ResetLemmas(const yvector<Wtroka>& lemmas, bool normalized) {
    ClearLemmas();
    for (size_t i = 0; i < lemmas.size(); ++i)
        AddLemma(SWordSequenceLemma(lemmas[i]), normalized);
}

void CWordSequence::reset()
{
    CWordsPair::reset();
    ClearLemmas();
}

void CWordSequence::PutNonTerminalSem(const Stroka& sNonTerminalSem)
{
    m_NonTerminalSem = sNonTerminalSem;
}

const Stroka& CWordSequence::GetNonTerminalSem() const
{
    return m_NonTerminalSem;
}



bool RequiresSpace(const SWordSequenceLemma& prevLemma, const SWordSequenceLemma& newLemma)
{
#define SHIFT(i) (ULL(1)<<(i))

    if(newLemma.m_LemmaToShow.length() == 1) {
          if (NUnicode::CharHasType(newLemma.m_LemmaToShow[0],
                  SHIFT(Pe_END) | SHIFT(Po_TERMINAL) | SHIFT(Pe_SINGLE_QUOTE) | SHIFT(Pf_SINGLE_QUOTE) |
                  SHIFT(Pe_QUOTE) | SHIFT(Pf_QUOTE)))
                return false;
    }

    if(prevLemma.m_LemmaToShow.length() == 1) {
        if (NUnicode::CharHasType(prevLemma.m_LemmaToShow[0],
            SHIFT(Ps_START) | SHIFT(Ps_SINGLE_QUOTE) | SHIFT(Pi_SINGLE_QUOTE) |
            SHIFT(Ps_QUOTE) | SHIFT(Pi_QUOTE)))
          return false;
    }
#undef SHIFT
      return true;
}



void CWordSequence::AddLemma(const SWordSequenceLemma& lemma, bool normalized)
{
    if (!normalized)
        IsNormalizedLemma_ = false;
    else if (Lemmas.empty())
        IsNormalizedLemma_ = true;

    if (!Lemmas.empty() && RequiresSpace(Lemmas.back(), lemma)) {
        CapitalizedLemmaText.append(' ');
        LemmaText.append(' ');
    }
    Lemmas.push_back(lemma);
    LemmaText.append(lemma.GetLemma(false));
    CapitalizedLemmaText.append(lemma.GetLemma(true));
}

void CWordSequence::CapitalizeFirstLetter1()
{
    if (Lemmas.size() > 0) {
        CapitalizeFirst1(Lemmas[0].m_CapitalizedLemma);
        CapitalizeFirst1(CapitalizedLemmaText);
    }
}

void CWordSequence::CapitalizeFirst1(Wtroka& s)
{
    NStr::ToFirstUpper(s);
}

void CWordSequence::CapitalizeFirst2(Wtroka& s)
{
    for (size_t i = 0; i < s.size(); ++i)
        if ((i == 0 || ::IsWhitespace(s[i - 1])) && ::IsLower(s[i]))
            s.begin()[i] = static_cast<wchar16>(::ToUpper(s[i]));
}

void CWordSequence::CapitalizeFirstLetter2()
{
    for (size_t i = 0; i < Lemmas.size(); i++)
        CapitalizeFirst2(Lemmas[i].m_CapitalizedLemma);
    CapitalizeFirst2(CapitalizedLemmaText);
}

void CWordSequence::CapitalizeAll()
{
    for (size_t i = 0; i < Lemmas.size(); ++i)
        TMorph::ToUpper(Lemmas[i].m_CapitalizedLemma);
    TMorph::ToUpper(CapitalizedLemmaText);
}

void CWordSequence::SetCommonLemmaString(const Wtroka& commonLemma)
{
    ClearLemmas();
    AddLemma(SWordSequenceLemma(commonLemma), true);
}

