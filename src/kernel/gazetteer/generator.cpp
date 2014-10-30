#include "generator.h"

#include "base.pb.h"
#include "syntax.pb.h"

#include <library/protobuf/util/traits.h>

#include <contrib/libs/protobuf/descriptor.h>

#include <util/charset/codepage.h>

namespace NGzt {

TGztGenerator::TGztGenerator(const TGazetteer* parentGzt, const Stroka& encoding)
    : ParentGzt(parentGzt)
    , GeneratedFile(new TGztFileDescriptorProto)
{
    if (CharsetByName(encoding) == CODES_UNKNOWN)
        ythrow yexception() << "Unknown encoding \"" << encoding << "\"";
    GeneratedFile->set_encoding(encoding);
}

TGztGenerator::~TGztGenerator() {
}

void TGztGenerator::AddImport(const Stroka& fileName) {
    GeneratedFile->add_dependency(fileName);
}

Stroka TGztGenerator::DebugString() {
    return GeneratedFile->DebugString();
}


void TGztGenerator::AddOptions(const TGztOptions& options) {
    // options is just an article as any other
    AddArticle(options);
}
/*
void TGztGenerator::AddSimpleArticle(const TWtringBuf& keyText, const TStringBuf& type = STRINGBUF("TArticle"), const TWtringBuf& title = TWtringBuf());
void TGztGenerator::AddSimpleArticle(const TSearchKey& key,     const TStringBuf& type = STRINGBUF("TArticle"), const TWtringBuf& title = TWtringBuf());
*/



typedef NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto> TFieldsProto;
static bool AddField(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field);

namespace {

template <NProtoBuf::FieldDescriptor::CppType cppType, bool repeated>
struct TAddField {
    typedef NProtoBuf::TFieldTraits<cppType, repeated> TTraits;

    static inline TArticleFieldDescriptorProto& AddNew(TFieldsProto& dst, const NProtoBuf::FieldDescriptor* field, TFieldValueDescriptorProto::EValueType type) {
        TArticleFieldDescriptorProto& dstField = *dst.Add();
        dstField.set_name(field->name());
        dstField.mutable_value()->set_type(type);
        return dstField;
    }

    static bool Int(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
        int initSize = dst.size();
        for (size_t i = 0; i < TTraits::Size(src, field); ++i) {
            TArticleFieldDescriptorProto& dstField = AddNew(dst, field, TFieldValueDescriptorProto::TYPE_INTEGER);
            dstField.mutable_value()->set_int_number(TTraits::Get(src, field, i));
        }
        return dst.size() > initSize;
    }

    static bool Float(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
        int initSize = dst.size();
        for (size_t i = 0; i < TTraits::Size(src, field); ++i) {
            TArticleFieldDescriptorProto& dstField = AddNew(dst, field, TFieldValueDescriptorProto::TYPE_FLOAT);
            dstField.mutable_value()->set_float_number(TTraits::Get(src, field, i));
        }
        return dst.size() > initSize;
    }

    static bool Enum(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
        int initSize = dst.size();
        for (size_t i = 0; i < TTraits::Size(src, field); ++i) {
            TArticleFieldDescriptorProto& dstField = AddNew(dst, field, TFieldValueDescriptorProto::TYPE_INTEGER);
            dstField.mutable_value()->set_int_number(TTraits::Get(src, field, i)->number());        // store enum numbers, not names
        }
        return dst.size() > initSize;
    }

    static bool String(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
        int initSize = dst.size();
        for (size_t i = 0; i < TTraits::Size(src, field); ++i) {
            TArticleFieldDescriptorProto& dstField = AddNew(dst, field, TFieldValueDescriptorProto::TYPE_STRING);
            dstField.mutable_value()->set_string_or_identifier(TTraits::Get(src, field, i));
        }
        return dst.size() > initSize;
    }

    static bool Message(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
        int initSize = dst.size();
        for (size_t i = 0; i < TTraits::Size(src, field); ++i) {
            TArticleFieldDescriptorProto& dstField = AddNew(dst, field, TFieldValueDescriptorProto::TYPE_BLOCK);
            const NProtoBuf::Message* msg = TTraits::Get(src, field, i);
            for (int j = 0; j < msg->GetDescriptor()->field_count(); ++j)
                AddField(*dstField.mutable_value()->mutable_sub_field(), *msg, msg->GetDescriptor()->field(j));
        }
        return dst.size() > initSize;
    }

};

}   // anonymous namespace


template <bool repeated>
static bool AddFieldImpl(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
    using namespace NProtoBuf;
    switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:   return TAddField<FieldDescriptor::CPPTYPE_INT32, repeated>::Int(dst, src, field);
        case FieldDescriptor::CPPTYPE_INT64:   return TAddField<FieldDescriptor::CPPTYPE_INT64, repeated>::Int(dst, src, field);
        case FieldDescriptor::CPPTYPE_UINT32:  return TAddField<FieldDescriptor::CPPTYPE_UINT32, repeated>::Int(dst, src, field);
        case FieldDescriptor::CPPTYPE_UINT64:  return TAddField<FieldDescriptor::CPPTYPE_UINT64, repeated>::Int(dst, src, field);
        case FieldDescriptor::CPPTYPE_BOOL:    return TAddField<FieldDescriptor::CPPTYPE_BOOL, repeated>::Int(dst, src, field);

        case FieldDescriptor::CPPTYPE_DOUBLE:  return TAddField<FieldDescriptor::CPPTYPE_DOUBLE, repeated>::Float(dst, src, field);
        case FieldDescriptor::CPPTYPE_FLOAT:   return TAddField<FieldDescriptor::CPPTYPE_FLOAT, repeated>::Float(dst, src, field);

        case FieldDescriptor::CPPTYPE_ENUM:    return TAddField<FieldDescriptor::CPPTYPE_ENUM, repeated>::Enum(dst, src, field);
        case FieldDescriptor::CPPTYPE_STRING:  return TAddField<FieldDescriptor::CPPTYPE_STRING, repeated>::String(dst, src, field);
        case FieldDescriptor::CPPTYPE_MESSAGE: return TAddField<FieldDescriptor::CPPTYPE_MESSAGE, repeated>::Message(dst, src, field);

        default:
            ythrow yexception() << "Unknown cpp type " << (size_t)field->cpp_type();
    }
}

static inline bool AddField(TFieldsProto& dst, const NProtoBuf::Message& src, const NProtoBuf::FieldDescriptor* field) {
    return field->is_repeated() ? AddFieldImpl<true>(dst, src, field) : AddFieldImpl<false>(dst, src, field);
}


// direct protobuf article
void TGztGenerator::AddArticle(const NProtoBuf::Message& src, const Stroka& title) {

    const NProtoBuf::Descriptor* descr = src.GetDescriptor();
    TArticleDescriptorProto* dst = GeneratedFile->add_article();
    dst->set_type(descr->full_name());
    dst->set_name(title);

    for (int i = 0; i < descr->field_count(); ++i)
        AddField(*dst->mutable_field(), src, descr->field(i));
}

TSearchKey* TGztGenerator::AddKey(NProtoBuf::RepeatedPtrField<TSearchKey>& keys, const Stroka& encodedKeyText,
                                  TMorphMatchRule::EType morph, docLanguage lang) {
    TSearchKey* key = keys.Add();
    key->add_text(encodedKeyText);
    if (morph != TMorphMatchRule::ALL_FORMS)
        key->add_morph()->set_type(morph);
    if (lang != LANG_UNK)
        key->add_lang()->add_allow(static_cast<TLanguageFilter::ELang>(lang));       // this cast is safe, see base.proto for details
    return key;
}



TAutoPtr<TGazetteer> TGztGenerator::MakeGazetteer() const {
    TGazetteerBuilder builder;
    builder.IntegrateFiles(BuiltinProto);
    if (ParentGzt)
        builder.UseDescriptors(*ParentGzt);
    return builder.BuildFromProtobuf(*GeneratedFile) ? builder.MakeGazetteer() : NULL;
}


}   // namespace NGzt
