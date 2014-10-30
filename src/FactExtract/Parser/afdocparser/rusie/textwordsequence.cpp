#include "textwordsequence.h"

#include <FactExtract/Parser/common/utilit.h>

CTextWS::CTextWS() :
    CWordSequenceWithAntecedent(),
    CFieldAlgorithmInfo()
{
    m_bGen = false;
    m_bAdj = false;
}

void CTextWS::SetGen()
{
    m_bGen = true;
}

void CTextWS::SetAdj()
{
    m_bAdj = true;
}

void CTextWS::UpdateFlags(const CTextWS& Other)
{
    m_bGen = m_bGen || Other.m_bGen;
    m_bAdj = m_bAdj || Other.m_bAdj;
}

void CTextWS::AddPostfix(const CTextWS& Postfix)
{
    m_MainWords.insert(m_MainWords.end(), Postfix.m_MainWords.begin(), Postfix.m_MainWords.end());
    UpdateFlags(Postfix);
}

void CTextWS::AddPrefix(const CTextWS& Prefix)
{
    m_MainWords.insert(m_MainWords.begin(), Prefix.m_MainWords.begin(), Prefix.m_MainWords.end());
    UpdateFlags(Prefix);
}

void CTextWS::AddWordInfo(const CTextWS& ws, bool bMainWords)
{
    for (size_t i = 0; i < ws.m_MainWords.size(); ++i)
        if (bMainWords)
            m_MainWords.push_back(ws.m_MainWords[i]);
        else {
            Wtroka s = ws.m_MainWords[i];
            if (s.size() && s[0] == '*')
                m_MainWords.push_back(s.substr(1));
            else
                m_MainWords.push_back(ws.m_MainWords[i]);
        }
}

void CTextWS::AddWordInfo(const Wtroka& s, bool isMain)
{
    const wchar16 MWCHAR = '*';
    m_MainWords.push_back(s);
    if (isMain && !s.has_prefix(&MWCHAR, 1))
        m_MainWords.back().prepend(MWCHAR);
};

void CTextWS::ClearWordsInfos()
{
    m_MainWords.clear();
}

Wtroka CTextWS::GetMainWordsDump() const
{
    Wtroka Result;
    if (m_bGen || m_bAdj) {
        if (m_bGen)
            Result += CharToWide("gen!");
        if (m_bAdj)
            Result += CharToWide("not_noun!");
        Result += ';';
    }

    for (yvector<Wtroka>::const_iterator it = m_MainWords.begin(); it != m_MainWords.end(); ++it) {
        if (NStr::IsEqual(*it, "&"))
            ythrow yexception() << "seems to be obsolete";
        else {
            Result += *it;
            Result += ';';
        }
    }
    return Result;
};

CTextWS::CTextWS(CWordSequenceWithAntecedent& ws)
{
    *((CWordSequenceWithAntecedent*)this) = ws;
}

DECLARE_STATIC_NAMES(TRusPronouns, "его ее их мой моя твой твоя свой своя");

bool IsPronoun(const Wtroka& s)
{
    return TRusPronouns::Has(s);
}

bool CTextWS::HasPossPronouns() const
{
    for (size_t k = 0; k < GetLemmas().size(); ++k)
        if (IsPronoun(GetLemmas()[k].m_LemmaToShow))
            return true;
    return false;
}

void CTextWS::DeletePronouns()
{
    yvector<size_t> erase;
    for (size_t k = 0; k < GetLemmas().size(); ++k) {
        const Wtroka& s = GetLemmas()[k].m_LemmaToShow;
        DECLARE_STATIC_RUS_WORD(Zhe, "же");
        if (IsPronoun(s) || s == Zhe)
            erase.push_back(k);
    }

    if (!erase.empty()) {
        yvector<SWordSequenceLemma> good;
        good.reserve(GetLemmas().size());
        for (size_t i = 0, ii = 0; i < GetLemmas().size(); ++i) {
            if (ii < erase.size() && i == erase[ii])
                ++ii;   // skip this lemma
            else
                good.push_back(GetLemmas()[i]);
        }
        bool isnorm = IsNormalizedLemma();
        ClearLemmas();
        for (size_t i = 0; i < good.size(); ++i)
            AddLemma(good[i], isnorm);
    }
}

void CTextWS::Clear()
{
    ClearLemmas();
    ClearWordsInfos();
}

