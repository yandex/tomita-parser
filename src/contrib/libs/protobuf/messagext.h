// The following code is the modified part of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

#ifndef GOOGLE_PROTOBUF_MESSAGEXT_H__
#define GOOGLE_PROTOBUF_MESSAGEXT_H__

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/io/coded_stream.h>
#include <util/stream/ios.h>

#include "io/zero_copy_stream_impl.h"

/// this file is Yandex extensions to protobuf

namespace google {
namespace protobuf {
namespace io {

/// Parse*Seq methods read message size from stream to find a message boundary
/// there is not parse from TInputStream, because it is not push-backable

bool ParseFromCodedStreamSeq(Message* msg, io::CodedInputStream* input);
bool ParseFromZeroCopyStreamSeq(Message* msg, io::ZeroCopyInputStream* input);

/// Serialize*Seq methods write message size as varint before writing a message
/// there is no serialize to TOutputStream, because it is not push-backable

bool SerializePartialToCodedStreamSeq(const Message* msg, io::CodedOutputStream* output);
bool SerializeToCodedStreamSeq(const Message* msg, io::CodedOutputStream* output);
bool SerializeToZeroCopyStreamSeq(const Message* msg, io::ZeroCopyOutputStream* output);

class TErrorState {
public:
    TErrorState()
        : HasError_(false)
    {
    }
    bool HasError() const {
        return HasError_;
    }
    void SetError() {
        HasError_ = true;
    }
private:
    bool HasError_;
};

class TInputStreamProxy: public io::CopyingInputStream, public TErrorState {
    public:
        inline TInputStreamProxy(TInputStream* slave)
            : mSlave(slave)
        {
        }

        virtual int Read(void* buffer, int size) {
            try {
                return (int)mSlave->Read(buffer, (size_t)size);
            } catch (...) {
                TErrorState::SetError();
                return -1;
            }
        }

    private:
        TInputStream* mSlave;
};

class TOutputStreamProxy: public io::CopyingOutputStream, public TErrorState {
    public:
        inline TOutputStreamProxy(TOutputStream* slave)
            : mSlave(slave)
        {
        }

        virtual bool Write(const void* buffer, int size) {
            try {
                mSlave->Write(buffer, (size_t)size);
                return true;
            } catch (...) {
                TErrorState::SetError();
                return false;
            }
        }

    private:
        TOutputStream* mSlave;
};


class TCopyingInputStreamAdaptor: public TInputStreamProxy, public CopyingInputStreamAdaptor {
public:
    TCopyingInputStreamAdaptor(TInputStream* inputStream)
        : TInputStreamProxy(inputStream)
        , CopyingInputStreamAdaptor(this)
    { }
};

class TCopyingOutputStreamAdaptor: public TOutputStreamProxy, public CopyingOutputStreamAdaptor {
public:
    TCopyingOutputStreamAdaptor(TOutputStream* outputStream)
        : TOutputStreamProxy(outputStream)
        , CopyingOutputStreamAdaptor(this)
    { }
};


class TProtoSerializer {
public:
    static void Save(TOutputStream* output, const Message& msg);
    static void Load(TInputStream* input, Message& msg);

    // similar interface for protobuf coded streams
    static inline bool Save(CodedOutputStream* output, const Message& msg) {
        return SerializeToCodedStreamSeq(&msg, output);
    }

    static inline bool Load(CodedInputStream* input, Message& msg) {
        return ParseFromCodedStreamSeq(&msg, input);
    }
};


}
}
}

// arcadia-style serialization
inline void Save(TOutputStream* output, const google::protobuf::Message& msg) {
    google::protobuf::io::TProtoSerializer::Save(output, msg);
}

inline void Load(TInputStream* input, google::protobuf::Message& msg) {
    google::protobuf::io::TProtoSerializer::Load(input, msg);
}

#endif
