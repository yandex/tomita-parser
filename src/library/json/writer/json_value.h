#pragma once

#include <util/generic/stroka.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NJson {

class TJsonException: public yexception {};

enum EJsonValueType {
    JSON_UNDEFINED,
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_INTEGER,
    JSON_DOUBLE,
    JSON_STRING,
    JSON_MAP,
    JSON_ARRAY,
    JSON_UINTEGER
};

class TJsonValue {
    void Clear();
public:

    typedef yhash_map<Stroka, TJsonValue> TMap;
    typedef yvector<TJsonValue> TArray;

    TJsonValue(EJsonValueType type = JSON_UNDEFINED);
    TJsonValue(bool value);
    TJsonValue(int value);
    TJsonValue(unsigned int value);
    TJsonValue(long value);
    TJsonValue(unsigned long value);
    TJsonValue(long long value);
    TJsonValue(unsigned long long value);
    TJsonValue(double value);
    TJsonValue(const Stroka& value);
    TJsonValue(const char* value);
    TJsonValue(const TStringBuf& value);
    TJsonValue(const TJsonValue& vval);

    TJsonValue& operator = (const TJsonValue& val);
    ~TJsonValue() { Clear(); }

    EJsonValueType GetType() const;
    TJsonValue& SetType(EJsonValueType type);

    TJsonValue& SetValue(const TJsonValue& value);

    TJsonValue& InsertValue(const Stroka& key, const TJsonValue& value); //for Map
    TJsonValue& InsertValue(const TStringBuf& key, const TJsonValue& value); //for Map
    TJsonValue& InsertValue(const char* key, const TJsonValue& value); //for Map
    TJsonValue& AppendValue(const TJsonValue& value); //for Array

    bool GetValueByPath(const Stroka& path, TJsonValue& result, const char* delimeter = ".");
    // faster TStringBuf version of previous, returns NULL on failure
    const TJsonValue* GetValueByPath(TStringBuf key, char delim) const;
    TJsonValue& SetValueByPath(TStringBuf key, char delim, const TJsonValue& value);


    void EraseValue(const TStringBuf& key);

    TJsonValue& operator [] (size_t idx);
    TJsonValue& operator [] (const TStringBuf& key);
    const TJsonValue& operator [] (size_t idx) const;
    const TJsonValue& operator [] (const TStringBuf& key) const;

    bool GetBoolean() const;
    long long GetInteger() const;
    unsigned long long GetUInteger() const;
    double GetDouble() const;
    const Stroka& GetString() const;
    const TMap& GetMap() const;
    const TArray& GetArray() const;

    //throwing TJsonException possible
    bool GetBooleanSafe() const;
    long long GetIntegerSafe() const;
    unsigned long long GetUIntegerSafe() const;
    double GetDoubleSafe() const;
    const Stroka& GetStringSafe() const;
    const TMap& GetMapSafe() const;
    const TArray& GetArraySafe() const;

    bool GetBooleanRobust() const;
    long long GetIntegerRobust() const;
    unsigned long long GetUIntegerRobust() const;
    double GetDoubleRobust() const;
    Stroka GetStringRobust() const;

    //exception-free accessors
    bool GetBoolean(bool* value) const;
    bool GetInteger(long long* value) const;
    bool GetUInteger(unsigned long long* value) const;
    bool GetDouble(double* value) const;
    bool GetString(Stroka* value) const;
    bool GetMap(TMap* value) const;
    bool GetArray(TArray* value) const;
    bool GetMapPointer(const TMap** value) const;
    bool GetArrayPointer(const TArray** value) const;
    bool GetValue(size_t index, TJsonValue* value) const;
    bool GetValue(const TStringBuf& key, TJsonValue* value) const;
    bool GetValuePointer(size_t index, const TJsonValue** value) const;
    bool GetValuePointer(const TStringBuf& key, const TJsonValue** value) const;

    bool IsNull() const;
    bool IsBoolean() const;
    bool IsDouble() const;
    bool IsString() const;
    bool IsMap() const;
    bool IsArray() const;

    /// @return true if JSON_INTEGER or (JSON_UINTEGER and Value <= Max<long long>)
    bool IsInteger() const;

    /// @return true if JSON_UINTEGER or (JSON_INTEGER and Value >= 0)
    bool IsUInteger() const;

    bool Has(const TStringBuf& key) const;
    bool Has(size_t key) const;

    /// Non-robust comparison.
    bool operator== (const TJsonValue& rhs) const;

    bool operator!= (const TJsonValue& rhs) const {
        return !(*this == rhs);
    }

    void Swap(TJsonValue& rhs);

private:
    EJsonValueType Type;
    union TValueUnion {
        bool Boolean;
        long long Integer;
        unsigned long long UInteger;
        double Double;
        Stroka* String;
        TMap* Map;
        TArray* Array;
    };
    TValueUnion Value;
};

bool GetBoolean(const TJsonValue& jv, size_t index, bool *value);
bool GetInteger(const TJsonValue& jv, size_t index, long long *value);
bool GetUInteger(const TJsonValue& jv, size_t index, unsigned long long *value);
bool GetDouble(const TJsonValue& jv, size_t index, double *value);
bool GetString(const TJsonValue& jv, size_t index, Stroka *value);
bool GetMapPointer(const TJsonValue& jv, size_t index, const TJsonValue::TMap **value);
bool GetArrayPointer(const TJsonValue& jv, size_t index, const TJsonValue::TArray **value);

bool GetBoolean(const TJsonValue& jv, const Stroka& key, bool *value);
bool GetInteger(const TJsonValue& jv, const Stroka& key, long long *value);
bool GetUInteger(const TJsonValue& jv, const Stroka& key, unsigned long long *value);
bool GetDouble(const TJsonValue& jv, const Stroka& key, double *value);
bool GetString(const TJsonValue& jv, const Stroka& key, Stroka *value);
bool GetMapPointer(const TJsonValue& jv, const Stroka& key, const TJsonValue::TMap **value);
bool GetArrayPointer(const TJsonValue& jv, const Stroka& key, const TJsonValue::TArray **value);

}
