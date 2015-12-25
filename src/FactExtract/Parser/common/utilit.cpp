#include "utilit.h"

#include <util/stream/file.h>
#include <util/charset/unidata.h>

bool RequiresSpace(const Wtroka& w1, const Wtroka& w2)
{
#define SHIFT(i) (ULL(1)<<(i))

    if(w1.length() == 1) {
        if (NUnicode::CharHasType(w1[0],
            SHIFT(Ps_START) | SHIFT(Ps_SINGLE_QUOTE) | SHIFT(Pi_SINGLE_QUOTE) |
            SHIFT(Ps_QUOTE) | SHIFT(Pi_QUOTE)))
          return false;
    }

    if(w2.length() == 1) {
          if (NUnicode::CharHasType(w2[0],
                  SHIFT(Pe_END) | SHIFT(Po_TERMINAL) | SHIFT(Pe_SINGLE_QUOTE) | SHIFT(Pf_SINGLE_QUOTE) |
                  SHIFT(Pe_QUOTE) | SHIFT(Pf_QUOTE)))
                return false;
    }

#undef SHIFT
    return true;
}

void WriteToLogFile(const Stroka& sGrammarFileLog, Stroka& str, bool bRW)
{
    if (sGrammarFileLog.empty())
        return;

    str += '\n';

    THolder<TFile> f;
    if (bRW)
        f.Reset(new TFile(sGrammarFileLog, CreateAlways | WrOnly | Seq));
    else
        f.Reset(new TFile(sGrammarFileLog, OpenAlways | WrOnly | Seq | ForAppend));

    TFileOutput out(*f);
    out.Write(str.c_str());
};

template <typename TStr>
static size_t ReplaceCharTempl(TStr& str, typename TStr::char_type from, typename TStr::char_type to)
{
    size_t count = 0;
    for (size_t i = 0; i < str.size(); ++i)
        if (str[i] == from) {
            for (typename TStr::char_type* a = str.begin() + i; a != str.end(); ++a)
                if (*a == from) {
                    *a = to;
                    ++count;
                }
            break;
        }
    return count;
}

namespace NStr
{

void DecodeUserInput(const TStringBuf& text, Wtroka& res, ECharset encoding, const Stroka& filename, size_t linenum)
{
    const size_t MAX_MSG_TEXT_LEN = 250;
    try {
        CharToWide(text, res, encoding);
    } catch (...) {
        Cerr << "Cannot decode supplied text, invalid encoding (expected " << NameByCharset(encoding) << "):\n\n";
        if (text.size() <= MAX_MSG_TEXT_LEN)
            Cerr << text;
        else
            Cerr << text.SubStr(0, MAX_MSG_TEXT_LEN) << "...";
        Cerr << "\n";
        if (!filename.empty()) {
            Cerr << "\n(File " << filename;
            if (linenum)
                Cerr << ", line " << linenum;
            Cerr << ")";
        }
        Cerr << Endl;
        throw;
    }
}


size_t ReplaceChar(Wtroka& str, wchar16 from, wchar16 to)
{
    return ReplaceCharTempl(str, from, to);
}

size_t ReplaceSubstr(Wtroka& str, const TWtringBuf& from, const TWtringBuf& to)
{
    size_t count = 0;
    size_t pos = str.off(TCharTraits<wchar16>::Find(~str, +str, ~from, +from));
    while (pos != Wtroka::npos) {
        str.replace(pos, +from, ~to, 0, Wtroka::npos, +to);
        ++count;

        size_t next = pos + to.size();
        pos = str.off(TCharTraits<wchar16>::Find(~str + next, +str - next, ~from, +from));
    }
    return count;
}

void ToFirstUpper(Wtroka& str)
{
    if (!str.empty() && ::IsLower(str[0]))
        *(str.begin()) = static_cast<wchar16>(::ToUpper(str[0]));

    // TODO: this does not work properly for surrogate pairs (UTF-16)
}

bool EqualCi(const TWtringBuf& s1, const TWtringBuf& s2) {
    if (s1.size() != s2.size())
        return false;
    for (size_t i = 0; i < s1.size(); ++i)
        if (::ToLower(s1[i]) != ::ToLower(s2[i]))
            return false;
    return true;
}

bool EqualCiRus(const Wtroka& s1, const char* s2) {
    static const CodePage& cp = *CodePageByCharset(CODES_WIN);
    const ui16* w = s1.begin();
    for (; w != s1.end() && *s2 != 0; ++w, ++s2)
        if (::ToLower(*w) != ::ToLower(cp.unicode[static_cast<ui8>(*s2)]))
            return false;
    return w == s1.end() && *s2 == 0;
}

} // namespace NStringOps
