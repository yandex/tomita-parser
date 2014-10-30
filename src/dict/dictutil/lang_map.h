#pragma once

#include <util/charset/doccodes.h>

////////////////////////////////////////////////////////////////////////////////
//
// LangMap
//

template<typename T>
class TLangMap {
    DECLARE_NOCOPY(TLangMap);

// Constructor
public:
    struct TValue {
        docLanguage LangID;
        T Value;
    };

    TLangMap(const TValue* data, size_t count)
      : Map(), Ready(false)
    {
        for (const TValue* p = data; p != data + count; ++p)
            Map[p->LangID] = p->Value;
        Ready = true;
    }

// Methods
public:
    const T& operator[](docLanguage langID) const {
        return Map[langID];
    }

// Fields
private:
    T Map[LANG_MAX + 1];
public:
    bool Ready;
};

////////////////////////////////////////////////////////////////////////////////
//
// LangMapOfPtr
//

template<typename T>
class TLangMapOfPtr {
    DECLARE_NOCOPY(TLangMapOfPtr);

// Constructor
public:
    struct TValue {
        docLanguage LangID;
        T Value;
    };

    TLangMapOfPtr(const TValue* data, size_t count)
      : Map()
    {
        for (const TValue* p = data; p != data + count; ++p)
            Map[p->LangID] = &p->Value;
    }

// Methods
public:
    const T* operator[](docLanguage langID) const {
        return Map[langID];
    }

// Fields
private:
    const T* Map[LANG_MAX + 1];
};
