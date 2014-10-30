#pragma once

#include "simple_reflection.h"

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/descriptor.h>

namespace NProtoBuf {

// Apply @onField processor to each field in @msg (even empty)
// TOnField is something like
//    bool (*TOnField)(Message& msg, const FieldDescriptor* fd);
// Returned bool defines if we should walk down deeper to current node children (true), or not (false)
//
template <typename TOnField>
void WalkReflection(Message& msg, TOnField& onField) {
    const Descriptor* descr = msg.GetDescriptor();
    for (int i = 0; i < descr->field_count(); ++i) {
        const FieldDescriptor* fd = descr->field(i);
        if (!onField(msg, fd))
            continue;

        TMutableField ff(msg, fd);
        if (ff.IsMessage()) {
            for (size_t i = 0; i < ff.Size(); ++i)
                WalkReflection(*ff.MutableMessage(i), onField);
        }
    }
}

template <typename TOnField>
void WalkReflection(const Message& msg, TOnField& onField) {
    const Descriptor* descr = msg.GetDescriptor();
    for (int i = 0; i < descr->field_count(); ++i) {
        const FieldDescriptor* fd = descr->field(i);
        if (!onField(msg, fd))
            continue;

        TConstField ff(msg, fd);
        if (ff.IsMessage()) {
            for (size_t i = 0, size = ff.Size(); i < size; ++i)
                WalkReflection(*ff.Get<Message>(i), onField);
        }
    }
}

}   // namespace NProtoBuf
