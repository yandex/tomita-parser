#include "fact-types.h"


/*----------------------------------class fact_field_descr_t-----------------------------*/

void fact_field_descr_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_Field_type);
    ::Save(buffer, m_strFieldName);
    ::Save(buffer, m_bNecessary);
    ::Save(buffer, m_bBoolValue);
    ::Save(buffer, m_bSaveTextInfo);
    ::Save(buffer, m_bCapitalizeFirstLetter1);
    ::Save(buffer, m_bCapitalizeFirstLetter2);
    ::Save(buffer, m_bCapitalizeAll);
    ::Save(buffer, m_StringValue);
    ::Save(buffer, m_options);
}

void fact_field_descr_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_Field_type);
    ::Load(buffer, m_strFieldName);
    ::Load(buffer, m_bNecessary);
    ::Load(buffer, m_bBoolValue);
    ::Load(buffer, m_bSaveTextInfo);
    ::Load(buffer, m_bCapitalizeFirstLetter1);
    ::Load(buffer, m_bCapitalizeFirstLetter2);
    ::Load(buffer, m_bCapitalizeAll);
    ::Load(buffer, m_StringValue);
    ::Load(buffer, m_options);
}

/*----------------------------------class CFieldAlgorithmInfo-----------------------------*/

void CFieldAlgorithmInfo::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_bConcatenation);
    ::Save(buffer, m_eFactAlgorithm);
    ::Save(buffer, m_iNormGrammems);
}

void CFieldAlgorithmInfo::Load(TInputStream* buffer)
{
    ::Load(buffer, m_bConcatenation);
    ::Load(buffer, m_eFactAlgorithm);
    ::Load(buffer, m_iNormGrammems);
}

/*----------------------------------class fact_field_reference_t-----------------------------*/

void fact_field_reference_t::Save(TOutputStream* buffer) const
{
    fact_field_descr_t::Save(buffer);
    CFieldAlgorithmInfo::Save(buffer);

    ::Save(buffer, m_strFactTypeName);
    ::Save(buffer, m_strSourceFieldName);
    ::Save(buffer, m_strSourceFactName);
    ::Save(buffer, m_bHasValue);
}

void fact_field_reference_t::Load(TInputStream* buffer)
{
    fact_field_descr_t::Load(buffer);
    CFieldAlgorithmInfo::Load(buffer);

    ::Load(buffer, m_strFactTypeName);
    ::Load(buffer, m_strSourceFieldName);
    ::Load(buffer, m_strSourceFactName);
    ::Load(buffer, m_bHasValue);
}

/*----------------------------------class fact_type_t-----------------------------*/

fact_type_t::fact_type_t()
{
    reset();
}

void fact_type_t::reset()
{
    m_strFactTypeName.clear();
    m_Fields.clear();
    m_bTemporary = false;
    m_bNoLead = false;
}

void fact_type_t::Save(TOutputStream* buffer) const
{
    ::Save(buffer, m_strFactTypeName);
    ::Save(buffer, m_bTemporary);
    ::Save(buffer, m_bNoLead);
    ::Save(buffer, m_Fields);
}

void fact_type_t::Load(TInputStream* buffer)
{
    ::Load(buffer, m_strFactTypeName);
    ::Load(buffer, m_bTemporary);
    ::Load(buffer, m_bNoLead);
    ::Load(buffer, m_Fields);
}
