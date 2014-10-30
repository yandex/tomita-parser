#include <util/system/oldfile.h>
#include <util/stream/str.h>
#include <util/string/base64.h>
#include "pb_io.h"

#include <contrib/libs/protobuf/text_format.h>
#include <contrib/libs/protobuf/io/zero_copy_stream_impl.h>


namespace NProtoBuf
{

void SaveLoadToBinSaver(IBinSaver& f, Message& m)
{
    TStringStream ss;
    if (f.IsReading()) {
        f.Add(0, &ss.Str());
        m.ParseFromStream(&ss);
    } else {
        m.SerializeToStream(&ss);
        f.Add(0, &ss.Str());
    }
}

void ParseFromBase64String(TStringBuf dataBase64, Message& m)
{
    if (!m.ParseFromString(Base64Decode(dataBase64)))
        ythrow yexception() << "can't parse " << m.GetTypeName() << " from base64-encoded string";
}

void SerializeToBase64String(const Message& m, Stroka& dataBase64)
{
    Stroka rawData;
    if (!m.SerializeToString(&rawData))
        ythrow yexception() << "can't serialize " << m.GetTypeName();
    Base64EncodeUrl(rawData, dataBase64);
}

const Stroka ShortUtf8DebugString(const Message& message) {
    TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.SetUseUtf8StringEscaping(true);
    Stroka result;
    printer.PrintToString(message, &result);
    return result;
}

} // NProtoBuf


void SerializeToTextFormat(const NProtoBuf::Message& m, const Stroka& fileName)
{
    Os::File out;
    out.OpenEx(~fileName, Os::File::oCreatAllways | Os::File::oWrOnly);
    google::protobuf::io::FileOutputStream outStr(out.GetHandle());
    if (!google::protobuf::TextFormat::Print(m, &outStr))
        ythrow yexception() << "SerializeToTextFormat failed on Print";
}

void ParseFromTextFormat(const Stroka& fileName, NProtoBuf::Message& m)
{
    m.Clear();
    Os::File in;
    in.OpenEx(~fileName, Os::File::oOpenExisting | Os::File::oRdOnly);
    google::protobuf::io::FileInputStream inStr(in.GetHandle());
    if (!google::protobuf::TextFormat::Parse(&inStr, &m))
        ythrow yexception() << "ParseFromTextFormat failed on Parse";
}

