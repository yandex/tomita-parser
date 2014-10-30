#include "proto2json.h"

#include <library/json/json_value.h>
#include <library/json/json_writer.h>

#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/message.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/system/yassert.h>


namespace {

template <typename TDestination>
void Proto2JsonImpl(const google::protobuf::Message& proto, TDestination& json,
                    const NProtobufJson::TProto2JsonConfig& config);

inline void Proto2JsonNoFlush(const google::protobuf::Message& proto, NJson::TJsonWriter& writer,
                              const NProtobufJson::TProto2JsonConfig& config)
{
    writer.OpenMap();
    Proto2JsonImpl(proto, writer, config);
    writer.CloseMap();
}

class TJsonKeyBuilder {
public:
    template <typename TKey>
    TJsonKeyBuilder(const TKey& key, const NProtobufJson::TProto2JsonConfig& config) {
        using NProtobufJson::TProto2JsonConfig;

        switch (config.FieldNameMode) {
        case TProto2JsonConfig::FieldNameOriginalCase: {
            NewKeyBuf = key;
            break;
        }

        case TProto2JsonConfig::FieldNameLowerCase: {
            NewKeyStr = key;
            NewKeyStr.to_lower();
            NewKeyBuf = NewKeyStr;
            break;
        }

        case TProto2JsonConfig::FieldNameUpperCase: {
            NewKeyStr = key;
            NewKeyStr.to_upper();
            NewKeyBuf = NewKeyStr;
            break;
        }

        default:
            VERIFY_DEBUG(false, "Unknown FieldNameMode.");
        }
    }

    const TStringBuf& GetKey() const {
        return NewKeyBuf;
    }

private:
    TStringBuf NewKeyBuf;
    Stroka NewKeyStr;
};

template <typename TValue>
void Value2Json(const TStringBuf& key, const TValue& value,
                NJson::TJsonValue& json,
                const NProtobufJson::TProto2JsonConfig& config)
{
    UNUSED(config);
    if (!!key.data())
        json.InsertValue(key, value);
    else
        json.AppendValue(value);
}

template <typename TValue>
void Value2Json(const TStringBuf& key, const TValue& value,
                NJson::TJsonWriter& json,
                const NProtobufJson::TProto2JsonConfig& config)
{
    UNUSED(config);
    if (!!key.data())
        json.Write(key);
    json.Write(value);
}

void Value2Json(const TStringBuf& key, const google::protobuf::Message& value,
                NJson::TJsonValue& json,
                const NProtobufJson::TProto2JsonConfig& config)
{
    if (!!key.data())
        Proto2Json(value, json.InsertValue(key, NJson::TJsonValue()), config);
    else
        Proto2Json(value, json.AppendValue(NJson::TJsonValue()), config);
}

void Value2Json(const TStringBuf& key, const google::protobuf::Message& value,
                NJson::TJsonWriter& json,
                const NProtobufJson::TProto2JsonConfig& config)
{
    if (!!key.data())
        json.Write(key);
    Proto2JsonNoFlush(value, json, config);
}

void Value2Json(const TStringBuf& key, const NJson::TJsonValue& value,
                NJson::TJsonWriter& json,
                const NProtobufJson::TProto2JsonConfig& config)
{
    UNUSED(config);
    if (!!key.data())
        json.Write(key);
    json.Write(&value);
}

template <typename TDestination>
void EnumValue2Json(const TStringBuf& key,
                    const google::protobuf::EnumValueDescriptor* value,
                    TDestination& json,
                    const NProtobufJson::TProto2JsonConfig& config)
{
    using namespace NProtobufJson;

    switch (config.EnumMode) {
    case TProto2JsonConfig::EnumNumber: {
        Value2Json(key, value->number(), json, config);
        break;
    }

    case TProto2JsonConfig::EnumName: {
        Value2Json(key, value->name(), json, config);
        break;
    }

    case TProto2JsonConfig::EnumFullName: {
        Value2Json(key, value->full_name(), json, config);
        break;
    }

    case TProto2JsonConfig::EnumNameLowerCase: {
        Stroka newName = value->name();
        newName.to_lower();
        Value2Json(key, newName, json, config);
        break;
    }

    case TProto2JsonConfig::EnumFullNameLowerCase: {
        Stroka newName = value->full_name();
        newName.to_lower();
        Value2Json(key, newName, json, config);
        break;
    }

    default:
        VERIFY_DEBUG(false, "Unknown EnumMode.");
    }
}

template <typename TDestination>
void StringValue2Json(const TStringBuf& key, Stroka& value,
                      TDestination& json,
                      const NProtobufJson::TProto2JsonConfig& config)
{
    for (size_t i = 0, endI = config.StringTransforms.size(); i < endI; ++i) {
        YASSERT(!!config.StringTransforms[i]);
        if (!!config.StringTransforms[i])
            config.StringTransforms[i]->Transform(value);
    }
    Value2Json(key, value, json, config);
}

template <typename TDestination>
void StringValue2Json(const TStringBuf& key, const Stroka& value,
                      TDestination& json,
                      const NProtobufJson::TProto2JsonConfig& config)
{
    Stroka newValue = value;
    StringValue2Json(key, newValue, json, config);
}

NJson::TJsonValue& OpenArray(const TStringBuf& key, NJson::TJsonValue& json) {
    return json.InsertValue(key, NJson::TJsonValue(NJson::JSON_ARRAY));
}

NJson::TJsonWriter& OpenArray(const TStringBuf& key, NJson::TJsonWriter& json) {
    json.Write(key);
    json.OpenArray();
    return json;
}

void CloseArray(NJson::TJsonValue&) {
}

void CloseArray(NJson::TJsonWriter& json) {
    json.CloseArray();
}


template <typename TDestination>
void SingleField2Json(const google::protobuf::Message& proto,
                      const google::protobuf::FieldDescriptor& field,
                      TDestination& json,
                      const NProtobufJson::TProto2JsonConfig& config)
{
    VERIFY(!field.is_repeated(), "field is repeated.");

    using namespace google::protobuf;
    using NProtobufJson::TProto2JsonConfig;

#define FIELD_TO_JSON(EProtoCppType, ProtoGet)                          \
    case FieldDescriptor::EProtoCppType: {                              \
        Value2Json(jsonKeyBuilder.GetKey(), reflection->ProtoGet(proto, &field), json, config); \
        break;                                                          \
    }

    const Reflection* reflection = proto.GetReflection();
    YASSERT(!!reflection);

    TJsonKeyBuilder jsonKeyBuilder(field.name(), config);
    if (reflection->HasField(proto, &field)) {
        switch (field.cpp_type()) {
            FIELD_TO_JSON(CPPTYPE_INT32, GetInt32);
            FIELD_TO_JSON(CPPTYPE_INT64, GetInt64);
            FIELD_TO_JSON(CPPTYPE_UINT32, GetUInt32);
            FIELD_TO_JSON(CPPTYPE_UINT64, GetUInt64);
            FIELD_TO_JSON(CPPTYPE_DOUBLE, GetDouble);
            FIELD_TO_JSON(CPPTYPE_FLOAT, GetFloat);
            FIELD_TO_JSON(CPPTYPE_BOOL, GetBool);
            FIELD_TO_JSON(CPPTYPE_MESSAGE, GetMessage);

        case FieldDescriptor::CPPTYPE_ENUM: {
            EnumValue2Json(jsonKeyBuilder.GetKey(), reflection->GetEnum(proto, &field),
                           json, config);
            break;
        }

        case FieldDescriptor::CPPTYPE_STRING: {
            Stroka scratch;
            const Stroka& value = reflection->GetStringReference(proto, &field, &scratch);
            StringValue2Json(jsonKeyBuilder.GetKey(), value, json, config);
            break;
        }

        default:
            ythrow yexception() << "Unknown protobuf field type: "
                                << static_cast<int>(field.cpp_type()) << ".";
        }
    } else {
        using namespace NProtobufJson;
        switch (config.MissingSingleKeyMode) {
        case TProto2JsonConfig::MissingKeyNull: {
            Value2Json(jsonKeyBuilder.GetKey(), NJson::TJsonValue(NJson::JSON_NULL), json, config);
            break;
        }

            // case TProto2JsonConfig::MissingKeyDefault: {
            //     Value2Json(jsonKeyBuilder.GetKey(), "", json, config);
            //     break;
            // }

        case TProto2JsonConfig::MissingKeySkip:
        default:
            break;
        }
    }
#undef FIELD_TO_JSON
}

template <typename TDestination>
void RepeatedField2Json(const google::protobuf::Message& proto,
                        const google::protobuf::FieldDescriptor& field,
                        TDestination& json,
                        const NProtobufJson::TProto2JsonConfig& config)
{
    VERIFY(field.is_repeated(), "field isn't repeated.");

    using namespace google::protobuf;

#define REPEATED_FIELD_TO_JSON(EProtoCppType, ProtoGet)                 \
    case FieldDescriptor::EProtoCppType: {                              \
        for (size_t i = 0, endI = reflection->FieldSize(proto, &field); i < endI; ++i) { \
            Value2Json(TStringBuf(), reflection->ProtoGet(proto, &field, i), array, config); \
        }                                                               \
        break;                                                          \
    }

    const Reflection* reflection = proto.GetReflection();
    YASSERT(!!reflection);

    TJsonKeyBuilder jsonKeyBuilder(field.name(), config);
    if (reflection->FieldSize(proto, &field) > 0) {
        TDestination& array = OpenArray(jsonKeyBuilder.GetKey(), json);

        switch (field.cpp_type()) {
            REPEATED_FIELD_TO_JSON(CPPTYPE_INT32, GetRepeatedInt32);
            REPEATED_FIELD_TO_JSON(CPPTYPE_INT64, GetRepeatedInt64);
            REPEATED_FIELD_TO_JSON(CPPTYPE_UINT32, GetRepeatedUInt32);
            REPEATED_FIELD_TO_JSON(CPPTYPE_UINT64, GetRepeatedUInt64);
            REPEATED_FIELD_TO_JSON(CPPTYPE_DOUBLE, GetRepeatedDouble);
            REPEATED_FIELD_TO_JSON(CPPTYPE_FLOAT, GetRepeatedFloat);
            REPEATED_FIELD_TO_JSON(CPPTYPE_BOOL, GetRepeatedBool);
            REPEATED_FIELD_TO_JSON(CPPTYPE_MESSAGE, GetRepeatedMessage);

        case FieldDescriptor::CPPTYPE_ENUM: {
            for (int i = 0, endI = reflection->FieldSize(proto, &field); i < endI; ++i) {
                EnumValue2Json(TStringBuf(), reflection->GetRepeatedEnum(proto, &field, i),
                               array, config);
            }
            break;
        }

        case FieldDescriptor::CPPTYPE_STRING: {
            Stroka scratch;
            for (int i = 0, endI = reflection->FieldSize(proto, &field); i < endI; ++i) {
                const Stroka& value =
                    reflection->GetRepeatedStringReference(proto, &field, i, &scratch);
                StringValue2Json(TStringBuf(), value, array, config);
            }
            break;
        }

        default:
            ythrow yexception() << "Unknown protobuf field type: "
                                << static_cast<int>(field.cpp_type()) << ".";
        }

        CloseArray(json);
    } else {
        using namespace NProtobufJson;
        switch (config.MissingRepeatedKeyMode) {
        case TProto2JsonConfig::MissingKeyNull: {
            Value2Json(jsonKeyBuilder.GetKey(), NJson::TJsonValue(NJson::JSON_NULL), json, config);
            break;
        }

            // case TProto2JsonConfig::MissingKeyDefault: {
            //     OpenArray(jsonKeyBuilder.GetKey(), json);
            //     CloseArray(json);
            //     break;
            // }

        case TProto2JsonConfig::MissingKeySkip:
        default:
            break;
        }
    }

#undef REPEATED_FIELD_TO_JSON
}

template <typename TDestination>
void Proto2JsonImpl(const google::protobuf::Message& proto, TDestination& json,
                    const NProtobufJson::TProto2JsonConfig& config)
{
    using namespace google::protobuf;

    const Descriptor* descriptor = proto.GetDescriptor();
    YASSERT(!!descriptor);

    for (int f = 0, endF = descriptor->field_count(); f < endF; ++f) {
        const FieldDescriptor* field = descriptor->field(f);
        YASSERT(!!field);

        if (field->is_repeated())
            RepeatedField2Json(proto, *field, json, config);
        else
            SingleField2Json(proto, *field, json, config);
    }
}

} // anonymous namespace

namespace NProtobufJson {

TProto2JsonConfig::TProto2JsonConfig()
    : FormatOutput(false)
    , MissingSingleKeyMode(MissingKeySkip)
    , MissingRepeatedKeyMode(MissingKeySkip)
    , AddMissingFields(false)
    , EnumMode(EnumNumber)
    , FieldNameMode(FieldNameOriginalCase)
{
}


void Proto2Json(const google::protobuf::Message& proto, NJson::TJsonValue& json,
                const TProto2JsonConfig& config) {
    json.SetType(NJson::JSON_MAP);
    Proto2JsonImpl(proto, json, config);
}

void Proto2Json(const google::protobuf::Message& proto, NJson::TJsonWriter& writer,
                const TProto2JsonConfig& config)
{
    Proto2JsonNoFlush(proto, writer, config);
    writer.Flush();
}

void Proto2Json(const google::protobuf::Message& proto, TOutputStream& out,
                const TProto2JsonConfig& config)
{
    NJson::TJsonWriterConfig jsonConfig;
    jsonConfig.FormatOutput = config.FormatOutput;
    jsonConfig.SortKeys = false;
    jsonConfig.ValidateUtf8 = false;
    jsonConfig.DontEscapeStrings = false;

    for (size_t i = 0; i < config.StringTransforms.size(); ++i) {
        YASSERT(!!config.StringTransforms[i]);
        if (config.StringTransforms[i]->GetType() == IStringTransform::EscapeTransform) {
            jsonConfig.DontEscapeStrings = true;
            break;
        }
    }

    NJson::TJsonWriter writer(&out, jsonConfig);
    Proto2Json(proto, writer, config);
    writer.Flush();
}

void Proto2Json(const google::protobuf::Message& proto, TStringStream& out,
                const TProto2JsonConfig& config)
{
    Proto2Json(proto, out.Str(), config);
}

void Proto2Json(const google::protobuf::Message& proto, Stroka& str,
                const TProto2JsonConfig& config)
{
    TStringOutput out(str);
    Proto2Json(proto, out, config);
}

Stroka Proto2Json(const ::google::protobuf::Message& proto,
                  const TProto2JsonConfig& config)
{
    Stroka res;
    NProtobufJson::Proto2Json(proto, res, config);
    return res;
}

} // namespace NProtobufJson
