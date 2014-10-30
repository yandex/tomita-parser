#pragma once

#include "string_transform.h"

#include <util/generic/vector.h>


namespace google {
namespace protobuf {
class Message;
} // namespace protobuf
} // namespace google

namespace NJson {
class TJsonValue;
class TJsonWriter;
} // namespace NJson

class Stroka;
class TOutputStream;
class TStringStream;


namespace NProtobufJson {

struct TProto2JsonConfig {
    typedef TProto2JsonConfig TSelf;

    TProto2JsonConfig();


    bool FormatOutput;

    enum MissingKeyMode {
        MissingKeySkip = 0,
        MissingKeyNull,
        /// @todo implement
        // MissingKeyDefault
    };
    MissingKeyMode MissingSingleKeyMode;
    MissingKeyMode MissingRepeatedKeyMode;

    /// Add null value for missing fields (false by default).
    bool AddMissingFields;

    enum EnumValueMode {
        EnumNumber = 0, // default
        EnumName,
        EnumFullName,
        EnumNameLowerCase,
        EnumFullNameLowerCase,
    };
    EnumValueMode EnumMode;

    enum FldNameMode {
        FieldNameOriginalCase = 0, // default
        FieldNameLowerCase,
        FieldNameUpperCase,
    };
    FldNameMode FieldNameMode;

    /// Transforms will be applied only to string values (== protobuf fields of string / bytes type).
    /// yajl_encode_string will be used if no transforms are specified.
    yvector<TStringTransformPtr> StringTransforms;

    TSelf& SetFormatOutput(bool format) {
        FormatOutput = format;
        return *this;
    }

    TSelf& SetMissingSingleKeyMode(MissingKeyMode mode) {
        MissingSingleKeyMode = mode;
        return *this;
    }

    TSelf& SetMissingRepeatedKeyMode(MissingKeyMode mode) {
        MissingRepeatedKeyMode = mode;
        return *this;
    }

    TSelf& SetAddMissingFields(bool add) {
        AddMissingFields = add;
        return *this;
    }

    TSelf& SetEnumMode(EnumValueMode mode) {
        EnumMode = mode;
        return *this;
    }

    TSelf& SetFieldNameMode(FldNameMode mode) {
        FieldNameMode = mode;
        return *this;
    }

    TSelf& AddStringTransform(TStringTransformPtr transform) {
        StringTransforms.push_back(transform);
        return *this;
    }
};

/// @throw yexception
void Proto2Json(const google::protobuf::Message& proto, NJson::TJsonValue& json,
                const TProto2JsonConfig& config = TProto2JsonConfig());

/// @throw yexception
void Proto2Json(const google::protobuf::Message& proto, NJson::TJsonWriter& writer,
                const TProto2JsonConfig& config = TProto2JsonConfig());

/// @throw yexception
void Proto2Json(const google::protobuf::Message& proto, TOutputStream& out,
                const TProto2JsonConfig& config = TProto2JsonConfig());
void Proto2Json(const google::protobuf::Message& proto, TStringStream& out,
                const TProto2JsonConfig& config = TProto2JsonConfig());

/// @throw yexception
void Proto2Json(const google::protobuf::Message& proto, Stroka& str,
                const TProto2JsonConfig& config = TProto2JsonConfig());

/// @throw yexception
Stroka Proto2Json(const google::protobuf::Message& proto,
                  const TProto2JsonConfig& config = TProto2JsonConfig());

} // namespace NProtobufJson
