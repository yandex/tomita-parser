#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "word.h"
#include <FactExtract/Parser/afdocparser/common/wordspair.h>



class CWordVector
{
public:
    CWordVector();

    void Reset();
    size_t OriginalWordsCount() const
    {
        return m_AllWords[s_OriginalWords].size();
    }

    const CHomonym& operator[] (const SWordHomonymNum& WordHomonymNum) const
    {
        this->Check(WordHomonymNum);
        bool WordsType = WordHomonymNum.m_bOriginalWord;
        return *m_AllWords[WordsType][WordHomonymNum.m_WordNum]->m_Homonyms[WordHomonymNum.m_HomNum];
    }

    CHomonym& operator[] (const SWordHomonymNum& WordHomonymNum)
    {
        this->Check(WordHomonymNum);
        bool WordsType = WordHomonymNum.m_bOriginalWord;
        return *m_AllWords[WordsType][WordHomonymNum.m_WordNum]->m_Homonyms[WordHomonymNum.m_HomNum];
    }

    const CWord& operator[] (int i) const
    {
        return *(m_AllWords[s_OriginalWords][i]);
    }

    CWord& operator[] (int i)
    {
        return *(m_AllWords[s_OriginalWords][i]);
    }

    const CWord& GetWord(const SWordHomonymNum& WordHomonymNum) const
    {
        this->Check(WordHomonymNum);
        return *(m_AllWords[WordHomonymNum.m_bOriginalWord][WordHomonymNum.m_WordNum]);
    }

    CWord& GetWord(const SWordHomonymNum& WordHomonymNum)
    {
        this->Check(WordHomonymNum);
        return *(m_AllWords[WordHomonymNum.m_bOriginalWord][WordHomonymNum.m_WordNum]);
    }

    void AddOriginalWord(CWord* word);
    void AddMultiWord(CWord* word);

    size_t GetMultiWordsCount() const
    {
        return m_AllWords[s_MultiWordWords].size();
    }

    size_t GetAllWordsCount() const;

    CWord& GetAnyWord(size_t i);
    const CWord& GetAnyWord(size_t i) const;

    const CWord& GetMultiWord(size_t i) const
    {
        return *(m_AllWords[s_MultiWordWords][i]);
    }

    CWord& GetMultiWord(size_t i) {
        return *(m_AllWords[s_MultiWordWords][i]);
    }

    int    FindMultiWord(int iW1, int iW2) const;
    int    HasMultiWord_i(int iW) const;
    bool HasMultiWord(int iW) const;
    void freeData();
    CWord& GetOriginalWord(int i) const
    {
        return *(m_AllWords[s_OriginalWords][i]);
    }

    const SWordHomonymNum& GetEndWordWH() const;

    void SortMultiWords();
    Wtroka SentToString() const;
    void SentToString(Wtroka& str) const;

    Wtroka ToString(const CWordsPair& wp) const;
    Wtroka ToString(const CWord& word) const {
        return ToString(word.GetSourcePair());
    }

    static const int s_OriginalWords;
    static const int s_MultiWordWords;

private:
    void SetPairToEOSWord();
    void Check(const SWordHomonymNum& WordHomonymNum) const
    {
        YASSERT(WordHomonymNum.m_WordNum >= 0 && (size_t)WordHomonymNum.m_WordNum < m_AllWords[WordHomonymNum.m_bOriginalWord].size());
    }

protected:

    // в m_AllWords[1] - слова из текста
    // в m_AllWords[0] - многословные порожденные слова
    yvector<CWord*> m_AllWords[2];
    //адрес слова, стоящего за последним словом в предложении
    //нужно для Томиты
    SWordHomonymNum    m_EndWordWH;
};
