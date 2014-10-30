#pragma once

#include "comptrie_builder.h"

template <class D, class S>
class TCompactTrieBuilder<bool, D, S> : protected TCompactTrieBuilder<char, D, S> {
    typedef TCompactTrieBuilder<char, D, S> TParent;
    typedef TCompactTrieBuilder<bool, D, S> TSelf;

public:
    typedef typename TParent::TSymbol TSymbol;
    typedef typename TParent::TData TData;
    typedef typename TParent::TPacker TPacker;
    typedef typename TParent::TKey TKey;

public:

    explicit TCompactTrieBuilder(TCompactTrieBuilderFlags flags = CTBF_NONE, TPacker packer = TPacker())
        : TParent(flags, packer) {}

    using TParent::Add;
    using TParent::AddPtr;

    using TParent::Find;

    using TParent::Save;
    using TParent::SaveToFile;

    using TParent::Clear;

    using TParent::GetEntryCount;
    using TParent::GetNodeCount;
};

