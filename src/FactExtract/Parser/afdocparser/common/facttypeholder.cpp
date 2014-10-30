#include "facttypeholder.h"

#include <kernel/gazetteer/protopool.h>

#include <FactExtract/Parser/afdocparser/builtins/facttypes_base.pb.h>


void TFactTypeHolder::Add(const TFactType& factType) {
    TPair<TIndex::iterator, bool> ins = Index.insert(MakePair(factType.m_strFactTypeName, FactTypes.size()));
    if (ins.second)
        FactTypes.push_back(factType);
}

EFactFieldType SelectFieldType(const NGzt::TProtoPool& pool, const NGzt::TFieldDescriptor* field) {
    switch (field->type()) {
        case NGzt::TFieldDescriptor::TYPE_BOOL: return BoolField;
        case NGzt::TFieldDescriptor::TYPE_STRING: return TextField;
        case NGzt::TFieldDescriptor::TYPE_MESSAGE: {
            const NGzt::TDescriptor* fieldType = field->message_type();
            if (pool.IsSameAsGeneratedType(fieldType, NFactType::TFio::descriptor()))
                return FioField;
            if (pool.IsSameAsGeneratedType(fieldType, NFactType::TDate::descriptor()))
                return DateField;
        }
        default: return FactFieldTypesCount;     // skip such fields
    };
}

static void ConvertToFactType(const NGzt::TProtoPool& pool, const NGzt::TDescriptor* descr, fact_type_t& factType) {
    factType.m_strFactTypeName = descr->name();
    for (int i = 0; i < descr->field_count(); ++i) {
        const NGzt::TFieldDescriptor* field = descr->field(i);
        EFactFieldType fieldType = SelectFieldType(pool, field);
        if (fieldType == FactFieldTypesCount)
            continue;

        fact_field_descr_t factField(field->name(), fieldType);
        factField.m_bNecessary = field->is_required();
        factField.m_bBoolValue = (fieldType == BoolField && field->default_value_bool());
        if (fieldType == TextField && !field->default_value_string().empty())
            factField.m_StringValue = UTF8ToWide(field->default_value_string());
        factField.m_bSaveTextInfo = field->options().GetExtension(NFactType::info);
        switch (field->options().GetExtension(NFactType::normcase)) {
            case NFactType::TITLE_CASE: factField.m_bCapitalizeFirstLetter1 = true; break;
            case NFactType::CAMEL_CASE: factField.m_bCapitalizeFirstLetter2 = true; break;
            case NFactType::UPPER_CASE: factField.m_bCapitalizeAll = true; break;
            default: break;
        }
        factType.m_Fields.push_back(factField);
    }
}

void TFactTypeHolder::AddFromGazetteer(const NGzt::TProtoPool& pool) {
    for (size_t i = 0; i < pool.Size(); ++i)
        if (pool.IsSubType(pool[i], NFactType::TFact::descriptor())) {
            TFactType factType;
            ConvertToFactType(pool, pool[i], factType);
            Add(factType);
        }
}
