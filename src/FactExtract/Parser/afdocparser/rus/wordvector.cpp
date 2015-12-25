#include "wordvector.h"

#include <FactExtract/Parser/common/utilit.h>
#include <util/string/strip.h>


CWordVector::CWordVector()
{
    const wchar16 EOS_WORD[] = {'E', 'O', 'S', 0};

    CHomonym* pH = new CHomonym(TMorph::GetMainLanguage(), EOS_WORD);
    CWord* pEndWord = new CWord(EOS_WORD, false);
    pEndWord->AddRusHomonym(pH);
    pEndWord->m_typ = UnknownPrim;
    m_AllWords[s_MultiWordWords].push_back(pEndWord);
    m_EndWordWH.m_bOriginalWord = false;
    m_EndWordWH.m_HomNum = 0;
    m_EndWordWH.m_WordNum = m_AllWords[s_MultiWordWords].size() - 1;
    SetPairToEOSWord();
}

const int CWordVector::s_OriginalWords = 1;
const int CWordVector::s_MultiWordWords = 0;

void CWordVector::freeData()
{
    for (size_t i = 0; i < m_AllWords[s_MultiWordWords].size(); ++i)
        delete m_AllWords[s_MultiWordWords][i];

    m_AllWords[s_MultiWordWords].clear();
}

bool my_sort_predicate2(CWord* e1, CWord* e2)
{
    return e1->LessBySourcePair(e2);
}

void CWordVector::SortMultiWords()
{
    std::sort(m_AllWords[s_MultiWordWords].begin(), m_AllWords[s_MultiWordWords].end(), my_sort_predicate2);
}

const SWordHomonymNum& CWordVector::GetEndWordWH() const
{
    return m_EndWordWH;
}

size_t CWordVector::GetAllWordsCount() const
{
    return m_AllWords[s_OriginalWords].size() + m_AllWords[s_MultiWordWords].size();
}

const CWord& CWordVector::GetAnyWord(size_t i) const
{
    if (i < OriginalWordsCount())
        return GetOriginalWord(i);
    else
        return GetMultiWord(i - OriginalWordsCount());
}

CWord& CWordVector::GetAnyWord(size_t i)
{
    if (i < OriginalWordsCount())
        return GetOriginalWord(i);
    else
        return GetMultiWord(i - OriginalWordsCount());
}

void CWordVector::AddMultiWord(CWord* word)
{
    m_AllWords[s_MultiWordWords].push_back(word);
}

void CWordVector::SetPairToEOSWord()
{
    if (m_EndWordWH.IsValid())
        GetWord(m_EndWordWH).SetSourcePair(m_AllWords[s_OriginalWords].size(), m_AllWords[s_OriginalWords].size());
}

void CWordVector::AddOriginalWord(CWord* word)
{
    word->SetSourcePair(m_AllWords[s_OriginalWords].size(), m_AllWords[s_OriginalWords].size());
    m_AllWords[s_OriginalWords].push_back(word);
    SetPairToEOSWord();
}

//int   CWordVector::OriginalWordsCount()
//{
//  return m_AllWords[s_OriginalWords].size();
//}

void CWordVector::Reset()
{
    m_AllWords[s_OriginalWords].clear();
    m_AllWords[s_MultiWordWords].clear();
}

//const CHomonym& CWordVector::operator[] (const SWordHomonymNum& WordHomonymNum) const
//{
//  BOOL WordsType = WordHomonymNum.m_bOriginalWord;

//  if( !((WordsType == s_OriginalWords) || (WordsType == s_MultiWordWords)) )
//      throw std::out_of_range("Wrong WordHomonymNum");

//  if( (WordHomonymNum.m_WordNum < 0) || (WordHomonymNum.m_WordNum >= m_AllWords[WordsType].size()))
//      throw std::out_of_range("Wrong WordHomonymNum");

//  CWord::hom_iter_const it = m_AllWords[WordsType][WordHomonymNum.m_WordNum]->m_Homonyms.find(WordHomonymNum.m_HomNum);

//  if( it == m_AllWords[WordsType][WordHomonymNum.m_WordNum]->m_Homonyms.end() )
//      throw std::out_of_range("Wrong WordHomonymNum.m_HomNum.");

//  return *(it->second);
//}

//const CWord& CWordVector::operator[] (int i) const
//{
//  return *(m_AllWords[s_OriginalWords][i]);
//}

//CWord& CWordVector::operator[] (int i)
//{
//  return *(m_AllWords[s_OriginalWords][i]);
//}

//int CWordVector::GetMultiWordsCount() const
//{
//  return m_AllWords[s_MultiWordWords].size();
//}

//CWord& CWordVector::GetMultiWord(int i)
//{
//  if( (i < 0) || (i >= m_AllWords[s_MultiWordWords].size()) )
//      throw std::out_of_range("Wrong index in GetMultiWord");

//  return *(m_AllWords[s_MultiWordWords][i]);
//}

//CWord& CWordVector::GetOriginalWord(int i)
//{
//  if( (i < 0) || (i >= m_AllWords[s_OriginalWords].size()) )
//      throw std::out_of_range("Wrong index in GetMultiWord");

//  return *(m_AllWords[s_OriginalWords][i]);
//}

//const CWord& CWordVector::GetMultiWord(int i) const
//{
//  if( (i < 0) || (i >= m_AllWords[s_MultiWordWords].size()) )
//      throw std::out_of_range("Wrong index in GetMultiWord");

//  return *(m_AllWords[s_MultiWordWords][i]);
//}

//const CWord& CWordVector::GetWord(const SWordHomonymNum& WordHomonymNum) const
//{
//  BOOL WordsType = WordHomonymNum.m_bOriginalWord;

//  if( !((WordsType == s_OriginalWords) || (WordsType == s_MultiWordWords)) )
//      throw std::out_of_range("Wrong WordHomonymNum");

//  if( (WordHomonymNum.m_WordNum < 0) || (WordHomonymNum.m_WordNum >= m_AllWords[WordsType].size()))
//      throw std::out_of_range("Wrong WordHomonymNum");

//  return *(m_AllWords[WordHomonymNum.m_bOriginalWord][WordHomonymNum.m_WordNum]);
//}

//CWord& CWordVector::GetWord(const SWordHomonymNum& WordHomonymNum)
//{
//  BOOL WordsType = (WordHomonymNum.m_bOriginalWord == true);

//  if( !((WordsType == s_OriginalWords) || (WordsType == s_MultiWordWords)) )
//      throw std::out_of_range("Wrong WordHomonymNum");

//  if( (WordHomonymNum.m_WordNum < 0) || (WordHomonymNum.m_WordNum >= m_AllWords[WordsType].size()))
//      throw std::out_of_range("Wrong WordHomonymNum");

//  return *(m_AllWords[WordHomonymNum.m_bOriginalWord][WordHomonymNum.m_WordNum]);
//}

int CWordVector::FindMultiWord(int iW1, int iW2) const
{
    for (size_t i = 0; i < m_AllWords[s_MultiWordWords].size(); i++) {
        if ((m_AllWords[s_MultiWordWords][i]->GetSourcePair().FirstWord() == iW1) &&
            (m_AllWords[s_MultiWordWords][i]->GetSourcePair().LastWord() == iW2))
            return i;
    }
    return -1;
}

bool CWordVector::HasMultiWord(int iW) const
{
    return HasMultiWord_i(iW) != -1;
}

int CWordVector::HasMultiWord_i(int iW) const
{
    for (size_t i = 0; i < m_AllWords[s_MultiWordWords].size(); i++) {
        if (m_AllWords[s_MultiWordWords][i]->GetSourcePair().Contains(iW))
            return i;
    }
    return -1;
}

Wtroka CWordVector::SentToString() const
{
    Wtroka str;
    SentToString(str);
    return str;
}

void CWordVector::SentToString(Wtroka& str) const
{
    str = ToString(CWordsPair(0, OriginalWordsCount()-1));
}

Wtroka CWordVector::ToString(const CWordsPair& wp) const
{
    if (!wp.IsValid())
        return Wtroka();

    Wtroka str;
    for (int i=wp.FirstWord(); i <= wp.LastWord(); ++i) {
        const CWord& w = GetOriginalWord(i);
        const Wtroka& s = w.GetOriginalText();
        if (i > 0 && RequiresSpace(GetOriginalWord(i-1).GetOriginalText(), s))
            str.append(' ');
        str += s;
    }
    return StripString(str);
}
