#pragma once

#include <util/system/defaults.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

template <typename TChar>
struct TCharStringTraits;

template <>
struct TCharStringTraits<char> {
    typedef Stroka TString;
    typedef TStringBuf TBuffer;
};

template <>
struct TCharStringTraits<wchar16> {
    typedef Wtroka TString;
    typedef TWtringBuf TBuffer;
};
