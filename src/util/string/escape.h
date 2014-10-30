#pragma once

#include <util/string/traits.h>
#include <util/generic/stroka.h>

template <class TChar>
typename TCharStringTraits<TChar>::TString& EscapeCImpl(const TChar* str, size_t len, typename TCharStringTraits<TChar>::TString&);

template <class TChar>
typename TCharStringTraits<TChar>::TString& UnescapeCImpl(const TChar* str, size_t len, typename TCharStringTraits<TChar>::TString&);

template <typename TChar>
static inline typename TCharStringTraits<TChar>::TString& EscapeC(const TChar* str, size_t len, typename TCharStringTraits<TChar>::TString& s) {
    return EscapeCImpl(str, len, s);
}

template <typename TChar>
static inline typename TCharStringTraits<TChar>::TString EscapeC(const TChar* str, size_t len) {
    typename TCharStringTraits<TChar>::TString s;
    return EscapeC(str, len, s);
}

template <typename TChar>
static inline typename TCharStringTraits<TChar>::TString& UnescapeC(const TChar* str, size_t len, typename TCharStringTraits<TChar>::TString& s) {
    return UnescapeCImpl(str, len, s);
}

template <typename TChar>
static inline typename TCharStringTraits<TChar>::TString UnescapeC(const TChar* str, size_t len) {
    typename TCharStringTraits<TChar>::TString s;
    return UnescapeCImpl(str, len, s);
}

template <typename TChar>
static inline typename TCharStringTraits<TChar>::TString EscapeC(TChar ch) {
    return EscapeC(&ch, 1);
}

Stroka& EscapeC(const Stroka& str, Stroka& res);
Wtroka& EscapeC(const Wtroka& str, Wtroka& res);

// these two need to be methods, because of TStringImpl::Quote implementation
Stroka EscapeC(const Stroka& str);
Wtroka EscapeC(const Wtroka& str);

Stroka& UnescapeC(const Stroka& str, Stroka& res);
Wtroka& UnescapeC(const Wtroka& str, Wtroka& res);

Stroka UnescapeC(const Stroka& str);
Wtroka UnescapeC(const Wtroka& wtr);
