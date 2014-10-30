#include "lang_ids.h"
#include "dictutil.h"
#include "exceptions.h"
#include "str.h"
#include <util/string/strip.h>

const char TLangIDs::ClassName[] = "LangIDs";

const TLangIDs TLangIDs::Empty;

TLangIDs::TLangIDs(docLanguage langId1, docLanguage langId2)
  : Ids()
{
    Set(langId1).Set(langId2);
}

TLangIDs::TLangIDs(const docLanguage langIds[], int count)
  : Ids()
{
    Set(langIds, count);
}

TLangIDs TLangIDs::Parse(const Stroka& str) {
    return DoParse(0, str, 0);
}

TLangIDs TLangIDs::Parse(const Stroka& str, Stroka& unknownLangs) {
    return DoParse(0, str, &unknownLangs);
}

TLangIDs TLangIDs::Parse(char unkLang, const Stroka& str, Stroka& unknownLangs) {
    return DoParse(unkLang, str, &unknownLangs);
}

TLangIDs TLangIDs::Parse(const VectorStrok& langs, Stroka& unknownLangs) {
    return DoParse(0, langs, &unknownLangs);
}

TLangIDs TLangIDs::DoParse(char unkLang, const Stroka& str, Stroka* unknownLangs) {
    return DoParse(unkLang, SplitBySet(str, " \t,;"), unknownLangs);
}

TLangIDs TLangIDs::DoParse(char unkLang, const VectorStrok& langs, Stroka* unknownLangs) {
    TLangIDs langIDs;
    Stroka badLangs;

    if (unknownLangs)
        unknownLangs->clear();

    for (int i = 0, n = (int)langs.size(); i < n; ++i) {
        Stroka lang = langs[i];
        strip(lang, lang);
        size_t pos = lang.find('_'); // example: en_US
        if (pos != Stroka::npos)
            lang = lang.substr(0, pos);
        if (lang.empty())
            continue;

        if (unkLang && lang.length() == 1 && lang[0] == unkLang) {
            langIDs.Set(LANG_UNK);
            continue;
        }

        docLanguage langID = LanguageByName(lang);
        if (langID == LANG_UNK) {
            if (!badLangs.empty())
                badLangs.push_back(',');
            badLangs.append(lang);
            continue;
        }
        langIDs.Set(langID);
    }

    if (badLangs.length()) {
        if (!unknownLangs)
            throw TException(EXSRC_METHOD("Parse"), "Unknown langs '%.30s'", ~badLangs);
        *unknownLangs = badLangs;
    }

    return langIDs;
}

TLangIDs TLangIDs::FromLong(ui64 ids) {
    TLangIDs result;
    result.Ids = ids & Mask;
    return result;
}

Stroka TLangIDs::ToString() const {
    Stroka result;
    for (docLanguage langID = FindFirst(); langID != LANG_MAX; langID = FindNext(langID)) {
        if (!result.empty())
            result.push_back(',');
        result.append(langID ? IsoNameByLanguage(langID) : "*");
    }
    return result;
}

TLangIDs& TLangIDs::Set(docLanguage langID, bool value) {
    ui64 bit = ui64(1) << langID;
    if (value)
        Ids |= bit;
    else
        Ids &= ~bit;
    return *this;
}

TLangIDs& TLangIDs::Set(const docLanguage langIds[], int count) {
    for (int i = 0; i < count; ++i)
        Ids |= ui64(1) << langIds[i];
    return *this;
}

docLanguage TLangIDs::FindFirst() const {
    return NextLangID(0);
}

docLanguage TLangIDs::FindNext(docLanguage langID) const {
    return NextLangID(ui64(1) << langID);
}

docLanguage TLangIDs::NextLangID(ui64 bit) const {
    if (!Ids)
        return LANG_MAX;

    bit = NextBit(Ids, bit);
    if (!bit)
        return LANG_MAX;

    ui32 hi = ui32(bit >> 32);
    int id = hi ? 32 + Log2i(hi) : Log2i(ui32(bit));
    return docLanguage(id);
}

int TLangIDs::Count() const {
    const ui32* ids = (const ui32*)(const void*)&Ids;
    return CountBits32(ids[0]) + CountBits32(ids[1]);
}
