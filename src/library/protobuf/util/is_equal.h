#pragma once

#include <contrib/libs/protobuf/message.h>

#include <util/generic/vector.h>

namespace NProtoBuf {

// Reflection-based equality check for arbitrary protobuf messages

// Strict comparison: optional field without value is NOT equal to
// a field with explicitly set default value.
bool IsEqual(const Message& m1, const Message& m2);
bool IsEqual(const Message& m1, const Message& m2, Stroka* differentPath);

// Non-strict version: optional field without explicit value is compared
// using its default value.
bool IsEqualDefault(const Message& m1, const Message& m2);

}
