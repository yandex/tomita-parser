#include "agreement.h"

#include <FactExtract/Parser/common/utilit.h>
#include <util/charset/utf.h>
#include <library/pire/pire.h>
#include <library/pire/regexp.h>

typedef NPire::TNonrelocScanner TPireScanner;

class TRegexMatcher::TScanner {
public:
    TPireScanner PireScanner;

    TScanner(NPire::TFsm& fsm)
        : PireScanner(fsm) {
    }
};

TRegexMatcher::TRegexMatcher(const TWtringBuf& pattern) {
    bool beg = (pattern.size() >= 1 && pattern[0] == '^');
    bool end = (pattern.size() >= 1 && pattern.back() == '$');

    TWtringBuf final;

    bool surround = false;
    Wtroka buffer;
    if (beg && end) {
        final = pattern.SubStr(1, pattern.size() - 2);
        surround = false;
    } else if (beg) {
        buffer.reserve(pattern.size() + 2);
        buffer.append(~pattern + 1, +pattern - 1).append('.').append('*');
        final = buffer;
        surround = false;
    } else if (end) {
        buffer.reserve(pattern.size() + 2);
        buffer.append('.').append('*').append(~pattern, +pattern - 1);
        final = buffer;
        surround = false;
    } else {
        final = pattern;
        surround = true;
    }

    NPire::TLexer lexer(final.begin(), final.end());
    lexer.SetEncoding(NPire::NEncodings::Utf8());
    NPire::TFsm fsm = lexer.Parse();
    if (surround)
        fsm.Surround();

    fsm.Determine();
    Impl.Reset(new TScanner(fsm));
}

TRegexMatcher::~TRegexMatcher() {
}

bool TRegexMatcher::IsCompatible(const TWtringBuf& pattern) {
    try {
        NPire::TLexer lexer(pattern.begin(), pattern.end());
        lexer.Parse();
        return true;
    } catch (...) {
        return false;
    }
}

bool TRegexMatcher::MatchesUtf8(const TStringBuf& text) const {
    TPireScanner::State state;
    Impl->PireScanner.Initialize(state);
    NPire::Run(Impl->PireScanner, state, text.data(), text.data() + text.size());
    if (Impl->PireScanner.Final(state))
        return true;
    else
        return false;
}




void CWordFormRegExp::SetRegexUtf8(const Stroka& pattern) {
    YASSERT(IsUtf(pattern));
    RegexPattern = pattern;
    if (pattern.empty())
        Matcher.Reset(NULL);
    else {
        NGzt::TStaticCharCodec<256> encoder;
        Matcher.Reset(new TRegexMatcher(encoder.Utf8ToWide(pattern)));
    }
}

bool CWordFormRegExp::Matches(const TWtringBuf& text) const {
    if (Matcher.Get() != NULL) {
        NGzt::TStaticCharCodec<256> encoder;
        return Matcher->MatchesUtf8(encoder.WideToUtf8(text));
    } else
        return false;
}

void CWordFormRegExp::Save(TOutputStream* buffer) const {
    ::Save(buffer, IsNegative);
    ::Save(buffer, Place);
    ::Save(buffer, RegexPattern);
}

void CWordFormRegExp::Load(TInputStream* buffer) {
    ::Load(buffer, IsNegative);
    ::Load(buffer, Place);

    Stroka pattern;
    ::Load(buffer, pattern);
    if (!pattern.empty())
        SetRegexUtf8(pattern);
}

const Stroka CWordFormRegExp::DebugString(int itemNo) const {
    const char* place;
    switch (Place) {
        case ApplyToFirstWord: place = "wff";
        case ApplyToLastWord: place = "wfl";
        default : place = "wfm";
    }
    const char* neg = IsNegative ? "~" : "";
    return Substitute("$0[$1]$2=/$3/", place, itemNo, neg, NStr::DebugEncode(UTF8ToWide(RegexPattern)));
}
