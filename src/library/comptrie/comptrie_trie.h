#pragma once

#include "comptrie_impl.h"
#include "node.h"
#include "leaf_skipper.h"
#include "key_selector.h"

template <class T, class D, class S>
class TCompactTrieBuilder;

// in case of <char> specialization cannot distinguish between "" and "\0" keys
template <class T = char, class D = ui64, class S = TCompactTriePacker<D> >
class TCompactTrie {
public:
    typedef T TSymbol;
    typedef D TData;
    typedef S TPacker;

    typedef typename TCompactTrieKeySelector<TSymbol>::TKey TKey;
    typedef typename TCompactTrieKeySelector<TSymbol>::TKeyBuf TKeyBuf;

    typedef TPair<TKey, TData> TValueType;
    typedef TPair<size_t, TData> TPhraseMatch;
    typedef yvector<TPhraseMatch> TPhraseMatchVector;

    typedef TCompactTrieBuilder<T, D, S> TBuilder;

protected:
    TBlob DataHolder;
    const char* EmptyValue;
    TPacker Packer;

public:
    TCompactTrie();
    TCompactTrie(const char* d, size_t len, TPacker packer = TPacker());
    explicit TCompactTrie(const TBlob& data, TPacker packer = TPacker());

    void Init(const char* d, size_t len, TPacker packer = TPacker());
    void Init(const TBlob& data, TPacker packer = TPacker());

    bool IsInitialized() const;
    bool IsEmpty() const;

    bool Find(const TSymbol *key, size_t keylen, TData* value = NULL) const;
    bool Find(const TKeyBuf& key, TData* value = NULL) const {
        return Find(~key, +key, value);
    }

    TData Get(const TSymbol* key, size_t keylen) const {
        TData value;
        if (!Find(key, keylen, &value))
            ythrow yexception() << "key " << TKey(key, keylen).Quote() << " not found in trie";
        return value;
    }
    TData Get(const TKeyBuf& key) const {
        return Get(~key, +key);
    }
    TData GetDefault(const TKeyBuf& key, const TData& def) const {
        TData value;
        if (!Find(~key, +key, &value))
            return def;
        else
            return value;
    }

    const TBlob& Data() const {
        return DataHolder;
    };

    TPacker GetPacker() const {
        return Packer;
    }

    void FindPhrases(const TSymbol* key, size_t keylen, TPhraseMatchVector& matches, TSymbol separator = TSymbol(' ')) const;
    void FindPhrases(const TKeyBuf& key, TPhraseMatchVector& matches, TSymbol separator = TSymbol(' ')) const {
        return FindPhrases(~key, +key, matches, separator);
    }
    bool FindLongestPrefix(const TSymbol *key, size_t keylen, size_t* prefixLen, TData* value = NULL, bool* hasNext = NULL) const;
    bool FindLongestPrefix(const TKeyBuf& key, size_t* prefixLen, TData* value = NULL, bool* hasNext = NULL) const {
        return FindLongestPrefix(~key, +key, prefixLen, value, hasNext);
    }

    // Return trie, containing all tails for the given key
    inline TCompactTrie<T, D, S> FindTails(const TSymbol* key, size_t keylen) const;
    TCompactTrie<T, D, S> FindTails(const TKeyBuf& key) const {
        return FindTails(~key, +key);
    }
    bool FindTails(const TSymbol* key, size_t keylen, TCompactTrie<T, D, S>& res) const;
    bool FindTails(const TKeyBuf& key, TCompactTrie<T, D, S>& res) const {
        return FindTails(~key, +key, res);
    }

    // same as FindTails(&key, 1), a bit faster
    // return false, if no arc with @label exists
    inline bool FindTails(TSymbol label, TCompactTrie<T, D, S>& res) const;

    // Iterator for incremental searching.
    // All Advance() methods shift the iterator using specifed key/char.
    // The subsequent Advance() call starts searching from the previous state.
    // The Advance() returns 'true' if specified key part exists in the trie and
    // returns 'false' for unsuccessful search. In case of 'false' result
    // all subsequent calls also will return 'false'.
    // If current iterator state is final then GetValue() method returns 'true' and
    // associated value.
    class TSearchIterator {
    public:
        inline bool Advance(TSymbol label) {
            if (DataPos == NULL || DataPos >= DataEnd) {
                return false;
            }
            return NCompactTrie::Advance(DataPos, DataEnd, ValuePos, label, Trie->Packer);
        }
        inline bool Advance(const TKeyBuf& key) {
            return Advance(~key, +key);
        }
        bool Advance(const TSymbol* key, size_t keylen);
        bool GetValue(TData* value = NULL) const;

    private:
        TSearchIterator(const TCompactTrie<T, D, S>& trie)
            : Trie(&trie)
            , DataPos(trie.DataHolder.AsCharPtr())
            , DataEnd(DataPos + trie.DataHolder.Length())
            , ValuePos(NULL)
        {
        }

    private:
        friend class TCompactTrie;

        const TCompactTrie<T, D, S>* Trie;
        const char* DataPos;
        const char* DataEnd;
        const char* ValuePos;
    };

    inline TSearchIterator GetSearchIterator() const {
        return TSearchIterator(*this);
    }

    class TConstIterator {
    private:
        friend class TCompactTrie;
        TConstIterator(const char* data, size_t len, const char* emptyValue, bool atend, TPacker packer); // only usable from Begin() and End() methods
        TConstIterator(const char* data, size_t len, const char* emptyValue, const TKeyBuf& key, TPacker packer); // only usable from UpperBound() method

    public:
        TConstIterator() { }
        bool IsEmpty() const { return !Impl; } // Almost no other method can be called.

        bool operator == (const TConstIterator& other) const;
        bool operator != (const TConstIterator& other) const;
        TConstIterator& operator++();
        TConstIterator operator++(int /*unused*/);
        TConstIterator& operator--();
        TConstIterator operator--(int /*unused*/);
        TValueType operator*();

        TKey GetKey() const;
        size_t GetKeySize() const;
        TData GetValue() const;
        void GetValue(TData& to) const;
        const char* GetValuePtr() const;

    private:
        class TImpl;
        TCopyPtr<TImpl> Impl;
    };

    TConstIterator Begin() const;
    TConstIterator End() const;

    // Returns an iterator pointing to the smallest key in the trie >= the argument.
    // TODO: misleading name. Should be called LowerBound for consistency with stl.
    // No. It is the STL that has a misleading name.
    // LowerBound of X cannot be greater than X.
    TConstIterator UpperBound(const TKeyBuf& key) const;

    void Print(TOutputStream& os);

    size_t Size() const;

    // Iterates over all prefixes of the given key in the trie.
    class TPrefixIterator {
    private:
        friend class TCompactTrie;
        const TCompactTrie<T,D,S>& Trie;
        const TSymbol* key;
        size_t keylen;
        const TSymbol* keyend;
        size_t prefixLen;
        const char* valuepos;
        const char* datapos;
        const char* dataend;
        TPacker Packer;
        const char* EmptyValue;
        bool result;

        bool Next();

        TPrefixIterator(const TCompactTrie<T,D,S>& trie, const TSymbol* key, size_t keylen)
            : Trie(trie)
            , key(key)
            , keylen(keylen)
            , keyend(key + keylen)
            , prefixLen(0)
            , valuepos(NULL)
            , datapos(trie.DataHolder.AsCharPtr())
            , dataend(datapos + trie.DataHolder.Length())
        {
            result = Next();
        }

    public:
        operator bool () const {
            return result;
        }

        TPrefixIterator& operator ++ () {
            result = Next();
            return *this;
        }

        size_t GetPrefixLen() const {
            return prefixLen;
        }

        void GetValue(TData& to) const {
            Trie.Packer.UnpackLeaf(valuepos, to);
        }
    };

    friend class TPrefixIterator;

    TPrefixIterator PrefixIterator(const TSymbol* key, size_t keylen) const {
        return TPrefixIterator(*this, key, keylen);
    }

protected:
    explicit TCompactTrie(const char* emptyValue);
    TCompactTrie(const TBlob& data, const char* emptyValue, TPacker packer = TPacker());

    bool LookupLongestPrefix(const TSymbol* key, size_t keylen, size_t& prefixLen, const char*& valuepos, bool& hasNext) const;
    bool LookupLongestPrefix(const TSymbol* key, size_t keylen, size_t& prefixLen, const char*& valuepos) const {
        bool hasNext;
        return LookupLongestPrefix(key, keylen, prefixLen, valuepos, hasNext);
    }
    void LookupPhrases(const char* datapos, size_t len, const TSymbol* key, size_t keylen, yvector<TPhraseMatch>& matches, TSymbol separator) const;

};

template <class T = char, class D = ui64, class S = TCompactTriePacker<D> >
class TCompactTrieHolder : public TCompactTrie<T, D, S>, NNonCopyable::TNonCopyable {
private:
    typedef TCompactTrie<T, D, S> TBase;
    TArrayHolder<char> Storage;

public:
    TCompactTrieHolder(TInputStream& is, size_t len);
};

//------------------------//
// Implementation section //
//------------------------//

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TSearchIterator::Advance(const TSymbol* key, size_t keylen) {
    if (!key || DataPos == NULL || DataPos >= DataEnd) {
        return false;
    }
    if (!keylen) {
        return true;
    }

    const TSymbol* keyend = key + keylen;
    while (key != keyend) {
        if (!NCompactTrie::Advance(DataPos, DataEnd, ValuePos, *(key++), Trie->Packer)) {
            return false;
        }
        if (key == keyend) {
            return true;
        }
    }
    return false;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TSearchIterator::GetValue(TData* value) const {
    if (value) {
        if (ValuePos) {
            Trie->Packer.UnpackLeaf(ValuePos, *value);
        } else if (Trie->EmptyValue) {
            Trie->Packer.UnpackLeaf(ValuePos, *value);
        }
    }
    return ValuePos || Trie->EmptyValue;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TPrefixIterator::Next() {
    using namespace NCompactTrie;
    if (!key || datapos == dataend)
        return false;

    if (!valuepos && Trie.EmptyValue) {
        valuepos = Trie.EmptyValue;
        return true;
    }

    char flags = MT_NEXT;

    while (key != keyend) {
        //Cerr << Wtroka(key, 1) << Endl;
        T label = *(key++);
        for (i64 i = (i64)ExtraBits<TSymbol>(); i >= 0; i -= 8) {
            flags = LeapByte(datapos, dataend, (char)(label >> i));
            if (!datapos) {
                return false; // no such arc
            }

            YASSERT(datapos <= dataend);
            if ((flags & MT_FINAL)) {
                prefixLen = keylen - (keyend - key) - (i ? 1 : 0);
                valuepos = datapos;
                datapos += Packer.SkipLeaf(datapos); // skip intermediate leaf nodes
                if (!i) { // got a match
                    if (!(flags & MT_NEXT))
                        key = keyend;
                    return true;
                }
            }

            if (!(flags & MT_NEXT)) {
                return false; // no further way
            }
        }
    }

    return false;
}


// TCompactTrie::TConstIterator::TImpl

template <class T, class D, class S>
class TCompactTrie<T, D, S>::TConstIterator::TImpl { // Iterator stuff. Stores a stack of visited forks.
private:
    class TFork;
    const char* const Data;
    size_t const Len;
    yvector<TFork> Forks;
    const char* const EmptyValue;
    bool AtEmptyValue;
    TPacker Packer;

public:
    TImpl(const char* data, size_t datalen, const char* emptyValue, bool atend, TPacker packer);
    TImpl(const char* data, size_t len, const char* emptyValue, const TKeyBuf& key, TPacker packer);

    bool operator == (const TImpl& rhs) const;

    bool Forward();
    bool Backward();
    bool UpperBound(const TKeyBuf& key); // True if matched exactly.
    TValueType Get() const;

    // in case of <char> specialization returns "\0" key as empty string
    TKey GetKey() const;
    size_t GetKeySize() const;
    TData GetValue() const;
    void GetValue(TData& to) const;
    const char* GetValuePtr() const; // 0 if none

private:
    static TKey WidenKey(const Stroka& rawkey);
    static size_t MeasureKey(size_t rawkey);
    Stroka GetKeyImpl() const;
    size_t MeasureKeyImpl() const;
    int LongestPrefix(const TKeyBuf& key); // Used in UpperBound.
};

// TCompactTrie::TConstIterator::TImpl::TFork

template <class T, class D, class S>
class TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork { // Auxiliary class for a branching point in the iterator
public:
    NCompactTrie::TNode Node;
    const char* Data;
    size_t Limit; // valid data is in range [Data + Node.GetOffset(), Data + Limit)
    char Direction[5];  // Direction variants: 'L' - left, 'R' - right, 'F' - final :), 'N' - next;
    const char* CurrentDirection;   // ptr to Direction
    TPacker Packer;

public:
    TFork(const char* data, size_t offset, size_t limit, TPacker packer);
    TFork(const TFork& rhs);
    TFork& operator=(const TFork& rhs);

    bool operator==(const TFork& rhs) const;

    bool NextDirection();
    bool PrevDirection();
    void LastDirection();

    // If the fork doesn't have the specified direction,
    // leaves the direction intact and returns false.
    // Otherwise returns true.
    bool SetDirection(char direction);
    TFork NextFork() const;

    char GetLabel() const;
    const char* GetValuePtr() const;
};

// TCompactTrie

template <class T, class D, class S>
TCompactTrie<T, D, S>::TCompactTrie()
    : EmptyValue(NULL)
{}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TCompactTrie(const TBlob& data, TPacker packer)
    : DataHolder(data)
    , EmptyValue(NULL)
    , Packer(packer)
{
    Init(data, packer);
}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TCompactTrie(const char* d, size_t len, TPacker packer)
    : EmptyValue(NULL)
    , Packer(packer)
{
    Init(d, len, packer);
}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TCompactTrie(const char* emptyValue)
    : EmptyValue(emptyValue)
{}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TCompactTrie(const TBlob& data, const char* emptyValue, TPacker packer)
    : DataHolder(data)
    , EmptyValue(emptyValue)
    , Packer(packer)
{}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::Init(const char* d, size_t len, TPacker packer) {
    Init(TBlob::NoCopy(d, len), packer);
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::Init(const TBlob& data, TPacker packer) {
    using namespace NCompactTrie;

    DataHolder = data;
    Packer = packer;

    const char* datapos = DataHolder.AsCharPtr();
    size_t len = DataHolder.Length();
    if (!len)
        return;

    const char* const dataend = datapos + len;

    const char* emptypos = datapos;
    char flags = LeapByte(emptypos, dataend, 0);
    if (emptypos && (flags & MT_FINAL)) {
        YASSERT(emptypos <= dataend);
        EmptyValue = emptypos;
    }
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::IsInitialized() const {
    return DataHolder.Data() != NULL;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::IsEmpty() const {
    return DataHolder.Size() == 0 && EmptyValue == NULL;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::Find(const TSymbol *key, size_t keylen, TData* value) const {
    size_t prefixLen = 0;
    const char* valuepos = NULL;
    bool hasNext;
    if (!LookupLongestPrefix(key, keylen, prefixLen, valuepos, hasNext) || prefixLen != keylen)
        return false;
    if (value)
        Packer.UnpackLeaf(valuepos, *value);
    return true;
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::FindPhrases(const TSymbol* key, size_t keylen, TPhraseMatchVector& matches, TSymbol separator) const {
    LookupPhrases(DataHolder.AsCharPtr(), DataHolder.Length(), key, keylen, matches, separator);
}

template <class T, class D, class S>
inline TCompactTrie<T, D, S> TCompactTrie<T, D, S>::FindTails(const TSymbol* key, size_t keylen) const {
    TCompactTrie<T, D, S> ret;
    FindTails(key, keylen, ret);
    return ret;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::FindTails(const TSymbol* key, size_t keylen, TCompactTrie<T, D, S>& res) const {
    using namespace NCompactTrie;

    size_t len = DataHolder.Length();

    if (!key || !len)
        return false;

    if (!keylen) {
        res = *this;
        return true;
    }

    const char* datastart = DataHolder.AsCharPtr();
    const char* datapos = datastart;
    const char* const dataend = datapos + len;

    const TSymbol* keyend = key + keylen;
    const char* value = NULL;

    while (key != keyend) {
        T label = *(key++);
        if (!NCompactTrie::Advance(datapos, dataend, value, label, Packer))
            return false;

        if (key == keyend) {
            if (datapos) {
                YASSERT(datapos >= datastart);
                res = TCompactTrie<T, D, S>(TBlob::NoCopy(datapos, dataend - datapos), value);
            } else {
                res = TCompactTrie<T, D, S>(value);
            }
            return true;
        } else if (!datapos) {
            return false; // No further way
        }
    }

    return false;
}

template <class T, class D, class S>
inline bool TCompactTrie<T, D, S>::FindTails(TSymbol label, TCompactTrie<T, D, S>& res) const {
    using namespace NCompactTrie;

    const size_t len = DataHolder.Length();
    if (!len)
        return false;

    const char* datastart = DataHolder.AsCharPtr();
    const char* dataend = datastart + len;
    const char* datapos = datastart;
    const char* value = NULL;

    if (!NCompactTrie::Advance(datapos, dataend, value, label, Packer))
        return false;

    if (datapos) {
        YASSERT(datapos >= datastart);
        res = TCompactTrie<T, D, S>(TBlob::NoCopy(datapos, dataend - datapos), value);
    } else {
        res = TCompactTrie<T, D, S>(value);
    }

    return true;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator TCompactTrie<T, D, S>::Begin() const {
    return TConstIterator(DataHolder.AsCharPtr(), DataHolder.Length(), EmptyValue, false, Packer);
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator TCompactTrie<T, D, S>::End() const {
    return TConstIterator(DataHolder.AsCharPtr(), DataHolder.Length(), EmptyValue, true, Packer);
}

template <class T, class D, class S>
size_t TCompactTrie<T, D, S>::Size() const {
    size_t res = 0;
    for (TConstIterator it = Begin(); it != End(); ++it)
        ++res;
    return res;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator TCompactTrie<T, D, S>::UpperBound(const TKeyBuf& key) const {
    return TConstIterator(DataHolder.AsCharPtr(), DataHolder.Length(), EmptyValue,
        key, Packer);
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::Print(TOutputStream& os) {
    typedef typename ::TCharStringTraits<T>::TBuffer TSBuffer;
    for (TConstIterator it = Begin(); it != End(); ++it) {
        os << TSBuffer(~(*it).first, +(*it).first) << "\t" << (*it).second << Endl;
    }
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::FindLongestPrefix(const TSymbol *key, size_t keylen, size_t* prefixLen, TData* value, bool* hasNext) const {
    const char* valuepos = NULL;
    size_t tempPrefixLen = 0;
    bool tempHasNext;
    bool found = LookupLongestPrefix(key, keylen, tempPrefixLen, valuepos, tempHasNext);
    if (prefixLen)
        *prefixLen = tempPrefixLen;
    if (found && value)
        Packer.UnpackLeaf(valuepos, *value);
    if (hasNext)
        *hasNext = tempHasNext;
    return found;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::LookupLongestPrefix(const TSymbol *key, size_t keylen, size_t& prefixLen, const char*& valuepos, bool& hasNext) const {
    using namespace NCompactTrie;

    const char* datapos = DataHolder.AsCharPtr();
    size_t len = DataHolder.Length();

    prefixLen = 0;
    hasNext = false;
    bool found = false;

    if (EmptyValue) {
        valuepos = EmptyValue;
        found = true;
    }

    if (!key || !len)
        return found;

    const char* const dataend = datapos + len;
    char flags = MT_NEXT;

    const T* keyend = key + keylen;
    while (key != keyend) {
        T label = *(key++);
        for (i64 i = (i64)ExtraBits<TSymbol>(); i >= 0; i -= 8) {
            flags = LeapByte(datapos, dataend, (char)(label >> i));
            if (!datapos) {
                return found; // no such arc
            }

            YASSERT(datapos <= dataend);
            if ((flags & MT_FINAL)) {
                prefixLen = keylen - (keyend - key) - (i ? 1 : 0);
                valuepos = datapos;
                hasNext = flags & MT_NEXT;
                found = true;

                if (!i && key == keyend) { // last byte, and got a match
                    return found;
                }
                datapos += Packer.SkipLeaf(datapos); // skip intermediate leaf nodes
            }

            if (!(flags & MT_NEXT)) {
                return found; // no further way
            }
        }
    }

    return found;
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::LookupPhrases(
    const char* datapos, size_t len, const TSymbol* key, size_t keylen,
    yvector<TPhraseMatch>& matches, TSymbol separator) const
{
    using namespace NCompactTrie;

    matches.clear();

    if (!key || !len)
        return;

    const T* const keystart = key;
    const T* const keyend = key + keylen;
    const char* const dataend = datapos + len;
    while (key != keyend) {
        T label = *(key++);
        for (int i = ExtraBits<TSymbol>(); i >= 0; i -= 8) {
            char flags = LeapByte(datapos, dataend, (char)(label >> i));
            if (!datapos)
                return; // no such arc

            if ((flags & MT_FINAL)) {
                if (!i && (key == keyend || *key == separator)) { // last byte, and got a match
                    size_t matchlength = (size_t)(key - keystart);
                    D data;
                    Packer.UnpackLeaf(datapos, data);
                    matches.push_back(TPhraseMatch(matchlength, data));
                }
                datapos += Packer.SkipLeaf(datapos);
            }

            if (!(flags & MT_NEXT))
                return; // no further way
        }
    }
}

// TCompactTrieHolder

template <class T, class D, class S>
TCompactTrieHolder<T, D, S>::TCompactTrieHolder(TInputStream& is, size_t len)
    : Storage(new char[len])
{
    if (is.Load(Storage.Get(), len) != len) {
        ythrow yexception() << "bad data load";
    }
    TBase::Init(Storage.Get(), len);
}

// TCompactTrie::TConstIterator

template <class T, class D, class S>
TCompactTrie<T, D, S>::TConstIterator::TConstIterator(const char* data, size_t len, const char* emptyValue, bool atend, TPacker packer)
    : Impl(new TImpl(data, len, emptyValue, atend, packer))
{}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TConstIterator::TConstIterator(const char* data, size_t len, const char* emptyValue, const TKeyBuf& key, TPacker packer)
    : Impl(new TImpl(data, len, emptyValue, key, packer))
{}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::operator == (const TConstIterator& other) const {
    if (!Impl)
        return !other.Impl;
    if (!other.Impl)
        return false;
    return *Impl == *other.Impl;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::operator != (const TConstIterator& other) const {
    return !operator==(other);
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator& TCompactTrie<T, D, S>::TConstIterator::operator++() {
    Impl->Forward();
    return *this;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator TCompactTrie<T, D, S>::TConstIterator::operator++(int /*unused*/) {
    TConstIterator copy(*this);
    Impl->Forward();
    return copy;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator& TCompactTrie<T, D, S>::TConstIterator::operator--() {
    Impl->Backward();
    return *this;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator TCompactTrie<T, D, S>::TConstIterator::operator--(int /*unused*/) {
    TConstIterator copy(*this);
    Impl->Backward();
    return copy;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TValueType TCompactTrie<T, D, S>::TConstIterator::operator*() {
    return Impl->Get();
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TKey TCompactTrie<T, D, S>::TConstIterator::GetKey() const {
    return Impl->GetKey();
}

template <class T, class D, class S>
size_t TCompactTrie<T, D, S>::TConstIterator::GetKeySize() const {
    return Impl->GetKeySize();
}

template <class T, class D, class S>
D TCompactTrie<T, D, S>::TConstIterator::GetValue() const {
    return Impl->GetValue();
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::TConstIterator::GetValue(D& d) const {
    return Impl->GetValue(d);
}

template <class T, class D, class S>
const char* TCompactTrie<T, D, S>::TConstIterator::GetValuePtr() const {
    return Impl->GetValuePtr();
}

// TCompactTrie::TConstIterator::TImpl

template <class T, class D, class S>
TCompactTrie<T, D, S>::TConstIterator::TImpl::TImpl(const char* data, size_t datalen, const char* emptyValue, bool atend, TPacker packer)
    : Data(data)
    , Len(datalen)
    , EmptyValue(emptyValue)
    , AtEmptyValue(emptyValue && !atend)
    , Packer(packer)
{
    if (!AtEmptyValue && !atend)
        Forward();
}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TConstIterator::TImpl::TImpl(const char* data, size_t datalen, const char* emptyValue, const TKeyBuf& key, TPacker packer)
    : Data(data)
    , Len(datalen)
    , EmptyValue(emptyValue)
    , AtEmptyValue(emptyValue && key.empty())
    , Packer(packer)
{
    if (!AtEmptyValue && data)
        UpperBound(key);
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::operator == (const TImpl& rhs) const {
    return (Data == rhs.Data &&
            Len == rhs.Len &&
            Forks == rhs.Forks &&
            EmptyValue == rhs.EmptyValue &&
            AtEmptyValue == rhs.AtEmptyValue
            );
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::Forward() {
    if (AtEmptyValue) {
        AtEmptyValue = false;
        bool res = Forward(); // TODO delete this after format change
        if (res && !GetKeyImpl().empty()) {
            return res; // there was not "\0" key
        }
        // otherwise we are skipping "\0" key
    }

    if (!Len)
        return false;

    if (Forks.empty()) {
        TFork fork(Data, 0, Len, Packer);
        Forks.push_back(fork);
    } else {
        TFork *top_fork = &Forks.back();
        while (!top_fork->NextDirection()) {
            if (top_fork->Node.GetOffset() >= Len)
                return false;
            Forks.pop_back();
            if (Forks.empty())
                return false;
            top_fork = &Forks.back();
        }
    }

    YASSERT(!Forks.empty());
    while (*(Forks.back().CurrentDirection) != 'F') {
        TFork next_fork = Forks.back().NextFork();
        Forks.push_back(next_fork);
    }
    return true;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::Backward() {
    if (AtEmptyValue)
        return false;

    if (!Len) {
        if (EmptyValue) {
            // A trie that has only the empty value;
            // we are not at the empty value, so move to it.
            AtEmptyValue = true;
            return true;
        } else {
            // Empty trie.
            return false;
        }
    }

    if (Forks.empty()) {
        TFork fork(Data, 0, Len, Packer);
        fork.LastDirection();
        Forks.push_back(fork);
    } else {
        TFork *top_fork = &Forks.back();
        while (!top_fork->PrevDirection()) {
            if (top_fork->Node.GetOffset() >= Len)
                return false;
            Forks.pop_back();
            if (!Forks.empty()) {
                top_fork = &Forks.back();
            } else {
                // When there are no more forks,
                // we have to iterate over the empty value.
                if (!EmptyValue)
                    return false;
                AtEmptyValue = true;
                return true;
            }
        }
    }

    YASSERT(!Forks.empty());
    while (*(Forks.back().CurrentDirection) != 'F') {
        TFork next_fork = Forks.back().NextFork();
        next_fork.LastDirection();
        Forks.push_back(next_fork);
    }
    if (GetKeyImpl().empty()) {
        // This is the '\0' key, skip it and get to the EmptyValue.
        AtEmptyValue = true;
        Forks.clear();
    }
    return true;
}

template <class T, class D, class S>
int TCompactTrie<T, D, S>::TConstIterator::TImpl::LongestPrefix(const TKeyBuf& key) {
    Forks.clear();
    TFork next(Data, 0, Len, Packer);
    for (size_t i = 0; i < key.size(); i++) {
        T symbol = key[i];
        const bool isLastSymbol = (i + 1 == key.size());
        for (i64 shift = (i64)NCompactTrie::ExtraBits<TSymbol>(); shift >= 0; shift -= 8) {
            const unsigned char label = (unsigned char)(symbol >> shift);
            const bool isLastByte = (isLastSymbol && shift == 0);
            do {
                Forks.push_back(next);
                TFork& top = Forks.back();
                if (label < (unsigned char)top.GetLabel()) {
                    if (!top.SetDirection('L'))
                        return 1;
                } else if (label > (unsigned char)top.GetLabel()) {
                    if (!top.SetDirection('R')) {
                        Forks.pop_back(); // We don't pass this fork on the way to the upper bound.
                        return -1;
                    }
                } else if (isLastByte) { // Here and below label == top.GetLabel().
                    if (top.SetDirection('F')) {
                        return 0; // Skip the NextFork() call at the end of the cycle.
                    } else {
                        top.SetDirection('N');
                        return 1;
                    }
                } else if (!top.SetDirection('N')) {
                    top.SetDirection('F');
                    return -1;
                }
                next = top.NextFork();
            } while (*(Forks.back().CurrentDirection) != 'N'); // Proceed to the next byte.
        }
    }
    // We get here only if the key was empty.
    Forks.push_back(next);
    return 1;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::UpperBound(const TKeyBuf& key) {
    const int defect = LongestPrefix(key);
    if (defect > 0) {
        // Continue the constructed forks with the smallest key possible.
        while (*(Forks.back().CurrentDirection) != 'F') {
            TFork next = Forks.back().NextFork();
            Forks.push_back(next);
        }
    } else if (defect < 0) {
        Forward();
    }
    return defect == 0;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TValueType TCompactTrie<T, D, S>::TConstIterator::TImpl::Get() const {
    if (AtEmptyValue) {
        D data;
        Packer.UnpackLeaf(EmptyValue, data);
        return TValueType(TKey(), data);
    }

    return TValueType(GetKey(), GetValue());
}


template <class TSymbol>
struct TConvertRawKey {
    typedef typename TCompactTrieKeySelector<TSymbol>::TKey TKey;
    static TKey Get(const Stroka& rawkey) {
        TKey result;
        const size_t sz = rawkey.size();
        result.reserve(sz / sizeof(TSymbol));
        for (size_t i = 0; i < sz; i += sizeof(TSymbol)) {
            TSymbol sym = 0;
            for (size_t j = 0; j < sizeof(TSymbol); j++) {
                if (sizeof(TSymbol) <= 1)
                    sym = 0;
                else
                    sym <<= 8;
                if (i + j < sz)
                    sym |= TSymbol(0x00FF & rawkey[i + j]);
            }
            result.push_back(sym);
        }
        return result;
    }

    static size_t Size(size_t rawsize) {
        return rawsize / sizeof(TSymbol);
    }
};

template <>
struct TConvertRawKey<char> {
    static Stroka Get(const Stroka& rawkey) {
        return rawkey;
    }

    static size_t Size(size_t rawsize) {
        return rawsize;
    }
};

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TKey TCompactTrie<T, D, S>::TConstIterator::TImpl::WidenKey(const Stroka& rawkey) {
    return TConvertRawKey<T>::Get(rawkey);
}


template <class T, class D, class S>
size_t TCompactTrie<T, D, S>::TConstIterator::TImpl::MeasureKey(size_t rawkey) {
    return TConvertRawKey<T>::Size(rawkey);
}

template <class T, class D, class S>
Stroka TCompactTrie<T, D, S>::TConstIterator::TImpl::GetKeyImpl() const {
    Stroka result;
    for (typename yvector<TFork>::const_iterator i = Forks.begin(); i != Forks.end(); ++i)
        if (*(i->CurrentDirection) == 'N' || *(i->CurrentDirection) == 'F')
            result.push_back(i->GetLabel());

    // Special case: if we get a single zero label, treat it as an empty key
    if (result.size() == 1 && result[0] == '\0') // TODO delete this after format change
        result.clear();

    return result;
}

template <class T, class D, class S>
size_t TCompactTrie<T, D, S>::TConstIterator::TImpl::MeasureKeyImpl() const {
    size_t result = 0;
    char firstchar = '\0';
    for (typename yvector<TFork>::const_iterator i = Forks.begin(); i != Forks.end(); ++i) {
        if (*(i->CurrentDirection) == 'N' || *(i->CurrentDirection) == 'F')
            result += 1;
        if (1 == result)
            firstchar = i->GetLabel();
    }

    // Special case: if we get a single zero label, treat it as an empty key
    if (result == 1 && firstchar == '\0') // TODO delete this after format change
        result = 0;

    return result;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TKey TCompactTrie<T, D, S>::TConstIterator::TImpl::GetKey() const {
    return WidenKey(GetKeyImpl());
}

template <class T, class D, class S>
size_t TCompactTrie<T, D, S>::TConstIterator::TImpl::GetKeySize() const {
    return MeasureKey(MeasureKeyImpl());
}


template <class T, class D, class S>
const char* TCompactTrie<T, D, S>::TConstIterator::TImpl::GetValuePtr() const {
    if (!Forks.empty()) {
        const typename TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork& lastfork = Forks.back();
        YASSERT(lastfork.Node.IsFinal() && *(lastfork.CurrentDirection) == 'F');
        return lastfork.GetValuePtr();
    }

    return EmptyValue;
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TData TCompactTrie<T, D, S>::TConstIterator::TImpl::GetValue() const {
    D data;
    GetValue(data);
    return data;
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::TConstIterator::TImpl::GetValue(typename TCompactTrie<T, D, S>::TData& data) const {
    const char* ptr = GetValuePtr();
    if (ptr) {
        Packer.UnpackLeaf(ptr, data);
    } else {
        data = typename TCompactTrie<T, D, S>::TData();
    }
}

// TCompactTrie::TConstIterator::TImpl::TFork

template <class T, class D, class S>
TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::TFork(const char* data, size_t offset, size_t limit, TPacker packer)
    : Node(data, offset, NCompactTrie::TPackerLeafSkipper<TPacker>(&packer))
    , Data(data)
    , Limit(limit)
    , CurrentDirection(&Direction[0])
    , Packer(packer)
{
    using namespace NCompactTrie;

#if COMPTRIE_DATA_CHECK
    if (Node.GetOffset() >= Limit - 1)
        ythrow yexception() << "gone beyond the limit, data is corrupted";
#endif
    char *result = Direction;
    if (Node.GetLeftOffset())
        *(result++) = 'L';
    if (Node.IsFinal())
        *(result++) = 'F';
    if (Node.GetForwardOffset())
        *(result++) = 'N';
    if (Node.GetRightOffset())
        *(result++) = 'R';
    *result = '\0';
}

template <class T, class D, class S>
TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::TFork(const TFork& rhs)
    : Node(rhs.Node)
    , Data(rhs.Data)
    , Limit(rhs.Limit)
    , Packer(rhs.Packer)
{
#if COMPTRIE_DATA_CHECK
    if (Node.GetOffset() >= Limit - 1)
        ythrow yexception() << "gone beyond the limit, data is corrupted";
#endif
    strncpy(Direction, rhs.Direction, 5);
    CurrentDirection = Direction + (rhs.CurrentDirection - rhs.Direction);
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork&
TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::operator=(const TFork& rhs) {
    if (this == &rhs)
        return *this;
    Node = rhs.Node;
    Data = rhs.Data;
    Limit = rhs.Limit;
    Packer = rhs.Packer;
#if COMPTRIE_DATA_CHECK
    if (Node.GetOffset() >= Limit - 1)
        ythrow yexception() << "gone beyond the limit, data is corrupted";
#endif
    strncpy(Direction, rhs.Direction, 5);
    CurrentDirection = Direction + (rhs.CurrentDirection - rhs.Direction);
    return *this;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::operator==(const TFork& rhs) const {
    return (Data == rhs.Data &&
            Node.GetOffset() == rhs.Node.GetOffset() &&
            strncmp(Direction, rhs.Direction, 5) == 0 &&
            CurrentDirection - Direction == rhs.CurrentDirection - rhs.Direction);
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::NextDirection() {
    if (*CurrentDirection && *(++CurrentDirection))
        return true;
    return false;
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::PrevDirection() {
    if (CurrentDirection > Direction) {
        --CurrentDirection;
        return true;
    }
    return false;
}

template <class T, class D, class S>
void TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::LastDirection() {
    while (NextDirection())
        ;
    PrevDirection();
}

template <class T, class D, class S>
bool TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::SetDirection(char direction) {
    const char* pos = Direction;
    while(*pos && *pos != direction)
        ++pos;
    if (*pos) {
        CurrentDirection = pos;
        YASSERT(Direction <= CurrentDirection && CurrentDirection <= Direction + 3);
        return true;
    } else {
        return false;
    }
}

template <class T, class D, class S>
typename TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::NextFork() const {
    size_t offset = 0;
    switch (*CurrentDirection) {
        case 'L':
            offset = Node.GetLeftOffset();
            break;
        case 'N':
            offset = Node.GetForwardOffset();
            break;
        case 'R':
            offset = Node.GetRightOffset();
            break;
        default:
            ythrow yexception() << "Wrong direction '" << *CurrentDirection << "'";
    }

    return TFork(Data, offset, Limit, Packer);
}

template <class T, class D, class S>
char TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::GetLabel() const {
    return Node.GetLabel();
}

template <class T, class D, class S>
const char* TCompactTrie<T, D, S>::TConstIterator::TImpl::TFork::GetValuePtr() const {
    return Node.GetLeafPos();
}

