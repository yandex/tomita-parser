#pragma once

#include "comptrie_trie.h"

#include <util/draft/bitutils.h>

template <class D, class S>
class TCompactTrie<bool, D, S> : protected TCompactTrie<char, D, S> {
    typedef TCompactTrie<char, D, S> TParent;
    typedef TCompactTrie<bool, D, S> TSelf;

    using TParent::DataHolder;
    using TParent::EmptyValue;
    using TParent::Packer;

public:

    typedef typename TParent::TSymbol TSymbol;
    typedef typename TParent::TKey TKey;
    typedef typename TParent::TData TData;
    typedef typename TParent::TValueType TValueType;
    typedef typename TParent::TPacker TPacker;

    typedef TCompactTrieBuilder<bool, D, S> TBuilder;

public:
    TCompactTrie()
        : TParent() {}

    TCompactTrie(const char* d, size_t len, TPacker packer = TPacker())
        : TParent(d, len, packer) {}

    TCompactTrie(const TParent& t)
        : TParent(t) {}

    explicit TCompactTrie(const TBlob& data, TPacker packer = TPacker())
        : TParent(data, packer) {}

    using TParent::Init;
    using TParent::IsInitialized;
    using TParent::IsEmpty;

    using TParent::Find;
    using TParent::FindPhrases;

    using TParent::Get;

    template <bool paddingbit>
    bool FindLongestPrefix(const TKey& key, size_t* prefixLen, TData* value = NULL) const {
        return FindLongestPrefix<paddingbit>(~key, +key, prefixLen, value);
    }

    template <bool paddingbit>
    bool FindLongestPrefix(const char* key, size_t keylen, size_t* prefixLen, TData* value) const {
        const char* valuepos = NULL;
        size_t tempPrefixLen = 0;
        bool found = LookupLongestPrefix<paddingbit>(key, keylen, tempPrefixLen, valuepos);
        if (prefixLen)
            *prefixLen = tempPrefixLen;

        if (found && value)
            Packer.UnpackLeaf(valuepos, *value);
        return found;
    }

public:
    typedef typename TParent::TConstIterator TConstIterator;

    using TParent::Begin;
    using TParent::End;
    using TParent::UpperBound;

    using TParent::Size;

public:
    using TParent::Print;

protected:
    explicit TCompactTrie(const char* emptyValue)
        : TParent(emptyValue) {}

    TCompactTrie(const TBlob& data, const char* emptyValue, TPacker packer = TPacker())
        : TParent(data, emptyValue, packer) {}

    template <bool paddingbit>
    bool LookupLongestPrefix(const char* key, size_t keylen, size_t& prefixlen, const char*& valuepos) const {
        using namespace NCompactTrie;

        const char* datapos = static_cast<const char*>(DataHolder.Data());
        size_t len = DataHolder.Length();

        bool found = false;

        if (EmptyValue) {
            valuepos = EmptyValue;
            found = true;
        }

        if (!key || !keylen || !len)
            return found;

        const char* const dataend = datapos + len;
        const char* olddatapos = datapos;
        char flags = MT_NEXT;

        const char* keyend = key + keylen;

        while (key != keyend) {
            char label = *(key++);
            olddatapos = datapos;
            flags = LeapByte(datapos, dataend, label);

            if (!datapos) {
                --key;
                break;
            }

            YASSERT(datapos <= dataend);

            if ((flags & MT_FINAL)) {
                prefixlen = keylen - (keyend - key);
                valuepos = datapos;
                found = true;

                if (key == keyend)
                    break;

                datapos += Packer.SkipLeaf(datapos); // skip intermediate leaf nodes
            }

            if (!(flags & MT_NEXT)) {
                --key;
                break;
            }
        }

        if (key != keyend) {
            bool first = keylen == (size_t)(keyend - key);
            // trying all bit suffixes starting longest to shortest
            for (ui32 i = 1; i < 8; ++i) {
                datapos = olddatapos;
                char label = (*key & NBitUtils::Mask(8 - i)) | (paddingbit ? NBitUtils::Mask(i, 8 - i) : 0);

                if (!label && first)
                    break;

                flags = LeapByte(datapos, dataend, label);

                if ((flags & MT_FINAL)) {
                    ++key;
                    prefixlen = keylen - (keyend - key);
                    valuepos = datapos;
                    return true;
                }
            }
        }

        return found;
    }
};

