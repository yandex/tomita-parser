// $Id: token_flags.cpp 1047639 2013-07-22 14:15:41Z axc $

#include "token_flags.h"
#include <util/string/split.h>

namespace NLanguageMasks {

TLangMask SameScriptLanguages(TLangMask src)
{
    TLangMask dst;
    for (docLanguage lg = src.FindFirst(); src.End != lg; lg = src.FindNext(lg))
    {
        TScriptMap::const_iterator it = ScriptMap().find(ScriptByLanguage(lg));
        if (ScriptMap().end() != it) {
            dst |= it->second;
            src &= ~it->second; // don't need others using the same script
        }
    }
    return dst;
}

template<typename T>
TLangMask CreateFromListImpl(const Stroka& list, T langGetter) {
    TLangMask result;
    yvector<Stroka> langVector;
    Split(list, ",", langVector);
    for (yvector<Stroka>::const_iterator i = langVector.begin(), e = langVector.end(); i != e; ++i) {
        docLanguage lang = langGetter(~strip(*i));
        if (lang == LANG_MAX)
            ythrow yexception() << "Unknown language: " << *i;
        result.SafeSet(lang);
    }
    return result;
}

TLangMask CreateFromList(const Stroka& list) {
    return CreateFromListImpl(list, LanguageByNameStrict);
}

TLangMask SafeCreateFromList(const Stroka& list) {
    return CreateFromListImpl(list, LanguageByName);
}

Stroka ToString(const TLangMask& langMask) {
    if (langMask.Empty())
        return NameByLanguage(LANG_UNK);
    Stroka result;
    for (docLanguage lang = langMask.FindFirst(); lang != langMask.End; lang = langMask.FindNext(lang)) {
        if (!!result)
            result += ",";
        result += NameByLanguage(lang);
    }
    return result;
}

}
