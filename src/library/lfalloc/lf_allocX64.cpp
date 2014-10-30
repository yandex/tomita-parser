#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
    #ifdef _M_X64
        #define _64_
    #endif
#endif
#include <intrin.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd)


#include <new>

#define PERTHREAD __declspec(thread)
#define _win_
#define FORCED_INLINE __forceinline

typedef volatile long TAtomic;

static inline long AtomicAdd(TAtomic& a, long b) {
    return _InterlockedExchangeAdd(&a, b) + b;
}

#ifndef NDEBUG
#include <assert.h>
#define YASSERT(x) { if (IsDebuggerPresent()) { if(!(x)) __debugbreak(); } else assert(x); }
#else
#define YASSERT(x) ((void)0)
#endif

#else

#include <util/system/defaults.h>
#include <util/system/atomic.h>
#include <util/system/yassert.h>

#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <memory.h>
#include <new>
#include <errno.h>

#define PERTHREAD __thread

#endif

#ifndef _darwin_

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef NDEBUG
#define DBG_FILL_MEMORY
#endif

// Buffers that are larger than this size will not be filled with 0xcf
#ifndef DBG_FILL_MAX_SIZE
#define DBG_FILL_MAX_SIZE 0x01000000000000ULL
#endif

template<class T>
inline T* DoCas(T *volatile *target, T *exchange, T *compare)
{
#ifdef _WIN32
#ifdef _64_
    return (T*)_InterlockedCompareExchange64((__int64*)target, (__int64)exchange, (__int64)compare);
#else
    //return (T*)InterlockedCompareExchangePointer(targetVoidP, exchange, compare);
    return (T*)_InterlockedCompareExchange((LONG*)target, (LONG)exchange, (LONG)compare);
#endif
#elif defined(__i386) || defined(__x86_64__)
    union
    {
        T * volatile* NP;
        void * volatile* VoidP;
    } gccSucks;
    gccSucks.NP = target;
    void * volatile* targetVoidP = gccSucks.VoidP;

    __asm__ __volatile__ (
        "lock\n\t"
        "cmpxchg %2,%0\n\t"
        : "+m" (*(targetVoidP)), "+a" (compare)
        : "r" (exchange)
        : "cc", "memory");
    return compare;
#else
#  error inline_cas not defined for this platform
#endif
}


#ifdef _64_
const uintptr_t  N_MAX_WORKSET_SIZE = 0x100000000ll * 200;
const uintptr_t  N_HUGE_AREA_FINISH = 0x700000000000ll;
#ifndef _freebsd_
char * const LINUX_MMAP_AREA_START = (char*)0x100000000ll;
static char* volatile linuxAllocPointer = LINUX_MMAP_AREA_START;
static char* volatile linuxAllocPointerHuge = LINUX_MMAP_AREA_START + N_MAX_WORKSET_SIZE;
#endif
#else
const uintptr_t  N_MAX_WORKSET_SIZE = 0xffffffff;
#endif
#define ALLOC_START ((char*)0)

const size_t N_CHUNK_SIZE = 1024 * 1024;
const size_t N_CHUNKS = N_MAX_WORKSET_SIZE / N_CHUNK_SIZE;
const size_t N_LARGE_ALLOC_SIZE = N_CHUNK_SIZE * 128;

// map size idx to size in bytes
const int N_SIZES = 25;
const int nSizeIdxToSize[N_SIZES] = {
    -1,
    8, 16, 24, 32, 48, 64, 96, 128,
    192, 256, 384, 512, 768, 1024, 1536, 2048,
    3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768
};
const size_t N_MAX_FAST_SIZE = 32768;
const unsigned char size2idxArr1[64 + 1] = {
    1,
    1, 2, 3, 4,  // 8, 16, 24, 32
    5, 5, 6, 6,  // 48, 64
    7, 7, 7, 7, 8, 8, 8, 8, // 96, 128
    9, 9, 9, 9, 9, 9, 9, 9,  10, 10, 10, 10, 10, 10, 10, 10,  // 192, 256
    11, 11, 11, 11, 11, 11, 11, 11,  11, 11, 11, 11, 11, 11, 11, 11,  // 384
    12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12   // 512
};
const unsigned char size2idxArr2[128] = {
    12, 12, 13, 14, // 512, 512, 768, 1024
    15, 15, 16, 16, // 1536, 2048
    17, 17, 17, 17, 18, 18, 18, 18, // 3072, 4096
    19, 19, 19, 19, 19, 19, 19, 19,  20, 20, 20, 20, 20, 20, 20, 20, // 6144, 8192
    21, 21, 21, 21, 21, 21, 21, 21,  21, 21, 21, 21, 21, 21, 21, 21, // 12288
    22, 22, 22, 22, 22, 22, 22, 22,  22, 22, 22, 22, 22, 22, 22, 22, // 16384
    23, 23, 23, 23, 23, 23, 23, 23,  23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23,  23, 23, 23, 23, 23, 23, 23, 23, // 24576
    24, 24, 24, 24, 24, 24, 24, 24,  24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24,  24, 24, 24, 24, 24, 24, 24, 24, // 32768
};

// map entry number to size idx
// special size idx's: 0 = not used, -1 = mem locked, but not allocated
static volatile char chunkSizeIdx[N_CHUNKS];
const int FREE_CHUNK_ARR_BUF = 0x20000; // this is effectively 128G of free memory (with 1M chunks), should not be exhausted actually
static volatile uintptr_t freeChunkArr[FREE_CHUNK_ARR_BUF];
static volatile int freeChunkCount;

static void AddFreeChunk(uintptr_t chunkId)
{
    chunkSizeIdx[chunkId] = -1;
    if (freeChunkCount == FREE_CHUNK_ARR_BUF)
        abort(); // free chunks arrray overflowed
    freeChunkArr[freeChunkCount++] = chunkId;
}

static bool GetFreeChunk(uintptr_t *res)
{
    if (freeChunkCount == 0) {
        *res = 0;
        return false;
    }
    *res = freeChunkArr[--freeChunkCount];
    return true;
}

//////////////////////////////////////////////////////////////////////////
int LFmmapAllocationErrno;
enum EMMapMode
{
    MM_NORMAL,
    MM_HUGE
};

#ifndef _MSC_VER
inline void VerifyMmapResult(void* result) {
    if (result == MAP_FAILED) {
        LFmmapAllocationErrno = errno;
        abort(); // negative size requested? or just out of mem
    }
}
#endif

#if !defined(_MSC_VER) && !defined (_freebsd_) && defined(_64_)
static char* AllocWithMMapLinuxImpl(uintptr_t sz, char *volatile* areaPtrArg, char *areaStart, uintptr_t areaFinish)
{
    char *volatile &areaPtr =*areaPtrArg;
    bool wrapped = false;
    char *largeBlock = 0;
    for(;;) {
        char *prevAllocPtr = areaPtr;
        char *nextAllocPtr = prevAllocPtr + sz;
        if (uintptr_t(nextAllocPtr - (char*)0) >= areaFinish) {
            if (wrapped) {
                // virtual memory is over fragmented
                abort();
            }
            // wrap after all area is used
            DoCas(&areaPtr, areaStart, prevAllocPtr);
            wrapped = true;
            continue;
        }

        if (DoCas(&areaPtr, nextAllocPtr, prevAllocPtr) != prevAllocPtr)
            continue;

        largeBlock = (char*)mmap(prevAllocPtr, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
        VerifyMmapResult(largeBlock);
        if (largeBlock == prevAllocPtr)
            break;
        if (largeBlock)
            munmap(largeBlock, sz);

        if (sz < 0x80000) {
            // skip utilized area with big steps
            DoCas(&areaPtr, nextAllocPtr + 0x10 * 0x10000, nextAllocPtr);
        }
    }
    return largeBlock;
}
#endif

static char* AllocWithMMap(uintptr_t sz, EMMapMode mode)
{
    (void)mode;
#ifdef _MSC_VER
    char *largeBlock = (char*)VirtualAlloc(0, sz, MEM_RESERVE, PAGE_READWRITE);
    if (uintptr_t(((char*)largeBlock - ALLOC_START) + sz) >= N_MAX_WORKSET_SIZE)
        abort(); // out of working set, something has broken
#else
#if defined (_freebsd_) || !defined(_64_)
    char *largeBlock = (char*)mmap(0, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (uintptr_t(((char*)largeBlock - ALLOC_START) + sz) >= N_MAX_WORKSET_SIZE)
        abort(); // out of working set, something has broken
    VerifyMmapResult(largeBlock);
#else
    char *largeBlock = 0;
    if (mode == MM_HUGE) {
        largeBlock = AllocWithMMapLinuxImpl(
            sz,
            &linuxAllocPointerHuge,
            LINUX_MMAP_AREA_START + N_MAX_WORKSET_SIZE,
            N_HUGE_AREA_FINISH);
    } else {
        largeBlock = AllocWithMMapLinuxImpl(
            sz,
            &linuxAllocPointer,
            LINUX_MMAP_AREA_START,
            N_MAX_WORKSET_SIZE);
        if (uintptr_t(((char*)largeBlock - ALLOC_START) + sz) >= N_MAX_WORKSET_SIZE)
            abort(); // out of working set, something has broken
    }
#endif
#endif
    if (largeBlock == 0)
        abort(); // out of memory
    return largeBlock;
}

static void FreeWithMMap(void *p, uintptr_t sz)
{
#ifdef _MSC_VER
    YASSERT(0);
#else
    munmap(p, sz);
#endif
}
//////////////////////////////////////////////////////////////////////////
const size_t LB_BUF_SIZE = 250;
const size_t LB_BUF_HASH = 5;
const int LB_LIMIT_TOTAL_SIZE = 500 * 1024 * 1024 / 4096; // do not keep more then this mem total in lbFreePtrs[]
static void * volatile lbFreePtrs[LB_BUF_HASH][LB_BUF_SIZE];
static TAtomic lbFreePageCount;

// size in pages
static size_t GetLargeBlockSize(const void *p)
{
    char *hdr = (char*)p - 4096ll;
    size_t pgCount = *(size_t*)hdr;
    return pgCount;
}
static void SetLargeBlockSize(const void *p, size_t pgCount)
{
    char *hdr = (char*)p - 4096ll;
    *(size_t*)hdr = pgCount;
}
static void *LargeBlockAlloc(size_t _nSize)
{
    size_t pgCount = (_nSize + 4095) / 4096;
#ifdef _MSC_VER
    char *pRes = (char*)VirtualAlloc(0, (pgCount + 1) * 4096ll, MEM_COMMIT, PAGE_READWRITE);
    if (pRes == 0)
        abort(); // out of memory
#else
    int lbHash = pgCount % LB_BUF_HASH;
    for (int i = 0; i < LB_BUF_SIZE; ++i) {
        void *p = lbFreePtrs[lbHash][i];
        if (p == 0)
            continue;
        if (DoCas(&lbFreePtrs[lbHash][i], (void*)0, p) == p) {
            size_t realPageCount = GetLargeBlockSize(p);
            if (realPageCount == pgCount) {
                AtomicAdd(lbFreePageCount, -pgCount);
                return p;
            } else {
                if (DoCas(&lbFreePtrs[lbHash][i], p, (void*)0) != (void*)0) {
                    // block was freed while we were busy
                    AtomicAdd(lbFreePageCount, -realPageCount);
                    FreeWithMMap((char*)p - 4096ll, (realPageCount + 1) * 4096ll);
                    --i;
                }
            }
        }
    }

    char *pRes = AllocWithMMap((pgCount + 1) * 4096ll, MM_HUGE);
#endif
    pRes += 4096ll;
    SetLargeBlockSize(pRes, pgCount);

    return pRes;
}

#ifndef _MSC_VER
static void FreeAllLargeBlockMem()
{
    for (int lbHash = 0; lbHash < LB_BUF_HASH; ++lbHash) {
        for (int i = 0; i < LB_BUF_SIZE; ++i) {
            void *p = lbFreePtrs[lbHash][i];
            if (p == 0)
                continue;
            if (DoCas(&lbFreePtrs[lbHash][i], (void*)0, p) == p) {
                int pgCount = GetLargeBlockSize(p);
                AtomicAdd(lbFreePageCount, -pgCount);
                FreeWithMMap((char*)p - 4096ll, (pgCount + 1) * 4096ll);
            }
        }
    }
}
#endif

static void LargeBlockFree(void *p)
{
    if (p == 0)
        return;
#ifdef _MSC_VER
    VirtualFree((char*)p - 4096ll, 0, MEM_RELEASE);
#else
    size_t pgCount = GetLargeBlockSize(p);

    if (lbFreePageCount > LB_LIMIT_TOTAL_SIZE)
        FreeAllLargeBlockMem();
    int lbHash = pgCount % LB_BUF_HASH;
    for (int i = 0; i < LB_BUF_SIZE; ++i) {
        if (lbFreePtrs[lbHash][i] == 0) {
            if (DoCas(&lbFreePtrs[lbHash][i], p, (void*)0) == 0) {
                AtomicAdd(lbFreePageCount, pgCount);
                return;
            }
        }
    }

    FreeWithMMap((char*)p - 4096ll, (pgCount + 1) * 4096ll);
#endif
}

static void *SystemAlloc(size_t _nSize)
{
    //HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, _nSize);
    return LargeBlockAlloc(_nSize);
}
static void SystemFree(void *p)
{
    //HeapFree(GetProcessHeap(), 0, p);
    LargeBlockFree(p);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static int * volatile nLock = 0;
static int nLockVar;
inline void RealEnterCritical(int * volatile * lockPtr)
{
    while (DoCas(lockPtr, &nLockVar, (int*)0) != 0)
        ;//pthread_yield();
}
inline void RealLeaveCritical(int * volatile * lockPtr)
{
    *lockPtr = 0;
}
class CCriticalSectionLockMMgr {
public:
    CCriticalSectionLockMMgr() { RealEnterCritical(&nLock); }
    ~CCriticalSectionLockMMgr() { RealLeaveCritical(&nLock); }
};


//////////////////////////////////////////////////////////////////////////
class TLFAllocFreeList
{
    struct TNode
    {
        TNode *Next;
    };

    TNode *volatile Head;
    TNode *volatile Pending;
    TAtomic AllocCount;
    void *Padding;

    static FORCED_INLINE void Enqueue(TNode *volatile *headPtr, TNode *n)
    {
        for(;;) {
            TNode *volatile prevHead = *headPtr;
            n->Next = prevHead;
            if (DoCas(headPtr, n, prevHead) == prevHead)
                break;
        }
    }
    FORCED_INLINE void *DoAlloc()
    {
        TNode *res;
        for (res = Head; res; res = Head) {
            if (DoCas(&Head, res->Next, res) == res)
                break;
        }
        return res;
    }
    void FreeList(TNode *fl)
    {
        if (!fl)
            return;
        TNode *flTail = fl;
        while (flTail->Next)
            flTail = flTail->Next;
        for(;;) {
            TNode *volatile prevHead = Head;
            flTail->Next = prevHead;
            if (DoCas(&Head, fl, prevHead) == prevHead)
                break;
        }
    }
public:
    FORCED_INLINE void Free(void *ptr)
    {
        TNode *newFree = (TNode*)ptr;
        if (AtomicAdd(AllocCount, 0) == 0)
            Enqueue(&Head, newFree);
        else
            Enqueue(&Pending, newFree);
    }
    FORCED_INLINE void *Alloc()
    {
        TNode *fl = Pending;
        if (AtomicAdd(AllocCount, 1) == 1) {
            // no other allocs in progress
            if (fl && DoCas(&Pending, (TNode*)0, fl) == fl) {
                // pick first element from Pending and return it
                void *res = fl;
                fl = fl->Next;
                // if there are other elements in Pending list, add them to main free list
                FreeList(fl);
                AtomicAdd(AllocCount, -1);
                return res;
            }
        }
        void *res = DoAlloc();
        AtomicAdd(AllocCount, -1);
        return res;
    }
    void *GetWholeList()
    {
        TNode *res;
        for (res = Head; res; res = Head) {
            if (DoCas(&Head, (TNode*)0, res) == res)
                break;
        }
        return res;
    }
    void ReturnWholeList(void *ptr)
    {
        while (AtomicAdd(AllocCount, 0) != 0) // theoretically can run into problems with parallel DoAlloc()
            ;//ThreadYield();
        for(;;) {
            TNode *prevHead = Head;
            if (DoCas(&Head, (TNode*)ptr, prevHead) == prevHead) {
                FreeList(prevHead);
                break;
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////
static TLFAllocFreeList globalFreeLists[N_SIZES];
static char *volatile globalCurrentPtr[N_SIZES];
static TLFAllocFreeList blockFreeList;

// globalFreeLists[] contains TFreeListGroup, each of them points up to 15 free blocks
const int FL_GROUP_SIZE = 15;
struct TFreeListGroup
{
    TFreeListGroup *Next;
    char *Ptrs[FL_GROUP_SIZE];
};
#ifdef _64_
const int FREE_LIST_GROUP_SIZEIDX = 8;
#else
const int FREE_LIST_GROUP_SIZEIDX = 6;
#endif

//////////////////////////////////////////////////////////////////////////
// find free chunks and reset chunk size so they can be reused by different sized allocations
// do not look at blockFreeList (TFreeListGroup has same size for any allocations)
static bool DefragmentMem()
{
    int *nFreeCount = (int*)SystemAlloc(N_CHUNKS * sizeof(int));
    if (!nFreeCount) {
        //__debugbreak();
        abort();
    }
    memset(nFreeCount, 0, N_CHUNKS * sizeof(int));

    TFreeListGroup *wholeLists[N_SIZES];
    for (int nSizeIdx = 0; nSizeIdx < N_SIZES; ++nSizeIdx) {
        wholeLists[nSizeIdx] = (TFreeListGroup*)globalFreeLists[nSizeIdx].GetWholeList();
        for (TFreeListGroup *g = wholeLists[nSizeIdx]; g; g = g->Next) {
            for (int i = 0; i < FL_GROUP_SIZE; ++i) {
                char *pData = g->Ptrs[i];
                if (pData) {
                    uintptr_t nChunk = (pData - ALLOC_START) / N_CHUNK_SIZE;
                    ++nFreeCount[nChunk];
                    YASSERT(chunkSizeIdx[nChunk] == nSizeIdx);
                }
            }
        }
    }

    bool bRes = false;
    for (size_t nChunk = 0; nChunk < N_CHUNKS; ++nChunk) {
        int fc = nFreeCount[nChunk];
        nFreeCount[nChunk] = 0;
        if (chunkSizeIdx[nChunk] <= 0)
            continue;
        int nEntries = N_CHUNK_SIZE / nSizeIdxToSize[static_cast<int>(chunkSizeIdx[nChunk])];
        YASSERT(fc <= nEntries); // can not have more free blocks then total count
        if (fc == nEntries) {
            bRes = true;
            nFreeCount[nChunk] = 1;
        }
    }
    if (bRes) {
        for (int nSizeIdx = 0; nSizeIdx < N_SIZES; ++nSizeIdx) {
            TFreeListGroup **ppPtr = &wholeLists[nSizeIdx];
            while (*ppPtr) {
                TFreeListGroup *g = *ppPtr;
                int dst = 0;
                for (int i = 0; i < FL_GROUP_SIZE; ++i) {
                    char *pData = g->Ptrs[i];
                    if (pData) {
                        uintptr_t nChunk = (pData - ALLOC_START) / N_CHUNK_SIZE;
                        if (nFreeCount[nChunk] == 0)
                            g->Ptrs[dst++] = pData; // block is not freed, keep pointer
                    }
                }
                if (dst == 0) {
                    // no valid pointers in group, free it
                    *ppPtr = g->Next;
                    blockFreeList.Free(g);
                } else {
                    // reset invalid pointers to 0
                    for (int i = dst; i < FL_GROUP_SIZE; ++i)
                        g->Ptrs[i] = 0;
                    ppPtr = &g->Next;
                }
            }
        }
        for (uintptr_t nChunk = 0; nChunk < N_CHUNKS; ++nChunk) {
            if (!nFreeCount[nChunk])
                continue;
            char *pStart = ALLOC_START + nChunk * N_CHUNK_SIZE;
#ifdef _win_
            VirtualFree(pStart, N_CHUNK_SIZE, MEM_DECOMMIT);
#elif defined(_freebsd_)
            madvise(pStart, N_CHUNK_SIZE, MADV_FREE);
#else
            madvise(pStart, N_CHUNK_SIZE, MADV_DONTNEED);
#endif
            AddFreeChunk(nChunk);
        }
    }

    for (int nSizeIdx = 0; nSizeIdx < N_SIZES; ++nSizeIdx)
        globalFreeLists[nSizeIdx].ReturnWholeList(wholeLists[nSizeIdx]);

    SystemFree(nFreeCount);
    return bRes;
}

static FORCED_INLINE void *LFAllocFromCurrentChunk(int nSizeIdx, int blockSize)
{
    char *volatile *pFreeArray = &globalCurrentPtr[nSizeIdx];
    while (char *newBlock = *pFreeArray) {
        char *nextFree = newBlock + blockSize;

        // check if there is space in chunk
        char *globalEndPtr = ALLOC_START + ((newBlock - ALLOC_START) & ~((uintptr_t)N_CHUNK_SIZE - 1)) + N_CHUNK_SIZE;
        if (nextFree >= globalEndPtr) {
            if (nextFree > globalEndPtr)
                break;
            nextFree = 0; // it was last block in chunk
        }
        if (DoCas(pFreeArray, nextFree, newBlock) == newBlock)
            return newBlock;
    }
    return 0;
}

static void *SlowLFAlloc(int nSizeIdx, int blockSize)
{
    CCriticalSectionLockMMgr ls;
    void *res = LFAllocFromCurrentChunk(nSizeIdx, blockSize);
    if (res)
        return res; // might happen when other thread allocated new current chunk

    for (;;) {
        uintptr_t nChunk;
        if (GetFreeChunk(&nChunk)) {
            char *newPlace = ALLOC_START + nChunk * N_CHUNK_SIZE;
#ifdef _MSC_VER
            void *pTest = VirtualAlloc(newPlace, N_CHUNK_SIZE, MEM_COMMIT, PAGE_READWRITE);
            YASSERT(pTest == newPlace);
#endif
            chunkSizeIdx[nChunk] = (char)nSizeIdx;
            globalCurrentPtr[nSizeIdx] = newPlace + blockSize;
            return newPlace;
        }

        // out of luck, try to defrag
        if (DefragmentMem())
            continue;

        char *largeBlock = AllocWithMMap(N_LARGE_ALLOC_SIZE + N_CHUNK_SIZE, MM_NORMAL);
        uintptr_t addr = largeBlock - ALLOC_START;
        addr = (addr + N_CHUNK_SIZE - 1) & (~(N_CHUNK_SIZE - 1));
        for (uintptr_t p = addr; p < addr + N_LARGE_ALLOC_SIZE; p += N_CHUNK_SIZE) {
            uintptr_t chunk = p / N_CHUNK_SIZE;
            YASSERT(chunk * N_CHUNK_SIZE == p);
            YASSERT(chunkSizeIdx[chunk] == 0);
            AddFreeChunk(chunk);
        }
    }
    return 0;
}

static FORCED_INLINE void *LFAllocNoCache(int nSizeIdx)
{
    int blockSize = nSizeIdxToSize[nSizeIdx];
    void *res = LFAllocFromCurrentChunk(nSizeIdx, blockSize);
    if (res)
        return res;

    return SlowLFAlloc(nSizeIdx, blockSize);
}

// buf should have space for at least FL_GROUP_SIZE elems
static FORCED_INLINE int GlobalGroupAlloc(int nSizeIdx, char **buf)
{
    TLFAllocFreeList &fl = globalFreeLists[nSizeIdx];
    TFreeListGroup *g = (TFreeListGroup*)fl.Alloc();
    if (g) {
        int resCount = 0;
        for (int i = 0; i < FL_GROUP_SIZE; ++i) {
            if (g->Ptrs[i])
                buf[resCount++] = g->Ptrs[i];
            else
                break;
        }
        blockFreeList.Free(g);
        return resCount;
    }
    return 0;
}

static FORCED_INLINE void GlobalGroupFree(ptrdiff_t nSizeIdx, char **buf, int count)
{
    for (int startIdx = 0; startIdx < count;) {
        TFreeListGroup *g = (TFreeListGroup*)blockFreeList.Alloc();
        YASSERT(sizeof(TFreeListGroup) == nSizeIdxToSize[FREE_LIST_GROUP_SIZEIDX]);
        if (!g)
            g = (TFreeListGroup*)LFAllocNoCache(FREE_LIST_GROUP_SIZEIDX);

        int groupSize = count - startIdx;
        if (groupSize > FL_GROUP_SIZE)
            groupSize = FL_GROUP_SIZE;
        for (int i = 0; i < groupSize; ++i)
            g->Ptrs[i] = buf[startIdx + i];
        for (int i = groupSize; i < FL_GROUP_SIZE; ++i)
            g->Ptrs[i] = 0;

        // add free group to the global list
        TLFAllocFreeList &fl = globalFreeLists[nSizeIdx];
        fl.Free(g);

        startIdx += groupSize;
    }
}


//////////////////////////////////////////////////////////////////////////
const int THREAD_BUF = 256;
static int borderSizes[N_SIZES];
const int MAX_MEM_PER_SIZE_PER_THREAD = 512 * 1024;
struct TThreadAllocInfo
{
    // FreePtrs - pointers to first free blocks in per thread block list
    // LastFreePtrs - pointers to last blocks in lists, may be invalid if FreePtr is zero
    char *FreePtrs[N_SIZES][THREAD_BUF];
    int FreePtrIndex[N_SIZES];
    TThreadAllocInfo *pNextInfo;
#ifdef _win_
    HANDLE hThread;
#endif

    void Init(TThreadAllocInfo **pHead)
    {
        memset(this, 0, sizeof(*this));
        for (int i = 0; i < N_SIZES; ++i)
            FreePtrIndex[i] = THREAD_BUF;
#ifdef _win_
        BOOL b = DuplicateHandle(
            GetCurrentProcess(), GetCurrentThread(),
            GetCurrentProcess(), &hThread,
            0, FALSE, DUPLICATE_SAME_ACCESS);
        YASSERT(b);
#endif
        pNextInfo = *pHead;
        *pHead = this;
        for (int k = 0; k < N_SIZES; ++k) {
            int maxCount = MAX_MEM_PER_SIZE_PER_THREAD / nSizeIdxToSize[k];
            if (maxCount > THREAD_BUF)
                maxCount = THREAD_BUF;
            borderSizes[k] = THREAD_BUF - maxCount;
        }
    }
    void Done()
    {
        for (int sizeIdx = 0; sizeIdx < N_SIZES; ++sizeIdx) {
            YASSERT(FreePtrIndex[sizeIdx] == THREAD_BUF);
        }
#ifdef _win_
        if (hThread)
            CloseHandle(hThread);
#endif
    }
};
PERTHREAD TThreadAllocInfo *pThreadInfo;
static TThreadAllocInfo *pThreadInfoList;

static int * volatile nLockThreadInfo = 0;
class TLockThreadListMMgr {
public:
    TLockThreadListMMgr() { RealEnterCritical(&nLockThreadInfo); }
    ~TLockThreadListMMgr() { RealLeaveCritical(&nLockThreadInfo); }
};

static void MoveSingleThreadFreeToGlobal(TThreadAllocInfo *pInfo)
{
    for (int sizeIdx = 0; sizeIdx < N_SIZES; ++sizeIdx) {
        int &freePtrIdx = pInfo->FreePtrIndex[sizeIdx];
        char **freePtrs = pInfo->FreePtrs[sizeIdx];
        GlobalGroupFree(sizeIdx, freePtrs + freePtrIdx, THREAD_BUF - freePtrIdx);
        freePtrIdx = THREAD_BUF;
    }
}

#ifdef _win_
static bool IsDeadThread(TThreadAllocInfo *pInfo)
{
    DWORD dwExit;
    bool isDead = !GetExitCodeThread(pInfo->hThread, &dwExit) || dwExit != STILL_ACTIVE;
    return isDead;
}

static void CleanupAfterDeadThreads()
{
    TLockThreadListMMgr ls;
    for (TThreadAllocInfo **p = &pThreadInfoList; *p;) {
        TThreadAllocInfo *pInfo = *p;
        if (IsDeadThread(pInfo)) {
            MoveSingleThreadFreeToGlobal(pInfo);
            pInfo->Done();
            *p = pInfo->pNextInfo;
            SystemFree(pInfo);
        } else
            p = &pInfo->pNextInfo;
    }
}
#endif

#ifndef _win_
static pthread_key_t ThreadCacheCleaner;
static void * volatile ThreadCacheCleanerStarted; // 0 = not started, -1 = started, -2 = is starting
static PERTHREAD bool IsStoppingThread;

static void FreeThreadCache(void*)
{
    TThreadAllocInfo *pToDelete = 0;
    {
        TLockThreadListMMgr ls;
        pToDelete = pThreadInfo;
        if (pToDelete == 0)
            return;

        // remove from the list
        for (TThreadAllocInfo **p = &pThreadInfoList; *p; p = &(*p)->pNextInfo) {
            if (*p == pToDelete) {
                *p = pToDelete->pNextInfo;
                break;
            }
        }
        IsStoppingThread = true;
        pThreadInfo = 0;
    }

    // free per thread buf
    MoveSingleThreadFreeToGlobal(pToDelete);
    pToDelete->Done();
    SystemFree(pToDelete);
}
#endif

static void AllocThreadInfo()
{
#ifndef _win_
    if (DoCas(&ThreadCacheCleanerStarted, (void*)-2, (void*)0) == (void*)0) {
        pthread_key_create(&ThreadCacheCleaner, FreeThreadCache);
        ThreadCacheCleanerStarted = (void*)-1;
    }
    if (ThreadCacheCleanerStarted != (void*)-1)
        return; // do not use ThreadCacheCleaner until it is constructed

    {
        TLockThreadListMMgr ls;
        if (IsStoppingThread)
            return;
        pThreadInfo = (TThreadAllocInfo*)SystemAlloc(sizeof(TThreadAllocInfo));
        pThreadInfo->Init(&pThreadInfoList);
    }
    pthread_setspecific(ThreadCacheCleaner, (void*)-1); // without value destructor will not be called
#else
    CleanupAfterDeadThreads();
    {
        TLockThreadListMMgr ls;
        pThreadInfo = (TThreadAllocInfo*)SystemAlloc(sizeof(TThreadAllocInfo));
        pThreadInfo->Init(&pThreadInfoList);
    }
#endif
}



//////////////////////////////////////////////////////////////////////////
static FORCED_INLINE void *LFAlloc(size_t _nSize)
{
    int nSizeIdx;
    if (_nSize > 512) {
        if (_nSize > N_MAX_FAST_SIZE)
            return LargeBlockAlloc(_nSize);
        nSizeIdx = size2idxArr2[(_nSize - 1) >> 8];
    } else
        nSizeIdx = size2idxArr1[1 + (((int)_nSize - 1) >> 3)];

    // check per thread buffer
    TThreadAllocInfo *thr = pThreadInfo;
    if (!thr) {
        AllocThreadInfo();
        thr = pThreadInfo;
        if (!thr) {
            return LFAllocNoCache(nSizeIdx);
        }
    }
    {
        int &freePtrIdx = thr->FreePtrIndex[nSizeIdx];
        if (freePtrIdx < THREAD_BUF)
            return thr->FreePtrs[nSizeIdx][freePtrIdx++];

        // try to alloc from global free list
        char *buf[FL_GROUP_SIZE];
        int count = GlobalGroupAlloc(nSizeIdx, buf);
        if (count > 0) {
            char **dstBuf = thr->FreePtrs[nSizeIdx] + freePtrIdx - 1;
            for (int i = 0; i < count - 1; ++i)
                dstBuf[-i] = buf[i];
            freePtrIdx -= count - 1;
            return buf[count - 1];
        }
    }
    // no blocks in free list, take from chunk
    return LFAllocNoCache(nSizeIdx);
}

#ifdef DBG_FILL_MEMORY
static void *LFAllocDbg(size_t _nSize)
{
    void *res = LFAlloc(_nSize);
    if (res && (_nSize <= DBG_FILL_MAX_SIZE)) {
        memset(res, 0xcf, _nSize);
    }
    return res;
}
#define ALLOC_FUNC LFAllocDbg
#else
#define ALLOC_FUNC LFAlloc
#endif

static FORCED_INLINE void LFFree(void *p)
{
    uintptr_t chkOffset = ((char*)p - ALLOC_START) - 1ll;
    if (chkOffset >= N_MAX_WORKSET_SIZE) {
        if (p == 0)
            return;
        LargeBlockFree(p);
        return;
    }
    uintptr_t chunk = ((char*)p - ALLOC_START) / N_CHUNK_SIZE;
    ptrdiff_t nSizeIdx = chunkSizeIdx[chunk];
    if (nSizeIdx <= 0) {
        LargeBlockFree(p);
        return;
    }
#ifdef DBG_FILL_MEMORY
    memset(p, 0xfe, nSizeIdxToSize[nSizeIdx]);
#endif
    // try to store info to per thread buf
    TThreadAllocInfo *thr = pThreadInfo;
    if (thr) {
        int &freePtrIdx = thr->FreePtrIndex[nSizeIdx];
        if (freePtrIdx > borderSizes[nSizeIdx]) {
            thr->FreePtrs[nSizeIdx][--freePtrIdx] = (char*)p;
            return;
        }

        // move several pointers to global free list
        int freeCount = FL_GROUP_SIZE;
        if (freeCount > THREAD_BUF - freePtrIdx)
            freeCount = THREAD_BUF - freePtrIdx;
        char **freePtrs = thr->FreePtrs[nSizeIdx];
        GlobalGroupFree(nSizeIdx, freePtrs + freePtrIdx, freeCount);
        freePtrIdx += freeCount;

        freePtrs[--freePtrIdx] = (char*)p;

    } else {
        AllocThreadInfo();
        GlobalGroupFree(nSizeIdx, (char**)&p, 1);
    }
}

static size_t LFGetSize(const void *p)
{
    uintptr_t chkOffset = ((char*)p - ALLOC_START);
    if (chkOffset >= N_MAX_WORKSET_SIZE) {
        if (p == 0)
            return 0;
        return GetLargeBlockSize(p) * 4096ll;
    }
    uintptr_t chunk = ((char*)p - ALLOC_START) / N_CHUNK_SIZE;
    ptrdiff_t nSizeIdx = chunkSizeIdx[chunk];
    if (nSizeIdx <= 0)
        return GetLargeBlockSize(p) * 4096ll;
    return nSizeIdxToSize[nSizeIdx];
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Output mem alloc stats
const int N_PAGE_SIZE = 4096;
static void DebugTraceMMgr(const char *pszFormat, ...) // __cdecl
{
    static char buff[20000];
    va_list va;
    //
    va_start(va, pszFormat);
    vsprintf(buff, pszFormat, va);
    va_end(va);
    //
    #ifdef _win_
    OutputDebugStringA(buff);
    #else
    fprintf(stderr, buff);
    #endif
}

struct TChunkStats
{
    char *Start, *Finish;
    int Size;
    char *Entries;
    int FreeCount;

    TChunkStats(size_t chunk, int size, char *entries)
        : Size(size)
        , Entries(entries)
        , FreeCount(0)
    {
        Start = ALLOC_START + chunk * N_CHUNK_SIZE;
        Finish = Start + N_CHUNK_SIZE;
    }
    void CheckBlock(char *pBlock)
    {
        if (pBlock && pBlock >= Start && pBlock < Finish) {
            ++FreeCount;
            ptrdiff_t nShift = pBlock - Start;
            ptrdiff_t nOffsetInStep = nShift & (N_CHUNK_SIZE - 1);
            Entries[nOffsetInStep / Size] = 1;
        }
    }
    void SetGlobalFree(char *ptr)
    {
        ptrdiff_t nShift = ptr - Start;
        ptrdiff_t nOffsetInStep = nShift & (N_CHUNK_SIZE - 1);
        while (nOffsetInStep + Size <= N_CHUNK_SIZE) {
            ++FreeCount;
            Entries[nOffsetInStep / Size] = 1;
            nOffsetInStep += Size;
        }
    }
};

static void DumpMemoryBlockUtilizationLocked()
{
    TFreeListGroup *wholeLists[N_SIZES];
    for (int nSizeIdx = 0; nSizeIdx < N_SIZES; ++nSizeIdx) {
        wholeLists[nSizeIdx] = (TFreeListGroup*)globalFreeLists[nSizeIdx].GetWholeList();
    }
    char *bfList = (char*)blockFreeList.GetWholeList();

    DebugTraceMMgr("memory blocks utilisation stats:\n");
    ptrdiff_t nTotalAllocated = 0, nTotalFree = 0, nTotalBadPages = 0, nTotalPages = 0, nTotalUsed = 0, nTotalLocked = 0;
    ptrdiff_t nTotalGroupBlocks = 0;
    char *entries;
    entries = (char*)SystemAlloc((N_CHUNK_SIZE / 4));
    for (size_t k = 0; k < N_CHUNKS; ++k) {
        if (chunkSizeIdx[k] <= 0) {
            if (chunkSizeIdx[k] == -1)
                nTotalLocked += N_CHUNK_SIZE;
            continue;
        }
        int nSizeIdx = chunkSizeIdx[k];
        int nSize = nSizeIdxToSize[nSizeIdx];
        TChunkStats cs(k, nSize, entries);
        int nEntriesTotal = N_CHUNK_SIZE / nSize;
        memset(entries, 0, nEntriesTotal);
        for (TFreeListGroup *g = wholeLists[nSizeIdx]; g; g = g->Next) {
            for (int i = 0; i < FL_GROUP_SIZE; ++i)
                cs.CheckBlock(g->Ptrs[i]);
        }
        TChunkStats csGB(k, nSize, entries);
        if (nSizeIdx == FREE_LIST_GROUP_SIZEIDX) {
            for (int nSizeIdx = 0; nSizeIdx < N_SIZES; ++nSizeIdx) {
                for (TFreeListGroup *g = wholeLists[nSizeIdx]; g; g = g->Next)
                    csGB.CheckBlock((char*)g);
            }
            for (char *blk = bfList; blk; blk = *(char**)blk)
                csGB.CheckBlock(blk);
            nTotalGroupBlocks += csGB.FreeCount * nSize;
        }
        if (((globalCurrentPtr[nSizeIdx] - ALLOC_START) / N_CHUNK_SIZE) == k)
            cs.SetGlobalFree(globalCurrentPtr[nSizeIdx]);
        nTotalUsed += (nEntriesTotal - cs.FreeCount - csGB.FreeCount) * nSize;

        char pages[N_CHUNK_SIZE / N_PAGE_SIZE];
        memset(pages, 0, sizeof(pages));
        for (int i = 0, nShift = 0; i < nEntriesTotal; ++i, nShift += nSize) {
            int nBit = 0;
            if (entries[i])
                nBit = 1; // free entry
            else
                nBit = 2; // used entry
            for (int nDelta = nSize - 1; nDelta >= 0; nDelta -= N_PAGE_SIZE)
                pages[(nShift + nDelta) / N_PAGE_SIZE] |= nBit;
        }
        int nBadPages = 0;
        for (int z = 0; z < (int)ARRAY_SIZE(pages); ++z) {
            nBadPages += pages[z] == 3;
            nTotalPages += pages[z] != 1;
        }
        DebugTraceMMgr("entry = %d; size = %d; free = %d; system %d; utilisation = %g%%, fragmentation = %g%%\n",
            k, nSize, cs.FreeCount * nSize, csGB.FreeCount * nSize,
            (N_CHUNK_SIZE - cs.FreeCount * nSize) * 100.0f / N_CHUNK_SIZE, 100.0f * nBadPages / ARRAY_SIZE(pages));
        nTotalAllocated += N_CHUNK_SIZE;
        nTotalFree += cs.FreeCount * nSize;
        nTotalBadPages += nBadPages;
    }
    SystemFree(entries);
    DebugTraceMMgr("Total allocated = %d, free = %d, system = %d, locked for future use %d, utilisation = %g, fragmentation = %g\n",
        nTotalAllocated, nTotalFree, nTotalGroupBlocks, nTotalLocked,
        100.0f * (nTotalAllocated - nTotalFree) / nTotalAllocated, 100.0f * nTotalBadPages / nTotalPages);
    DebugTraceMMgr("Total %d bytes used, %d bytes in used pages\n", nTotalUsed, nTotalPages * N_PAGE_SIZE);

    for (int nSizeIdx = 0; nSizeIdx < N_SIZES; ++nSizeIdx)
        globalFreeLists[nSizeIdx].ReturnWholeList(wholeLists[nSizeIdx]);
    blockFreeList.ReturnWholeList(bfList);
}

void FlushThreadFreeList()
{
    if (pThreadInfo)
        MoveSingleThreadFreeToGlobal(pThreadInfo);
}

void DumpMemoryBlockUtilization()
{
    // move current thread free to global lists to get better statistics
    FlushThreadFreeList();
    {
        CCriticalSectionLockMMgr ls;
        DumpMemoryBlockUtilizationLocked();
    }
}

//////////////////////////////////////////////////////////////////////////
// hooks
#if defined(USE_INTELCC) || defined(_darwin_) || defined(_freebsd_) || defined(_STLPORT_VERSION)
#define OP_THROWNOTHING throw ()
#define OP_THROWBADALLOC throw (std::bad_alloc)
#else
#define OP_THROWNOTHING
#define OP_THROWBADALLOC
#endif

#if !defined(YMAKE)
void* operator new(size_t size) OP_THROWBADALLOC {
    return ALLOC_FUNC(size);
}

void* operator new(size_t size, const std::nothrow_t&) OP_THROWNOTHING {
    return ALLOC_FUNC(size);
}

void operator delete(void* p) OP_THROWNOTHING {
    LFFree(p);
}

void operator delete(void* p, const std::nothrow_t&) OP_THROWNOTHING {
    LFFree(p);
}

void* operator new[](size_t size) OP_THROWBADALLOC {
    return ALLOC_FUNC(size);
}

void* operator new[](size_t size, const std::nothrow_t&) OP_THROWNOTHING {
    return ALLOC_FUNC(size);
}

void operator delete[](void* p) OP_THROWNOTHING {
    LFFree(p);
}

void operator delete[](void* p, const std::nothrow_t&) OP_THROWNOTHING {
    LFFree(p);
}
#endif

//#ifndef _MSC_VER
static void* SafeMalloc(size_t size) {
    return ALLOC_FUNC(size);
}

extern "C" void* malloc(size_t size) {
    return SafeMalloc(size);
}

extern "C" void* valloc(size_t size) {
    const size_t pg = N_PAGE_SIZE;
    size_t bigsize = (size + pg - 1) & (~(pg-1));
    void *p = SafeMalloc(bigsize);

    YASSERT((intptr_t)p % N_PAGE_SIZE == 0);
    return p;
}

extern "C" int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (alignment > 4096) {
#ifdef _win_
        OutputDebugStringA("Larger alignment are not guaranteed with this implementation\n");
#else
        fprintf(stderr, "Larger alignment are not guaranteed with this implementation\n");
#endif
        abort();
    }
    size_t bigsize = size;
    if (bigsize <= alignment) {
        bigsize = alignment;
    } else if (bigsize < 2 * alignment) {
        bigsize = 2 * alignment;
    }
    *memptr = SafeMalloc(bigsize);
    return 0;
}

extern "C" void* memalign(size_t alignment, size_t size) {
    void* ptr;
    int res = posix_memalign(&ptr, alignment, size);
    return res? 0 : ptr;
}

#if !defined(_MSC_VER) && !defined (_freebsd_)
// Workaround for pthread_create bug in linux.
extern "C" void* __libc_memalign(size_t alignment, size_t size) {
    return memalign(alignment, size);
}
#endif

extern "C" void free(void* ptr) {
    LFFree(ptr);
}

extern "C" void* calloc(size_t n, size_t elem_size) {
    // Overflow check
    const size_t size = n * elem_size;
    if (elem_size != 0 && size / elem_size != n) return NULL;

    void* result = SafeMalloc(size);
    if (result != NULL) {
        memset(result, 0, size);
    }
    return result;
}

extern "C" void cfree(void* ptr) {
    LFFree(ptr);
}

extern "C" void* realloc(void* old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        void* result = SafeMalloc(new_size);
        return result;
    }
    if (new_size == 0) {
        LFFree(old_ptr);
        return NULL;
    }

    void* new_ptr = SafeMalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }
    size_t old_size = LFGetSize(old_ptr);
    memcpy(new_ptr, old_ptr, ((old_size < new_size) ? old_size : new_size));
    LFFree(old_ptr);
    return new_ptr;
}

#if defined(_MSC_VER) && !defined(_DEBUG)
extern "C" size_t _msize(void* memblock) {
    return LFGetSize(memblock);
}
// MSVC runtime uses realloc, but not calloc;
// instead standard calloc and calloc_crt both call _calloc_impl.
extern "C" void* _calloc_impl(size_t num, size_t size, int* errno_tmp) {
    void* p = calloc(num, size);
    if (!p) {
        if (errno_tmp)
            *errno_tmp = ENOMEM;
    }
    return p;
}
#endif
//#endif
#endif
