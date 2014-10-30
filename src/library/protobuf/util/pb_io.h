#pragma once

#include <util/generic/stroka.h>

#include <library/binsaver/BinSaver.h>

#include <contrib/libs/protobuf/message.h>

namespace NProtoBuf
{

void SaveLoadToBinSaver(IBinSaver& f, Message& m);

void ParseFromBase64String(TStringBuf dataBase64, Message& m);
void SerializeToBase64String(const Message& m, Stroka& dataBase64);

const Stroka ShortUtf8DebugString(const Message& message);

} // NProtoBuf


void SerializeToTextFormat(const NProtoBuf::Message& m, const Stroka& fileName);
void ParseFromTextFormat(const Stroka& fileName, NProtoBuf::Message& m);

