#include "langdata.h"
#include <util/stream/file.h>
#include <util/string/strip.h>
#include <FactExtract/Parser/common/string_tokenizer.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/common/utilit.h>

#define RUS_ABBREV_FILE "rusabbrev.txt"
#define RUS_EXTENSION_FILE "russextension.txt"

void CLangData::LoadWordsWithPoint(yset<Wtroka>& words, const Stroka& path, ECharset encoding)
{
    if (!PathHelper::Exists(path)) {
        // do not halt, just make it as if the file were empty
        return;
        //ythrow yexception() << "Cannot open " << path << " in CLangData::loadWordsWithPoint(): file not found.";
    }

    TIFStream file(path);
    Stroka str;
    for (size_t linenum = 1; file.ReadLine(str); ++linenum) {
        str = StripString(str);
        if (str.find('.') == Stroka::npos)
            continue;
        Wtroka wstr = NStr::DecodeUserInput(str, encoding, path, linenum);
        wstr.to_lower();
        words.insert(wstr);
    }
}

void CLangData::AddWordsWithPoint(const yset<Wtroka>& list) {
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    if (NULL == WordsWithPoint.Get())
        WordsWithPoint.Reset(new yset<Wtroka>);
    WordsWithPoint.Get()->insert(list.begin(), list.end());
}

void CLangData::AddAbbrevs(const TMapAbbrev& abbrev) {
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    if (NULL == Abbrev.Get())
        Abbrev.Reset(new TMapAbbrev);
    Abbrev.Get()->insert(abbrev.begin(), abbrev.end());
}

void CLangData::LoadFileAbbrev(TMapAbbrev& abbrevs, const Stroka& path, ECharset encoding)
{
    if (!PathHelper::Exists(path)) {
        return;
        //ythrow yexception() << "Cannot open " << path << " in CLangData::loadFileAbbrev(): file not found.";
    }

    Stroka str, strAbr;
    TIFStream file(path);
    for (size_t i = 1; file.ReadLine(str); ++i) {
        TStrokaTokenizer StringTokenizer(str, " ");
        if (!StringTokenizer.NextAsString(strAbr))
            continue;
        unsigned int value;
        if (!StringTokenizer.NextAsNumber(value))
            continue;

        Wtroka abbr = NStr::DecodeUserInput(strAbr, encoding, path, i);
        abbr.to_lower();
        abbrevs[abbr] = (ETypeOfAbbrev)value;
    }
}
