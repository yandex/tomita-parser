#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "textwordsequence.h"
#include <FactExtract/Parser/afdocparser/rus/fiowordsequence.h>
#include "factaddress.h"
#include <FactExtract/Parser/afdocparser/common/datetime.h>

typedef CDateGroup CDateWS;

class CBoolWS: public CWordsPair
{

public:
    inline CBoolWS()
        : m_bValue(false)
    {
    }

    inline CBoolWS(const CWordsPair& wp, bool val)
        : CWordsPair(wp)
        , m_bValue(val)
    {
    }

    explicit inline CBoolWS(bool val)
        : m_bValue(val)
    {
    }

    inline void PutValue(bool val)
    {
        m_bValue = val;
    }

    inline bool GetValue() const
    {
        return m_bValue;
    }
protected:
    bool m_bValue;
};

class CFioWS: public CFioWordSequence,
              public CFieldAlgorithmInfo
{
public:
    CFioWS(const SFullFIO& fio)
        : CFioWordSequence(fio)
    {
    }

    CFioWS()
        : CFioWordSequence()
    {
    }

    CFioWS(const CFioWordSequence& fiows)
        : CFioWordSequence(fiows)
    {
    }

    CFioWS& operator=(const CFioWordSequence& fioWS)
    {
        if ((CFioWordSequence*)this != &fioWS)
            (CFioWordSequence&)(*this) = fioWS;
        return *this;
    }
};

struct SLeadInfo
{
    SLeadInfo():    m_iFirstSent(-1),
                    m_iLastSent(-1)
    {}
    //интервал предложений, которые нужно включать в лид
    int m_iFirstSent;
    int m_iLastSent;
    bool operator== (const SLeadInfo& leadInfo) const
    {
        return (m_iFirstSent == leadInfo.m_iFirstSent) && (m_iLastSent == leadInfo.m_iLastSent);
    }
};

class CFactFields
{
    friend class CFactsWS;
public:
    SLeadInfo m_LeadInfo;

    typedef ymap<Stroka, CTextWS>::const_iterator TextFieldIterConst;
    typedef ymap<Stroka, CBoolWS>::const_iterator BoolFieldIterConst;
    typedef ymap<Stroka, CDateWS>::const_iterator DateFieldIterConst;
    typedef ymap<Stroka, CFioWS>::const_iterator FioFieldIterConst;

    inline CFactFields(const Stroka& strFactName)
        : m_strFactName(strFactName)
    {
    }

    void AddValue(const Stroka& strFieldName, const CTextWS& value, bool bConCat = false);
    inline void AddValue(const Stroka& strFieldName, const CBoolWS& value);
    inline void AddValue(const Stroka& strFieldName, const CDateWS& value);
    inline void AddValue(const Stroka& strFieldName, const CFioWS& value);
    void AddValue(const Stroka& strFieldName, const CWordsPair& value, EFactFieldType type, bool bConCat = false);

    inline bool HasValue(const Stroka& strFieldName) const;

    inline bool HasTextValue(const Stroka& strFieldName) const;
    inline bool HasBoolValue(const Stroka& strFieldName) const;
    inline bool HasDateValue(const Stroka& strFieldName) const;
    inline bool HasFioValue(const Stroka& strFieldName) const;

    inline CTextWS* GetTextValue(const Stroka& strFieldName);
    inline const CTextWS* GetTextValue(const Stroka& strFieldName) const;

    inline bool GetBoolValue(const Stroka& strFieldName) const;

    inline CDateWS* GetDateValue(const Stroka& strFieldName);
    inline const CDateWS* GetDateValue(const Stroka& strFieldName) const;

    inline CFioWS* GetFioValue(const Stroka& strFieldName);
    inline const CFioWS* GetFioValue(const Stroka& strFieldName) const;

    inline const CBoolWS* GetBoolWSValue(const Stroka& strFieldName) const;
    inline CBoolWS* GetBoolWSValue(const Stroka& strFieldName);

    const CWordsPair* GetValue(const Stroka& strFieldName) const;
    CWordsPair* GetValue(const Stroka& strFieldName);

    const CWordsPair* GetAnyNonArtificialPair(yset<const CWordsPair*, SPeriodOrder>& allPairs) const;

    inline const Stroka& GetFactName() const
    {
        return m_strFactName;
    }

    void UpdateArtificialPairs();

    inline TextFieldIterConst GetTextFieldsSequenceBegin() const;
    inline TextFieldIterConst GetTextFieldsSequenceEnd() const;
    inline FioFieldIterConst GetFioFieldsSequenceBegin() const;
    inline FioFieldIterConst GetFioFieldsSequenceEnd() const;

    void DeleteField(const Stroka& strFactFieldName);

protected:

    bool UpdateArtificialPair(CWordsPair& wp, const CWordsPair*& pNonArtificialPair, yset<const CWordsPair*, SPeriodOrder>& allPairs);

    ymap<Stroka, CTextWS> m_TextFields;
    ymap<Stroka, CBoolWS> m_BoolFields;
    ymap<Stroka, CDateWS> m_DateFields;
    ymap<Stroka, CFioWS> m_FioFields;

    typedef ymap<Stroka, CTextWS>::iterator TextFieldIter;
    typedef ymap<Stroka, CBoolWS>::iterator BoolFieldIter;
    typedef ymap<Stroka, CDateWS>::iterator DateFieldIter;
    typedef ymap<Stroka, CFioWS>::iterator FioFieldIter;

    Stroka m_strFactName;

public:
    // some pre-defined field names
    static const Stroka
        Fio, Post, Status,
        CompanyName, CompanyShortName, CompanyIsLemma, CompanyDescr,
        QuoteValue, IsSubject,
        Street, StreetDescr, House, Korpus, Settlement, SettlementDescr;
};

class CFactsWS: public CTextWS
{
    friend class CSitFactInterpretation;
    friend class CCommonGrammarInterpretation;

public:
    inline int GetFactsCount() const
    {
        return m_Facts.size();
    }

    inline const CFactFields& GetFact(size_t i) const
    {
        return m_Facts[i];
    }

    inline CFactFields& GetFact(size_t i)
    {
        return m_Facts[i];
    }

    inline void AddFact(const CFactFields& facts)
    {
        m_Facts.push_back(facts);
    }

    inline void DeleteFact(size_t iFactNum)
    {
        m_Facts.erase(m_Facts.begin() + iFactNum);
    }

protected:
    yvector<CFactFields> m_Facts;
};

// inlined definitions

bool CFactFields::HasTextValue(const Stroka& strFieldName) const
{
    return  m_TextFields.find(strFieldName) != m_TextFields.end();
}

bool CFactFields::HasBoolValue(const Stroka& strFieldName) const
{
    return m_BoolFields.find(strFieldName) != m_BoolFields.end();
}

bool CFactFields::HasDateValue(const Stroka& strFieldName) const
{
    return m_DateFields.find(strFieldName) != m_DateFields.end();
}

bool CFactFields::HasFioValue(const Stroka& strFieldName) const
{
    return m_FioFields.find(strFieldName) != m_FioFields.end();
}

bool CFactFields::HasValue(const Stroka& strFieldName) const
{
    return  HasTextValue(strFieldName) ||
            HasBoolValue(strFieldName) ||
            HasDateValue(strFieldName) ||
            HasFioValue(strFieldName);
}

void CFactFields::AddValue(const Stroka& strFieldName, const CBoolWS& value)
{
    YASSERT(value.IsValid());
    m_BoolFields[strFieldName] = value;
}

void CFactFields::AddValue(const Stroka& strFieldName, const CDateWS& value)
{
    YASSERT(value.IsValid());
    m_DateFields[strFieldName] = value;
}

void CFactFields::AddValue(const Stroka& strFieldName, const CFioWS& value)
{
    YASSERT(value.IsValid());
    m_FioFields.insert(MakePair(strFieldName, value));
}

CTextWS* CFactFields::GetTextValue(const Stroka& strFieldName)
{
    TextFieldIter it = m_TextFields.find(strFieldName);
    return (it != m_TextFields.end()) ? &it->second : NULL;
}

const CTextWS* CFactFields::GetTextValue(const Stroka& strFieldName) const
{
    TextFieldIterConst it = m_TextFields.find(strFieldName);
    return (it != m_TextFields.end()) ? &it->second : NULL;
}

bool CFactFields::GetBoolValue(const Stroka& strFieldName) const
{
    BoolFieldIterConst it = m_BoolFields.find(strFieldName);
    return (it != m_BoolFields.end()) ? it->second.GetValue() : false;
}

const CDateWS* CFactFields::GetDateValue(const Stroka& strFieldName) const
{
    DateFieldIterConst it = m_DateFields.find(strFieldName);
    return (it != m_DateFields.end()) ? &it->second : NULL;
}

CDateWS* CFactFields::GetDateValue(const Stroka& strFieldName)
{
    DateFieldIter it = m_DateFields.find(strFieldName);
    return (it != m_DateFields.end()) ? &it->second : NULL;
}

const CFioWS* CFactFields::GetFioValue(const Stroka& strFieldName) const
{
    FioFieldIterConst it = m_FioFields.find(strFieldName);
    return (it != m_FioFields.end()) ? &it->second : NULL;
}

CFioWS* CFactFields::GetFioValue(const Stroka& strFieldName)
{
    FioFieldIter it = m_FioFields.find(strFieldName);
    return (it != m_FioFields.end()) ? &it->second : NULL;
}

const CBoolWS* CFactFields::GetBoolWSValue(const Stroka& strFieldName) const
{
    BoolFieldIterConst it = m_BoolFields.find(strFieldName);
    return (it != m_BoolFields.end()) ? &it->second : NULL;
}

CBoolWS* CFactFields::GetBoolWSValue(const Stroka& strFieldName)
{
    BoolFieldIter it = m_BoolFields.find(strFieldName);
    return (it != m_BoolFields.end()) ? &it->second : NULL;
}

CFactFields::FioFieldIterConst CFactFields::GetFioFieldsSequenceBegin() const
{
    return m_FioFields.begin();
}

CFactFields::FioFieldIterConst CFactFields::GetFioFieldsSequenceEnd() const
{
    return m_FioFields.end();
}

CFactFields::TextFieldIterConst CFactFields::GetTextFieldsSequenceBegin() const
{
    return m_TextFields.begin();
}

CFactFields::TextFieldIterConst CFactFields::GetTextFieldsSequenceEnd() const
{
    return m_TextFields.end();
}
