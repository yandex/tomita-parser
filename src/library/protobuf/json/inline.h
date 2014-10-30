#pragma once

// A printer from protobuf to json string, with ability to inline some string fields of given protobuf message
// into output as ready json without additional escaping. These fields should be marked using special field option.
// An example of usage:
// 1) Define a field option in your .proto to identify fields which should be inlined, e.g.
//
//     import "contrib/libs/protobuf/descriptor.proto";
//     extend google.protobuf.FieldOptions {
//         optional bool this_is_json = 58253;   // do not forget assign some more or less unique tag
//     }
//
// 2) Mark some fields of your protobuf message with this option, e.g.:
//
//     message TMyObject {
//         optional string A = 1 [(this_is_json) = true];
//     }
//
// 3) In the C++ code you prepare somehow an object of TMyObject type
//
//     TMyObject o;
//     o.Set("{\"inner\":\"value\"}");
//
// 4) And then serialize it to json string with inlining, e.g.:
//
//     Cout << NProtobufJson::PrintInlined(o, this_is_json) << Endl;
//
// which will print following json to stdout:
//     {"A":{"inner":"value"}}
// instead of
//     {"A":"{\"inner\":\"value\"}"}
// which would be printed with normal Proto2Json printer.
//
// See ut/inline_ut.cpp for additional examples of usage.


#include "proto2json.h"

#include <library/protobuf/util/walk.h>

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/descriptor.pb.h>

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

namespace NProtobufJson {


class TInliningPrinter {
public:
    // Note that @msg will be modified during this printing.
    // So if you need it after printing, you should explicily copy it.

    // TExtensionId is a type of this options (it is generated extension identifier in fact,
    // but you normally do not need to known what it is)
    template <typename TExtensionId>
    Stroka Print(NProtoBuf::Message& msg, const TExtensionId& isJson,
                 const TProto2JsonConfig& config = TProto2JsonConfig())
    {
        // note that @msg is modified here and not restored to original state
        PushMarkers(isJson, msg);
        Proto2Json(msg, Json, config);
        PopMarkers(Json);
        return Json;
    }

    void Reset() {
        // should be cleared before possible re-use
        Json.clear();
        Entries.clear();
    }

private:
    struct TEntry {
        Stroka Json, Marker;
    };

    template <typename TExtensionId>
    struct TReplacer {
        const TExtensionId& IsJson;
        TInliningPrinter& Parent;

        TReplacer(const TExtensionId& isJson, TInliningPrinter& parent)
            : IsJson(isJson)
            , Parent(parent)
        {
        }

        bool operator()(NProtoBuf::Message& msg, const NProtoBuf::FieldDescriptor* fd) {
            using namespace NProtoBuf;
            TMutableField f(msg, fd);
            const FieldOptions& opt = fd->options();
            if (opt.HasExtension(IsJson) && opt.GetExtension(IsJson)) {
                if (!f.IsString())
                    ythrow yexception() << "TInliningPrinter: json field " << fd->name() << " should be a string";
                // replace json string with special marker and remember replaced value inside Entries
                for (size_t i = 0; i < f.Size(); ++i)
                    f.Set(Parent.AddMarker(f.Get<Stroka>(i)), i);
            }
            return true;
        }
    };

    template <typename TExtensionId>
    void PushMarkers(const TExtensionId& isJson, NProtoBuf::Message& msg) {
        TReplacer<TExtensionId> r(isJson, *this);
        NProtoBuf::WalkReflection(msg, r);
    }

    void PopMarkers(Stroka& json) const;
    Stroka AddMarker(const Stroka& json);
    bool ReplaceMarker(Stroka& trgJson, const TEntry& entry) const;

private:
    Stroka Json;
    yvector<TEntry> Entries;
};

template <typename TExtensionId>
inline Stroka PrintInlined(NProtoBuf::Message& msg, const TExtensionId& isJson, const TProto2JsonConfig& config = TProto2JsonConfig()) {
    return TInliningPrinter().Print(msg, isJson, config);
}


}   // namespace NProtobufJson
