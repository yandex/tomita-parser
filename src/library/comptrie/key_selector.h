#pragma once

#include <util/generic/vector.h>
#include <util/string/traits.h>

template<class T>
struct TCompactTrieKeySelector {
    typedef yvector<T> TKey;
    typedef yvector<T> TKeyBuf;
};

template<class TChar>
struct TCompactTrieCharKeySelector {
    typedef typename TCharStringTraits<TChar>::TString TKey;
    typedef typename TCharStringTraits<TChar>::TBuffer TKeyBuf;
};

template<>
struct TCompactTrieKeySelector<char> : public TCompactTrieCharKeySelector<char>{
};

template<>
struct TCompactTrieKeySelector<wchar16> : public TCompactTrieCharKeySelector<wchar16>{
};

