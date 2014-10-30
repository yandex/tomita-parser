#include "abc.h"
#include "alphaux.h"
#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/ylimits.h>
#include <util/charset/unidata.h>
#include <util/generic/strbuf.h>

namespace NLemmer {
/////////////////////////////////// TAlphabet
    class TAlphabet::TComposer {
        typedef yhash<Wtroka, TChar, THash<TWtringBuf>, TEqualTo<TWtringBuf> > TCharMap;
        TCharMap Hash;
        size_t MaxLen;
    public:
        TComposer()
        : MaxLen(0)
        {}

        void Add(const wchar32* s, TChar c) {
            Wtroka buffer;
            for (; *s; ++s) {
                if (*s <= Max<TChar>())
                    buffer.append(static_cast<TChar>(*s));
            }
            TChar* p = buffer.begin();
            Sort(p, p + buffer.length());
            Hash[buffer] = c;
            if (+buffer > MaxLen)
                MaxLen = +buffer;
        }

        TChar Find(const TChar* s, size_t length) const {
            TCharMap::const_iterator i = Hash.find(TWtringBuf(s, length));
            if (i == Hash.end())
                return 0;
            return i->second;
        }

        size_t MaxDecompLen() const {
            return MaxLen;
        }
    };

    TAlphabet::TAlphabet(
            const TChar* alphaRequired,
            const TChar* alphaNormal,
            const TChar* alphaAlien,
            const TChar* diaAccidental,
            const TChar* signs)
        : Composer(new TComposer)
    {
        Create(alphaRequired, alphaNormal, alphaAlien, diaAccidental, signs);
    }

    TAlphabet::TAlphabet()
        : Composer(new TComposer)
    { }

    void TAlphabet::Create(
            const TChar* alphaRequired,
            const TChar* alphaNormal,
            const TChar* alphaAlien,
            const TChar* diaAccidental,
            const TChar* signs)
    {
        memset(Map, 0, sizeof(Map));
        AddAlphabet(alphaNormal, CharAlphaNormal, CharAlphaNormalDec);
        AddAlphabet(alphaRequired, CharAlphaNormal | CharAlphaRequired, CharAlphaNormalDec);
        AddAlphabet(alphaAlien, CharAlphaAlien, CharAlphaAlienDec);

        if (diaAccidental) {
            while (*diaAccidental) {
                YASSERT(IsCombining(*diaAccidental));
                Map[*diaAccidental++] |= CharDia;
            }
        }
        if (signs) {
            while (*signs) {
                YASSERT(!IsCombining(*signs));
                Map[*signs++] |= CharSign;
            }
        }
    }

    TAlphabet::~TAlphabet() {
    }

    void AddAlphaNormal(TAlphabet::TCharClass* map, TChar a, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed) {
        map[a] &= ~clDecomposed;
        map[a] |= cl;
    }

    void AddAlphaDecomposed(TAlphabet::TCharClass* map, TChar a, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed) {
        if (!(map[a] & cl))
            map[a] |= clDecomposed;
    }

    void TAlphabet::AddAlphabet(const TChar* arr, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed) {
        if (!arr)
            return;
        for (; *arr; ++arr) {
            AddAlphaNormal(Map, *arr, cl, clDecomposed);
            const wchar32 *decomp = NUnicode::Decomposition<true>(*arr);
            if (decomp) {
                AddDecomposedChain(decomp, cl, clDecomposed);
                Composer->Add(decomp, *arr);
            }
        }
    }

    void TAlphabet::AddDecomposedChain(const wchar32* chain, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed) {
        for (; *chain; ++chain) {
            if (*chain > Max<TChar>())
                continue;
            TChar c = static_cast<TChar>(*chain);
            if (!IsCombining(c))
                AddAlphaDecomposed(Map, c, cl, clDecomposed);
            else
                Map[c] |= (clDecomposed | CharDia | CharDiaSignificant);
        }
    }
namespace NDetail {
///////////////////////////////////////
/////////////////////////////////// TTr
    struct TTr::TAdvancedMap {
        TChr From;
        const TChr* To;
        bool operator < (const TAdvancedMap& ot) const {
            return From < ot.From;
        }
        bool operator == (const TAdvancedMap& ot) const {
            return From == ot.From;
        }
    };

    TTr::TTr(const TChr *from, const TChr *to, const TChr *kill, const TChr* fromAdv, const TChr** toAdv)
    : AdvancedMapSize(fromAdv ? TCharTraits<TChr>::GetLength(fromAdv) : 0)
    {
        for (size_t i = 0; i < MapSize; ++i)
            Map[i] = (TChr)i;
        Map[NeedAdv] = 0;
        for (; *from && *to; from++, to++)
            Map[(TChr)*from] = *to;
        YASSERT(!*from && !*to);
        if (AdvancedMapSize) {
            AdvancedMap.Reset(new TAdvancedMap[AdvancedMapSize]);
            for (size_t i = 0; i < AdvancedMapSize; ++i, fromAdv++, toAdv++) {
                Map[(TChr)*fromAdv] = NeedAdv;
                TAdvancedMap t = {*fromAdv, *toAdv};
                AdvancedMap[i] = t;
            }
            Sort(AdvancedMap.Get(), AdvancedMap.Get() + AdvancedMapSize);
        }

        if (kill) {
            for (; *kill; ++kill)
                Map[(TChr)*kill] = 0;
        }
    }

    size_t TTr::operator() (const TChr *src, TChr *dst) const {
        TChr *p = dst;
        for (; *src; ++src) {
            *dst = Map[(TChr)*src];
            if (*dst == 0xFFFF) {
                const TChr* t = GetAdvDec(*src);
                while(*t)
                    *(dst++) = *(t++);
            } else if (*dst)
                ++dst;
        }
        *dst = 0;
        return dst - p;
    }

    TTransdRet TTr::operator() (const TChr *src, size_t len, TChr *dst, size_t bufLen) const {
        const TChr *pd = dst;
        const TChr *ps = src;
        bool changed = false;
        for (; len && *src && bufLen; ++src, --len) {
            *dst = Map[(TChr)*src];
            if (*dst != *src)
                changed = true;
            if (*dst == 0xFFFF) {
                const TChr* t = GetAdvDec(*src);
                while(*t && bufLen) {
                    *(dst++) = *(t++);
                    --bufLen;
                }
            } else if (*dst)
                ++dst, --bufLen;
        }
        if (bufLen)
            *dst = 0;
        return TTransdRet(dst - pd, src - ps, !len, changed || len);
    }

    const TTr::TChr* TTr::GetAdvDec(TTr::TChr c) const {
        TAdvancedMap t = {c, 0};
        return LowerBound(AdvancedMap.Get(), AdvancedMap.Get() + AdvancedMapSize, t)->To;
    }

    TTr::~TTr() {
    }

///////////////////////////////////////
/////////////////////////////////// Compose
    TTransdRet ComposeWord(const TAlphabet& alphabet, const TChar* word, size_t length, TChar* buffer, size_t bufSize) {
        size_t resLen = 0;
        size_t i = 0;
        bool valid = true;
        bool changed = false;
        while (i < length && resLen < bufSize) {
            TTransdRet r = ComposeAlpha(alphabet, word + i, length - i, buffer + resLen, bufSize - resLen);
            resLen += r.Length;
            i += r.Processed;
            valid = valid && r.Valid;
            changed = changed || r.Changed;
        }
        return TTransdRet(resLen, i, valid, changed);
    }

    TTransdRet ComposeAlpha(const TAlphabet& alphabet, const TChar* word, size_t length, TChar* buffer, size_t bufSize) {
        YASSERT(word && length);
        YASSERT(buffer && bufSize);

        static const size_t minBufferSize = 64;
        const size_t bufferSize = Max(alphabet.Composer->MaxDecompLen() + 1, minBufferSize);
        TChar* intBuf = static_cast<TChar*>(alloca(bufferSize * sizeof(TChar)));

        size_t resLen = 1;
        size_t j = 1;
        intBuf[0] = word[0];
        for(; j < length && resLen < bufferSize && (alphabet.CharClass(word[j]) & TAlphabet::CharDia); ++j) {
            if (alphabet.CharClass(word[j]) & TAlphabet::CharDiaSignificant)
                intBuf[resLen++] = word[j];
        }
        if (resLen <= alphabet.Composer->MaxDecompLen() || resLen == 1) {
            TChar c = word[0];
            if (resLen > 1) {
                Sort(intBuf, intBuf + resLen);
                c = alphabet.Composer->Find(intBuf, resLen);
            }
            if (c) {
                *buffer = c;
                return TTransdRet(1, j, true, resLen > 1);
            }
        }

        // bad case
        for(; j < length && resLen < bufferSize && IsCombining(word[j]); ++j) {
            if (alphabet.IsSignificant(word[j]))
                intBuf[resLen++] = word[j];
        }

        resLen = NLemmerAux::Compose(intBuf, resLen, buffer, bufSize).Length;
        return TTransdRet(resLen, j, false, true);
    }
} // NDetail
} // NLemmer
