#pragma once

#include <library/json/json_reader.h>
#include <library/json/json_value.h>

#include <util/stream/input.h>
#include <util/stream/str.h>


namespace google {
namespace protobuf {
class Message;
} // namespace protobuf
} // namespace google


namespace NProtobufJson {

/// @throw yexception
void Json2Proto(const NJson::TJsonValue& json, google::protobuf::Message& proto);

/// @throw yexception
template<typename T>
T Json2Proto(TInputStream& in, NJson::TJsonReaderConfig config) {
    NJson::TJsonValue jsonValue;
    NJson::ReadJsonTree(&in, &config, &jsonValue, true);
    T protoValue;
    Json2Proto(jsonValue, protoValue);
    return protoValue;
}

/// @throw yexception
template<typename T>
T Json2Proto(TInputStream& in) {
    NJson::TJsonReaderConfig config;
    config.DontValidateUtf8 = true;
    return Json2Proto<T>(in, config);
}

/// @throw yexception
template<typename T>
T Json2Proto(const Stroka& value) {
    TStringInput in(value);
    return Json2Proto<T>(in);
}

} // namespace NProtobufJson
