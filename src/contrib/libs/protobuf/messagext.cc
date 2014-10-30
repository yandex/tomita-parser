// The following code is the modified part of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

#include <coded_stream.h>
#include <io/zero_copy_stream_impl.h>
#include <messagext.h>
#include <messageint.h>

#include <util/ysaveload.h>
#include <util/generic/yexception.h>
#include <util/stream/length.h>


namespace google {
namespace protobuf {
namespace io {

bool ParseFromCodedStreamSeq(Message* msg, io::CodedInputStream* input) {
    msg->Clear();
    ui32 size;
    if (!input->ReadVarint32(&size)) {
        return false;
    }

    io::CodedInputStream::Limit limitState = input->PushLimit(size);
    bool res = msg->ParseFromCodedStream(input);
    input->PopLimit(limitState);
    return res;
}

bool ParseFromZeroCopyStreamSeq(Message* msg, io::ZeroCopyInputStream* input) {
    io::CodedInputStream decoder(input);
    return ParseFromCodedStreamSeq(msg, &decoder);
}

bool SerializePartialToCodedStreamSeq(const Message* msg, io::CodedOutputStream* output) {
    uint32 size = msg->ByteSize();  // Force size to be cached.
    output->WriteVarint32(size);
    msg->SerializeWithCachedSizes(output);
    return !output->HadError();
}

bool SerializeToCodedStreamSeq(const Message* msg, io::CodedOutputStream* output) {
    GOOGLE_DCHECK(msg->IsInitialized()) << InitializationErrorMessage("serialize", *msg);
    return SerializePartialToCodedStreamSeq(msg, output);
}

bool SerializeToZeroCopyStreamSeq(const Message* msg, io::ZeroCopyOutputStream* output) {
    io::CodedOutputStream encoder(output);
    return SerializeToCodedStreamSeq(msg, &encoder);
}



void TProtoSerializer::Save(TOutputStream* out, const Message& msg) {
    TCopyingOutputStreamAdaptor adaptor(out);
    io::CodedOutputStream encoder(&adaptor);
    if (!SerializeToCodedStreamSeq(&msg, &encoder))
        ythrow yexception() << "Cannot write protobuf::Message to output stream";
}

// Reading varint32 directly from TInputStream (might be slow if input requires buffering).
// Copy-pasted with small modifications from protobuf/io/coded_stream (ReadVarint32FromArray)
static bool ReadVarint32(TInputStream* input, ui32& size) {
    size_t res;
    ui8 b;
    ::Load(input, b); res  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
    ::Load(input, b); res |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
    ::Load(input, b); res |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
    ::Load(input, b); res |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
    ::Load(input, b); res |=  b         << 28; if (!(b & 0x80)) goto done;

    // If the input is larger than 32 bits, we still need to read it all
    // and discard the high-order bits.
    for (int i = 0; i < 5; i++) {
        ::Load(input, b); if (!(b & 0x80)) goto done;
    }
    // We have overrun the maximum size of a varint (10 bytes).  Assume the data is corrupt.
    return false;

done:
    size = res;
    return true;
}

class TTempBufHelper {
public:
    TTempBufHelper(size_t size) {
        if (size <= SmallBufSize) {
            Buffer = SmallBuf;
        } else {
            LargeBuf.Reset(new TTempBuf(size));
            Buffer = reinterpret_cast<ui8*>(LargeBuf->Data());
        }
    }

    ui8* Data() {
        return Buffer;
    }

private:
    static const size_t SmallBufSize = 1024;
    ui8 SmallBuf[SmallBufSize];

    THolder<TTempBuf> LargeBuf;

    ui8* Buffer;
};

void TProtoSerializer::Load(TInputStream* input, Message& msg) {
    ui32 size;
    if (ReadVarint32(input, size)) {
        TTempBufHelper buf(size);
        ::LoadPodArray(input, buf.Data(), size);
        CodedInputStream decoder(buf.Data(), size);
        if (msg.ParseFromCodedStream(&decoder))
            return;
    }
    ythrow yexception() << "Cannot read protobuf::Message from input stream";
}

}
}
}
