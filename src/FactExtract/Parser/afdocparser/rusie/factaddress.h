#pragma once

#include <FactExtract/Parser/common/wordhomonymnum.h>

struct SMWAddress: public SWordHomonymNum
{
    SMWAddress()
    {
        m_iSentNum = -1;
    }

    SMWAddress(int iSentNum, int WordNum, int HomNum, bool bOriginalWord = true) :
        SWordHomonymNum(WordNum, HomNum, bOriginalWord)
    {
        m_iSentNum = iSentNum;
    }

    SMWAddress(int iSentNum, const SWordHomonymNum& WordHomonymNum) :
        SWordHomonymNum(WordHomonymNum)
    {
        m_iSentNum = iSentNum;
    }

    virtual bool IsValid() const
    {
        return SWordHomonymNum::IsValid() && (m_iSentNum != -1);
    }

    bool operator==(const SMWAddress& MWAddress) const
    {
        return    SWordHomonymNum::operator ==(MWAddress) &&
            m_iSentNum == MWAddress.m_iSentNum;
    }

    virtual void Reset()
    {
        m_iSentNum = -1;
        SWordHomonymNum::Reset();

    }

    int m_iSentNum;

};

struct SFactAddress: public SMWAddress
{
    SFactAddress()
    {
        m_iSentNum = -1;
        m_iFactNum = -1;
    }

    SFactAddress(int iSentNum, int iFactNum, int WordNum, int HomNum, bool bOriginalWord = true) :
        SMWAddress(iSentNum, WordNum, HomNum, bOriginalWord)
    {
        m_iFactNum = iFactNum;
    }

    SFactAddress(int iSentNum, int iFactNum, const SWordHomonymNum& WordHomonymNum) :
        SMWAddress(iSentNum, WordHomonymNum)
    {
        m_iFactNum = iFactNum;
    }

    virtual bool IsValid() const
    {
        return SWordHomonymNum::IsValid();
    }

    virtual void Reset()
    {
        m_iFactNum = -1;
        SMWAddress::Reset();

    }

    bool operator<(const SFactAddress& FactAddress) const
    {
        if (m_iSentNum != FactAddress.m_iSentNum)
            return m_iSentNum < FactAddress.m_iSentNum;
        if (m_bOriginalWord != FactAddress.m_bOriginalWord)
            return m_bOriginalWord < FactAddress.m_bOriginalWord;
        if (m_WordNum != FactAddress.m_WordNum)
            return m_WordNum < FactAddress.m_WordNum;
        if (m_HomNum != FactAddress.m_HomNum)
            return m_HomNum < FactAddress.m_HomNum;
        if (m_iFactNum != FactAddress.m_iFactNum)
            return m_iFactNum < FactAddress.m_iFactNum;
        return false;
    }

    bool operator==(const SFactAddress& FactAddress) const
    {

        return    SMWAddress::operator ==(FactAddress) &&
            m_iSentNum == FactAddress.m_iSentNum &&
            m_iFactNum == FactAddress.m_iFactNum;
    }

    int m_iFactNum;
};
