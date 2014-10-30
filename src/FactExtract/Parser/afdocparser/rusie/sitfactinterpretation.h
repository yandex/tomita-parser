#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "normalization.h"
#include "valencyvalues.h"
#include "dictsholder.h"
#include "commongrammarinterpretation.h"

class CSitFactInterpretation : public CCommonGrammarInterpretation

{
public:
    CSitFactInterpretation(const CWordVector& Words, yvector<CWordSequence*>& tempFacts);
    virtual ~CSitFactInterpretation(void)
    {
    }
    TIntrusivePtr<CFactsWS> Interpret(const  SValenciesValues& valValues);

protected:
    void FillFactField(const fact_field_reference_t& fact_field, const SWordHomonymNum& value, yvector<CFactFields>& newFacts);
    void FillFioFactField(const fact_field_reference_t& fact_field, const CWordSequence* pWS, yvector<CFactFields>& newFacts, const SWordHomonymNum& val);
    void FillTextFactField(const fact_field_reference_t& fact_field, const CWordSequence* pWS, yvector<CFactFields>& newFacts);
    void FillDateFactField(const fact_field_reference_t& fact_field, const CWordSequence* pWS, yvector<CFactFields>& newFacts);
    void FillWSFactField(const fact_field_reference_t& fact_field, const CWordsPair& newWS, yvector<CFactFields>& newFacts);
    void FillDefaultFieldValue(const fact_field_reference_t& fact_field, yvector<CFactFields>& newFacts, const SWordHomonymNum& valValue);
    void FillConstFieldValue(const fact_field_reference_t& fact_field, yvector<CFactFields>& newFacts, const SWordHomonymNum& valValue);
    Wtroka ToString() const;

};
