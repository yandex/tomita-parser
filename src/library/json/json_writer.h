#pragma once

// Deprecated. Use library/json/writer in new code.

#include "json_value.h"

#include <util/stream/output.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <library/json/writer/json.h>

namespace NJson {

struct TJsonWriterConfig {
    TJsonWriterConfig();

    bool FormatOutput;
    bool SortKeys;
    bool ValidateUtf8;
    bool DontEscapeStrings;
};

class TJsonWriter {
    TOutputStream *Out;
    NJsonWriter::TBuf Buf;
    bool SortKeys;
    bool ValidateUtf8;
    bool DontEscapeStrings;
    const  bool DontFlushInDestructor;

public:
    TJsonWriter(TOutputStream *out, bool formatOutput, bool sortkeys = false, bool validateUtf8 = true);
    TJsonWriter(TOutputStream *out, const TJsonWriterConfig& config, bool DFID = false);
    ~TJsonWriter();

    void Flush();

    void OpenMap();
    void CloseMap();

    void OpenArray();
    void CloseArray();

    void WriteNull();

    void Write(const TStringBuf& value);
    void Write(double value);
    void Write(bool value);
    void Write(const TJsonValue* value);

    // must use all variations of integer types since long
    // and long long are different types but with same size
    void Write(         long long value);
    void Write(unsigned long long value);
    void Write(         long      value) { Write((long long) value); }
    void Write(unsigned long      value) { Write((unsigned long long) value); }
    void Write(         int       value) { Write((long long) value); }
    void Write(unsigned int       value) { Write((unsigned long long) value); }
    void Write(         short     value) { Write((long long) value); }
    void Write(unsigned short     value) { Write((unsigned long long) value); }

    void Write(const unsigned char *value) {
        Write((const char *) value);
    }
    void Write(const char *value) {
        Write(TStringBuf(value));
    }
    void Write(const Stroka &value) {
        Write(TStringBuf(value));
    }

    template <typename T> void Write(const TStringBuf &key, const T &value) {
        Buf.WriteKey(key);
        Write(value);
    }

    void WriteNull(const TStringBuf& key) {
        Buf.WriteKey(key);
        WriteNull();
    }

};

void WriteJson(TOutputStream*, const TJsonValue*,
    bool formatOutput = false, bool sortkeys = false, bool validateUtf8 = true);

Stroka WriteJson(const TJsonValue*,
    bool formatOutput = true, bool sortkeys = false, bool validateUtf8 = false);

void WriteJson(TOutputStream*, const TJsonValue*, const TJsonWriterConfig& config);

}
