#pragma once

#include <util/generic/stroka.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/charset/doccodes.h>

#include <FactExtract/Parser/afdocparser/builtins/articles_base.pb.h>

struct typeLangAltInfo
{
    docLanguage lang;
    bool bUseAsPrime;
    bool bUseAlways;
};

//////////////////////////////////////////////////////////////////////////////
// ETypeOfAbrrev - now in articles_basa.pb.h

typedef ymap<Wtroka, ETypeOfAbbrev> TMapAbbrev;


class CLangData {
public:
    CLangData() {
    }

    inline bool HasAbbrev(const Wtroka& text, ETypeOfAbbrev& type) const {
        if (Abbrev.Get() == NULL)
            return false;
        TMapAbbrev::const_iterator it = Abbrev->find(text);
        if (it == Abbrev->end())
            return false;
        type = it->second;
        return true;
    }

    inline bool HasWordWithPoint(const Wtroka& word) const {
        return WordsWithPoint.Get() != NULL &&
               WordsWithPoint->find(word) != WordsWithPoint->end();
    }

    void AddWordsWithPoint(const yset<Wtroka>& list);
    void AddAbbrevs(const TMapAbbrev& abbrev);

private:
    // can only be created as singleton
    DECLARE_SINGLETON_FRIEND(CLangData)

    static void LoadFileAbbrev(TMapAbbrev& abbrev, const Stroka& path, ECharset encoding);
    static void LoadWordsWithPoint(yset<Wtroka>& words, const Stroka& path, ECharset encoding);

private:
    THolder<TMapAbbrev> Abbrev;
    THolder<yset<Wtroka> > WordsWithPoint;
};

inline const CLangData* GetGlobalLangData() {
    return Singleton<CLangData>();
}

#define GlobalLangData (GetGlobalLangData())
