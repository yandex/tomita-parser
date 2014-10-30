#pragma once

#include <util/charset/doccodes.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>

////////////////////////////////////////////////////////////////////////////////
//
// Definitions
//

#define LID_(ID) (ui64(1) << LANG_##ID)

typedef yvector<Stroka> VectorStrok;

////////////////////////////////////////////////////////////////////////////////
//
// LangIDs
//

class TLangIDs {
// Constructor
public:
    TLangIDs() : Ids(0) {}
    TLangIDs(docLanguage langId) : Ids(ui64(1) << langId) {}
    TLangIDs(docLanguage langId1, docLanguage langId2);
    TLangIDs(const TLangIDs& langs) : Ids(langs.Ids) {}
    TLangIDs(const docLanguage langIds[], int count);
    static TLangIDs Parse(const Stroka& str);
    static TLangIDs Parse(const Stroka& str, Stroka& unknownLangs);
    static TLangIDs Parse(char unkLang, const Stroka& str, Stroka& unknownLangs);
    static TLangIDs Parse(const VectorStrok& langs, Stroka& unknownLangs);

    // first LANG_MAX bits are used to initialize the LangIDs
    static TLangIDs FromLong(ui64 ids);

// Constants
public:
    static const TLangIDs Empty;

// Operators
public:
    bool operator==(const TLangIDs& langs) const {
        return Ids == langs.Ids;
    }

    bool operator!=(const TLangIDs& langs) const {
        return Ids != langs.Ids;
    }

    TLangIDs& operator=(const TLangIDs& langs) {
        Ids = langs.Ids;
        return *this;
    }

    TLangIDs& operator&=(const TLangIDs& langs) {
        Ids &= langs.Ids;
        return *this;
    }

    TLangIDs& operator|=(const TLangIDs& langs) {
        Ids |= langs.Ids;
        return *this;
    }

    friend TLangIDs operator&(const TLangIDs& a, const TLangIDs& b) {
        TLangIDs result(a);
        return result &= b;
    }

    friend TLangIDs operator|(const TLangIDs& a, const TLangIDs& b) {
        TLangIDs result(a);
        return result |= b;
    }

    TLangIDs operator~() const {
        TLangIDs result(*this);
        result.Ids ^= Mask;
        return result;
    }

// Methods
public:
    TLangIDs& Set(docLanguage langID, bool value = true);
    TLangIDs& Set(const docLanguage langIds[], int count);
    bool Test(docLanguage langID) const { return (Ids & (ui64(1) << langID)) != 0; }
    bool Intersects(const TLangIDs& langs) const { return (Ids & langs.Ids) != 0; }
    ui64 ToLong() const { return Ids; }
    Stroka ToString() const;
    int Count() const;
    bool IsEmpty() const { return Ids == 0; }
    docLanguage FindFirst() const;
    docLanguage FindNext(docLanguage langID) const;
    void Clear() { Ids = 0; }
    void Clear(const TLangIDs& langs) { Ids &= ~langs.Ids; }

// Helper methods
private:
    docLanguage NextLangID(ui64 bit) const;
    static TLangIDs DoParse(char unkLang, const Stroka& str, Stroka* unknownLangs);
    static TLangIDs DoParse(char unkLang, const VectorStrok& langs, Stroka* unknownLangs);

// Fields
private:
    static const char ClassName[];
    static const ui64 Mask = ui64(-1);
    ui64 Ids;
};
