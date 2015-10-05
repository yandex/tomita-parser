#pragma once

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/map.h>
#include <util/charset/wide.h>

#include <library/lemmer/alpha/abc.h>

#include <kernel/gazetteer/common/recode.h>


#include <contrib/libs/protobuf/stubs/substitute.h>
using ::google::protobuf::strings::Substitute;

bool RequiresSpace(const Wtroka& w1, const Wtroka& w2);

typedef enum { morphUnknown = 0, morphRussian = 1, morphEnglish = 2, morphGerman = 3 } MorphLanguageEnum;

void WriteToLogFile(const Stroka& sGrammarFileLog, Stroka& str, bool bRW = false);

// some string operations
namespace NStr
{
    // more careful decoding of user-supplied data
    void DecodeUserInput(const TStringBuf& text, Wtroka& res, ECharset encoding, const Stroka& filename = Stroka(), size_t linenum = 0);
    inline Wtroka DecodeUserInput(const TStringBuf& text, ECharset encoding, const Stroka& filename = Stroka(), size_t linenum = 0) {
        Wtroka res;
        DecodeUserInput(text, res, encoding, filename, linenum);
        return res;
    }

    // For aux_dic_kw.cxx parsing: currently only win-1251
    inline Wtroka DecodeAuxDic(const TStringBuf& str) {
        return CharToWide(str, CODES_UTF8);
    }

    // For tomita parsing: tomaparser recodes everything to utf-8
    inline Wtroka DecodeTomita(const TStringBuf& str) {
        return CharToWide(str, CODES_UTF8);
    }

    // Default debug output encoding is win-1251, it nothing else specified
    inline Stroka DebugEncode(const TWtringBuf& str) {
        return WideToChar(str, CODES_UTF8);
    }

    size_t ReplaceChar(Stroka& str, char from, char to);
    size_t ReplaceChar(Wtroka& str, wchar16 from, wchar16 to);

    size_t ReplaceSubstr(Wtroka& str, const TWtringBuf& from, const TWtringBuf& to);

    size_t RemoveChar(Wtroka& str, wchar16 ch);

    // make first letter upper-cased (but not touch the rest ones)
    void ToFirstUpper(Wtroka& str);

    inline bool IsAsciiChar(char c) {
        return static_cast<ui8>(c) <= 0x7F;
    }

    inline bool IsAsciiString(const char* first) {
        for (; *first != 0; ++first)
            if (!IsAsciiChar(*first))
                return false;
        return true;
    }

    inline bool IsEqual(const TWtringBuf& str, const char* ascii) {
        YASSERT(IsAsciiString(ascii));
        const ui16* cur = str.begin();
        for (; cur != str.end() && *ascii != 0; ++cur, ++ascii)
            if (*cur != static_cast<ui8>(*ascii))
                return false;
        return cur == str.end() && *ascii == 0;
    }

    inline void Append(Wtroka& str, const char* ascii) {
        YASSERT(IsAsciiString(ascii));
        for (; *ascii != 0; ++ascii)
            str.append(static_cast<ui8>(*ascii));
    }

    inline void Assign(Wtroka& str, const char* ascii) {
        str.clear();
        NStr::Append(str, ascii);
    }

    // case-insensitive operator==
    inline bool EqualCi(const Stroka& s1, const Stroka& s2) {
        return stroka(s1) == stroka(s2);
    }

    bool EqualCi(const TWtringBuf& s1, const TWtringBuf& s2);

    // case-insensitive comparing of wide string and string literal (treated as YANDEX_CODE)
    bool EqualCiRus(const Wtroka& s1, const char* s2);

    inline bool IsLangAlpha(wchar16 ch, docLanguage lang)
    {
        const NLemmer::TAlphabetWordNormalizer* awn = NLemmer::GetAlphaRules(lang);
        return awn->GetAlphabet()->CharClass(ch) & NLemmer::TAlphabet::CharAlphaNormal;
}

}   // namespace NStr

