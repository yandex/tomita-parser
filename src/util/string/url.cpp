#include "url.h"
#include "cast.h"
#include "util.h"

#include <util/system/maxlen.h>
#include <util/system/defaults.h>
#include <util/memory/tempbuf.h>
#include <util/generic/ptr.h>
#include <util/charset/recyr.hh>
#include <util/generic/hash_set.h>
#include <util/generic/yexception.h>
#include <util/string/cstriter.h>

namespace {

    const char szHexDigits[] = "0123456789ABCDEF";

    template <class I1, class I2, class I3, class I4>
    inline I1 EncodeRFC1738Impl(I1 f, I2 l, I3 b, I4 e) throw () {
        while (b != e && f != l) {
            const unsigned char c = *b;

            if (c == '+') {
                *f++ = '%';
                if (f == l) {
                    return f;
                }
                *f++ = '2';
                if (f == l) {
                    return f;
                }
                *f++ = '0';
            } else if (c <= 0x20 || c >= 0x80) {
                *f++ = '%';
                if (f == l) {
                    return f;
                }
                *f++ = szHexDigits[c >> 4];
                if (f == l) {
                    return f;
                }
                *f++ = szHexDigits[c & 15];
            } else {
                *f++ = c;
            }

            ++b;
        }

        return f;
    }

    template <typename TChar>
    inline bool UrlEncodeRFC1738(TChar* buf, size_t len, const char* url) throw () {
        TChar* const ret = EncodeRFC1738Impl(buf, buf + len,url, TCStringEndIterator());

        if (ret == buf + len) {
            return false;
        } else {
            if (ret[-1] == '/') {
                ret[-1] = 0;
            } else {
                *ret = 0;
            }

            return true;
        }
    }

}

char* EncodeRFC1738(char* buf, size_t len, const char* url, size_t urllen) throw () {
    char* ret = EncodeRFC1738Impl(buf, buf + len, url, url + urllen);

    if (ret != buf + len) {
        *ret++ = 0;
    }

    return ret;
}

bool NormalizeUrl(char *pszRes, size_t nResBufSize, const char *pszUrl, size_t pszUrlSize) {
    YASSERT(nResBufSize);
    pszUrlSize = pszUrlSize == Stroka::npos? strlen(pszUrl) : pszUrlSize;
    size_t nBufSize = pszUrlSize * 3 + 10;
    TTempBuf buffer(nBufSize);
    char* const szUrlUtf = buffer.Data();
    size_t nRead, nWrite;
    Recode(CODES_WIN, CODES_UTF8, pszUrl, szUrlUtf, pszUrlSize, nBufSize, nRead, nWrite);
    szUrlUtf[nWrite] = 0;
    return UrlEncodeRFC1738(pszRes, nResBufSize, szUrlUtf);
}

Stroka NormalizeUrl(const TStringBuf& url) {
    size_t len = +url + 1;
    while (true) {
       TTempBuf buffer(len);
       if (NormalizeUrl(buffer.Data(), len, ~url, +url))
           return buffer.Data();
       len <<= 1;
    }
    ythrow yexception() << "impossible";
}

Wtroka NormalizeUrl(const TWtringBuf& url) {
    const size_t buflen = url.size() * sizeof(wchar16);
    TTempBuf buffer(buflen + 1);
    char* const data = buffer.Data();
    size_t read = 0, written = 0;
    RecodeFromUnicode(CODES_UTF8, url.c_str(), data, url.size(), buflen, read, written);
    data[written] = 0;

    size_t len = written + 1;
    while (true) {
       TArrayHolder<wchar16> p(new wchar16[len]);
       if (UrlEncodeRFC1738(p.Get(), len, data))
           return p.Get();
       len <<= 1;
    }
    ythrow yexception() << "impossible";
}

Stroka StrongNormalizeUrl(const TStringBuf& url) {
    Stroka result(~url, +url);
    result.to_lower();
    if (result.has_prefix("http://"))
        result = result.substr(7);
    if (result.has_prefix("www."))
        result = result.substr(4);
    TTempBuf normalizedUrl(FULLURL_MAX + 7);
    if (!::NormalizeUrl(normalizedUrl.Data(), FULLURL_MAX, ~result))
        return result;
    else
        return normalizedUrl.Data();
}

namespace {

    struct TUncheckedSize {
        bool Has(size_t) const {
            return true;
        }
    };

    struct TKnownSize {
        size_t MySize;
        TKnownSize(size_t sz)
            : MySize(sz)
        {}
        bool Has(size_t sz) const {
            return sz <= MySize;
        }
    };

    template <typename TChar1, typename TChar2>
    int Compare1Case2(const TChar1* s1, const TChar2* s2, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            if (TCharTraits<TChar1>::ToLower(s1[i]) != s2[i])
                return TCharTraits<TChar1>::ToLower(s1[i]) < s2[i] ? -1 : 1;
        }
        return 0;
    }

    template <typename TChar, typename TTraits, typename TBounds>
    inline size_t GetHttpPrefixSizeImpl(const TChar* url, const TBounds &urlSize, bool ignorehttps) {
        const TChar httpPrefix[] = { 'h', 't', 't', 'p', ':', '/', '/', 0 };
        const TChar httpsPrefix[] = { 'h', 't', 't', 'p', 's', ':', '/', '/', 0 };
        if (urlSize.Has(7) && Compare1Case2(url, httpPrefix, 7) == 0)
            return 7;
        if (!ignorehttps && urlSize.Has(8) && Compare1Case2(url, httpsPrefix, 8) == 0)
            return 8;
        return 0;
    }

    template <typename T>
    inline T CutHttpPrefixImpl(const T& url, bool ignorehttps) {
        typedef typename T::char_type TChar;
        typedef typename T::traits_type TTraits;
        size_t prefixSize = GetHttpPrefixSizeImpl<TChar,TTraits>(url.c_str(), TKnownSize(url.size()), ignorehttps);
        if (prefixSize)
            return url.substr(prefixSize);
        return url;
    }

}

size_t GetHttpPrefixSize(const char* url, bool ignorehttps) {
    return GetHttpPrefixSizeImpl<char,TCharTraits<char> >(url, TUncheckedSize(), ignorehttps);
}

size_t GetHttpPrefixSize(const wchar16* url, bool ignorehttps) {
    return GetHttpPrefixSizeImpl<wchar16, TCharTraits<wchar16> >(url, TUncheckedSize(), ignorehttps);
}

size_t GetHttpPrefixSize(const TStringBuf& url, bool ignorehttps) {
    return GetHttpPrefixSizeImpl<char, TCharTraits<char> >(url.c_str(), TKnownSize(url.size()), ignorehttps);
}

size_t GetHttpPrefixSize(const TWtringBuf& url, bool ignorehttps) {
    return GetHttpPrefixSizeImpl<wchar16, TCharTraits<wchar16> >(url.c_str(), TKnownSize(url.size()), ignorehttps);
}

TStringBuf CutHttpPrefix(const TStringBuf& url, bool ignorehttps) {
    return CutHttpPrefixImpl(url, ignorehttps);
}

TWtringBuf CutHttpPrefix(const TWtringBuf& url, bool ignorehttps) {
    return CutHttpPrefixImpl(url, ignorehttps);
}

size_t GetSchemePrefixSize(const TStringBuf& url) {
    static str_spn delim("!-/:-@[-`{|}", true);
    const char *n = delim.brk(~url, url.end());
    if (n + 2 >= url.end() || *n != ':' || n[1] != '/' || n[2] != '/')
        return 0;
    return n + 3 - url.begin();
}

TStringBuf GetSchemePrefix(const TStringBuf& url) {
    return url.Head(GetSchemePrefixSize(url));
}

TStringBuf CutSchemePrefix(const TStringBuf& url) {
    return url.Tail(GetSchemePrefixSize(url));
}

template<bool KeepPort>
static inline TStringBuf GetHostAndPortImpl(const TStringBuf& url) {
    static str_spn hstend(KeepPort ? "/;?#" : "/:;?#");
    const char *n = hstend.brk(~url, url.end());
    if (n != url.end())
        return url.substr(0, n - ~url);
    else
        return url;
}

TStringBuf GetHost(const TStringBuf& url) {
    return GetHostAndPortImpl<false>(url);
}

TStringBuf GetHostAndPort(const TStringBuf& url) {
    return GetHostAndPortImpl<true>(url);
}

TStringBuf GetSchemeHostAndPort(const TStringBuf& url, bool trimHttp, bool trimDefaultPort)
{
    const size_t schemeSize = GetSchemePrefixSize(url);
    const TStringBuf scheme = url.Head(schemeSize);

    const bool isHttp = (schemeSize == 0 || scheme == STRINGBUF("http://"));

    TStringBuf hostAndPort = GetHostAndPort(url.Tail(schemeSize));

    if (trimDefaultPort) {
        const size_t pos = hostAndPort.find(':');
        if (pos != TStringBuf::npos) {
            const bool isHttps = (scheme == STRINGBUF("https://"));

            const TStringBuf port = hostAndPort.Tail(pos + 1);
            if ((isHttp && port == STRINGBUF("80"))
                || (isHttps && port == STRINGBUF("443")))
            {
                // trimming default port
                hostAndPort = hostAndPort.Head(pos);
            }
        }
    }

    if (isHttp && trimHttp) {
        return hostAndPort;
    } else {
        return TStringBuf(scheme.begin(), hostAndPort.end());
    }
}

TStringBuf GetOnlyHost(const TStringBuf& url) {
    return GetHost(CutSchemePrefix(url));
}

TStringBuf GetPathAndQuery(const TStringBuf& url) {
    const size_t off = url.find('/', GetHttpPrefixSize(url));
    if (TStringBuf::npos == off) {
        return "/";
    }
    const size_t end = url.find('#', 1 + off);
    return TStringBuf::npos == end
        ? url.SubStr(off)
        : url.SubStr(off, end - off);
}

// this strange creature returns 2nd level domain, possibly with port
TStringBuf GetDomain(const TStringBuf &host) {
    const char *c = !host? ~host : host.end() - 1;
    for (bool wasPoint = false; c != ~host; --c) {
        if (*c == '.') {
            if (wasPoint) {
                c++;
                break;
            }
            wasPoint = true;
        }
    }
    return TStringBuf(c, host.end());
}

TStringBuf GetParentDomain(const TStringBuf& host, size_t level) {
    size_t pos = host.size();
    for (size_t i = 0; i < level; ++i) {
        pos = host.rfind('.', pos);
        if (pos == Stroka::npos)
            return host;
    }
    return host.SubStr(pos + 1);
}

TStringBuf GetZone(const TStringBuf& host) {
    return GetParentDomain(host, 1);
}

TStringBuf CutWWWPrefix(const TStringBuf& url) {
    if (+url >= 4 && url[3] == '.' && !strnicmp(~url, "www", 3))
        return url.substr(4);
    return url;
}

static inline bool IsSchemeChar(char c) throw () {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool HasPrefix(const Stroka& url) throw () {
    size_t pos = url.find("://");

    if (pos == Stroka::npos) {
        return false;
    }

    for (size_t i = 0; i < pos; ++i) {
        if (!IsSchemeChar(url[i])) {
            return false;
        }
    }

    return true;
}

Stroka AddSchemePrefix(const Stroka& url, Stroka scheme) {
    if (HasPrefix(url)) {
        return url;
    }

    scheme.append("://");
    scheme.append(url);

    return scheme;
}

#define X(c) (c >= 'A' ? ((c & 0xdf) - 'A') + 10 : (c - '0'))

static inline int x2c(unsigned char* x) {
    if (!isxdigit(x[0]) || !isxdigit(x[1]))
        return -1;
    return X(x[0]) * 16 + X(x[1]);
}

#undef X

static inline int Unescape(char *str) {
    char *to, *from;
    int dlen = 0;
    if ((str = strchr(str, '%')) == 0)
        return dlen;
    for (to = str, from = str; *from; from++, to++) {
        if ((*to = *from) == '%') {
            int c = x2c((unsigned char *)from+1);
            *to = char((c > 0) ? c : '0');
            from += 2;
            dlen += 2;
        }
    }
    *to = 0;  /* terminate it at the new length */
    return dlen;
}

size_t NormalizeUrlName(char* dest, const TStringBuf& source, size_t dest_size) {
    if (source.Empty() || source[0] == '?')
        return strlcpy(dest, "/", dest_size);
    size_t len = Min(dest_size - 1, source.length());
    memcpy(dest, source.data(), len);
    dest[len] = 0;
    len -= Unescape(dest);
    strlwr(dest);
    return len;
}

size_t NormalizeHostName(char* dest, const TStringBuf& source, size_t dest_size, ui16 defport) {
    size_t len = Min(dest_size - 1, source.length());
    memcpy(dest, source.data(), len);
    dest[len] = 0;
    char buf[8] = ":";
    size_t buflen = 1 + ToString(defport, buf + 1, sizeof(buf) - 2);
    buf[buflen] = '\0';
    char *ptr = strstr(dest, buf);
    if (ptr && ptr[buflen] == 0) {
        len -= buflen;
        *ptr = 0;
    }
    strlwr(dest);
    return len;
}

// actual list can be found at http://data.iana.org/TLD/tlds-alpha-by-domain.txt
static const char* TopLevelDomains[] = {
    "ac", "academy", "actor", "ad", "ae", "aero", "af", "ag", "agency", "ai", "al", "am", "an", "ao", "aq", "ar", "archi", "arpa", "as", "asia", "at", "au", "aw", "ax", "axa", "az",
    "ba", "bar", "bargains", "bb", "bd", "be", "berlin", "best", "bf", "bg", "bh", "bi", "bid", "bike", "biz", "bj", "black", "blue", "bm", "bn", "bo", "boutique", "br", "bs", "bt", "build", "builders", "buzz", "bv", "bw", "by", "bz",
    "ca", "cab", "camera", "camp", "cards", "careers", "cat", "catering", "cc", "cd", "center", "ceo", "cf", "cg", "ch", "cheap", "christmas", "ci", "ck", "cl", "cleaning", "clothing", "club", "cm", "cn", "co", "codes", "coffee", "cologne", "com", "community", "company", "computer", "condos", "construction", "consulting", "contractors", "cooking", "cool", "coop", "country", "cr", "cruises", "cu", "cv", "cw", "cx", "cy", "cz",
    "dance", "dating", "de", "democrat", "diamonds", "directory", "dj", "dk", "dm", "dnp", "do", "domains", "dz",
    "ec", "edu", "education", "ee", "eg", "email", "enterprises", "equipment", "er", "es", "estate", "et", "eu", "events", "expert", "exposed",
    "farm", "fi", "fish", "fishing", "fj", "fk", "flights", "florist", "fm", "fo", "foundation", "fr", "futbol",
    "ga", "gallery", "gb", "gd", "ge", "gf", "gg", "gh", "gi", "gift", "gl", "glass", "gm", "gn", "gop", "gov", "gp", "gq", "gr", "graphics", "gs", "gt", "gu", "guitars", "guru", "gw", "gy",
    "haus", "hk", "hm", "hn", "holdings", "holiday", "horse", "house", "hr", "ht", "hu",
    "id", "ie", "il", "im", "immobilien", "in", "industries", "info", "ink", "institute", "int", "international", "io", "iq", "ir", "is", "it",
    "je", "jetzt", "jm", "jo", "jobs", "jp",
    "kaufen", "ke", "kg", "kh", "ki", "kim", "kitchen", "kiwi", "km", "kn", "koeln", "kp", "kr", "kred", "kw", "ky", "kz",
    "la", "land", "lb", "lc", "li", "lighting", "limo", "link", "lk", "london", "lr", "ls", "lt", "lu", "luxury", "lv", "ly",
    "ma", "maison", "management", "mango", "marketing", "mc", "md", "me", "meet", "menu", "mg", "mh", "miami", "mil", "mk", "ml", "mm", "mn", "mo", "mobi", "moda", "moe", "monash", "mp", "mq", "mr", "ms", "mt", "mu", "museum", "mv", "mw", "mx", "my", "mz",
    "na", "nagoya", "name", "nc", "ne", "net", "neustar", "nf", "ng", "ni", "ninja", "nl", "no", "np", "nr", "nu", "nyc", "nz",
    "okinawa", "om", "onl", "org",
    "pa", "partners", "parts", "pe", "pf", "pg", "ph", "photo", "photography", "photos", "pics", "pink", "pk", "pl", "plumbing", "pm", "pn", "post", "pr", "pro", "productions", "properties", "ps", "pt", "pub", "pw", "py",
    "qa", "qpon",
    "re", "recipes", "red", "ren", "rentals", "repair", "report", "rest", "reviews", "rich", "ro", "rodeo", "rs", "ru", "ruhr", "rw", "ryukyu",
    "sa", "saarland", "sb", "sc", "sd", "se", "sexy", "sg", "sh", "shiksha", "shoes", "si", "singles", "sj", "sk", "sl", "sm", "sn", "so", "social", "sohu", "solar", "solutions", "sr", "st", "su", "supplies", "supply", "support", "sv", "sx", "sy", "systems", "sz",
    "tattoo", "tc", "td", "technology", "tel", "tf", "tg", "th", "tienda", "tips", "tj", "tk", "tl", "tm", "tn", "to", "today", "tokyo", "tools", "tp", "tr", "trade", "training", "travel", "tt", "tv", "tw", "tz",
    "ua", "ug", "uk", "uno", "us", "uy", "uz",
    "va", "vacations", "vc", "ve", "vegas", "ventures", "vg", "vi", "viajes", "villas", "vision", "vn", "vodka", "vote", "voting", "voto", "voyage", "vu",
    "wang", "watch", "webcam", "wed", "wf", "wien", "wiki", "works", "ws",
    "xn--3bst00m", "xn--3ds443g", "xn--3e0b707e", "xn--45brj9c", "xn--55qw42g", "xn--55qx5d", "xn--6frz82g", "xn--6qq986b3xl", "xn--80ao21a", "xn--80asehdb", "xn--80aswg", "xn--90a3ac", "xn--c1avg", "xn--cg4bki", "xn--clchc0ea0b2g2a9gcd", "xn--czru2d", "xn--d1acj3b", "xn--fiq228c5hs", "xn--fiq64b", "xn--fiqs8s", "xn--fiqz9s", "xn--fpcrj9c3d", "xn--fzc2c9e2c", "xn--gecrj9c", "xn--h2brj9c", "xn--i1b6b1a6a2e", "xn--io0a7i", "xn--j1amh", "xn--j6w193g", "xn--kprw13d", "xn--kpry57d", "xn--l1acc", "xn--lgbbat1ad8j", "xn--mgb9awbf", "xn--mgba3a4f16a", "xn--mgbaam7a8h", "xn--mgbab2bd", "xn--mgbayh7gpa", "xn--mgbbh1a71e", "xn--mgbc0a9azcg", "xn--mgberp4a5d4ar", "xn--mgbx4cd0ab", "xn--ngbc5azd", "xn--nqv7f", "xn--nqv7fs00ema", "xn--o3cw4h", "xn--ogbpf8fl", "xn--p1ai", "xn--pgbs0dh", "xn--q9jyb4c", "xn--rhqv96g", "xn--s9brj9c", "xn--unup4y", "xn--wgbh1c", "xn--wgbl6a", "xn--xkc2al3hye2a", "xn--xkc2dl3a5ee0h", "xn--yfro4i67o", "xn--ygbi2ammx", "xn--zfr164b",
    "xxx", "xyz",
    "ye", "yokohama", "yt",
    "za", "zm", "zone", "zw",
    0
};

const char** GetTlds() {
    return TopLevelDomains;
}

struct TTLDHash: public yhash_set<TStringBuf, TCIHash<TStringBuf>, TCIEqualTo<TStringBuf> > {
    TTLDHash()
    {
        for (const char** tld = GetTlds(); *tld; ++tld) {
            insert(*tld);
        }
    }
};

bool IsTld(TStringBuf s) {
    const TTLDHash& hash = Default<TTLDHash>();
    return hash.find(s) != hash.end();
}

bool InTld(TStringBuf s) {
    size_t p = s.rfind('.');
    return p != TStringBuf::npos && IsTld(s.Skip(p + 1));
}
