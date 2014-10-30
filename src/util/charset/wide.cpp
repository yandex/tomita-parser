#include "wide.h"

namespace {

    //! the constants are not zero-terminated
    const wchar16 LT[] = { '&', 'l', 't', ';' };
    const wchar16 GT[] = { '&', 'g', 't', ';' };
    const wchar16 AMP[] = { '&', 'a', 'm', 'p', ';' };
    const wchar16 BR[] = { '<', 'B', 'R', '>' };
    const wchar16 QUOT[] = { '&', 'q', 'u', 'o', 't', ';' };

    template<bool insertBr>
    inline size_t EscapedLen(wchar16 c) {
        switch (c) {
        case '<':
            return ARRAY_SIZE(LT);
        case '>':
            return ARRAY_SIZE(GT);
        case '&':
            return ARRAY_SIZE(AMP);
        case '\"':
            return ARRAY_SIZE(QUOT);
        default:
            if (insertBr && (c == '\r' || c == '\n'))
                return ARRAY_SIZE(BR);
            else
                return 1;
        }
    }

}

void Collapse(Wtroka& w) {
    size_t len = w.size();

    for (size_t start = 0; start < len; ++start)
    {
        size_t n = 0;
        for (; start + n < len; ++n)
        {
            if (!IsWhitespace(w[start + n]))
                break;
        }

        if (n > 1 || (n == 1 && w[start] != ' ')) {
            w.replace(start, n, 1, ' ');
            len = w.size();
        }
    }
}

size_t Collapse(wchar16* s, size_t n) {
    for (size_t start = 0; start < n; ++start) {
        size_t next = 0; // the next non-whitespace character
        for (; start + next < n; ++next) {
            if (!IsWhitespace(s[start + next]))
                break;
        }

        if (next > 1) {
            s[start] = ' ';
            const size_t i = start + next;
            const size_t nchars = n - i;
            if (nchars)
                TCharTraits<wchar16>::Move(&s[start + 1], &s[i], nchars);
            n -= (next - 1);
        } else if (next == 1 && s[start] != ' ') {
            s[start] = ' ';
        }
    }
    return n;
}

void Strip(Wtroka& w) {
    const wchar16* p = w.c_str();
    const wchar16* pe = p + w.size();

    while (p != pe) {
        if (!IsWhitespace(*p)) {
            if (p != w.c_str()) {
                w.erase(w.c_str(), p);
            }

            pe = w.c_str() - 1;
            p = pe + w.size();
            while (p != pe) {
                if (!IsWhitespace(*p))
                    break;
                --p;
            }

            w.remove(p - pe); // it will not change the string if (p - pe) is not less than size
            return;
        }
        ++p;
    }

    // all characters are spaces
    w.clear();
}

template<bool insertBr>
void EscapeHtmlChars(Wtroka& str) {
    static const Wtroka lt(LT, ARRAY_SIZE(LT));
    static const Wtroka gt(GT, ARRAY_SIZE(GT));
    static const Wtroka amp(AMP, ARRAY_SIZE(AMP));
    static const Wtroka br(BR, ARRAY_SIZE(BR));
    static const Wtroka quot(QUOT, ARRAY_SIZE(QUOT));

    size_t escapedLen = 0;

    const Wtroka& cs = str;

    for (size_t i = 0; i < cs.size(); ++i)
        escapedLen += EscapedLen<insertBr>(cs[i]);

    if (escapedLen == cs.size())
        return;

    Wtroka res;
    res.reserve(escapedLen);

    size_t start = 0;

    for (size_t i = 0; i < cs.size(); ++i) {
        const Wtroka* ent = NULL;
        switch (cs[i]) {
            case '<':
                ent = &lt;
                break;
            case '>':
                ent = &gt;
                break;
            case '&':
                ent = &amp;
                break;
            case '\"':
                ent = &quot;
                break;
            default:
                if (insertBr && (cs[i] == '\r' || cs[i] == '\n')) {
                    ent = &br;
                    break;
                } else
                    continue;
        }

        res.append(cs.begin() + start, cs.begin() + i);
        res.append(ent->begin(), ent->end());
        start = i + 1;
    }

    res.append(cs.begin() + start, cs.end());
    res.swap(str);
}

template void EscapeHtmlChars<false>(Wtroka& str);
template void EscapeHtmlChars<true>(Wtroka& str);

bool CanBeEncoded(TWtringBuf text, ECharset encoding) {
    const size_t LEN = 16;
    const size_t BUFSIZE = LEN * 4;
    char encodeBuf[BUFSIZE];
    wchar16 decodeBuf[BUFSIZE];

    while (!text.empty()) {
        TWtringBuf src = text.NextTokAt(LEN);
        TStringBuf encoded = NDetail::NBaseOps::Recode(src, encodeBuf, encoding);
        TWtringBuf decoded = NDetail::NBaseOps::Recode(encoded, decodeBuf, encoding);
        if (decoded != src)
            return false;
    }

    return true;
}

static const char* SkipUTF8Chars(const char *begin, const char* end, size_t numChars) {
    const unsigned char* uEnd = reinterpret_cast<const unsigned char*>(end);
    while (begin != end && numChars > 0) {
        const unsigned char* uBegin = reinterpret_cast<const unsigned char*>(begin);
        size_t runeLen;
        if (NDetail::GetUTF8CharLen(runeLen, uBegin, uEnd) != RECODE_OK) {
            throw yexception() << "invalid UTF-8 char";
        }
        begin += runeLen;
        YASSERT(begin <= end);
        --numChars;
    }
    return begin;
}

TStringBuf SubstrUTF8(const TStringBuf& str, size_t pos, size_t len) {
    const char* start = SkipUTF8Chars(str.begin(), str.end(), pos);
    const char* end = SkipUTF8Chars(start, str.end(), len);
    return TStringBuf(start, end - start);
}
