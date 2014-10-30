#include "string_transform.h"

#include <contrib/libs/protobuf/stubs/strutil.h>

namespace NProtobufJson {

void TCEscapeTransform::Transform(Stroka& str) const {
    str = google::protobuf::CEscape(str);
}

void TSafeUtf8CEscapeTransform::Transform(Stroka& str) const {
    str = google::protobuf::strings::Utf8SafeCEscape(str);
}

} // namespace NProtobufJson
