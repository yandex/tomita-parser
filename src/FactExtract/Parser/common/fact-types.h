#pragma once

#include <util/generic/vector.h>

#include "toma_constants.h"
#include "serializers.h"

#include <library/lemmer/dictlib/grammarhelper.h>
using NSpike::TGrammarBunch;

struct fact_field_descr_t
{
    fact_field_descr_t(const Stroka& strFieldName, EFactFieldType field_type):
        m_Field_type(field_type),
        m_strFieldName(strFieldName),
        m_bNecessary(true),
        m_bBoolValue(false),
        m_bSaveTextInfo(false),
        m_bCapitalizeAll(false),
        m_bCapitalizeFirstLetter1(false),
        m_bCapitalizeFirstLetter2(false)
    {
    }
    fact_field_descr_t():
        m_Field_type(FactFieldTypesCount),
        m_bNecessary(true),
        m_bBoolValue(false),
        m_bSaveTextInfo(false),
        m_bCapitalizeAll(false),
        m_bCapitalizeFirstLetter1(false),
        m_bCapitalizeFirstLetter2(false)
    {
    }

    EFactFieldType m_Field_type;
    Stroka m_strFieldName;
    bool m_bNecessary;
    bool m_bBoolValue;
    bool m_bSaveTextInfo;
    bool m_bCapitalizeAll;
    bool m_bCapitalizeFirstLetter1;//капитализировать только первую букву первого слова
    bool m_bCapitalizeFirstLetter2;//капитализировать только первую букву каждого слова
    Wtroka m_StringValue;
    yset<Stroka> m_options; //строчки, передающиеся напрямую в xml для запихивания в базу

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

class CFieldAlgorithmInfo
{
public:
    CFieldAlgorithmInfo()
        : m_bConcatenation(false)
    {
    }

    bool HasAlgorithmInfo() const {
        return m_bConcatenation || m_eFactAlgorithm.any();
    }

    bool HasAnaphoraOrEllipsis() const
    {
        const TFactAlgorithmBitSet mask = TFactAlgorithmBitSet(ePronominalAnaphora, eAbbrevAnaphora) |
                                          TFactAlgorithmBitSet(eCompDescAnaphora, ePronCompAnaphora, ePossCompAnaphora);
        return m_eFactAlgorithm.HasAny(mask);
    }

    //маркирует поле для конкатенации +<имя поля> в грамматике
    //пример обработки конкатенации для поля +FdoFact.Post, где FdoFact.Post = "по развитию":
    //'Директор МТС по развитию И. Иванов.'
    //хорошо бы потом переделать на отдельную структуру параллельную CFactSynGroup::m_Facts
    bool m_bConcatenation;
    //вектор всевозможных алгоритмов, проверяющих поле факта
    //например, проверка ShortName для компаний или проверка легких слов для статуса

    TFactAlgorithmBitSet m_eFactAlgorithm;
    //граммемы для управления нормализацией поля интерпретации
    TGramBitSet m_iNormGrammems;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

//в структуре определяются операции, которые выполняются на процедуре Reduce правила грамматики для данного экземпляра поля факта
//fact_field_processing_t
struct fact_field_reference_t : public fact_field_descr_t,
                                 public CFieldAlgorithmInfo
{
    fact_field_reference_t() : fact_field_descr_t(),
                                CFieldAlgorithmInfo(),
                                m_bHasValue(false)
    {}
    Stroka    m_strFactTypeName;
    Stroka    m_strSourceFieldName;    //имя поля, откуда копировать значение этого поля (оператор from)
    Stroka    m_strSourceFactName;    //имя факта, откуда копировать значение этого поля (оператор from)
    bool    m_bHasValue;

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};

struct fact_type_t
{
    fact_type_t();
    Stroka    m_strFactTypeName;
    bool    m_bTemporary;//временная струтура, не для окончательной записи в выходной XML
    bool    m_bNoLead;   //В выходном файле не нужны лиды для этих фактов
    yvector<fact_field_descr_t> m_Fields;
    void reset();

    // this should be inlined here for IFactTypeDictionary in tomalanglib
    const fact_field_descr_t* get_field(const TStringBuf& s) const {
        for (size_t i = 0; i < m_Fields.size(); ++i)
            if (s == TStringBuf(m_Fields[i].m_strFieldName))
                return &m_Fields[i];
        return NULL;
    }

    void Save(TOutputStream* buffer) const;
    void Load(TInputStream* buffer);
};
