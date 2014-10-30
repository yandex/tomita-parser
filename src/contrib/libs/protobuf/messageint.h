// The following code is the modified part of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

#ifndef GOOGLE_PROTOBUF_MESSAGEINT_H__
#define GOOGLE_PROTOBUF_MESSAGEINT_H__

#include <contrib/libs/protobuf/io/coded_stream.h>
#include <util/stream/ios.h>
#include <descriptor.h>
#include <stubs/substitute.h>

namespace google {
namespace protobuf {

static string InitializationErrorMessage(const char* action,
                                         const Message& message) {
  return strings::Substitute(
    "Can't $0 message of type \"$1\" because it is missing required "
    "fields: $2",
    action, message.GetDescriptor()->full_name(),
    message.InitializationErrorString());
  return "";
}

}
}

#endif
