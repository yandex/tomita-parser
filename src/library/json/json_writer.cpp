#include "json_writer.h"

#include <util/charset/utf.h>
#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/system/yassert.h>

namespace NJson {

TJsonWriterConfig::TJsonWriterConfig()
    : FormatOutput(false)
    , SortKeys(false)
    , ValidateUtf8(true)
    , DontEscapeStrings(false)
{
}

TJsonWriter::TJsonWriter(TOutputStream *out, bool formatOutput, bool sortkeys, bool validateUtf8)
    : Out(out)
    , Buf(NJsonWriter::HEM_UNSAFE)
    , SortKeys(sortkeys)
    , ValidateUtf8(validateUtf8)
    , DontEscapeStrings(false)
    , DontFlushInDestructor(false)
{
    Buf.SetIndentSpaces(formatOutput ? 2 : 0);
}

TJsonWriter::TJsonWriter(TOutputStream *out, const TJsonWriterConfig& config, bool DFID)
    : Out(out)
    , Buf(NJsonWriter::HEM_UNSAFE)
    , SortKeys(config.SortKeys)
    , ValidateUtf8(config.ValidateUtf8)
    , DontEscapeStrings(config.DontEscapeStrings)
    , DontFlushInDestructor(DFID)
{
    Buf.SetIndentSpaces(config.FormatOutput ? 2 : 0);
}

TJsonWriter::~TJsonWriter() {
    // if we write to socket it's possible to get exception here
    // don't use exceptions in destructors
    if (!DontFlushInDestructor)
        Flush();
}

void TJsonWriter::Flush() {
    Buf.FlushTo(Out);
}

void TJsonWriter::OpenMap() {
    Buf.BeginObject();
}

void TJsonWriter::CloseMap() {
    Buf.EndObject();
}

void TJsonWriter::OpenArray() {
    Buf.BeginList();
}

void TJsonWriter::CloseArray() {
    Buf.EndList();
}

void TJsonWriter::Write(const TStringBuf &value) {
    if (ValidateUtf8 && !IsUtf(value))
        throw yexception() << "JSON writer: invalid UTF-8";
    if (Buf.KeyExpected()) {
        Buf.WriteKey(value);
    } else {
        if (DontEscapeStrings) {
            Buf.UnsafeWriteValue(Stroka("\"") + value + '"');
        } else {
            Buf.WriteString(value);
        }
    }
}

void TJsonWriter::WriteNull() {
    Buf.WriteNull();
}

void TJsonWriter::Write(double value) {
    Buf.WriteDouble(value);
}

void TJsonWriter::Write(long long value) {
    Buf.WriteLongLong(value);
}

void TJsonWriter::Write(unsigned long long value) {
    Buf.WriteULongLong(value);
}

void TJsonWriter::Write(bool value) {
    Buf.WriteBool(value);
}

struct TLessStrPtr {
    bool operator ()(const Stroka* a, const Stroka* b) const {
        return *a < *b;
    }
};

void TJsonWriter::Write(const TJsonValue* v) {
    Buf.WriteJsonValue(v, SortKeys);
}

Stroka WriteJson(const TJsonValue* value, bool formatOutput, bool sortkeys, bool validateUtf8) {
    TStringStream ss;
    WriteJson(&ss, value, formatOutput, sortkeys, validateUtf8);
    return ss;
}

void WriteJson(TOutputStream* out, const TJsonValue* val, bool formatOutput, bool sortkeys, bool validateUtf8) {
    TJsonWriter w(out, formatOutput, sortkeys, validateUtf8);
    w.Write(val);
    w.Flush();
}

void WriteJson(TOutputStream* out, const TJsonValue* val, const TJsonWriterConfig& config) {
    TJsonWriter w(out, config, true);
    w.Write(val);
    w.Flush();
}

}
