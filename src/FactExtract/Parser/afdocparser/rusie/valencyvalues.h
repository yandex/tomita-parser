#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>


struct SValencyValue
{
    SValencyValue(const SWordHomonymNum& wh)
        : m_iVal(-1)
        , m_Value(wh)
    {
    }

    SValencyValue(const SWordHomonymNum& wh, int iVal, const Wtroka& strVal)
        : m_iVal(iVal)
        , m_strVal(strVal)
        , m_Value(wh)
    {
    }

    int m_iVal; //номер валентности в статье
    Wtroka m_strVal;
    SWordHomonymNum m_Value;
};

struct SValenciesValues
{
    SValenciesValues(const SWordHomonymNum& wh, int iSubord, const article_t* pArt, const SDictIndex& index)
        : m_pArt(pArt), m_iSubord(iSubord), m_DictIndex(index), m_NodeWH(wh) {}

    bool Includes(const SValenciesValues& valenciesValues, const CWordVector& allWords) const;

    SWordHomonymNum GetValue(int iVal)
    {
        for (size_t i = 0; i < m_values.size(); i++) {
            if (m_values[i].m_iVal == iVal)
                return m_values[i].m_Value;
        }
        return SWordHomonymNum();
    }

    SWordHomonymNum GetValue(const Wtroka& strValency) const
    {
        for (size_t i = 0; i < m_values.size(); ++i)
            if (m_values[i].m_strVal == strValency)
                return m_values[i].m_Value;
        return SWordHomonymNum();
    }

    const article_t* m_pArt;
    yvector<SValencyValue> m_values;
    int m_iSubord;
    SDictIndex m_DictIndex;
    SWordHomonymNum m_NodeWH;

    bool LessByValuesCount(const SValenciesValues& v) const
    {
        return m_values.size() < v.m_values.size();
    }
};
