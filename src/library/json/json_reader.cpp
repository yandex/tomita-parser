#include "json_reader.h"

#include <contrib/libs/yajl/yajl_parse.h>

#include <util/system/yassert.h>

namespace NJson {

static const size_t DEFAULT_BUFFER_LEN = 65536;

static int OnNull(void *ctx) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnNull();
}

static int OnBoolean(void *ctx, int boolean) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnBoolean(boolean != 0);
}

static int OnInteger(void *ctx, long long value) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnInteger(value);
}

static int OnUInteger(void *ctx, unsigned long long value) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnUInteger(value);
}

static int OnDouble(void *ctx, double value) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnDouble(value);
}

static int OnString(void *ctx, const unsigned char *val, size_t len) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnString(TStringBuf((const char *)val, len));
}

static int OnStartMap(void *ctx) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnOpenMap();
}

static int OnMapKey(void *ctx, const unsigned char *val, size_t len) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnMapKey(TStringBuf((const char *)val, len));
}

static int OnEndMap(void *ctx) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnCloseMap();
}

static int OnStartArray(void *ctx) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnOpenArray();
}

static int OnEndArray(void *ctx) {
    TJsonCallbacks *cb = (TJsonCallbacks *)ctx;
    return cb->OnCloseArray();
}

static yajl_callbacks callbacks = {
    OnNull,
    OnBoolean,
    OnInteger,
    OnUInteger,
    OnDouble,
    NULL,
    OnString,
    OnStartMap,
    OnMapKey,
    OnEndMap,
    OnStartArray,
    OnEndArray
};

bool ReadJson(TInputStream *in, TJsonCallbacks *cbs) {
    return ReadJson(in, false, cbs);
}

bool ReadJson(TInputStream *in, bool allowComments, TJsonCallbacks *cbs) {
    TJsonReaderConfig config;
    config.AllowComments = allowComments;
    return ReadJson(in, &config, cbs);
}

void ReportError(TJsonCallbacks* cbs, yajl_handle handle, const unsigned char* buffer, size_t rb) {
    unsigned char* err = yajl_get_error(handle, 1, buffer, rb);
    size_t offset = yajl_get_bytes_consumed(handle);
    cbs->OnError(offset, TStringBuf((char*)err));
    yajl_free_error(handle, err);
}

bool ReadJson(TInputStream *in, const TJsonReaderConfig *config, TJsonCallbacks *cbs) {
    YASSERT(!!in);
    YASSERT(!!config);

    bool result = true;
    unsigned char* buffer = (unsigned char*)alloca(config->GetBufferSize());
    yajl_handle handle = yajl_alloc(&callbacks, NULL, (void *) cbs);
    yajl_config(handle, yajl_allow_comments, config->AllowComments ? 1 : 0);
    yajl_config(handle, yajl_dont_validate_strings, config->DontValidateUtf8 ? 1 : 0);
    size_t rb = 0;

    for(;;) {
        rb = in->Read(buffer, config->GetBufferSize()-1);
        if(rb == 0)
            break;
        buffer[rb] = 0;

        if (yajl_parse(handle, buffer, rb) == yajl_status_error) {
            result = false;
            ReportError(cbs, handle, buffer, rb);
            break;
        }
    }

    if (yajl_complete_parse(handle) == yajl_status_error) {
        result = false;
        ReportError(cbs, handle, buffer, rb);
    }

    yajl_free(handle);
    return result && cbs->OnEnd();
}



    bool TParserCallbacks::OpenComplexValue(EJsonValueType type) {
        TJsonValue *pvalue;
        switch (CurrentState) {
        case START:
            Value.SetType(type);
            ValuesStack.push_back(&Value);
            break;
        case IN_ARRAY:
            pvalue = &ValuesStack.back()->AppendValue(type);
            ValuesStack.push_back(pvalue);
            break;
        case AFTER_MAP_KEY:
            pvalue = &ValuesStack.back()->InsertValue(Key, type);
            ValuesStack.push_back(pvalue);
            CurrentState = IN_MAP;
            break;
        default:
            return false;
        }
        return true;
    }

    bool TParserCallbacks::CloseComplexValue() {
        ValuesStack.pop_back();
        if(!ValuesStack.empty()) {
            switch(ValuesStack.back()->GetType()) {
            case JSON_ARRAY:
                CurrentState = IN_ARRAY;
                break;
            case JSON_MAP:
                CurrentState = IN_MAP;
                break;
            default:
                return false;
            }
        } else {
            CurrentState = FINISH;
        }
        return true;
    }

    TParserCallbacks::TParserCallbacks(TJsonValue &value, bool throwOnError)
        : TJsonCallbacks(throwOnError)
        , Value(value)
        , CurrentState(START)
    {
    }

    bool TParserCallbacks::OnNull() {
        return SetValue(JSON_NULL);
    }

    bool TParserCallbacks::OnBoolean(bool val) {
        return SetValue(val);
    }

    bool TParserCallbacks::OnInteger(long long val) {
        return SetValue(val);
    }

    bool TParserCallbacks::OnUInteger(unsigned long long val) {
        return SetValue(val);
    }

    bool TParserCallbacks::OnString(const TStringBuf &val) {
        return SetValue(val);
    }

    bool TParserCallbacks::OnDouble(double val) {
        return SetValue(val);
    }

    bool TParserCallbacks::OnOpenArray() {
        bool res = OpenComplexValue(JSON_ARRAY);
        if (res) CurrentState = IN_ARRAY;
        return res;
    }

    bool TParserCallbacks::OnCloseArray() {
        return CloseComplexValue();
    }

    bool TParserCallbacks::OnOpenMap() {
        bool res = OpenComplexValue(JSON_MAP);
        if (res) CurrentState = IN_MAP;
        return res;
    }

    bool TParserCallbacks::OnCloseMap() {
        return CloseComplexValue();
    }

    bool TParserCallbacks::OnMapKey(const TStringBuf &val) {
        switch(CurrentState) {
        case IN_MAP:
            Key = val;
            CurrentState = AFTER_MAP_KEY;
            break;
        default:
            return false;
        }
        return true;
    }


TJsonReaderConfig::TJsonReaderConfig()
    : AllowComments(false)
    , DontValidateUtf8(false)
    , BufferSize(DEFAULT_BUFFER_LEN)
{
}

void TJsonReaderConfig::SetBufferSize(size_t bufferSize) {
    BufferSize = Max((size_t)1, Min(bufferSize, DEFAULT_BUFFER_LEN));
}

size_t TJsonReaderConfig::GetBufferSize() const {
    return BufferSize;
}

bool ReadJsonTree(TInputStream *in, TJsonValue *out, bool throwOnError) {
    return ReadJsonTree(in, false, out, throwOnError);
}

bool ReadJsonTree(TInputStream *in, bool allowComments, TJsonValue *out, bool throwOnError) {
    TParserCallbacks cb(*out, throwOnError);
    return ReadJson(in, allowComments, &cb);
}

bool ReadJsonTree(TInputStream *in, const TJsonReaderConfig *config, TJsonValue *out, bool throwOnError) {
    TParserCallbacks cb(*out, throwOnError);
    return ReadJson(in, config, &cb);
}

}
