#include "factfields.h"
#include <util/generic/cast.h>

//---------------------------CFactFields-----------------------------------------------------

// initialization of pre-defined factfields names
#define INIT_PREDEFINED_FIELD_NAME(fieldname) const Stroka CFactFields::fieldname(#fieldname)

INIT_PREDEFINED_FIELD_NAME(Fio);
INIT_PREDEFINED_FIELD_NAME(Post);
INIT_PREDEFINED_FIELD_NAME(Status);

INIT_PREDEFINED_FIELD_NAME(CompanyName);
INIT_PREDEFINED_FIELD_NAME(CompanyShortName);
INIT_PREDEFINED_FIELD_NAME(CompanyIsLemma);
INIT_PREDEFINED_FIELD_NAME(CompanyDescr);

INIT_PREDEFINED_FIELD_NAME(QuoteValue);
INIT_PREDEFINED_FIELD_NAME(IsSubject);

INIT_PREDEFINED_FIELD_NAME(Street);
INIT_PREDEFINED_FIELD_NAME(StreetDescr);
INIT_PREDEFINED_FIELD_NAME(House);
INIT_PREDEFINED_FIELD_NAME(Korpus);
INIT_PREDEFINED_FIELD_NAME(Settlement);
INIT_PREDEFINED_FIELD_NAME(SettlementDescr);

#undef INIT_PREDEFINED_FIELD_NAME


void CFactFields::AddValue(const Stroka& strFieldName, const CTextWS& value, bool bConCat /*= false*/)
{
    YASSERT(value.IsValid());
    ymap<Stroka, CTextWS>::iterator it = m_TextFields.find(strFieldName);
    if (!bConCat)
        m_TextFields[strFieldName] = value;
    else {
        if (it != m_TextFields.end())
            for (size_t i = 0; i < value.GetLemmas().size(); ++i)
                it->second.AddLemma(value.GetLemmas()[i]);
        else
            m_TextFields[strFieldName] = value;
    }
}

void CFactFields::AddValue(const Stroka& strFieldName, const CWordsPair& value, EFactFieldType type, bool bConCat /*= false*/)
{
    switch (type) {
        case FioField:
            AddValue(strFieldName, *CheckedCast<const CFioWS*>(&value));
            break;

        case TextField:
            AddValue(strFieldName, *CheckedCast<const CTextWS*>(&value), bConCat);
            break;

        case DateField:
            AddValue(strFieldName, *CheckedCast<const CDateWS*>(&value));
            break;

        case BoolField:
            AddValue(strFieldName, *CheckedCast<const CBoolWS*>(&value));
            break;

        default:
            ythrow yexception() << "Can't add value in \"CFactFields::AddValue\".";
    }

}

const CWordsPair* CFactFields::GetAnyNonArtificialPair(yset<const CWordsPair*, SPeriodOrder>& allPairs) const
{
    const CWordsPair* res = NULL;

    for (FioFieldIterConst it = m_FioFields.begin(); it != m_FioFields.end(); ++it)
        if (!it->second.IsArtificialPair()) {
            if (res == NULL)
                res = &it->second;
            allPairs.insert(&it->second);
        }

    for (TextFieldIterConst it = m_TextFields.begin(); it != m_TextFields.end(); ++it)
        if (!it->second.IsArtificialPair()) {
            if (res == NULL)
                res = &it->second;
            allPairs.insert(&it->second);
        }

    for (DateFieldIterConst it = m_DateFields.begin(); it != m_DateFields.end(); ++it)
        if (!it->second.IsArtificialPair()) {
            if (res == NULL)
                res = &it->second;
            allPairs.insert(&it->second);
        }

    //m_BoolFields - не проверяем, они всегда искусственные (не имеют настоящей цепочки в предложении)
    return res;
}

bool CFactFields::UpdateArtificialPair(CWordsPair& wp, const CWordsPair*& pNonArtificialPair,
                                       yset<const CWordsPair*, SPeriodOrder>& allPairs)
{
    if (pNonArtificialPair == NULL) {
        pNonArtificialPair = GetAnyNonArtificialPair(allPairs);
        if (pNonArtificialPair == NULL)
            return false;
    }
    if (allPairs.find(&wp) == allPairs.end())
        wp.SetPair(*pNonArtificialPair);
    return true;
}

//пытаемся тем значениям, которым пары слов присвоены искусственно (например, m_BoolFields), приписать более
//реалистичные значения из данного факта
void CFactFields::UpdateArtificialPairs()
{
    const CWordsPair* pNonArtificialPair = NULL;
    yset<const CWordsPair*, SPeriodOrder> allPairs;

    for (TextFieldIter it = m_TextFields.begin(); it != m_TextFields.end(); ++it)
        if (it->second.IsArtificialPair())
            if (!UpdateArtificialPair(it->second, pNonArtificialPair, allPairs))
                return;

    for (DateFieldIter it = m_DateFields.begin(); it != m_DateFields.end(); ++it)
        if (it->second.IsArtificialPair())
            if (!UpdateArtificialPair(it->second, pNonArtificialPair, allPairs))
                return;

    for (BoolFieldIter it = m_BoolFields.begin(); it != m_BoolFields.end(); ++it)
        if (!UpdateArtificialPair(it->second, pNonArtificialPair, allPairs))
            return;
}

const CWordsPair* CFactFields::GetValue(const Stroka& strFieldName) const
{
    const CWordsPair* res = NULL;

    res = GetTextValue(strFieldName);
    if (res != NULL)
        return res;

    res = GetDateValue(strFieldName);
    if (res != NULL)
        return res;

    res = GetFioValue(strFieldName);
    if (res != NULL)
        return res;

    return GetBoolWSValue(strFieldName);
}

CWordsPair* CFactFields::GetValue(const Stroka& strFieldName)
{
    CWordsPair* res = NULL;

    res = GetTextValue(strFieldName);
    if (res != NULL)
        return res;

    res = GetDateValue(strFieldName);
    if (res != NULL)
        return res;

    res = GetFioValue(strFieldName);
    if (res != NULL)
        return res;

    return GetBoolWSValue(strFieldName);
}

void CFactFields::DeleteField(const Stroka& strFieldName)
{
    {
        CFactFields::FioFieldIter it = m_FioFields.find(strFieldName);
        if (it != m_FioFields.end()) {
            m_FioFields.erase(it);
            return;
        }
    }

    {
        CFactFields::TextFieldIter it = m_TextFields.find(strFieldName);
        if (it != m_TextFields.end()) {
            m_TextFields.erase(it);
            return;
        }
    }

    {
        CFactFields::DateFieldIter it = m_DateFields.find(strFieldName);
        if (it != m_DateFields.end()) {
            m_DateFields.erase(it);
            return;
        }
    }

    {
        CFactFields::BoolFieldIter it = m_BoolFields.find(strFieldName);
        if (it != m_BoolFields.end()) {
            m_BoolFields.erase(it);
            return;
        }
    }

}
