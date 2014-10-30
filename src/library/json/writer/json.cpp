#include "json.h"
#include <library/json/json_value.h>
#include <util/string/cast.h>
#include <util/generic/algorithm.h>
#include <util/generic/static_assert.h>
#include <util/generic/ymath.h>

namespace NJsonWriter {

TBuf::TBuf(EHtmlEscapeMode mode, TOutputStream* stream)
    : Stream(stream)
    , NeedComma(false)
    , NeedNewline(false)
    , EscapeMode(mode)
    , IndentSpaces(0)
    , Charset(CODES_UTF8)
{
    YASSERT(mode == HEM_DONT_ESCAPE_HTML ||
            mode == HEM_ESCAPE_HTML ||
            mode == HEM_RELAXED ||
            mode == HEM_UNSAFE);
    if (!Stream) {
       StringStream.Reset(new TStringStream);
       Stream = StringStream.Get();
    }

    Stack.reserve(64);      // should be enough for most cases
    StackPush(JE_OUTER_SPACE);
}

static const char* EntityToStr(EJsonEntity e) {
    switch(e) {
        case JE_OUTER_SPACE: return "JE_OUTER_SPACE";
        case JE_LIST: return "JE_LIST";
        case JE_OBJECT: return "JE_OBJECT";
        case JE_PAIR: return "JE_PAIR";
        default: return "JE_unknown";
    }
}

inline void TBuf::StackPush(EJsonEntity e) {
    Stack.push_back(e);
}

inline EJsonEntity TBuf::StackTop() const {
    return Stack.back();
}

inline void TBuf::StackPop() {
    YASSERT(!Stack.empty());
    const EJsonEntity current = StackTop();
    Stack.pop_back();
    switch (current) {
        case JE_OUTER_SPACE: ythrow TError() << "JSON writer: stack empty";
        case JE_LIST: PrintIndentation(true); RawWriteChar(']'); break;
        case JE_OBJECT: PrintIndentation(true); RawWriteChar('}'); break;
        case JE_PAIR: break;
    }
    NeedComma = true;
    NeedNewline = true;
}

inline void TBuf::CheckAndPop(EJsonEntity e) {
    if (EXPECT_FALSE(StackTop() != e)) {
        ythrow TError() << "JSON writer: unexpected value "
            << EntityToStr(StackTop()) << " on the stack";
    }
    StackPop();
}



void TBuf::PrintIndentation(bool closing) {
    if (!IndentSpaces)
        return;
    const int indentation = IndentSpaces * (Stack.size() - 1);
    if (!indentation && !closing)
        return;
    RawWriteChar('\n');
    for (int i = 0; i < indentation; ++i)
        RawWriteChar(' ');
}

inline void TBuf::WriteComma() {
    if (NeedComma) {
        RawWriteChar(',');
    }
    NeedComma = true;

    if (NeedNewline) {
        PrintIndentation(false);
    }
    NeedNewline = true;
}

inline void TBuf::BeginValue() {
    if (EXPECT_FALSE(KeyExpected())) {
        ythrow TError() << "JSON writer: value written, "
                           "but expected a key:value pair";
    }
    WriteComma();
}

inline void TBuf::BeginKey() {
    if (EXPECT_FALSE(!KeyExpected())) {
        ythrow TError() << "JSON writer: key written outside of an object";
    }
    WriteComma();
    StackPush(JE_PAIR);
    NeedComma = false;
    NeedNewline = false;
}

inline void TBuf::EndValue() {
    if (StackTop() == JE_PAIR) {
        StackPop();
    }
}

TValueContext TBuf::BeginList() {
    NeedNewline = true;
    BeginValue();
    RawWriteChar('[');
    StackPush(JE_LIST);
    NeedComma = false;
    return TValueContext(*this);
}

TPairContext TBuf::BeginObject() {
    NeedNewline = true;
    BeginValue();
    RawWriteChar('{');
    StackPush(JE_OBJECT);
    NeedComma = false;
    return TPairContext(*this);
}

TAfterColonContext TBuf::WriteKey(const TStringBuf& s) {
    // use the default escaping mode for this object
    return WriteKey(s, EscapeMode);
}

TAfterColonContext TBuf::WriteKey(const TStringBuf& s, EHtmlEscapeMode hem) {
    BeginKey();
    WriteBareString(s, hem);
    RawWriteChar(':');
    return TAfterColonContext(*this);
}

TAfterColonContext TBuf::CompatWriteKeyWithoutQuotes(const TStringBuf& s) {
    BeginKey();
    YASSERT(AllOf(s, [](char x){ return 'a' <= x && x <= 'z'; }));
    UnsafeWriteRawBytes(s);
    RawWriteChar(':');
    return TAfterColonContext(*this);
}

TBuf& TBuf::EndList() {
    CheckAndPop(JE_LIST);
    EndValue();
    return *this;
}

TBuf& TBuf::EndObject() {
    CheckAndPop(JE_OBJECT);
    EndValue();
    return *this;
}

TValueContext TBuf::WriteString(const TStringBuf& s) {
    // use the default escaping mode for this object
    return WriteString(s, EscapeMode);
}

TValueContext TBuf::WriteString(const TStringBuf& s, EHtmlEscapeMode hem) {
    BeginValue();
    WriteBareString(s, hem);
    EndValue();
    return TValueContext(*this);
}

TValueContext TBuf::WriteNull() {
    UnsafeWriteValue(STRINGBUF("null"));
    return TValueContext(*this);
}

TValueContext TBuf::WriteBool(bool b) {
    UnsafeWriteValue(b ? STRINGBUF("true") : STRINGBUF("false"));
    return TValueContext(*this);
}

TValueContext TBuf::WriteInt(int i) {
    char buf[22]; // enough to hold any 64-bit number
    int len = sprintf(buf, "%d", i);
    UnsafeWriteValue(buf, len);
    return TValueContext(*this);
}

TValueContext TBuf::WriteLongLong(long long i) {
    STATIC_ASSERT(sizeof(long long) <= 8);
    char buf[22]; // enough to hold any 64-bit number
    int len = sprintf(buf, "%lld", i);
    UnsafeWriteValue(buf, len);
    return TValueContext(*this);
}

TValueContext TBuf::WriteULongLong(unsigned long long i) {
    char buf[22]; // enough to hold any 64-bit number
    int len = sprintf(buf, "%llu", i);
    UnsafeWriteValue(buf, len);
    return TValueContext(*this);
}

TValueContext TBuf::WriteFloat(float f) {
    if (EXPECT_FALSE(!IsValidFloat(f)))
        ythrow TError() << "JSON writer: invalid float value";
    char buf[256];   // enough to hold most floats, the same buffer is used in ToString implementation
    size_t len = ToString(f, buf, ARRAY_SIZE(buf));
    UnsafeWriteValue(buf, len);
    return TValueContext(*this);
}

TValueContext TBuf::WriteDouble(double f) {
    if (EXPECT_FALSE(!IsValidFloat(f)))
        ythrow TError() << "JSON writer: invalid double value";
    char buf[256];   // enough to hold most doubles, the same buffer is used in ToString implementation
    size_t len = ToString(f, buf, ARRAY_SIZE(buf));
    UnsafeWriteValue(buf, len);
    return TValueContext(*this);
}

static inline bool IsJsonpSpecial(TStringBuf::const_iterator i, TStringBuf::const_iterator e) {
    return *i == '\xe2' && (e - i) > 2
        && *(i+1) == '\x80' && ((*(i+2) | 1) == '\xa9');
}

void TBuf::EscapedWriteStringContent(const TStringBuf& s, EHtmlEscapeMode hem) {
    const char* b = s.begin();
    for (TStringBuf::const_iterator i = s.begin(); i != s.end(); ++i) {
        // Special-case U+2028 and U+2029: they break JSONP services.
        // UTF-8 encodings: U+2028 is "\xe2\x80\xa8", U+2029 is "\xe2\x80\xa9".
        if (EXPECT_FALSE(IsJsonpSpecial(i, s.end()) && Charset == CODES_UTF8)) {
            UnsafeWriteRawBytes(b, i - b);
            const char* replacement = (*(i+2) == '\xa9') ? "\\u2029" : "\\u2028";
            UnsafeWriteRawBytes(replacement, 6);
            i += 2;
            b = i + 1;
        } else if (EscapedWriteChar(b, i, hem))
            b = i + 1;
    }
    UnsafeWriteRawBytes(b, s.end() - b);
}

inline void TBuf::WriteBareString(const TStringBuf& s, EHtmlEscapeMode hem) {
    RawWriteChar('"');
    EscapedWriteStringContent(s, hem);
    RawWriteChar('"');
}

inline void TBuf::RawWriteChar(char c) {
    Stream->Write(c);
}

void TBuf::WriteHexEscape(unsigned char c) {
    YASSERT(c < 0x80);
    UnsafeWriteRawBytes("\\u00", 4);
    static const char hexDigits[] = "0123456789ABCDEF";
    RawWriteChar(hexDigits[(c & 0xf0) >> 4]);
    RawWriteChar(hexDigits[(c & 0x0f)]);
}


#define MATCH(sym, string) \
            case sym: UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes(STRINGBUF(string)); return true

inline bool TBuf::EscapedWriteChar(const char* beg, const char* cur, EHtmlEscapeMode hem) {
    unsigned char c = *cur;
    if (hem == HEM_ESCAPE_HTML) {
        switch(c) {
            MATCH('"',  "&quot;");
            MATCH('\'', "&#39;");
            MATCH('<',  "&lt;");
            MATCH('>',  "&gt;");
            MATCH('&',  "&amp;");
        }
        //for other characters, we fall through to the non-HTML-escaped part
    }

    if (hem == HEM_RELAXED && c == '/')
        return false;

    if (hem != HEM_UNSAFE) {
        switch(c) {
            case '/': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\/", 2); return true;
            case '<' :
            case '>' :
            case '\'': UnsafeWriteRawBytes(beg, cur - beg); WriteHexEscape(c); return true;
        }
        // for other characters, fall through to the non-escaped part
    }

    switch(c) {
        case '"' : UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\\"", 2); return true;
        case '\\': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\\\", 2); return true;
        case '\b': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\b", 2); return true;
        case '\f': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\f", 2); return true;
        case '\n': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\n", 2); return true;
        case '\r': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\r", 2); return true;
        case '\t': UnsafeWriteRawBytes(beg, cur - beg); UnsafeWriteRawBytes("\\t", 2); return true;
    }
    if (c < 0x20) {
        UnsafeWriteRawBytes(beg, cur - beg);
        WriteHexEscape(c);
        return true;
    }

    return false;
}

#undef MATCH

static bool LessStrPtr(const Stroka* a, const Stroka* b) {
    return *a < *b;
}

TValueContext TBuf::WriteJsonValue(const NJson::TJsonValue* v, bool sortKeys) {
    using namespace NJson;
    switch (v->GetType()) {
    default:
    case JSON_NULL:
        WriteNull();
        break;
    case JSON_BOOLEAN:
        WriteBool(v->GetBoolean());
        break;
    case JSON_DOUBLE:
        WriteDouble(v->GetDouble());
        break;
    case JSON_INTEGER:
        WriteLongLong(v->GetInteger());
        break;
    case JSON_UINTEGER:
        WriteULongLong(v->GetUInteger());
        break;
    case JSON_STRING:
        WriteString(v->GetString());
        break;
    case JSON_ARRAY: {
        BeginList();
        const TJsonValue::TArray& arr = v->GetArray();
        for (TJsonValue::TArray::const_iterator it = arr.begin(); it != arr.end(); ++it)
            WriteJsonValue(&*it, sortKeys);
        EndList();
        break;
    } case JSON_MAP: {
        BeginObject();
        const TJsonValue::TMap& map = v->GetMap();
        if (sortKeys) {
            typedef yvector<const Stroka*> TKeys;
            TKeys keys;
            keys.reserve(map.size());
            for (TJsonValue::TMap::const_iterator it = map.begin(); it != map.end(); ++it) {
                keys.push_back(&(it->first));
            }
            Sort(keys.begin(), keys.end(), LessStrPtr);
            for (TKeys::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                TJsonValue::TMap::const_iterator kv = map.find(**it);
                WriteKey(kv->first);
                WriteJsonValue(&kv->second, sortKeys);
            }
        } else {
            for (TJsonValue::TMap::const_iterator it = map.begin(); it != map.end(); ++it) {
                WriteKey(it->first);
                WriteJsonValue(&it->second, sortKeys);
            }
        }
        EndObject();
        break;
    }
    }
    return TValueContext(*this);
}

TPairContext TBuf::UnsafeWritePair(const TStringBuf& s) {
    if (EXPECT_FALSE(StackTop() != JE_OBJECT)) {
        ythrow TError() << "JSON writer: key:value pair written outside of an object";
    }
    WriteComma();
    UnsafeWriteRawBytes(s);
    return TPairContext(*this);
}

void TBuf::UnsafeWriteValue(const TStringBuf& s) {
    BeginValue();
    UnsafeWriteRawBytes(s);
    EndValue();
}

void TBuf::UnsafeWriteValue(const char* s, size_t len) {
    BeginValue();
    UnsafeWriteRawBytes(s, len);
    EndValue();
}

void TBuf::UnsafeWriteRawBytes(const char* src, size_t len) {
    Stream->Write(src, len);
}

void TBuf::UnsafeWriteRawBytes(const TStringBuf& s) {
    UnsafeWriteRawBytes(~s, +s);
}

const Stroka& TBuf::Str() const {
    if (!StringStream) {
        ythrow TError() << "JSON writer: Str() called "
            "but writing to an external stream";
    }
    if (!(Stack.size() == 1 && StackTop() == JE_OUTER_SPACE)) {
        ythrow TError() << "JSON writer: incomplete object converted to string";
    }
    return StringStream->Str();
}

void TBuf::FlushTo(TOutputStream* stream) {
    if (!StringStream) {
        ythrow TError() << "JSON writer: FlushTo() called "
            "but writing to an external stream";
    }
    stream->Write(StringStream->Str());
    StringStream->clear();
}

}
