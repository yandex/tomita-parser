#pragma once

#include "file.h"
#include "align.h"
#include "yassert.h"

#include <util/generic/ptr.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>

#include <new>
#include <cstdio>

struct TMemoryMapCommon {
    struct TMapResult {
        inline size_t MappedSize() const throw () {
            return Size - Head;
        }

        inline void* MappedData() const throw () {
            return Ptr ? (void*)((char*)Ptr + Head) : 0;
        }

        inline bool IsMapped() const throw () {
            return Ptr != 0;
        }

        inline void Reset() throw () {
            Ptr = 0;
            Size = 0;
            Head = 0;
        }

        void* Ptr;
        size_t Size;
        i32 Head;

        TMapResult(void) throw () {
            Reset();
        }
    };

    enum OMode {
        oRdOnly = 1,
        oRdWr = 2,

        oAccessMask = 3,
        oNotGreedy = 8,
        oPrecharge = 16,
    };

    /**
      * Name that will be printed in exceptions if not specifed.
      * Overridden by name obtained from TFile if it's not empty.
      **/
    static const char* const UnknownFileName;
};

class TMemoryMap: public TMemoryMapCommon {
    public:
        explicit TMemoryMap(const Stroka& name, OMode om = oRdOnly);
        TMemoryMap(const Stroka& name, i64 length, OMode om = oRdOnly);
        TMemoryMap(FILE* f, OMode om = oRdOnly, const char* dbgName = UnknownFileName);
        TMemoryMap(const TFile& file, OMode om = oRdOnly, const char* dbgName = UnknownFileName);

        ~TMemoryMap() throw ();

        TMapResult Map(i64 offset, size_t size, const char* dbgName = UnknownFileName);
        bool Unmap(TMapResult region);

        i64 Length() const throw ();
        bool IsOpen() const throw ();
        TFile GetFile() const throw ();

        void SetSequential();
        void Evict(void* ptr, size_t len);
        void Evict();

        /*
         * deprecated
         */
        bool Unmap(void* ptr, size_t size);

    private:
        class TImpl;
        TSimpleIntrusivePtr<TImpl> Impl_;
};

class TFileMap: public TMemoryMapCommon {
    public:
        TFileMap(const TMemoryMap& map) throw ();
        TFileMap(const Stroka& name, OMode om = oRdOnly);
        TFileMap(const Stroka& name, i64 length, OMode om = oRdOnly);
        TFileMap(FILE* f, OMode om = oRdOnly);
        TFileMap(const TFile& file, OMode om = oRdOnly);
        TFileMap(const TFileMap& fm) throw ();

        ~TFileMap() throw ();

        void Map(i64 offset, size_t size, const char* dbgName = UnknownFileName);
        void Unmap();

        void Flush(void* ptr, size_t size) {
            Flush(ptr, size, true);
        }

        void Flush() {
            Flush(Ptr(), MappedSize());
        }

        void FlushAsync(void* ptr, size_t size) {
            Flush(ptr, size, false);
        }

        void FlushAsync() {
            FlushAsync(Ptr(), MappedSize());
        }

        inline i64 Length() const throw () {
            return Map_.Length();
        }

        inline bool IsOpen() const throw () {
            return Map_.IsOpen();
        }

        inline void* Ptr() const throw () {
            return Region_.MappedData();
        }

        inline size_t MappedSize() const throw () {
            return Region_.MappedSize();
        }

        TFile GetFile() const throw () {
            return Map_.GetFile();
        }

        int Precharge(size_t pos = 0, size_t size = (size_t)-1) const;

    private:
        void Flush(void* ptr, size_t size, bool sync);

        TMemoryMap Map_;
        TMapResult Region_;
};

/// Deprecated (by pg@), use TFileMap or TMemoryMap instead
class TMappedFile {
private:
    TFileMap* Map_;

private:
    TMappedFile(TFileMap* map, const char *dbgName);

public:
    TMappedFile() {
        Map_ = NULL;
    }

    ~TMappedFile() {
        term();
    }

    explicit TMappedFile(const Stroka& name) {
        Map_ = NULL;
        init(name);
    }

    TMappedFile(const TFile& file, TFileMap::OMode om = TFileMap::oRdOnly, const char* dbgName = "unknown");

    void init(const Stroka& name, TFileMap::OMode om = TFileMap::oRdOnly);

    void init(const TFile&, TFileMap::OMode om = TFileMap::oRdOnly, const char* dbgName = "unknown");

    void init(const Stroka& name, size_t length, TFileMap::OMode om = TFileMap::oRdOnly);

    void flush();

    void term() {
        if (Map_) {
            Map_->Unmap();
            delete Map_;
            Map_ = NULL;
        }
    }

    size_t getSize() const {
        return (Map_ ? Map_->MappedSize() : 0);
    }

    void* getData(size_t pos = 0) const {
        YASSERT(!Map_ || (pos <= getSize()));
        return (Map_ ? (void *)((unsigned char *)Map_->Ptr() + pos) : NULL);
    }

    int precharge(size_t pos = 0, size_t size = (size_t)-1) const;

    void swap(TMappedFile &file) throw () {
        DoSwap(Map_, file.Map_);
    }
};

template<class T>
class TFileMappedArray {
private:
    const T* Ptr_;
    const T* End_;
    size_t Size_;
    char DummyData_[sizeof(T) + PLATFORM_DATA_ALIGN];
    mutable THolder<T, TDestructor> Dummy_;
    THolder<TFileMap> DataHolder_;
public:
    TFileMappedArray()
        : Ptr_(0)
        , End_(0)
        , Size_(0)
    {
    }
    ~TFileMappedArray() {
        Ptr_ = 0;
        End_ = 0;
    }
    void Init(const char* name) {
        DataHolder_.Reset(new TFileMap(name));
        DoInit(name);
    }
    void Init(const TFileMap& fileMap) {
        DataHolder_.Reset(new TFileMap(fileMap));
        DoInit(fileMap.GetFile().GetName());
    }
    /// deprecated, use Init(const char* name) instead
    void init(const char* name) {
        Init(name);
    }
    /// deprecated, use Term() instead
    void term() {
        Term();
    }
    void Term() {
        DataHolder_.Destroy();
        Ptr_ = 0;
        Size_ = 0;
        End_ = 0;
    }
    /// deprecated, use Precharge() instead
    void precharge() {
        Precharge();
    }
    void Precharge() {
        DataHolder_->Precharge();
    }
    const T &operator[](size_t pos) const {
        YASSERT(pos < size());
        return Ptr_[pos];
    }
    /// for STL compatibility only, Size() usage is recommended
    size_t size() const {
        return Size_;
    }
    size_t Size() const {
        return Size_;
    }
    const T& GetAt(size_t pos) const {
        if (pos < Size_)
            return Ptr_[pos];
        return Dummy();
    }
    void SetDummy(const T &n_Dummy) {
        Dummy_.Destroy();
        Dummy_.Reset(new (DummyData()) T(n_Dummy));
    }
    inline char* DummyData() const throw () {
        return AlignUp((char*)DummyData_);
    }
    inline const T& Dummy() const {
        if (!Dummy_) {
            Dummy_.Reset(new (DummyData()) T());
        }

        return *Dummy_;
    }
    /// for STL compatibility only, Empty() usage is recommended
    bool empty() const throw () {
        return Empty();
    }
    bool Empty() const throw () {
        return 0 == Size_;
    }
    /// for STL compatibility only, Begin() usage is recommended
    const T* begin() const throw () {
        return Begin();
    }
    const T* Begin() const throw () {
        return Ptr_;
    }
    /// for STL compatibility only, End() usage is recommended
    const T* end() const throw () {
        return End_;
    }
    const T* End() const throw () {
        return End_;
    }
private:
    void DoInit(const Stroka& fileName) {
        DataHolder_->Map(0, DataHolder_->Length());
        if (DataHolder_->Length() % sizeof(T)) {
            Term();
            ythrow yexception() << "Incorrect size of file " << fileName.Quote();
        }
        Ptr_ = (const T*)DataHolder_->Ptr();
        Size_ = DataHolder_->Length() / sizeof(T);
        End_ = Ptr_ + Size_;
    }
};

class TMappedAllocation {
public:
    TMappedAllocation(size_t size = 0, bool shared = false, void* addr = 0);
    ~TMappedAllocation() {
        Dealloc();
    }
    void* Alloc(size_t size, void* addr = 0);
    void Dealloc();
    void* Ptr() const {
        return Ptr_;
    }
    char* Data(ui32 pos = 0) const {
        return (char*)(Ptr_ ? ((char*)Ptr_ + pos) : 0);
    }
    char* Begin() const throw () {
        return (char*)Ptr();
    }
    char* End() const throw () {
        return Begin() + MappedSize();
    }
    size_t MappedSize() const {
        return Size_;
    }
    void swap(TMappedAllocation &with);
private:
    void* Ptr_;
    size_t Size_;
    bool Shared_;
#ifdef _win_
    void* Mapping_;
#endif
    DECLARE_NOCOPY(TMappedAllocation);
};

template<class T>
class TMappedArray : private TMappedAllocation {
public:
    TMappedArray(size_t siz = 0)
      : TMappedAllocation(0) {
        if (siz)
            Create(siz);
    }
    ~TMappedArray() {
        Destroy();
    }
    T *Create(size_t siz) {
        YASSERT(MappedSize() == 0 && Ptr() == 0);
        T* arr = (T*)Alloc((sizeof(T) * siz));
        if (!arr)
            return 0;
        YASSERT(MappedSize() == sizeof(T) * siz);
        for (size_t n = 0; n < siz; n++)
            new(&arr[n]) T();
        return arr;
    }
    void Destroy() {
        T* arr = (T*)Ptr();
        if (arr) {
            for (size_t n = 0; n < size(); n++)
                arr[n].~T();
            Dealloc();
        }
    }
    T &operator[](size_t pos) {
        YASSERT(pos < size());
        return ((T*)Ptr())[pos];
    }
    const T &operator[](size_t pos) const {
        YASSERT(pos < size());
        return ((T*)Ptr())[pos];
    }
    T* begin(){
        return (T*)Ptr();
    }
    T* end(){
        return (T*)((char*)Ptr() + MappedSize());
    }
    size_t size() const {
        return MappedSize()/sizeof(T);
    }
    void swap(TMappedArray<T> &with) {
        TMappedAllocation::swap(with);
    }
    DECLARE_NOCOPY(TMappedArray);
};
