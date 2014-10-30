#include "json2proto.h"

#include <library/json/json_value.h>

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/descriptor.h>

#include <util/string/cast.h>

#define JSON_TO_FIELD(EProtoCppType, json, JsonCheckType, ProtoSet, JsonGet) \
    case FieldDescriptor::EProtoCppType: {                              \
        if (!json.JsonCheckType()) {                                    \
            ythrow yexception() << "Invalid type of JSON field: "       \
                                << #JsonCheckType << "() failed while " \
                                << #EProtoCppType << " is expected.";   \
        }                                                               \
        reflection->ProtoSet(&proto, &field, json.JsonGet());           \
        break;                                                          \
    }

static void
JsonEnum2Field(const NJson::TJsonValue& json,
               google::protobuf::Message& proto,
               const google::protobuf::FieldDescriptor& field)
{
    using namespace google::protobuf;

    const Reflection* reflection = proto.GetReflection();
    YASSERT(!!reflection);

    const EnumDescriptor* enumField = field.enum_type();
    YASSERT(!!enumField);

    /// @todo configure name/numerical value
    const EnumValueDescriptor* enumFieldValue = NULL;

    if (json.IsInteger()) {
        enumFieldValue = enumField->FindValueByNumber(json.GetInteger());
    } else if (json.IsString()) {
        enumFieldValue = enumField->FindValueByName(json.GetString());
    } else {
        ythrow yexception() << "Invalid type of JSON enum field: not an integer/string.";
    }

    if (!!enumFieldValue) {
        if (field.is_repeated())
            reflection->AddEnum(&proto, &field, enumFieldValue);
        else
            reflection->SetEnum(&proto, &field, enumFieldValue);
    } else {
        ythrow yexception() << "Invalid value of JSON enum field.";
    }
}

static void
Json2SingleField(const NJson::TJsonValue& json,
                 google::protobuf::Message& proto,
                 const google::protobuf::FieldDescriptor& field)
{
    using namespace google::protobuf;

    if (!json.Has(field.name())
        || json[field.name()].GetType() == NJson::JSON_UNDEFINED
        || json[field.name()].GetType() == NJson::JSON_NULL)
    {
        if (field.is_required() && !field.has_default_value()) {
            ythrow yexception() << "JSON has no field for required field "
                                << field.name() << ".";
        }

        return;
    }

    const NJson::TJsonValue& fieldJson = json[field.name()];
    const Reflection* reflection = proto.GetReflection();
    YASSERT(!!reflection);

    switch (field.cpp_type()) {
        JSON_TO_FIELD(CPPTYPE_INT32, fieldJson, IsInteger, SetInt32, GetInteger);
        JSON_TO_FIELD(CPPTYPE_INT64, fieldJson, IsInteger, SetInt64, GetInteger);
        JSON_TO_FIELD(CPPTYPE_UINT32, fieldJson, IsInteger, SetUInt32, GetInteger);
        JSON_TO_FIELD(CPPTYPE_UINT64, fieldJson, IsUInteger, SetUInt64, GetUInteger);
        JSON_TO_FIELD(CPPTYPE_DOUBLE, fieldJson, IsDouble, SetDouble, GetDouble);
        JSON_TO_FIELD(CPPTYPE_FLOAT, fieldJson, IsDouble, SetFloat, GetDouble);
        JSON_TO_FIELD(CPPTYPE_BOOL, fieldJson, IsBoolean, SetBool, GetBoolean);
        JSON_TO_FIELD(CPPTYPE_STRING, fieldJson, IsString, SetString, GetString);

    case FieldDescriptor::CPPTYPE_ENUM: {
        JsonEnum2Field(fieldJson, proto, field);
        break;
    }

    case FieldDescriptor::CPPTYPE_MESSAGE: {
        Message* innerProto = reflection->MutableMessage(&proto, &field);
        YASSERT(!!innerProto);
        NProtobufJson::Json2Proto(fieldJson, *innerProto);

        break;
    }

    default:
        ythrow yexception() << "Unknown protobuf field type: "
                            << static_cast<int>(field.cpp_type()) << ".";
    }
}

static void
Json2RepeatedField(const NJson::TJsonValue& json,
                   google::protobuf::Message& proto,
                   const google::protobuf::FieldDescriptor& field)
{
    using namespace google::protobuf;

    if (!json.Has(field.name()))
        return;

    const NJson::TJsonValue& fieldJson = json[field.name()];
    if (fieldJson.GetType() == NJson::JSON_UNDEFINED
        || fieldJson.GetType() == NJson::JSON_NULL)
        return;

    if (fieldJson.GetType() != NJson::JSON_ARRAY) {
        ythrow yexception() << "JSON field doesn't represent an array for "
                            << field.name()
                            << "(actual type is "
                            << static_cast<int>(fieldJson.GetType()) << ").";
    }

    const Reflection* reflection = proto.GetReflection();
    YASSERT(!!reflection);

    typedef NJson::TJsonValue::TArray TJsonArray;
    const TJsonArray& jsonArray = fieldJson.GetArray();
    for (TJsonArray::const_iterator it = jsonArray.begin(); it != jsonArray.end(); ++it) {
        const NJson::TJsonValue& jsonValue = *it;

        switch (field.cpp_type()) {
            JSON_TO_FIELD(CPPTYPE_INT32, jsonValue, IsInteger, AddInt32, GetInteger);
            JSON_TO_FIELD(CPPTYPE_INT64, jsonValue, IsInteger, AddInt64, GetInteger);
            JSON_TO_FIELD(CPPTYPE_UINT32, jsonValue, IsInteger, AddUInt32, GetInteger);
            JSON_TO_FIELD(CPPTYPE_UINT64, jsonValue, IsUInteger, AddUInt64, GetUInteger);
            JSON_TO_FIELD(CPPTYPE_DOUBLE, jsonValue, IsDouble, AddDouble, GetDouble);
            JSON_TO_FIELD(CPPTYPE_FLOAT, jsonValue, IsDouble, AddFloat, GetDouble);
            JSON_TO_FIELD(CPPTYPE_BOOL, jsonValue, IsBoolean, AddBool, GetBoolean);
            JSON_TO_FIELD(CPPTYPE_STRING, jsonValue, IsString, AddString, GetString);

        case FieldDescriptor::CPPTYPE_ENUM: {
            JsonEnum2Field(*it, proto, field);
            break;
        }

        case FieldDescriptor::CPPTYPE_MESSAGE: {
            Message* innerProto = reflection->AddMessage(&proto, &field);
            YASSERT(!!innerProto);
            NProtobufJson::Json2Proto(*it, *innerProto);

            break;
        }

        default:
            ythrow yexception() << "Unknown protobuf field type: "
                                << static_cast<int>(field.cpp_type()) << ".";
        }
    }
}

namespace NProtobufJson {

void Json2Proto(const NJson::TJsonValue& json, google::protobuf::Message& proto) {
    /// @todo add ability to choose between re-creation and merging
    proto.Clear();

    const google::protobuf::Descriptor* descriptor = proto.GetDescriptor();
    YASSERT(!!descriptor);

    for (int f = 0, endF = descriptor->field_count(); f < endF; ++f) {
        const google::protobuf::FieldDescriptor* field = descriptor->field(f);
        YASSERT(!!field);

        if (field->is_repeated())
            Json2RepeatedField(json, proto, *field);
        else
            Json2SingleField(json, proto, *field);
    }
}

} // namespace NProtobufJson
