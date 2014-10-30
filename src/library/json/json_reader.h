#pragma once

#include "json_value.h"

#include <util/stream/input.h>
#include <util/stream/str.h>
#include <util/stream/mem.h>

namespace NJson {

class TJsonCallbacks {
public:
    explicit TJsonCallbacks(bool throwException = false)
        : ThrowException(throwException)
    {}

    virtual ~TJsonCallbacks() {}
    virtual bool OnNull() { return true; }
    virtual bool OnBoolean(bool) { return true; }
    virtual bool OnInteger(long long) { return true; }
    virtual bool OnUInteger(unsigned long long) { return true; }
    virtual bool OnDouble(double) { return true; }
    virtual bool OnString(const TStringBuf &) { return true; }
    virtual bool OnOpenMap() { return true; }
    virtual bool OnMapKey(const TStringBuf &) { return true; }
    virtual bool OnCloseMap() { return true; }
    virtual bool OnOpenArray() { return true; }
    virtual bool OnCloseArray() { return true; }

    virtual bool OnStringNoCopy(const TStringBuf& s) {
        return OnString(s);
    }

    virtual bool OnMapKeyNoCopy(const TStringBuf& s) {
        return OnMapKey(s);
    }

    virtual bool OnEnd() { return true; }

    virtual void OnError(size_t off, TStringBuf reason) {
        if (ThrowException)
            ythrow TJsonException() << "JSON error at offset " << off << " (" << reason << ")";
    }

protected:
    bool ThrowException;
};

struct TJsonReaderConfig {
    TJsonReaderConfig();

    bool AllowComments;
    bool DontValidateUtf8;

    void SetBufferSize(size_t bufferSize);

    size_t GetBufferSize() const;

private:
    size_t BufferSize;
};

bool ReadJsonFast(TStringBuf in, TJsonCallbacks* callbacks);

inline bool ValidateJsonFast(TStringBuf in, bool throwOnError = false) {
    TJsonCallbacks c(throwOnError);
    return ReadJsonFast(in, &c);
}

bool ReadJson(TInputStream *in, TJsonCallbacks *callbacks);
bool ReadJsonTree(TInputStream *in, TJsonValue *out, bool throwOnError = false);

bool ReadJson(TInputStream *in, bool allowComments, TJsonCallbacks *callbacks);
bool ReadJsonTree(TInputStream *in, bool allowComments, TJsonValue *out, bool throwOnError = false);

bool ReadJson(TInputStream *in, const TJsonReaderConfig *config, TJsonCallbacks *callbacks);
bool ReadJsonTree(TInputStream *in, const TJsonReaderConfig *config, TJsonValue *out, bool throwOnError = false);

inline bool ValidateJson(TInputStream* in, const TJsonReaderConfig* config, bool throwOnError = false) {
    TJsonCallbacks c(throwOnError);
    return ReadJson(in, config, &c);
}

inline bool ValidateJson(TStringBuf in, const TJsonReaderConfig& config = TJsonReaderConfig(), bool throwOnError = false) {
    TMemoryInput min(~in, +in);
    return ValidateJson(&min, &config, throwOnError);
}

class TParserCallbacks: public TJsonCallbacks {
public:
    TParserCallbacks(TJsonValue &value, bool throwOnError = false);
    bool OnNull();
    bool OnBoolean(bool val);
    bool OnInteger(long long val);
    bool OnUInteger(unsigned long long val);
    bool OnString(const TStringBuf &val);
    bool OnDouble(double val);
    bool OnOpenArray();
    bool OnCloseArray();
    bool OnOpenMap();
    bool OnCloseMap();
    bool OnMapKey(const TStringBuf &val);
protected:
    TJsonValue &Value;
    Stroka Key;
    yvector<TJsonValue *> ValuesStack;

    enum {
        START,
        AFTER_MAP_KEY,
        IN_MAP,
        IN_ARRAY,
        FINISH
    } CurrentState;

    template <class T> bool SetValue(const T &value) {
        switch(CurrentState) {
        case START:
            Value.SetValue(value);
            break;
        case AFTER_MAP_KEY:
            ValuesStack.back()->InsertValue(Key, value);
            CurrentState = IN_MAP;
            break;
        case IN_ARRAY:
            ValuesStack.back()->AppendValue(value);
            break;
        default:
            return false;
        }
        return true;
    }

    bool OpenComplexValue(EJsonValueType type);
    bool CloseComplexValue();
};

}
