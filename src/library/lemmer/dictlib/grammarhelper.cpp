#include <util/string/vector.h>
#include <util/string/base64.h>
#include <util/stream/str.h>
#include <util/charset/wide.h>

#include "grammarhelper.h"

using NTGrammarProcessing::ch2tg;

template <typename TChr>
static TGrammar GrammarByName(const TStringBufImpl<TChr>& name);

template <>
TGrammar GrammarByName(const TStringBufImpl<char>& name) {
    return TGrammarIndex::GetCode(name);
}

template <>
TGrammar GrammarByName(const TStringBufImpl<wchar16>& name) {
    char buffer[256];
    if (name.size() > 256)
        return gInvalid;
    size_t len = ::WideToChar(~name, +name, buffer);
    return TGrammarIndex::GetCode(TStringBuf(buffer, len));
}


template <typename TChr>
static TGramBitSet ParseFromString(TStringBufImpl<TChr> grammems, const TStringBufImpl<TChr>& delim) {
    TGramBitSet res;

    grammems = ::StripString(grammems);
    while (!grammems.empty()) {
        size_t pos = grammems.find_first_of(delim);
        if (pos == TStringBufImpl<TChr>::npos)
            pos = grammems.size();

        TStringBufImpl<TChr> gram = grammems.Head(pos);
        grammems = ::StripString(grammems.SubStr(pos + 1));

        TGrammar code = GrammarByName<TChr>(gram);
        if (code == gInvalid)
            ythrow TGramBitSet::TException() << "Unknown grammem \"" << gram << "\"";
        else
            res.Set(code);
    }
    return res;
}

TGramBitSet TGramBitSet::FromString(const TStringBuf& s, const char* delim) {
    return ParseFromString<char>(s, delim);
}

TGramBitSet TGramBitSet::FromWtring(const TWtringBuf& s, const char* delim) {

    // use only 256 first chars from @delim
    TStringBuf sep(delim);
    sep.Trunc(256);

    wchar16 buffer[256];
    ::CharToWide(~sep, +sep, buffer);
    return ParseFromString<wchar16>(s, TWtringBuf(buffer, +sep));
}

TGramBitSet TGramBitSet::FromUnsafeString(const TStringBuf& s, const char* delim)
{
    try {
        return TGramBitSet::FromString(s, delim);
    } catch (TException&)
    {
        return TGramBitSet();
    }
}

void TGramBitSet::ToString(TOutputStream& stream, const char* delim /* = " " */, bool latinic /*= false */) const
{
    TGrammar first = FindFirst();
    if (first >= TGramBitSet::End)
        return;

    if (latinic)
        stream.Write(TGrammarIndex::GetLatinName(first));
    else
        stream.Write(TGrammarIndex::GetName(first));

    for (TGrammar g = FindNext(first); g < TGramBitSet::End; g = FindNext(g))
    {
        stream.Write(delim);
        if (latinic)
            stream.Write(TGrammarIndex::GetLatinName(g));
        else
            stream.Write(TGrammarIndex::GetName(g));
    }
}

Stroka TGramBitSet::ToString(const char* delim /* = " " */, bool latinic /*= false */) const
{
    TStringStream s;
    TGramBitSet::ToString(s, delim, latinic);
    return s.Str();
}

Wtroka TGramBitSet::ToWtring(const char* delim /*= " "*/, bool latinic /*= true*/) const {
    return CharToWide(ToString(delim, latinic));
}

TGramBitSet TGramBitSet::FromBytes(const char* bytes)
{
    TGramBitSet res;
    if (bytes != NULL)
        for (; *bytes != 0; ++bytes)
            //res.Set(static_cast<TGrammar>(*bytes));     //non-safe!
            res.Set(ch2tg(*bytes));     //non-safe!
    return res;
}

TGramBitSet TGramBitSet::FromBytes(const char* bytes, size_t len)
{
    TGramBitSet res;
    if (bytes != NULL)
        for (size_t i = 0; i < len; ++i)
            //res.Set(static_cast<TGrammar>(bytes[i]));     //non-safe!
            res.Set(ch2tg(bytes[i]));     //non-safe!
    return res;
}

void TGramBitSet::ToBytes(Stroka& res) const
{
    for (TGrammar g = FindFirst(); g < TGramBitSet::End; g = FindNext(g))
        res += (char)g;
}

TGramBitSet TGramBitSet::FromHexString(const Stroka& s)
{
    Stroka bytes;
    bytes.reserve(s.length()/2);
    for (size_t i = 0; i < s.length(); i += 2)
    {
        ui8 byte = 0;
        for (int b = 0; b < 2; ++b)
        {
            char ch = s[i + b];
            ui8 part = 0;
            if (ch >= '0' && ch <= '9')
                part = ch - '0';
            else if (ch >= 'A' && ch <= 'F')
                part = ch - 'A' + 10;
            else if (ch >= 'a' && ch <= 'f')
                part = ch - 'a' + 10;
            else
                ythrow TException() << "String \"" << s << "\" is not a valid hex.";
            byte = (byte << 4) | part;
        }
        bytes += static_cast<char>(byte);
    }
    return TGramBitSet::FromBytes(bytes);
}

Stroka TGramBitSet::ToHexString() const
{
    Stroka bytes, res;
    ToBytes(bytes);
    res.reserve(2 * bytes.length());
    for (size_t i = 0; i < bytes.length(); ++i)
    {
        ui8 byte_parts[2] = {ui8(static_cast<ui8>(bytes[i]) >> 4),     //most significant 4 bits
                             ui8(static_cast<ui8>(bytes[i]) & 15)};    //least significant 4 bits
        for (int b = 0; b < 2; ++b)
            res += char(((byte_parts[b] < 10) ? '0' : 'A' - 10) + byte_parts[b]);
    }
    return res;
}



namespace NSpike {

void ToGramBitset(const char* gram, TGramBitSet& bitset) {
    if (!gram)
        return;
    for (int i = 0; gram[i]; ++i)
        bitset.Set(ch2tg(gram[i]));
}

void ToGramBitset(const char* stem, const char* flex, TGramBitSet& bitset) {
    ToGramBitset(stem, bitset); ToGramBitset(flex, bitset);
}

void ToGrammarBunch(const char* stem, const char* const flex[], size_t size, TGrammarBunch& bunch) {
    TGramBitSet stemgram;
    ToGramBitset(stem, stemgram);
    if (size == 0) {
        if (stemgram.any())
            bunch.insert(stemgram);
    } else for (size_t i = 0; i < size; ++i) {
        TGramBitSet bits = stemgram;
        ToGramBitset(flex[i], bits);
        if (bits.any())
            bunch.insert(bits);
    }

}

void ToGrammarBunch(const char* stem, TGrammarBunch& bunch) {
    TGramBitSet bits;
    ToGramBitset(stem, bits);
    if (bits.any())
        bunch.insert(bits);
}

TGramBitSet Normalize(const TGramBitSet& gram) {
    TGramBitSet res = gram;
    res.Tr(gMasFem, TGramBitSet(gMasculine, gFeminine), true)
        .Tr(TGramBitSet(gAdjective, gPlural), NSpike::AllMajorGenders)
        //.Tr(gPartitive, gGenitive, true)
        //.Tr(gLocative, gAblative, true)
        //.Tr(gVocative, gNominative, true)
        ;
    return res;
}

void Normalize(TGrammarBunch* bunch, docLanguage lang) {
    switch (lang) {
        case LANG_UNK:
        case LANG_RUS:
        case LANG_UKR:
        case LANG_BLR:
        case LANG_BUL:
            break;
        default:
            return;
    }
    const TGramBitSet mask(gMasFem, gAdjective);
    TGrammarBunch result;
    TGrammarBunch::const_iterator it = bunch->begin();
    for (;it != bunch->end(); ++it) {
        if (it->HasAny(mask)) {
            for (it = bunch->begin(); it != bunch->end(); ++it)
                result.insert(Normalize(*it));
            bunch->swap(result);
            return;
        }
    }
}

} // end of namespace NSpike
