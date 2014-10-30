#pragma once

struct SWordHomonymNum
{
    SWordHomonymNum()
    {
        m_WordNum = -1;
        m_HomNum = -1;
        m_bOriginalWord = true;
    };

    SWordHomonymNum(int WordNum, int HomNum, bool bOriginalWord = true)
    {
        m_WordNum = WordNum;
        m_HomNum = HomNum;
        m_bOriginalWord = bOriginalWord;
    };

    virtual ~SWordHomonymNum() {}

    virtual bool IsValid() const
    {
        return (m_WordNum != -1) && (m_HomNum != -1);
    }

    virtual void Reset()
    {
        m_WordNum = -1;
        m_HomNum = -1;
    }

    bool EqualByWord(const SWordHomonymNum& WordHomonymNum) const
    {
        return (WordHomonymNum.m_WordNum == m_WordNum) &&
               (WordHomonymNum.m_bOriginalWord == m_bOriginalWord);
    }

    bool IsValidWordNum() const
    {
        return (m_WordNum != -1);
    }

    bool operator==(const SWordHomonymNum& WordHomonymNum) const
    {
        return (WordHomonymNum.m_HomNum == m_HomNum) &&
               (WordHomonymNum.m_WordNum == m_WordNum) &&
               (WordHomonymNum.m_bOriginalWord == m_bOriginalWord);
    }

    bool operator < (const SWordHomonymNum& WordHomonymNum) const
    {
        if (WordHomonymNum.m_bOriginalWord != m_bOriginalWord)
            return WordHomonymNum.m_bOriginalWord < m_bOriginalWord;
        if (WordHomonymNum.m_WordNum != m_WordNum)
            return WordHomonymNum.m_WordNum > m_WordNum;
        return WordHomonymNum.m_HomNum > m_HomNum;
    }

    int    m_WordNum;
    int m_HomNum;
    bool m_bOriginalWord;
};
