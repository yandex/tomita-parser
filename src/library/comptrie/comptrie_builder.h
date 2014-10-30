#pragma once

#include "comptrie_impl.h"
#include "key_selector.h"
#include "minimize.h"
#include "make_fast_layout.h"

// --------------------------------------------------------------------------------------
// Data Builder
// To build the data buffer, we first create an automaton in memory. The automaton
// is created incrementally. It actually helps a lot to have the input data prefix-grouped
// by key; otherwise, memory consumption becomes a tough issue.
// NOTE: building and serializing the automaton may be lengthy, and takes lots of memory.


// PREFIX_GROUPED means that if we, while constructing a trie, add to the builder two keys with the same prefix,
// then all the keys that we add between these two also have the same prefix.
// Actually in this mode the builder can accept even more freely ordered input,
// but for input as above it is guaranteed to work.
enum ECompactTrieBuilderFlags {
    CTBF_NONE = 0,
    CTBF_PREFIX_GROUPED = 1 << 0,
    CTBF_VERBOSE = 1 << 1,
    CTBF_UNIQUE = 1 << 2,
};

typedef ECompactTrieBuilderFlags TCompactTrieBuilderFlags;

inline TCompactTrieBuilderFlags operator |(TCompactTrieBuilderFlags first, TCompactTrieBuilderFlags second) {
    return static_cast<TCompactTrieBuilderFlags>(static_cast<int>(first) | second);
}

inline TCompactTrieBuilderFlags& operator |=(TCompactTrieBuilderFlags& first, TCompactTrieBuilderFlags second) {
    return first = first | second;
}

template <class T = char, class D = ui64, class S = TCompactTriePacker<D> >
class TCompactTrieBuilder {
public:
    typedef T TSymbol;
    typedef D TData;
    typedef S TPacker;
    typedef typename TCompactTrieKeySelector<TSymbol>::TKey TKey;
    typedef typename TCompactTrieKeySelector<TSymbol>::TKeyBuf TKeyBuf;

    explicit TCompactTrieBuilder(TCompactTrieBuilderFlags flags = CTBF_NONE, TPacker packer = TPacker());

    // All Add.. methods return true if it was a new key, false if the key already existed.

    bool Add(const TSymbol* key, size_t keylen, const TData& value);
    bool Add(const TKeyBuf& key, const TData& value) {
        return Add(~key, +key, value);
    }

    // add already serialized data
    bool AddPtr(const TSymbol* key, size_t keylen, const char* data);
    bool AddPtr(const TKeyBuf& key, const char* data) {
        return AddPtr(~key, +key, data);
    }

    bool AddSubtreeInFile(const TSymbol* key, size_t keylen, const Stroka& filename);
    bool AddSubtreeInFile(const TKeyBuf& key, const Stroka& filename) {
        return AddSubtreeInFile(~key, +key, filename);
    }

    bool Find(const TSymbol* key, size_t keylen, TData* value) const;
    bool Find(const TKeyBuf& key, TData* value = NULL) const {
        return Find(~key, +key, value);
    }

    bool FindLongestPrefix(const TSymbol *key, size_t keylen, size_t* prefixLen, TData* value = NULL) const;

    size_t Save(TOutputStream& os) const;
    size_t SaveToFile(const Stroka& fileName) const {
        TBufferedFileOutput out(fileName);
        return Save(out);
    }

    void Clear(); // Returns all memory to the system and resets the builder state.

    size_t GetEntryCount() const;
    size_t GetNodeCount() const;

protected:
    class TCompactTrieBuilderImpl;
    THolder<TCompactTrieBuilderImpl> Impl;
};

//----------------------------------------------------------------------------------------------------------------------
// Minimize the trie. The result is equivalent to the original
// trie, except that it takes less space (and has marginally lower
// performance, because of eventual epsilon links).
// The algorithm is as follows: starting from the largest pieces, we find
// nodes that have identical continuations  (Daciuk's right language),
// and repack the trie. Repacking is done in-place, so memory is less
// of an issue; however, it may take considerable time.

// IMPORTANT: never try to reminimize an already minimized trie or a trie with fast layout.
// If you want both minimizatioin and fast layout, do the minimization first.
// Because of non-local structure and epsilon links, it won't work
// as you expect it to, and can destroy the trie in the making.

template <class TPacker>
size_t CompactTrieMinimize(TOutputStream& os, const char *data, size_t datalength, bool verbose = false, const TPacker& packer = TPacker());

template <class TTrieBuilder>
size_t CompactTrieMinimize(TOutputStream& os, const TTrieBuilder &builder, bool verbose = false);

//----------------------------------------------------------------------------------------------------------------
// Lay the trie in memory in such a way that there are less cache misses when jumping from root to leaf.
// The trie becomes about 2% larger, but the access became about 25% faster in our experiments.
// Can be called on minimized and non-minimized tries, in the first case in requires half a trie more memory.
// Calling it the second time on the same trie does nothing.
//
// The algorithm is based on van Emde Boas layout as described in the yandex data school lectures on external memory algoritms
// by Maxim Babenko and Ivan Puzyrevsky. The difference is that when we cut the tree into levels
// two nodes connected by a forward link are put into the same level (because they usually lie near each other in the original tree).
// The original paper (describing the layout in Section 2.1) is:
// Michael A. Bender, Erik D. Demaine, Martin Farach-Colton. Cache-Oblivious B-Trees
//      SIAM Journal on Computing, volume 35, number 2, 2005, pages 341-358.
// Available on the web: http://erikdemaine.org/papers/CacheObliviousBTrees_SICOMP/
// Or: Michael A. Bender, Erik D. Demaine, and Martin Farach-Colton. Cache-Oblivious B-Trees
//      Proceedings of the 41st Annual Symposium
//      on Foundations of Computer Science (FOCS 2000), Redondo Beach, California, November 12-14, 2000, pages 399-409.
// Available on the web: http://erikdemaine.org/papers/FOCS2000b/
// (there is not much difference between these papers, actually).
//
template <class TPacker>
size_t CompactTrieMakeFastLayout(TOutputStream& os, const char *data, size_t datalength, bool verbose = false, const TPacker& packer = TPacker());

template <class TTrieBuilder>
size_t CompactTrieMakeFastLayout(TOutputStream& os, const TTrieBuilder &builder, bool verbose = false);

// TCompactTrieBuilder::TCompactTrieBuilderImpl

template <class T, class D, class S>
class TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl {
protected:
    TMemoryPool Pool;
    size_t PayloadSize;
    THolder<TFixedSizeAllocator> NodeAllocator;
    class TNode;
    class TArc;
    TNode* Root;
    TCompactTrieBuilderFlags Flags;
    size_t EntryCount;
    size_t NodeCount;
    TPacker Packer;

    enum EPayload {
        DATA_ABSENT,
        DATA_INSIDE,
        DATA_MALLOCED,
        DATA_IN_MEMPOOL,
    };

protected:
    void ConvertSymbolArrayToChar(const TSymbol* key, size_t keylen, TTempBuf& buf, size_t ckeylen) const;
    void NodeLinkTo(TNode* thiz, const TBlob& label, TNode* node);
    TNode* NodeForwardCheck(TNode* thiz, const char* label, size_t len, size_t& passed) const;
    TNode* NodeForwardAdd(TNode* thiz, const char* label, size_t len, size_t& passed, size_t* nodeCount);

    size_t NodeMeasureSubtree(TNode* thiz) const;
    ui64 NodeSaveSubtree(TNode* thiz, TOutputStream& os) const;
    void NodeBufferSubtree(TNode* thiz);

    size_t NodeMeasureLeafValue(TNode* thiz) const;
    ui64 NodeSaveLeafValue(TNode* thiz, TOutputStream& os) const;

    ui64 ArcMeasure(const TArc* thiz, size_t leftsize, size_t rightsize) const;

    ui64 ArcSave(const TArc* thiz, TOutputStream& os) const;

public:
    TCompactTrieBuilderImpl(TCompactTrieBuilderFlags flags, TPacker packer);
    ~TCompactTrieBuilderImpl();

    void DestroyNode(TNode* node);
    void NodeReleasePayload(TNode* thiz);

    char* AddEntryForData(const TSymbol* key, size_t keylen, size_t dataLen, bool& isNewAddition);
    TNode* AddEntryForSomething(const TSymbol* key, size_t keylen, bool& isNewAddition);

    bool AddEntry(const TSymbol* key, size_t keylen, const TData& value);
    bool AddEntryPtr(const TSymbol* key, size_t keylen, const char* value);
    bool AddSubtreeInFile(const TSymbol* key, size_t keylen, const Stroka& fileName);
    bool FindEntry(const TSymbol* key, size_t keylen, TData* value) const;
    bool FindLongestPrefix(const TSymbol* key, size_t keylen, size_t *prefixlen, TData* value) const;

    size_t Save(TOutputStream& os) const;
    void Clear();

    // lies is some key was added at least twice
    size_t GetEntryCount() const;
    size_t GetNodeCount() const;
};

template <class T, class D, class S>
class TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TArc {
public:
    TBlob Label;
    TNode* Node;
    mutable size_t LeftOffset;
    mutable size_t RightOffset;

    TArc(const TBlob& lbl, TNode* nd);
};

template <class T, class D, class S>
class TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode {
public:
    typedef typename TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl TBuilderImpl;
    typedef typename TBuilderImpl::TArc TArc;

    struct ISubtree {
        virtual ~ISubtree() { }
        virtual bool IsLast() const = 0;
        virtual ui64 Measure(const TBuilderImpl* builder) const = 0;
        virtual ui64 Save(const TBuilderImpl* builder, TOutputStream& os) const = 0;
        virtual void Destroy(TBuilderImpl*) { }
    };

    class TArcSet: public ISubtree, public TCompactVector<TArc> {
    public:
        typedef typename yvector<TArc>::iterator iterator;

        iterator Find(char ch);
        void Add(const TBlob& s, TNode* node);

        bool IsLast() const {
            return this->Empty();
        }

        ui64 Measure(const TBuilderImpl* builder) const {
            return MeasureRange(builder, 0, this->size());
        }

        ui64 MeasureRange(const TBuilderImpl* builder, size_t from, size_t to) const {
            if (from >= to)
                return 0;

            size_t median = (from + to) / 2;
            size_t leftsize = (size_t)MeasureRange(builder, from, median);
            size_t rightsize = (size_t)MeasureRange(builder, median + 1, to);

            return builder->ArcMeasure(&(*this)[median], leftsize, rightsize);
        }

        ui64 Save(const TBuilderImpl* builder, TOutputStream& os) const {
            return SaveRange(builder, 0, this->size(), os);
        }

        ui64 SaveRange(const TBuilderImpl* builder, size_t from, size_t to, TOutputStream& os) const {
            if (from >= to)
                return 0;

            size_t median = (from + to) / 2;

            ui64 written = builder->ArcSave(&(*this)[median], os);
            written += SaveRange(builder, from, median, os);
            written += SaveRange(builder, median + 1, to, os);
            return written;
        }

        void Destroy(TBuilderImpl* builder) {
            // Delete all nodes down the stream.
            for (iterator it = this->begin(); it != this->end(); ++it) {
                builder->DestroyNode(it->Node);
            }
            this->clear();
        }

        ~TArcSet() {
            YASSERT(this->empty());
        }

    };

    struct TBufferedSubtree: public ISubtree {
        TArrayWithSizeHolder<char> Buffer;

        bool IsLast() const {
            return Buffer.Empty();
        }

        ui64 Measure(const TBuilderImpl*) const {
            return Buffer.Size();
        }

        ui64 Save(const TBuilderImpl*, TOutputStream& os) const {
            os.Write(Buffer.Get(), Buffer.Size());
            return Buffer.Size();
        }
    };

    struct TSubtreeInFile: public ISubtree {
        struct TData {
            Stroka FileName;
            ui64 Size;
        };
        THolder<TData> Data;

        TSubtreeInFile(const Stroka& fileName) {
            // stupid API
            TFile file(fileName, RdOnly);
            i64 size = file.GetLength();
            if (size < 0)
                ythrow yexception() << "unable to get file " << fileName.Quote() << " size for unknown reason";
            Data.Reset(new TData);
            Data->FileName = fileName;
            Data->Size = size;
        }

        bool IsLast() const {
            return Data->Size == 0;
        }

        ui64 Measure(const TBuilderImpl*) const {
            return Data->Size;
        }

        ui64 Save(const TBuilderImpl*, TOutputStream& os) const {
            TFileInput is(Data->FileName);
            ui64 written = TransferData(&is, &os);
            if (written != Data->Size)
                ythrow yexception() << "file " << Data->FileName.Quote() << " size changed";
            return written;
        }
    };

    union {
        char ArcsData[COMPTRIE_MAX3(sizeof(TArcSet), sizeof(TBufferedSubtree), sizeof(TSubtreeInFile))];
        TAligner Aligner;
    };

    inline ISubtree* Subtree() {
        return reinterpret_cast<ISubtree*>(ArcsData);
    }

    inline const ISubtree* Subtree() const {
        return reinterpret_cast<const ISubtree*>(ArcsData);
    }

    EPayload PayloadType;

    inline char* PayloadPtr() {
        return ((char*) this) + sizeof(TNode);
    }

    // *Payload()
    inline char*& PayloadAsPtr() {
        char** payload = (char**) PayloadPtr();
        return *payload;
    }

    inline char* GetPayload() {
        switch (PayloadType) {
        case DATA_INSIDE:
            return PayloadPtr();
        case DATA_MALLOCED:
        case DATA_IN_MEMPOOL:
            return PayloadAsPtr();
        case DATA_ABSENT:
        default:
            abort();
        }
    }

    bool IsFinal() const {
        return PayloadType != DATA_ABSENT;
    }

    bool IsLast() const {
        return Subtree()->IsLast();
    }

    inline void* operator new(size_t, TFixedSizeAllocator& pool) {
        return pool.Allocate();
    }

    inline void operator delete(void* ptr, TFixedSizeAllocator& pool) throw () {
        pool.Release(ptr);
    }

    TNode()
        : PayloadType(DATA_ABSENT)
    {
        new (Subtree()) TArcSet;
    }

    ~TNode() {
        Subtree()->~ISubtree();
        YASSERT(PayloadType == DATA_ABSENT);
    }

};

// TCompactTrieBuilder

template <class T, class D, class S>
TCompactTrieBuilder<T, D, S>::TCompactTrieBuilder(TCompactTrieBuilderFlags flags, TPacker packer)
    : Impl(new TCompactTrieBuilderImpl(flags, packer))
{
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::Add(const TSymbol* key, size_t keylen, const TData& value) {
    return Impl->AddEntry(key, keylen, value);
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::AddPtr(const TSymbol* key, size_t keylen, const char* value) {
    return Impl->AddEntryPtr(key, keylen, value);
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::AddSubtreeInFile(const TSymbol* key, size_t keylen, const Stroka& fileName) {
    return Impl->AddSubtreeInFile(key, keylen, fileName);
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::Find(const TSymbol* key, size_t keylen, TData* value) const {
    return Impl->FindEntry(key, keylen, value);
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::FindLongestPrefix(
                const TSymbol* key, size_t keylen, size_t *prefixlen, TData* value) const {
    return Impl->FindLongestPrefix(key, keylen, prefixlen, value);
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::Save(TOutputStream& os) const {
    return Impl->Save(os);
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::Clear() {
    Impl->Clear();
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::GetEntryCount() const {
    return Impl->GetEntryCount();
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::GetNodeCount() const {
    return Impl->GetNodeCount();
}

// TCompactTrieBuilder::TCompactTrieBuilderImpl

template <class T, class D, class S>
TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TCompactTrieBuilderImpl(TCompactTrieBuilderFlags flags, TPacker packer)
    : Pool(1000000, TMemoryPool::TLinearGrow::Instance())
    , PayloadSize(sizeof(void*)) // XXX: find better value
    , NodeAllocator(new TFixedSizeAllocator(sizeof(TNode) + PayloadSize, TDefaultAllocator::Instance()))
    , Flags(flags)
    , EntryCount(0)
    , NodeCount(1)
    , Packer(packer)
{
    Root = new (*NodeAllocator) TNode;
}

template <class T, class D, class S>
TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::~TCompactTrieBuilderImpl() {
    DestroyNode(Root);
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::ConvertSymbolArrayToChar(
                const TSymbol* key, size_t keylen, TTempBuf& buf, size_t buflen) const {
    char* ckeyptr = buf.Data();

    for (size_t i = 0; i < keylen; ++i) {
        TSymbol label = key[i];
        for (int j = (int)NCompactTrie::ExtraBits<TSymbol>(); j >= 0; j -= 8) {
            YASSERT(ckeyptr < buf.Data() + buflen);
            *(ckeyptr++) = (char)(label >> j);
        }
    }

    buf.Proceed(buflen);
    YASSERT(ckeyptr == buf.Data() + buf.Filled());
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::DestroyNode(TNode* thiz) {
    thiz->Subtree()->Destroy(this);
    NodeReleasePayload(thiz);
    thiz->~TNode();
    NodeAllocator->Release(thiz);
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeReleasePayload(TNode* thiz) {
    switch (thiz->PayloadType) {
    case DATA_ABSENT:
    case DATA_INSIDE:
    case DATA_IN_MEMPOOL:
        break;
    case DATA_MALLOCED:
        delete[] thiz->PayloadAsPtr();
        break;
    default:
        abort();
    }
    thiz->PayloadType = DATA_ABSENT;
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::AddEntry(
                const TSymbol* key, size_t keylen, const TData& value) {
    size_t datalen = Packer.MeasureLeaf(value);

    bool isNewAddition = false;
    char* place = AddEntryForData(key, keylen, datalen, isNewAddition);
    Packer.PackLeaf(place, value, datalen);
    return isNewAddition;
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::AddEntryPtr(
                const TSymbol* key, size_t keylen, const char* value) {
    size_t datalen = Packer.SkipLeaf(value);

    bool isNewAddition = false;
    char* place = AddEntryForData(key, keylen, datalen, isNewAddition);
    memcpy(place, value, datalen);
    return isNewAddition;
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::AddSubtreeInFile(
                const TSymbol* key, size_t keylen, const Stroka& fileName) {
    typedef typename TNode::ISubtree ISubtree;
    typedef typename TNode::TSubtreeInFile TSubtreeInFile;

    bool isNewAddition = false;
    TNode* node = AddEntryForSomething(key, keylen, isNewAddition);
    node->Subtree()->Destroy(this);
    node->Subtree()->~ISubtree();

    new (node->Subtree()) TSubtreeInFile(fileName);
    return isNewAddition;
}

template <class T, class D, class S>
typename TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode*
                TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::AddEntryForSomething(
                                const TSymbol* key, size_t keylen, bool& isNewAddition) {
    using namespace NCompactTrie;

    EntryCount++;

    if (Flags & CTBF_VERBOSE)
        ShowProgress(EntryCount);

    TNode* current = Root;
    size_t passed;

    // Special case of empty key: create a zero link from the root
    if (keylen == 0) {
        const char zero = 0;
        current = NodeForwardCheck(Root, &zero, 1, passed);

        if (!current) {
            NodeCount++;
            current = new (*NodeAllocator) TNode();
            NodeLinkTo(Root, TBlob::Copy(&zero, 1), current);
        }
    } else {

        size_t ckeylen = keylen * sizeof(TSymbol);
        TTempBuf ckeybuf(ckeylen);
        ConvertSymbolArrayToChar(key, keylen, ckeybuf, ckeylen);
        char* ckey = ckeybuf.Data();

        TNode* next;
        while ((ckeylen > 0) && (next = NodeForwardAdd(current, ckey, ckeylen, passed, &NodeCount)) != 0) {
            current = next;
            ckeylen -= passed;
            ckey += passed;
        }

        if (ckeylen != 0) {
            //new leaf
            NodeCount++;
            TNode* leaf = new (*NodeAllocator) TNode();
            NodeLinkTo(current, TBlob::Copy(ckey, ckeylen), leaf);
            current = leaf;
        }
    }
    isNewAddition = (current->PayloadType == DATA_ABSENT);
    if ((Flags & CTBF_UNIQUE) && !isNewAddition)
        ythrow yexception() << "Duplicate key";
    return current;
}

template <class T, class D, class S>
char* TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::AddEntryForData(const TSymbol* key, size_t keylen,
        size_t datalen, bool& isNewAddition) {
    TNode* current = AddEntryForSomething(key, keylen, isNewAddition);
    NodeReleasePayload(current);
    if (datalen <= PayloadSize) {
        current->PayloadType = DATA_INSIDE;
    } else if (Flags & CTBF_PREFIX_GROUPED) {
        current->PayloadType = DATA_MALLOCED;
        current->PayloadAsPtr() = new char[datalen];
    } else {
        current->PayloadType = DATA_IN_MEMPOOL;
        current->PayloadAsPtr() = (char*) Pool.Allocate(datalen); // XXX: allocate unaligned
    }
    return current->GetPayload();
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::FindEntry(const TSymbol* key, size_t keylen, TData* value) const {
    using namespace NCompactTrie;

    if (Flags & CTBF_PREFIX_GROUPED)
        ythrow yexception() << "cannot find entries in prefix-grouped TCompactTrieBuilder";

    TNode* current = Root;
    size_t passed;

    if (!keylen) { // special case of empty key, stored as "\0"
        const char zero = 0;
        current = NodeForwardCheck(Root, &zero, 1, passed);
        if (!current)
            return false;
    }

    size_t ckeylen = keylen * sizeof(TSymbol);
    TTempBuf ckeybuf(ckeylen);
    ConvertSymbolArrayToChar(key, keylen, ckeybuf, ckeylen);
    char* ckey = ckeybuf.Data();

    TNode* next;
    while ((ckeylen > 0) && (next = NodeForwardCheck(current, ckey, ckeylen, passed)) != 0) {
        if (!next)
            return false;

        current = next;
        ckeylen -= passed;
        ckey += passed;
    }

    if (ckeylen == 0 && current->IsFinal()) {
        if (value)
            Packer.UnpackLeaf(current->GetPayload(), *value);

        return true;
    }

    return false;
}

template <class T, class D, class S>
bool TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::FindLongestPrefix(
                const TSymbol* key, size_t keylen, size_t *prefixlen, TData* value) const {
    using namespace NCompactTrie;

    TNode* current;
    TNode* last_final = NULL;
    *prefixlen = 0;
    size_t passed;

    // if there is an empty key set in trie, put it to last_final
    const char zero = 0;
    current = NodeForwardCheck(Root, &zero, 1, passed);
    if (current && current->IsFinal()) {
        last_final = current;
    }

    if (!keylen) {
        if (last_final && value)
            Packer.UnpackLeaf(last_final->GetPayload(), *value);
        return (last_final != NULL);
    }

    current = Root;

    size_t ckeylen = keylen * sizeof(TSymbol);
    TTempBuf ckeybuf(ckeylen);
    ConvertSymbolArrayToChar(key, keylen, ckeybuf, ckeylen);
    char* ckey = ckeybuf.Data();

    TNode* next;
    while ((ckeylen > 0) && (next = NodeForwardCheck(current, ckey, ckeylen, passed))) {
        current = next;
        ckeylen -= passed;
        ckey += passed;
        if (current->IsFinal()) {
            last_final = current;
            *prefixlen = ckey - ckeybuf.Data();
        }
    }

    if (last_final) {
        if (value)
            Packer.UnpackLeaf(last_final->GetPayload(), *value);
        return true;
    }

    return false;
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::Clear() {
    DestroyNode(Root);
    Pool.Clear();
    NodeAllocator.Reset(new TFixedSizeAllocator(sizeof(TNode) + PayloadSize, TDefaultAllocator::Instance()));
    Root = new (*NodeAllocator) TNode;
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::Save(TOutputStream& os) const {
    const size_t len = NodeMeasureSubtree(Root);
    if (len != NodeSaveSubtree(Root, os))
        ythrow yexception() << "something wrong";

    return len;
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::GetEntryCount() const {
    return EntryCount;
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::GetNodeCount() const {
    return NodeCount;
}

template <class T, class D, class S>
typename TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode*
                TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeForwardCheck(
                                TNode* thiz, const char* label, size_t len, size_t& passed) const {
    typename TNode::TArcSet* arcSet = dynamic_cast<typename TNode::TArcSet*>(thiz->Subtree());
    typename TNode::TArcSet::iterator it = arcSet->Find(*label);

    if (it != arcSet->end()) {
        const char* arcLabel = it->Label.AsCharPtr();
        size_t arcLabelLen = it->Label.Length();

        if (len >= arcLabelLen && memcmp(label, arcLabel, arcLabelLen) == 0) {
            passed = arcLabelLen;
            return it->Node;
        }
    }

    return 0;
}

template <class T, class D, class S>
typename TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode*
                TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeForwardAdd(
                                TNode* thiz, const char* label, size_t len, size_t& passed, size_t* nodeCount) {
    typename TNode::TArcSet* arcSet = dynamic_cast<typename TNode::TArcSet*>(thiz->Subtree());
    if (!arcSet)
        ythrow yexception() << "Bad input order - expected input strings to be prefix-grouped.";

    typename TNode::TArcSet::iterator it = arcSet->Find(*label);

    if (it != arcSet->end()) {
        const char* arcLabel = it->Label.AsCharPtr();
        size_t arcLabelLen = it->Label.Length();

        for (passed = 0; (passed < len) && (passed < arcLabelLen) && (label[passed] == arcLabel[passed]); ++passed) {
            //just count
        }

        if (passed < arcLabelLen) {
            (*nodeCount)++;
            TNode* node = new (*NodeAllocator) TNode();
            NodeLinkTo(node, it->Label.SubBlob(passed, arcLabelLen), it->Node);

            it->Node = node;
            it->Label = it->Label.SubBlob(passed);
        }

        return it->Node;
    }

    return 0;
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeLinkTo(TNode* thiz, const TBlob& label, TNode* node) {
    typename TNode::TArcSet* arcSet = dynamic_cast<typename TNode::TArcSet*>(thiz->Subtree());
    if (!arcSet)
        ythrow yexception() << "Bad input order - expected input strings to be prefix-grouped.";

    // Buffer the node at the last arc
    if ((Flags & CTBF_PREFIX_GROUPED) && !arcSet->empty())
        NodeBufferSubtree(arcSet->back().Node);

    arcSet->Add(label, node);
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeMeasureSubtree(TNode* thiz) const {
    return (size_t)thiz->Subtree()->Measure(this);
}

template <class T, class D, class S>
ui64 TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeSaveSubtree(TNode* thiz, TOutputStream& os) const {
    return thiz->Subtree()->Save(this, os);
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeBufferSubtree(TNode* thiz) {
    typedef typename TNode::TArcSet TArcSet;

    TArcSet* arcSet = dynamic_cast<TArcSet*>(thiz->Subtree());
    if (!arcSet)
        return;

    size_t bufferLength = (size_t)arcSet->Measure(this);
    TArrayWithSizeHolder<char> buffer;
    buffer.Resize(bufferLength);

    TMemoryOutput bufout(buffer.Get(), buffer.Size());

    ui64 written = arcSet->Save(this, bufout);
    YASSERT(written == bufferLength);

    arcSet->Destroy(this);
    arcSet->~TArcSet();

    typename TNode::TBufferedSubtree* bufferedArcSet = new (thiz->Subtree()) typename TNode::TBufferedSubtree;

    bufferedArcSet->Buffer.Swap(buffer);
}

template <class T, class D, class S>
size_t TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeMeasureLeafValue(TNode* thiz) const {
    if (!thiz->IsFinal())
        return 0;

    return Packer.SkipLeaf(thiz->GetPayload());
}

template <class T, class D, class S>
ui64 TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::NodeSaveLeafValue(TNode* thiz, TOutputStream& os) const {
    if (!thiz->IsFinal())
        return 0;

    size_t len = Packer.SkipLeaf(thiz->GetPayload());
    os.Write(thiz->GetPayload(), len);
    return len;
}

// TCompactTrieBuilder::TCompactTrieBuilderImpl::TNode::TArc

template <class T, class D, class S>
TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TArc::TArc(const TBlob& lbl, TNode* nd)
    : Label(lbl)
    , Node(nd)
    , LeftOffset(0)
    , RightOffset(0)
{}

template <class T, class D, class S>
ui64 TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::ArcMeasure(
                const TArc* thiz, size_t leftsize, size_t rightsize) const {
    using namespace NCompactTrie;

    size_t coresize = 2 + NodeMeasureLeafValue(thiz->Node); // 2 == (char + flags)
    size_t treesize = NodeMeasureSubtree(thiz->Node);

    if (thiz->Label.Length() > 0)
        treesize += 2 * (thiz->Label.Length() - 1);

    // Triple measurements are needed because the space needed to store the offset
    // shall be added to the offset itself. Hence three iterations.
    size_t leftoffsetsize = leftsize ? MeasureOffset(coresize + treesize) : 0;
    size_t rightoffsetsize = rightsize ? MeasureOffset(coresize + treesize + leftsize) : 0;
    leftoffsetsize = leftsize ? MeasureOffset(coresize + treesize + leftoffsetsize + rightoffsetsize) : 0;
    rightoffsetsize = rightsize ? MeasureOffset(coresize + treesize + leftsize + leftoffsetsize + rightoffsetsize) : 0;
    leftoffsetsize = leftsize ? MeasureOffset(coresize + treesize + leftoffsetsize + rightoffsetsize) : 0;
    rightoffsetsize = rightsize ? MeasureOffset(coresize + treesize + leftsize + leftoffsetsize + rightoffsetsize) : 0;

    coresize += leftoffsetsize + rightoffsetsize;
    thiz->LeftOffset = leftsize ? coresize + treesize : 0;
    thiz->RightOffset = rightsize ? coresize + treesize + leftsize : 0;

    return coresize + treesize + leftsize + rightsize;
}

template <class T, class D, class S>
ui64 TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::ArcSave(const TArc* thiz, TOutputStream& os) const {
    using namespace NCompactTrie;

    ui64 written = 0;

    size_t leftoffsetsize = MeasureOffset(thiz->LeftOffset);
    size_t rightoffsetsize = MeasureOffset(thiz->RightOffset);

    size_t labelLen = thiz->Label.Length();

    for (size_t i = 0; i < labelLen; ++i) {
        char flags = 0;

        if (i == 0) {
            flags |= (leftoffsetsize << MT_LEFTSHIFT);
            flags |= (rightoffsetsize << MT_RIGHTSHIFT);
        }

        if (i == labelLen-1) {
            if (thiz->Node->IsFinal())
               flags |= MT_FINAL;

            if (!thiz->Node->IsLast())
                flags |= MT_NEXT;
        } else {
            flags |= MT_NEXT;
        }

        os.Write(&flags, 1);
        os.Write(&thiz->Label.AsCharPtr()[i], 1);
        written += 2;

        if (i == 0) {
            written += ArcSaveOffset(thiz->LeftOffset, os);
            written += ArcSaveOffset(thiz->RightOffset, os);
        }
    }

    written += NodeSaveLeafValue(thiz->Node, os);
    written += NodeSaveSubtree(thiz->Node, os);
    return written;
}

// TCompactTrieBuilder::TCompactTrieBuilderImpl::TNode::TArcSet

template <class T, class D, class S>
typename TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode::TArcSet::iterator
                    TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode::TArcSet::Find(char ch) {
    using namespace NCompTriePrivate;
    iterator it = LowerBound(this->begin(), this->end(), ch, TCmp());

    if (it != this->end() && it->Label[0] == (unsigned char)ch) {
        return it;
    }

    return this->end();
}

template <class T, class D, class S>
void TCompactTrieBuilder<T, D, S>::TCompactTrieBuilderImpl::TNode::TArcSet::Add(const TBlob& s, TNode* node) {
    using namespace NCompTriePrivate;
    this->insert(LowerBound(this->begin(), this->end(), s[0], TCmp()), TArc(s, node));
}

// Different

//----------------------------------------------------------------------------------------------------------------------
// Minimize the trie. The result is equivalent to the original
// trie, except that it takes less space (and has marginally lower
// performance, because of eventual epsilon links).
// The algorithm is as follows: starting from the largest pieces, we find
// nodes that have identical continuations  (Daciuk's right language),
// and repack the trie. Repacking is done in-place, so memory is less
// of an issue; however, it may take considerable time.

// IMPORTANT: never try to reminimize an already minimized trie or a trie with fast layout.
// Because of non-local structure and epsilon links, it won't work
// as you expect it to, and can destroy the trie in the making.

template <class TPacker>
size_t CompactTrieMinimize(TOutputStream& os, const char *data, size_t datalength, bool verbose /*= false*/, const TPacker& packer /*= TPacker()*/) {
    using namespace NCompactTrie;
    return CompactTrieMinimizeImpl(os, data, datalength, verbose, &packer);
}

template <class TTrieBuilder>
size_t CompactTrieMinimize(TOutputStream& os, const TTrieBuilder &builder, bool verbose /*=false*/) {
    TBufferStream buftmp;
    size_t len = builder.Save(buftmp);
    return CompactTrieMinimize<typename TTrieBuilder::TPacker>(os, buftmp.Buffer().Data(), len, verbose);
}

//----------------------------------------------------------------------------------------------------------------
// Lay the trie in memory in such a way that there are less cache misses when jumping from root to leaf.
// The trie becomes about 2% larger, but the access became about 25% faster in our experiments.
// Can be called on minimized and non-minimized tries, in the first case in requires half a trie more memory.
// Calling it the second time on the same trie does nothing.
//
// The algorithm is based on van Emde Boas layout as described in the yandex data school lectures on external memory algoritms
// by Maxim Babenko and Ivan Puzyrevsky. The difference is that when we cut the tree into levels
// two nodes connected by a forward link are put into the same level (because they usually lie near each other in the original tree).
// The original paper (describing the layout in Section 2.1) is:
// Michael A. Bender, Erik D. Demaine, Martin Farach-Colton. Cache-Oblivious B-Trees
//      SIAM Journal on Computing, volume 35, number 2, 2005, pages 341-358.
// Available on the web: http://erikdemaine.org/papers/CacheObliviousBTrees_SICOMP/
// Or: Michael A. Bender, Erik D. Demaine, and Martin Farach-Colton. Cache-Oblivious B-Trees
//      Proceedings of the 41st Annual Symposium
//      on Foundations of Computer Science (FOCS 2000), Redondo Beach, California, November 12-14, 2000, pages 399-409.
// Available on the web: http://erikdemaine.org/papers/FOCS2000b/
// (there is not much difference between these papers, actually).
//
template <class TPacker>
size_t CompactTrieMakeFastLayout(TOutputStream& os, const char *data, size_t datalength, bool verbose /*= false*/, const TPacker& packer /*= TPacker()*/) {
    using namespace NCompactTrie;
    return CompactTrieMakeFastLayoutImpl(os, data, datalength, verbose, &packer);
}

template <class TTrieBuilder>
size_t CompactTrieMakeFastLayout(TOutputStream& os, const TTrieBuilder &builder, bool verbose /*=false*/) {
    TBufferStream buftmp;
    size_t len = builder.Save(buftmp);
    return CompactTrieMakeFastLayout<typename TTrieBuilder::TPacker>(os, buftmp.Buffer().Data(), len, verbose);
}
