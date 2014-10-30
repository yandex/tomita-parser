#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/intrlist.h>
#include <util/generic/ylimits.h>

#include <util/memory/pool.h>


#include "bytesize.h"


namespace NGzt {

template <typename TKey, typename TIndex, typename THasher = THash<TKey>, typename TKeyCollection = yvector<TKey> >
class THashIndex {
public:
    THashIndex(const TKeyCollection& directKeys)
        : Keys(directKeys)
        , SizeData()
        , Buckets(SizeData.BucketCount())
    {
    }

    ~THashIndex() {
        Clear();
    }

    size_t Size() const {
        return SizeData.Size();
    }

    bool Insert(const TKey& key, TIndex& index) {
        size_t hash = GetHash(key);
        TBucket* bucket = &GetBucket(hash);
        for (typename TBucket::TIterator it(*bucket); it.Ok(); ++it)
            if (Keys[*it] == key) {
                index = *it;
                return false;   // didn't insert, @key is already in
            }
        ++SizeData;
        if (SizeData.BucketCount() > Buckets.size()) {
            RearrangeBuckets();
            bucket = &GetBucket(hash);
        }
        bucket->PushBack(index, ChunkPool);
        return true;            // did insert @key
    }

    void Clear() {
        for (size_t i = 0; i < Buckets.size(); ++i)
            while (!Buckets[i].Empty())
                ChunkPool.PushFree(Buckets[i].PopFront());
        SizeData.Resize(0);
    }

    // for debugging - estimation of used memory
    size_t ByteSize() const {
        return ChunkPool.ByteSize() + ::ByteSize(Buckets);
    }

private:
    class TChunkPool;

    class TChunk: public TIntrusiveSListItem<TChunk> {
    public:
        static const size_t CHUNK_SIZE = 16;
        TChunk()
            : Left_(CHUNK_SIZE)
        {
        }

        void Reset() {
            Left_ = CHUNK_SIZE;
            this->SetNext(NULL);
        }

        TChunk* PushBack(TIndex idx, TChunkPool& pool) {
            TChunk* chunk = this;
            if (Left_ == 0) {
                chunk = pool.New();
                this->SetNext(chunk);
            }
            chunk->Data[CHUNK_SIZE - chunk->Left_] = idx;
            --chunk->Left_;
            return chunk;
        }

        const TIndex* Begin() const {
            return Data;
        }

        const TIndex* End() const {
            return Data + CHUNK_SIZE - Left_;
        }

    private:
        TIndex Data[CHUNK_SIZE];
        size_t Left_;
    };

    class TChunkPool {
    public:
        TChunkPool()
            : Pool(1024)
        {
        }

        ~TChunkPool() {
            while (!FreeChunks.Empty())
                FreeChunks.PopFront()->~TChunk();
        }

        TChunk* New() {
            if (FreeChunks.Empty())
                return Pool.New<TChunk>();
            else
                return PopFree();
        }

        void PushFree(TChunk* chunk) {
            FreeChunks.PushFront(chunk);
        }

        size_t ByteSize() const {
            return ::ByteSize(Pool);
        }

    private:
        TChunk* PopFree() {
            TChunk* ret = FreeChunks.PopFront();
            ret->Reset();
            return ret;
        }

    private:
        TMemoryPool Pool;
        TIntrusiveSList<TChunk> FreeChunks;
    };

    class TBucket {
    public:
        TBucket()
            : First(NULL)
            , Last(NULL)
        {
        }

        bool Empty() const {
            return First == NULL;
        }

        void PushBack(TIndex idx, TChunkPool& pool) {
            if (Empty())
                First = Last = pool.New();
            Last = Last->PushBack(idx, pool);
        }

        TChunk* PopFront() {
            TChunk* ret = First;
            First = First->Next()->Node();
            return ret;
        }

        size_t ByteSize() const {
            return sizeof(TBucket);
        }

        class TIterator {
        public:
            TIterator(const TBucket& bucket)
                : Chunk(bucket.First)
            {
                StartChunk();
            }

            inline bool Ok() const {
                return Chunk != NULL;
            }

            void operator++() {
                YASSERT(Ok());
                ++Cur;
                if (Cur == End) {
                    Chunk = Chunk->Next()->Node();
                    StartChunk();
                }
            }

            TIndex operator*() const {
                return *Cur;
            }

        private:
            void StartChunk() {
                if (Chunk != NULL) {
                    Cur = Chunk->Begin();
                    End = Chunk->End();
                    YASSERT(Cur != End);    // chunk is never empty
                } else {
                    Cur = NULL;
                    End = NULL;
                }
            }

        private:
            const TChunk* Chunk;
            const TIndex* Cur;
            const TIndex* End;
        };

    private:
        TChunk* First;
        TChunk* Last;
    };


    class TPrimeSize {
    public:
        TPrimeSize()
            : FirstPrime(__y_prime_list)                                    // using primes table from util/generic/hash.h
            , LastPrime(__y_prime_list + ARRAY_SIZE(__y_prime_list) - 1)    // TODO: make a copy
            , Prime(FirstPrime)
            , Size_(0)
        {
        }

        size_t BucketCount() const {
            return *Prime;
        }

        size_t Size() const {
            return Size_;
        }

        void Resize(size_t n) {
            Size_ = n;
            while (Prime < LastPrime && *(Prime + 1) <= Size_)
                ++Prime;
            YASSERT(FirstPrime <= Prime && Prime <= LastPrime);
        }

        void operator++() {
            Resize(Size_ + 1);
        }

    private:
        const unsigned long* FirstPrime;
        const unsigned long* LastPrime;
        const unsigned long* Prime;
        size_t Size_;
    };

private:
    size_t GetHash(const TKey& key) const {
        return THasher()(key);
    }

    TBucket& GetBucket(size_t hash) {
        return Buckets[hash % Buckets.size()];
    }

    TIntrusiveSList<TChunk> ExtractChunks() {
        TIntrusiveSList<TChunk> chunks;
        for (size_t i = 0; i < Buckets.size(); ++i)
            while (!Buckets[i].Empty())
                chunks.PushFront(Buckets[i].PopFront());
        return chunks;
    }

    void RearrangeBuckets() {
        TIntrusiveSList<TChunk> chunks = ExtractChunks();
        Buckets.resize(SizeData.BucketCount());
        while (!chunks.Empty()) {
            TChunk* chunk = chunks.PopFront();
            const TIndex* end = chunk->End();
            for (const TIndex* idx = chunk->Begin(); idx < end; ++idx) {
                TBucket& bucket = GetBucket(GetHash(Keys[*idx]));
                bucket.PushBack(*idx, ChunkPool);
            }
            ChunkPool.PushFree(chunk);
        }
    }

private:
    const TKeyCollection& Keys;         // TIndex -> TKey
    TChunkPool ChunkPool;
    TPrimeSize SizeData;
    yvector<TBucket> Buckets;
};


template <typename T, bool Uniq = true, typename THasher = THash<T> >
class TIndexator {
public:
    TIndexator()
        : Storage()
        , Index(Storage)
    {
    }

    size_t Add(T& item) {
        size_t idx = Size();
        if (!Uniq || Index.Insert(item, idx)) {
            Storage.push_back();
            ::DoSwap(Storage.back(), item);
        }
        return idx;
    }

    size_t Size() const {
        return Storage.size();
    }

    bool Empty() const {
        return Size() == 0;
    }

    const T& operator[](size_t i) const {
        return Storage[i];
    }

    T& operator[](size_t i) {
        return Storage[i];
    }

    const T& Back() const {
        return Storage.back();
    }

    T& Back() {
        return Storage.back();
    }

    const yvector<T>& Items() const {
        return Storage;
    }

    size_t ByteSize() const {
        return ::ByteSize(Storage) + ::ByteSize(Index);
    }

    Stroka DebugByteSize() const {
        TStringStream str;
        str << "TIndexator:\n" << DEBUG_BYTESIZE(Storage) << DEBUG_BYTESIZE(Index);
        return str;
    }

private:
    yvector<T> Storage;
    THashIndex<T, size_t, THasher> Index;
};


} // namespace NGzt

