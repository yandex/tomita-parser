#include "json_value.h"
#include "json.h"

#include <util/generic/ymath.h>
#include <util/generic/ylimits.h>
#include <util/generic/utility.h>
#include <util/stream/str.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/string/vector.h>
#include <util/system/yassert.h>

static bool
AreJsonMapsEqual(const NJson::TJsonValue& lhs, const NJson::TJsonValue& rhs) {
    using namespace NJson;

    VERIFY(lhs.GetType() == JSON_MAP, "lhs has not a JSON_MAP type.");

    if (rhs.GetType() != JSON_MAP)
        return false;

    typedef TJsonValue::TMap TMap;
    const TMap& lhsMap = lhs.GetMap();
    const TMap& rhsMap = rhs.GetMap();

    if (lhsMap.size() != rhsMap.size())
        return false;

    for (TMap::const_iterator lhsIt = lhsMap.begin(), end = lhsMap.end();
         lhsIt != end; ++lhsIt)
    {
        TMap::const_iterator rhsIt = rhsMap.find(lhsIt->first);
        if (rhsIt == rhsMap.end())
            return false;

        if (lhsIt->second != rhsIt->second)
            return false;
    }

    return true;
}

static bool
AreJsonArraysEqual(const NJson::TJsonValue& lhs, const NJson::TJsonValue& rhs) {
    using namespace NJson;

    VERIFY(lhs.GetType() == JSON_ARRAY, "lhs has not a JSON_ARRAY type.");

    if (rhs.GetType() != JSON_ARRAY)
        return false;

    typedef TJsonValue::TArray TArray;
    const TArray& lhsArray = lhs.GetArray();
    const TArray& rhsArray = rhs.GetArray();

    if (lhsArray.size() != rhsArray.size())
        return false;

    for (TArray::const_iterator lhsIt = lhsArray.begin(), rhsIt = rhsArray.begin();
         lhsIt != lhsArray.end(); ++lhsIt, ++rhsIt)
    {
        if (*lhsIt != *rhsIt)
            return false;
    }

    return true;
}

namespace NJson {

const char *GetTypeName(EJsonValueType type) {
    switch (type) {
    case JSON_STRING:
        return "String";
    case JSON_MAP:
        return "Map";
    case JSON_ARRAY:
        return "Array";
    case JSON_UNDEFINED:
        return "Undefined";
    case JSON_NULL:
        return "Null";
    case JSON_BOOLEAN:
        return "Boolean";
    case JSON_INTEGER:
        return "Integer";
    case JSON_UINTEGER:
        return "UInteger";
    case JSON_DOUBLE:
        return "Double";
    default:
        return "UnknownType";
    }
}

TJsonValue::TJsonValue(EJsonValueType type)
    : Type(JSON_UNDEFINED)
{
    SetType(type);
}

TJsonValue::TJsonValue(const TJsonValue &val)
{
    Type = val.Type;
    switch (Type) {
    case JSON_STRING:
        Value.String = new Stroka(val.GetString());
        break;
    case JSON_MAP:
        Value.Map = new yhash_map<Stroka, TJsonValue>(val.GetMap());
        break;
    case JSON_ARRAY:
        Value.Array = new yvector<TJsonValue>(val.GetArray());
        break;
    case JSON_UNDEFINED:
    case JSON_NULL:
    case JSON_BOOLEAN:
    case JSON_INTEGER:
    case JSON_UINTEGER:
    case JSON_DOUBLE:
        Value = val.Value;
        break;
    }
}

TJsonValue &TJsonValue::operator = (const TJsonValue &val) {

    if (this == &val)
        return *this;

    Clear();
    Type = val.Type;

    switch (Type) {
    case JSON_STRING:
        Value.String = new Stroka(val.GetString());
        break;
    case JSON_MAP:
        Value.Map = new yhash_map<Stroka, TJsonValue>(val.GetMap());
        break;
    case JSON_ARRAY:
        Value.Array = new yvector<TJsonValue>(val.GetArray());
        break;
    case JSON_UNDEFINED:
    case JSON_NULL:
    case JSON_BOOLEAN:
    case JSON_INTEGER:
    case JSON_UINTEGER:
    case JSON_DOUBLE:
        Value = val.Value;
        break;
    }
    return *this;
}

TJsonValue::TJsonValue(bool value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_BOOLEAN);
    Value.Boolean = value;
}

TJsonValue::TJsonValue(long long value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_INTEGER);
    Value.Integer = value;
}

TJsonValue::TJsonValue(unsigned long long value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_UINTEGER);
    Value.UInteger = value;
}

TJsonValue::TJsonValue(int value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_INTEGER);
    Value.Integer = value;
}

TJsonValue::TJsonValue(unsigned int value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_UINTEGER);
    Value.UInteger = value;
}

TJsonValue::TJsonValue(long value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_INTEGER);
    Value.Integer = value;
}

TJsonValue::TJsonValue(unsigned long value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_UINTEGER);
    Value.UInteger = value;
}

TJsonValue::TJsonValue(double value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_DOUBLE);
    Value.Double = value;
}

TJsonValue::TJsonValue(const Stroka& value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_STRING);
    *Value.String = value;
}

TJsonValue::TJsonValue(const TStringBuf& value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_STRING);
    *Value.String = value;
}

TJsonValue::TJsonValue(const char* value)
    : Type(JSON_UNDEFINED)
{
    SetType(JSON_STRING);
    *Value.String = value;
}

EJsonValueType TJsonValue::GetType() const {
    return Type;
}

TJsonValue& TJsonValue::SetType(EJsonValueType type) {
    if (Type == type)
        return *this;

    Clear();
    Type = type;

    switch (Type) {
    case JSON_STRING:
        Value.String = new Stroka();
        break;
    case JSON_MAP:
        Value.Map = new yhash_map<Stroka, TJsonValue>();
        break;
    case JSON_ARRAY:
        Value.Array = new yvector<TJsonValue>();
        break;
    case JSON_UNDEFINED:
    case JSON_NULL:
    case JSON_BOOLEAN:
    case JSON_INTEGER:
    case JSON_UINTEGER:
    case JSON_DOUBLE:
        break;
    }

    return *this;
}

TJsonValue& TJsonValue::SetValue(const TJsonValue &value) {
    return *this = value;
}

TJsonValue& TJsonValue::InsertValue(const Stroka& key, const TJsonValue& value) {
    SetType(JSON_MAP);
    return (*Value.Map)[key] = value;
}

TJsonValue& TJsonValue::InsertValue(const TStringBuf& key, const TJsonValue& value) {
    return InsertValue(Stroka(key), value);
}

TJsonValue& TJsonValue::InsertValue(const char* key, const TJsonValue& value) {
    return InsertValue(Stroka(key), value);
}

TJsonValue& TJsonValue::AppendValue(const TJsonValue& value) {
    SetType(JSON_ARRAY);
    Value.Array->push_back(value);
    return Value.Array->back();
}

void TJsonValue::EraseValue(const TStringBuf& key) {
    if (IsMap()) {
        TMap::iterator it = Value.Map->find(key);
        if (it != Value.Map->end())
            Value.Map->erase(it);
    }
}

void TJsonValue::Clear() {

    switch (Type) {
    case JSON_STRING:
        delete Value.String;
        break;
    case JSON_MAP:
        delete Value.Map;
        break;
    case JSON_ARRAY:
        delete Value.Array;
        break;
    case JSON_UNDEFINED:
    case JSON_NULL:
    case JSON_BOOLEAN:
    case JSON_INTEGER:
    case JSON_UINTEGER:
    case JSON_DOUBLE:
        break;

    }
    Type = JSON_UNDEFINED;
}

TJsonValue &TJsonValue::operator[] (size_t idx) {
    SetType(JSON_ARRAY);
    if (Value.Array->size() <= idx)
        Value.Array->resize(idx + 1);
    return (*Value.Array)[idx];
}

TJsonValue &TJsonValue::operator[] (const TStringBuf& key) {
    SetType(JSON_MAP);
    return (*Value.Map)[key];
}

struct TDefaultsHolder {
    Stroka String;
    TJsonValue::TMap Map;
    TJsonValue::TArray Array;
    TJsonValue Value;
};

const TJsonValue &TJsonValue::operator[] (size_t idx) const {
    const TJsonValue* ret = NULL;
    if (GetValuePointer(idx, &ret))
        return *ret;

    return Singleton<TDefaultsHolder>()->Value;
}

const TJsonValue &TJsonValue::operator[] (const TStringBuf& key) const {
    const TJsonValue* ret = NULL;
    if (GetValuePointer(key, &ret))
        return *ret;

    return Singleton<TDefaultsHolder>()->Value;
}

bool TJsonValue::GetBoolean() const {
    return Type != JSON_BOOLEAN ? false : Value.Boolean;
}

long long TJsonValue::GetInteger() const {
    if (!IsInteger())
        return 0;

    switch (Type) {
    case JSON_INTEGER:
        return Value.Integer;

    case JSON_UINTEGER:
        return Value.UInteger;

    case JSON_DOUBLE:
        return Value.Double;

    default:
        YASSERT(false && "Unexpected type.");
        return 0;
    }
}

unsigned long long TJsonValue::GetUInteger() const {
    if (!IsUInteger())
        return 0;

    switch (Type) {
    case JSON_UINTEGER:
        return Value.UInteger;

    case JSON_INTEGER:
        return Value.Integer;

    case JSON_DOUBLE:
        return Value.Double;

    default:
        YASSERT(false && "Unexpected type.");
        return 0;
    }
}

double TJsonValue::GetDouble() const {
    if (!IsDouble())
        return 0.0;

    switch (Type) {
    case JSON_DOUBLE:
        return Value.Double;

    case JSON_INTEGER:
        return Value.Integer;

    case JSON_UINTEGER:
        return Value.UInteger;

    default:
        YASSERT(false && "Unexpected type.");
        return 0.0;
    }
}

const Stroka& TJsonValue::GetString() const {
    return Type != JSON_STRING ? Singleton<TDefaultsHolder>()->String : *Value.String;
}

const TJsonValue::TMap& TJsonValue::GetMap() const {
    return Type != JSON_MAP ? Singleton<TDefaultsHolder>()->Map : *Value.Map;
}

const TJsonValue::TArray &TJsonValue::GetArray() const {
    return (Type != JSON_ARRAY) ? Singleton<TDefaultsHolder>()->Array : *Value.Array;
}

bool TJsonValue::GetBooleanSafe() const {
    if (Type != JSON_BOOLEAN)
        ythrow TJsonException() << "Not a boolean";

    return Value.Boolean;
}

long long TJsonValue::GetIntegerSafe() const {
    if (!IsInteger())
        ythrow TJsonException() << "Not an integer";

    return GetInteger();
}

unsigned long long TJsonValue::GetUIntegerSafe() const {
    if (!IsUInteger())
        ythrow TJsonException() << "Not an unsigned integer";

    return GetUInteger();
}

double TJsonValue::GetDoubleSafe() const {
    if (!IsDouble())
        ythrow TJsonException() << "Not a double";

    return GetDouble();
}

const Stroka& TJsonValue::GetStringSafe() const {
    if (Type != JSON_STRING)
        ythrow TJsonException() << "Not a string";

    return *Value.String;
}

const TJsonValue::TMap& TJsonValue::GetMapSafe() const {
    if (Type != JSON_MAP)
        ythrow TJsonException() << "Not a map";

    return *Value.Map;
}

const TJsonValue::TArray& TJsonValue::GetArraySafe() const {
    if (Type != JSON_ARRAY)
        ythrow TJsonException() << "Not an array";

    return *Value.Array;
}

bool TJsonValue::GetBooleanRobust() const {
    switch (Type) {
    case JSON_ARRAY:
        return !Value.Array->empty();
    case JSON_MAP:
        return !Value.Map->empty();
    case JSON_INTEGER:
    case JSON_UINTEGER:
    case JSON_DOUBLE:
        return GetIntegerRobust();
    case JSON_STRING:
        return GetIntegerRobust() || istrue(*Value.String);
    case JSON_NULL:
    case JSON_UNDEFINED:
    default:
        return 0;
    case JSON_BOOLEAN:
        return Value.Boolean;
    }
}

long long TJsonValue::GetIntegerRobust() const {
    switch (Type) {
    case JSON_ARRAY:
        return Value.Array->size();
    case JSON_MAP:
        return Value.Map->size();
    case JSON_BOOLEAN:
        return Value.Boolean;
    case JSON_DOUBLE:
    case JSON_STRING:
        return GetDoubleRobust();
    case JSON_NULL:
    case JSON_UNDEFINED:
    default:
        return 0;
    case JSON_INTEGER:
    case JSON_UINTEGER:
        return Value.Integer;
    }
}

unsigned long long TJsonValue::GetUIntegerRobust() const {
    switch (Type) {
    case JSON_ARRAY:
        return Value.Array->size();
    case JSON_MAP:
        return Value.Map->size();
    case JSON_BOOLEAN:
        return Value.Boolean;
    case JSON_DOUBLE:
    case JSON_STRING:
        return GetDoubleRobust();
    case JSON_NULL:
    case JSON_UNDEFINED:
    default:
        return 0;
    case JSON_INTEGER:
    case JSON_UINTEGER:
        return Value.UInteger;
    }
}

double TJsonValue::GetDoubleRobust() const {
    switch (Type) {
    case JSON_ARRAY:
        return Value.Array->size();
    case JSON_MAP:
        return Value.Map->size();
    case JSON_BOOLEAN:
        return Value.Boolean;
    case JSON_INTEGER:
        return Value.Integer;
    case JSON_UINTEGER:
        return Value.UInteger;
    case JSON_STRING:
        try {
            return !!Value.String ? FromString<double>(*Value.String) : 0;
        } catch (const yexception&) {
            return 0;
        }
    case JSON_NULL:
    case JSON_UNDEFINED:
    default:
        return 0;
    case JSON_DOUBLE:
        return Value.Double;
    }
}

Stroka TJsonValue::GetStringRobust() const {
    switch (Type) {
    case JSON_ARRAY:
    case JSON_MAP:
    case JSON_BOOLEAN:
    case JSON_DOUBLE:
    case JSON_INTEGER:
    case JSON_UINTEGER:
    case JSON_NULL:
    case JSON_UNDEFINED:
    default: {
        NJsonWriter::TBuf sout;
        sout.WriteJsonValue(this);
        return sout.Str();
    } case JSON_STRING:
        return *Value.String;
    }
}

bool TJsonValue::GetBoolean(bool *value) const {
    if (Type != JSON_BOOLEAN)
        return false;

    *value = Value.Boolean;
    return true;
}

bool TJsonValue::GetInteger(long long *value) const {
    if (!IsInteger())
        return false;

    *value = GetInteger();
    return true;
}

bool TJsonValue::GetUInteger(unsigned long long *value) const {
    if (!IsUInteger())
        return false;

    *value = GetUInteger();
    return true;
}

bool TJsonValue::GetDouble(double *value) const {
    if (!IsDouble())
        return false;

    *value = GetDouble();
    return true;
}

bool TJsonValue::GetString(Stroka *value) const {
    if (Type != JSON_STRING)
        return false;

    *value = *Value.String;
    return true;
}

bool TJsonValue::GetMap(TJsonValue::TMap *value) const {
    if (Type != JSON_MAP)
        return false;

    *value = *Value.Map;
    return true;
}

bool TJsonValue::GetArray(TJsonValue::TArray *value) const {
    if (Type != JSON_ARRAY)
        return false;

    *value = *Value.Array;
    return true;
}

bool TJsonValue::GetMapPointer(const TJsonValue::TMap **value) const {
    if (Type != JSON_MAP)
        return false;

    *value = Value.Map;
    return true;
}

bool TJsonValue::GetArrayPointer(const TJsonValue::TArray **value) const {
    if (Type != JSON_ARRAY)
        return false;

    *value = Value.Array;
    return true;
}


bool TJsonValue::GetValue(size_t index, TJsonValue *value) const {
    const TJsonValue* tmp = NULL;
    if (GetValuePointer(index, &tmp)) {
        *value = *tmp;
        return true;
    }
    return false;
}

bool TJsonValue::GetValue(const TStringBuf &key, TJsonValue *value) const {
    const TJsonValue* tmp = NULL;
    if (GetValuePointer(key, &tmp)) {
        *value = *tmp;
        return true;
    }
    return false;
}

bool TJsonValue::GetValuePointer(size_t index, const TJsonValue **value) const {
    if (Type == JSON_ARRAY && index < Value.Array->size()) {
        *value = &(*Value.Array)[index];
        return true;
    }
    return false;
}

bool TJsonValue::GetValuePointer(const TStringBuf &key, const TJsonValue **value) const {
    if (Type == JSON_MAP) {
        TMap::const_iterator it = Value.Map->find(key);
        if (it != Value.Map->end()) {
            *value = &(it->second);
            return true;
        }
    }
    return false;
}

bool TJsonValue::IsNull() const {
    return Type == JSON_NULL;
}

bool TJsonValue::IsBoolean() const {
    return Type == JSON_BOOLEAN;
}

bool TJsonValue::IsInteger() const {
    switch (Type) {
    case JSON_INTEGER:
        return true;

    case JSON_UINTEGER:
        return (Value.UInteger <= static_cast<unsigned long long>(Max<long long>()));

    case JSON_DOUBLE:
        return ((long long)Value.Double == Value.Double);

    default:
        return false;
    }
}

bool TJsonValue::IsUInteger() const {
    switch (Type) {
    case JSON_UINTEGER:
        return true;

    case JSON_INTEGER:
        return (Value.Integer >= 0);

    case JSON_DOUBLE:
        return ((unsigned long long)Value.Double == Value.Double);

    default:
        return false;
    }
}

bool TJsonValue::IsDouble() const {
    // Check whether we can convert integer to floating-point
    // without precision loss.
    switch (Type) {
    case JSON_DOUBLE:
        return true;

    case JSON_INTEGER:
        return ((long long)(double)Value.Integer == Value.Integer);

    case JSON_UINTEGER:
        return ((unsigned long long)(double)Value.UInteger == Value.UInteger);

    default:
        return false;
    }
}

bool TJsonValue::GetValueByPath(const Stroka& path, TJsonValue& result, const char* delimeter) {
    VectorStrok pathSteps = splitStrokuBySet(~path, delimeter);
    TJsonValue currentJson = *this;
    for (ui32 i = 0; i < pathSteps.size(); ++i) {
        TMap mapInfo;
        if (!currentJson.GetMap(&mapInfo))
            return false;
        TMap::const_iterator it = mapInfo.find(pathSteps[i]);
        if (it != mapInfo.end()) {
            currentJson = it->second;
        }
        else {
            return false;
        }
    }
    result = currentJson;
    return true;
}

const TJsonValue* TJsonValue::GetValueByPath(TStringBuf key, char delim) const {
    const TJsonValue* ret = this;
    while (!key.empty()) {
        if (!ret->GetValuePointer(key.NextTok(delim), &ret))
            return NULL;
    }
    return ret;
}

TJsonValue& TJsonValue::SetValueByPath(TStringBuf key, char delim, const TJsonValue& value) {
    TJsonValue* ret = this;
    while (!key.empty())
        ret = &ret->operator[](key.NextTok(delim));
    *ret = value;
    return *ret;
}

bool TJsonValue::IsString() const {
    return Type == JSON_STRING;
}

bool TJsonValue::IsMap() const {
    return Type == JSON_MAP;
}

bool TJsonValue::IsArray() const {
    return Type == JSON_ARRAY;
}

bool TJsonValue::Has(const TStringBuf& key) const {
    return Type == JSON_MAP && Value.Map->has(key);
}

bool TJsonValue::Has(size_t key) const {
    return Type == JSON_ARRAY && Value.Array->size() > key;
}

bool TJsonValue::operator== (const TJsonValue& rhs) const {
    switch (Type) {
    case JSON_UNDEFINED: {
        return (rhs.GetType() == JSON_UNDEFINED);
    }

    case JSON_NULL: {
        return rhs.IsNull();
    }

    case JSON_BOOLEAN: {
        return (rhs.IsBoolean() && Value.Boolean == rhs.Value.Boolean);
    }

    case JSON_INTEGER: {
        return (rhs.IsInteger()
                && GetInteger() == rhs.GetInteger());
    }

    case JSON_UINTEGER: {
        return (rhs.IsUInteger()
                && GetUInteger() == rhs.GetUInteger());
    }

    case JSON_STRING: {
        return (rhs.IsString()
                && !!Value.String && !!rhs.Value.String
                && *Value.String == *rhs.Value.String);
    }

    case JSON_DOUBLE: {
        return (rhs.IsDouble()
                && fabs(GetDouble() - rhs.GetDouble()) <= FLT_EPSILON);
    }

    case JSON_MAP:
        return AreJsonMapsEqual(*this, rhs);

    case JSON_ARRAY:
        return AreJsonArraysEqual(*this, rhs);

    default:
        YASSERT(false && "Unknown type.");
        return false;
    }
}

void TJsonValue::Swap(TJsonValue& rhs) {
    ::DoSwap(Type, rhs.Type);

    // DoSwap does not support unions
    TValueUnion tmp = Value;
    Value = rhs.Value;
    rhs.Value = tmp;
}

//****************************************************************

bool GetBoolean(const TJsonValue &jv, size_t index, bool *value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsBoolean())
        return false;

    *value = v.GetBoolean();
    return true;
}

bool GetInteger(const TJsonValue &jv, size_t index, long long *value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsInteger())
        return false;

    *value = v.GetInteger();
    return true;
}

bool GetUInteger(const TJsonValue &jv, size_t index, unsigned long long *value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsUInteger())
        return false;

    *value = v.GetUInteger();
    return true;
}

bool GetDouble(const TJsonValue &jv, size_t index, double *value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsDouble())
        return false;

    *value = v.GetDouble();
    return true;
}

bool GetString(const TJsonValue &jv, size_t index, Stroka *value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsString())
        return false;

    *value = v.GetString();
    return true;
}

bool GetMapPointer(const TJsonValue &jv, size_t index, const TJsonValue::TMap **value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsMap())
        return false;

    *value = &v.GetMap();
    return true;
}

bool GetArrayPointer(const TJsonValue &jv, size_t index, const TJsonValue::TArray **value) {
    TJsonValue v;
    if (!jv.GetValue(index, &v) || !v.IsArray())
        return false;

    *value = &v.GetArray();
    return true;
}

bool GetBoolean(const TJsonValue &jv, const Stroka &key, bool *value) {
    TJsonValue v;
    if (!jv.GetValue(key, &v) || !v.IsBoolean())
        return false;

    *value = v.GetBoolean();
    return true;
}

bool GetInteger(const TJsonValue &jv, const Stroka &key, long long *value) {
    TJsonValue v;
    if (!jv.GetValue(key, &v) || !v.IsInteger())
        return false;

    *value = v.GetInteger();
    return true;
}

bool GetUInteger(const TJsonValue &jv, const Stroka &key, unsigned long long *value) {
    TJsonValue v;
    if (!jv.GetValue(key, &v) || !v.IsUInteger())
        return false;

    *value = v.GetUInteger();
    return true;
}

bool GetDouble(const TJsonValue &jv, const Stroka &key, double *value) {
    TJsonValue v;
    if (!jv.GetValue(key, &v) || !v.IsDouble())
        return false;

    *value = v.GetDouble();
    return true;
}

bool GetString(const TJsonValue &jv, const Stroka &key, Stroka *value) {
    TJsonValue v;
    if (!jv.GetValue(key, &v) || !v.IsString())
        return false;

    *value = v.GetString();
    return true;
}

bool GetMapPointer(const TJsonValue &jv, const Stroka &key, const TJsonValue::TMap **value) {
    const TJsonValue *v;
    if (!jv.GetValuePointer(key, &v) || !v->IsMap())
        return false;

    *value = &v->GetMap();
    return true;
}

bool GetArrayPointer(const TJsonValue &jv, const Stroka &key, const TJsonValue::TArray **value) {
    const TJsonValue *v;
    if (!jv.GetValuePointer(key, &v) || !v->IsArray())
        return false;

    *value = &v->GetArray();
    return true;
}

}

template <>
void Out<NJson::TJsonValue>(TOutputStream& out, const NJson::TJsonValue& v) {
    NJsonWriter::TBuf buf(NJsonWriter::HEM_DONT_ESCAPE_HTML, &out);
    buf.WriteJsonValue(&v);
}
