#pragma once

#include <library/iter/indexed_container.h>

#include <util/generic/intrlist.h>

namespace NGzt {

template <typename TTrie, typename TWord>
class TSearchTree {
    typedef typename TTrie::TSymbol TWordId;

    struct TLinkedWord: public TIntrusiveSListItem<TLinkedWord> {
        TWord Word;
        size_t MultitokenSize;      // number of sub-tokens

        TLinkedWord()
            : MultitokenSize(1)
        {
        }

        void Reset(const TWord& word) {
            Word = word;
            MultitokenSize = 1;
            TIntrusiveSListItem<TLinkedWord>::SetNext(NULL);
        }
    };

    typedef TIntrusiveSList<TLinkedWord> TWordList;       // words are linked in reverse order: from leaf to root
    typedef typename TWordList::TConstIterator TWordListIter;


public:
    // a sequence of words ending with a trie with possible further phrase tails
    class TBranch {
    public:
        inline TBranch(const TTrie& trie)
            : Tails(trie)
            , Size(0)
        {
        }

        inline TBranch(const TBranch& parent, const TTrie& trie)
            : Tails(trie)
            , Words(parent.Words)
            , Size(parent.Size)
        {
        }

        inline void Append(TLinkedWord* word) {
            YASSERT(word != NULL);
            Words.PushFront(word);
            Size += 1;
        }

        TLinkedWord& LastWord() {
            return *Words.Begin();
        }

        inline size_t Length() const {
            return Size;
        }

        inline bool GetFinalData(typename TTrie::TData& data) const {
            TWordId empty = 0;
            return Tails.Find(&empty, 0, &data);
        }

        // Also, builds a mapping from full-sized single-level key sequence to input sequence (with possible multitokens)
        void CopyTo(yvector<TWord>& words, yvector<size_t>& indexes) const {
            words.resize(Length());
            size_t fullSize = 0;
            typename yvector<TWord>::reverse_iterator vecIt = words.rbegin();
            for (TWordListIter listIt = Words.Begin(); listIt != Words.End(); ++listIt, ++vecIt) {
                *vecIt = listIt->Word;
                fullSize += listIt->MultitokenSize;
            }
            YASSERT(vecIt == words.rend());
            if (EXPECT_TRUE(fullSize == words.size()))
                indexes.clear();
            else
                BuildIndexTable(fullSize, indexes);
        }

        inline bool FindTails(TWordId id, TTrie& tails) const {
            return Tails.FindTails(id, tails);
        }

    private:
        void BuildIndexTable(size_t fullSize, yvector<size_t>& indexes) const {
            indexes.resize(fullSize);
            size_t last = Length();
            yvector<size_t>::reverse_iterator vecIt = indexes.rbegin();
            for (TWordListIter listIt = Words.Begin(); listIt != Words.End(); ++listIt) {
                --last;
                for (size_t i = 0; i < listIt->MultitokenSize; ++i, ++vecIt)
                    *vecIt = last;
            }
            YASSERT(last == 0 && vecIt == indexes.rend());
        }

    private:
        TTrie Tails;
        TWordList Words;
        size_t Size;
    };

public:
    TSearchTree(const TTrie* root, size_t initialCapacity = 256)
        : Pool(sizeof(TLinkedWord)*initialCapacity)
        , UsedWords(0)
        , Root(root)
    {
    }

    ~TSearchTree() {
        Reset(NULL);
        DestroyWords();
    }

    void Reset(const TTrie* root) {
        Branches.clear();
        NextBranches.clear();
        UsedWords = 0;
        Root = root;
    }

    inline void AppendRoot() {
        YASSERT(Root != NULL);
        // add empty branch to search starting from root
        Branches.push_back(TBranch(*Root));
    }

    void TryAppendWord(TWordId id, const TWord& word) {
        for (size_t i = 0; i < Branches.size(); ++i)
            if (Branches[i].FindTails(id, TmpTrie))
                // add new branch to NextState
                AddNextStateNewBranch(Branches[i], TmpTrie, word);
    }

    template <typename TSubtokenIter>
    bool TryAppendMultitoken(TSubtokenIter ids, const TWord& word) {
        size_t startSize = NextBranches.size();
        for (size_t i = 0; i < Branches.size(); ++i)
            TryAppendMultitoken(Branches[i], ids, word);    // make a copy of ids iterator
        return NextBranches.size() != startSize;
    }

    inline void SwitchNextState() {
        Branches.swap(NextBranches);
        NextBranches.clear();
    }

    typedef NIter::TIndexedContainerIterator<const TBranch, const yvector<TBranch>, false> TBranchIterator;
    inline TBranchIterator IterBranches() const {
        // returned iterator remains valid after insertions into Branches
        return TBranchIterator(Branches);
    }

protected:
    inline TLinkedWord* NewWord(const TWord& word) {
        TLinkedWord* ret = (UsedWords < AllWords.size()) ? AllWords[UsedWords] : AllocateNew();
        ++UsedWords;
        ret->Reset(word);
        return ret;
    }

private:
    template <typename TSubtokenIter>
    void TryAppendMultitoken(const TBranch& branch, TSubtokenIter ids, const TWord& word) {
        YASSERT(ids.Ok());
        if (branch.FindTails(*ids, TmpTrie)) {
            size_t size = 1;
            for (++ids; ids.Ok(); ++ids)
                if (TmpTrie.FindTails(*ids, TmpTrie))
                    ++size;
                else
                    return;

            // add new branch to NextState
            TBranch& newBranch = AddNextStateNewBranch(branch, TmpTrie, word);
            newBranch.LastWord().MultitokenSize = size;
        }
    }

    // Remove some longest branches so that no more than @goalSize branches remain
    void ClearLongestBranches(size_t goalSize) {
        // find longest branch length
        size_t maxLen = 0, maxLenCount = 0;
        for (size_t i = 0; i < NextBranches.size(); ++i) {
            if (NextBranches[i].Length() > maxLen) {
                maxLen = NextBranches[i].Length();
                maxLenCount = 1;
            } else if (NextBranches[i].Length() == maxLen) {
                ++maxLenCount;
            }
        }

        size_t dropLen = maxLen;
        size_t dropCount = maxLenCount;
        while (NextBranches.size() > goalSize + dropCount) {
            dropLen /= 2;
            dropCount = 0;
            for (size_t i = 0; i < NextBranches.size(); ++i)
                if (NextBranches[i].Length() >= dropLen)
                    ++dropCount;
        }

        size_t clearedSize = NextBranches.size() - dropCount;
        size_t survive = clearedSize < goalSize ? goalSize - clearedSize : 0;
        for (size_t i = 0; i < NextBranches.size(); ++i) {
            if (NextBranches[i].Length() >= dropLen) {
                if (survive > 0)
                    --survive;
                else
                    continue;
            }
            TmpBranches.push_back(NextBranches[i]);
        }

        NextBranches.clear();
        NextBranches.swap(TmpBranches);
    }

    inline TBranch& AddNextStateNewBranch(const TBranch& parent, const TTrie& trie, const TWord& word) {
        // fast hack: limit number of variants to avoid combinatorial explosion
        // TODO: make it smarter (e.g. store variants more compactly)
        const size_t MAX_BRANCHES = 10000;
        if (EXPECT_FALSE(NextBranches.size() > MAX_BRANCHES)) {
            //ythrow yexception() << "Max number of lemma combinations exceeded during gazetteer search.";
            // remove 90% longest branches, so that we are still able to find shorter keys
            ClearLongestBranches(MAX_BRANCHES/10);
        }

        NextBranches.push_back(TBranch(parent, trie));
        TBranch& branch = NextBranches.back();
        branch.Append(NewWord(word));
        return branch;
    }

    inline TLinkedWord* AllocateNew() {
        TLinkedWord* ret = Pool.New<TLinkedWord>();
        AllWords.push_back(ret);
        return ret;
    }

    inline void DestroyWords() {
        for (size_t i = 0; i < AllWords.size(); ++i)
            AllWords[i]->~TLinkedWord();
    }

private:
    TMemoryPool Pool;               // owns all words
    yvector<TLinkedWord*> AllWords; // all words, allocated via Pool
    size_t UsedWords;               // number of words from AllWords, used in current iteration (between two Reset())

    DECLARE_NOCOPY(TSearchTree);

protected:
    const TTrie* Root;
    TTrie TmpTrie;
    yvector<TBranch> Branches, NextBranches, TmpBranches;
};


// appends next words directly to Branches, without removing old (current) state branches
// maintains an index of branches having the specified end position
template <typename TTrie, typename TWord>
class TFullSearchTree: private TSearchTree<TTrie, TWord> {
    typedef TSearchTree<TTrie, TWord> TBase;
public:
    TFullSearchTree(const TTrie* root)
        : TBase(root)
    {
    }

    void Reset(const TTrie* root) {
        ClearIndex();
        TBase::Reset(root);
    }

    template <typename TWordIter>
    void TryAppendSequence(TWordId id, TWordIter words, size_t startIndex, size_t wordCount) {
        YASSERT(TBase::Root != NULL);
        size_t nextWordIndex = startIndex + wordCount;
        // search @id from root
        const TBranch rootBranch(*TBase::Root);
        TryAppendTo(rootBranch, id, words, nextWordIndex);

        if (startIndex < PositionIndex.size()) {
            //const TIndexVector& indices = PositionIndex[startIndex];  -- never do it again, this reference becomes invalid aftex PositionIndex resize.
            for (size_t i = 0; i < PositionIndex[startIndex].size(); ++i)
                TryAppendTo(TBase::Branches[PositionIndex[startIndex][i]], id, words, nextWordIndex);
        }
    }

    typedef typename TBase::TBranch TBranch;
    typedef typename TBase::TBranchIterator TBranchIterator;
    using TBase::IterBranches;

private:
    template <typename TWordIter>
    inline void TryAppendTo(const TBranch& srcBranch, TWordId id, TWordIter words, size_t nextWordIndex) {
        if (srcBranch.FindTails(id, TBase::TmpTrie)) {
            size_t branchIndex = TBase::Branches.size();
            CopyAndAppend(srcBranch, TBase::TmpTrie, words);
            AddToIndex(branchIndex, nextWordIndex);
        }
    }

    template <typename TWordIter>
    inline void CopyAndAppend(const TBranch& srcBranch, const TTrie& trie, TWordIter words) {
        TBase::Branches.push_back(TBranch(srcBranch, trie));
        TBranch& newBranch = TBase::Branches.back();
        for (; words.Ok(); ++words)
            newBranch.Append(this->NewWord(*words));
    }

    inline void AddToIndex(size_t branchIndex, size_t nextWordIndex) {
        if (PositionIndex.size() <= nextWordIndex)
            PositionIndex.resize(nextWordIndex + 1);
        PositionIndex[nextWordIndex].push_back(branchIndex);
    }

    inline void ClearIndex() {
        for (size_t i = 0; i < PositionIndex.size(); ++i)
            PositionIndex[i].clear();
    }

private:
    typedef yvector<size_t> TIndexVector;
    yvector<TIndexVector> PositionIndex;     // branches ordered by NextWordIndex
};


}   // namespace NGzt

