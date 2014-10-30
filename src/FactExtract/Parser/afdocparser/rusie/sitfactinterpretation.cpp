#include "sitfactinterpretation.h"
#include <util/string/strip.h>

CSitFactInterpretation::CSitFactInterpretation(const CWordVector& Words, yvector<CWordSequence*>& tempFacts)
    : CCommonGrammarInterpretation(Words, CWorkGrammar(), tempFacts,
                                   yvector<SWordHomonymNum>(), NULL)
{
}

TIntrusivePtr<CFactsWS> CSitFactInterpretation::Interpret(const SValenciesValues& valValues)
{
    if (valValues.m_values.size() == 0)
        return NULL;

    TIntrusivePtr<CFactsWS> factWS(new CFactsWS());
    factWS->PutAuxArticle(valValues.m_DictIndex);
    SWordHomonymNum val;
    for (size_t i = 0; i < valValues.m_values.size(); i++) {
        const SValencyValue& value =    valValues.m_values[i];
        const or_valencie_list_t& valencie = valValues.m_pArt->get_subordination(valValues.m_iSubord)[value.m_iVal];
        for (size_t j = 0; j < valencie.m_interpretations.size(); j++) {
            const fact_field_reference_t& fact_field = valencie.m_interpretations[j];
            FillDefaultFieldValue(fact_field, factWS->m_Facts, value.m_Value);
            FillFactField(fact_field, value.m_Value, factWS->m_Facts);
            val = value.m_Value;
        }
    }

    if (!val.IsValid())
        return NULL;

    const CWord& w = m_Words.GetWord(val);

    CheckNecessaryFields(factWS->m_Facts, w.GetSourcePair());

    if (factWS->GetFactsCount() == 0)
        return NULL;
    else
        return factWS;
}

void CSitFactInterpretation::FillConstFieldValue(const fact_field_reference_t& fact_field, yvector<CFactFields>& newFacts, const SWordHomonymNum& valValue)
{
    const CWord& w = m_Words.GetWord(valValue);
    switch (fact_field.m_Field_type) {
        case TextField: {
            CTextWS ws;
            ws.SetArtificialPair(w.GetSourcePair());
            ws.AddLemma(SWordSequenceLemma(fact_field.m_StringValue));
            FillWSFactField(fact_field, ws, newFacts);
            break;
        } case BoolField: {
            CBoolWS ws(fact_field.m_bBoolValue);
            ws.SetPair(w.GetSourcePair());
            FillWSFactField(fact_field, ws, newFacts);
            break;
        } default:
            break;
    }
}

void CSitFactInterpretation::FillDefaultFieldValue(const fact_field_reference_t& fact_field, yvector<CFactFields>& newFacts, const SWordHomonymNum& valValue)
{
    if ((fact_field.m_Field_type != TextField) &&
        (fact_field.m_Field_type != BoolField))
        return;

    const fact_type_t* pFactType = &GlobalDictsHolder->RequireFactType(fact_field.m_strFactTypeName);
    const fact_field_descr_t* pFieldDescr = pFactType->get_field(fact_field.m_strFieldName);

    if (pFieldDescr == NULL)
        ythrow yexception() << "Bad field name in \"CSitFactInterpretation::FillDefaultFieldValue\".";

    if ((pFieldDescr->m_StringValue.length() == 0) &&
        (fact_field.m_Field_type == TextField))
        return;

    fact_field_reference_t ref;
    *((fact_field_descr_t*)&ref) = *pFieldDescr;
    ref.m_strFactTypeName = fact_field.m_strFactTypeName;
    FillConstFieldValue(ref, newFacts, valValue);
}

//Пытаемся из данного омонима (SWordHomonymNum& value) вытащить CWordSequence, который потом нужно засунуть
//в поле CFactFields по адресу fact_field
// если SourceWordSequence у value сам является множеством фактов (CFactsWS), то пытаемся вытащить из этих фактов
//      вытащить зачение поля указанного в  fact_field.m_strFieldName вне зависимости от типа факта (fact_field.m_strFactTypeName)
//иначе  пытаемся преобразовать SourceWordSequence в зависимости от типа поля  fact_field.m_Field_type к CFioWS, CTextWS или CDateWS

void CSitFactInterpretation::FillFactField(const fact_field_reference_t& fact_field, const SWordHomonymNum& value, yvector<CFactFields>& newFacts)
{

    if (fact_field.m_bHasValue &&
        ((fact_field.m_Field_type == TextField) ||
            (fact_field.m_Field_type == BoolField))) {
        FillConstFieldValue(fact_field, newFacts, value);
        return;
    }

    CWordSequence* pWS = m_Words[value].GetSourceWordSequence();
    if (pWS == NULL)
        ythrow yexception() << "Bad wordsequence in \"CSitFactInterpretation::FillFactField\"";
    CFactsWS* factWS = dynamic_cast<CFactsWS*>(pWS);

    if (factWS != NULL)
        FillFactFieldFromFactWS(fact_field, factWS , newFacts);
    else
        switch (fact_field.m_Field_type) {
            case FioField:  FillFioFactField(fact_field, pWS, newFacts, value); break;
            case TextField: FillTextFactField(fact_field, pWS, newFacts); break;
            case DateField: FillDateFactField(fact_field, pWS, newFacts); break;
            default: break;
        }
}

Wtroka CSitFactInterpretation::ToString() const
{
    Wtroka str;
    for (size_t i = 0; i < m_Words.GetMultiWordsCount(); ++i) {
        const CWord* pW = &m_Words.GetOriginalWord(i);
        if (i > 0 && (!pW->IsPunct() || pW->IsOpenBracket() || pW->IsCloseBracket()))
            str += ' ';
        str += pW->GetOriginalText();
    }
    return StripString(str);
}

void CSitFactInterpretation::FillFioFactField(const fact_field_reference_t& fact_field, const CWordSequence* pWS,
                                              yvector<CFactFields>& newFacts, const SWordHomonymNum& val)
{
    const CFioWordSequence* pFioWS = dynamic_cast<const CFioWordSequence*>(pWS);
    if (pFioWS == NULL) {
        Stroka s = Substitute("Can't get fio value for $0.$1 in sentence \"$2\"", fact_field.m_strFactTypeName,
                              fact_field.m_strFieldName, NStr::DebugEncode(this->ToString()));
        Cerr << s << Endl;
        return;
    }
    CFioWS newFioWS(*pFioWS);
    newFioWS.SetMainWord(val);
    FillWSFactField(fact_field, newFioWS, newFacts);
}

void CSitFactInterpretation::FillTextFactField(const fact_field_reference_t& fact_field, const CWordSequence* pWS,
                                               yvector<CFactFields>& newFacts)
{
    CTextWS newWS;
    const CTextWS* pTextWS = dynamic_cast<const CTextWS*>(pWS);
    if (pTextWS) {
        newWS = *pTextWS;
    } else {
        *((CWordSequence*)(&newWS)) = *pWS;
    }

    FillWSFactField(fact_field, newWS, newFacts);
}

void CSitFactInterpretation::FillDateFactField(const fact_field_reference_t& fact_field, const CWordSequence* pWS, yvector<CFactFields>& newFacts)
{
    FillWSFactField(fact_field, *pWS, newFacts);
}

void CSitFactInterpretation::FillWSFactField(const fact_field_reference_t& fact_field, const CWordsPair& newWS, yvector<CFactFields>& newFacts)
{
    bool bAdded = false;
    if (newFacts.size() > 0) {
        for (size_t i = 0; i < newFacts.size(); i++) {
            if (newFacts[i].GetFactName() == fact_field.m_strFactTypeName.c_str()) {
                newFacts[i].AddValue(fact_field.m_strFieldName, newWS, fact_field.m_Field_type, fact_field.m_bConcatenation);
                bAdded = true;
            }
        }
    }

    if (!bAdded) {
        CFactFields fact(fact_field.m_strFactTypeName.c_str());
        fact.AddValue(fact_field.m_strFieldName, newWS, fact_field.m_Field_type, fact_field.m_bConcatenation);
        newFacts.push_back(fact);
    }
}
