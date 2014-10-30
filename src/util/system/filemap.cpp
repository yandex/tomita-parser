#include "info.h"
#include "defaults.h"

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>

#if defined(_win_)
    #include "winint.h"
#elif defined(_unix_)
    #include <sys/types.h>
    #include <sys/mman.h>

    #ifndef MAP_NOCORE
        #define MAP_NOCORE 0
    #endif
#else
    #error todo
#endif

#include <util/generic/utility.h>

#include "filemap.h"

#undef _unix_mmap_64_
#if defined(_unix_) && defined(_64_) && !defined(WITH_VALGRIND)
    #define _unix_mmap_64_
#endif

#undef PAGE_SIZE
#undef GRANULARITY

static inline size_t CalcGranularity() throw () {
#if defined(_win_)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwAllocationGranularity;
#else
    return NSystemInfo::GetPageSize();
#endif
}

static const size_t GRANULARITY = CalcGranularity();
static const size_t PAGE_SIZE = NSystemInfo::GetPageSize();

const char* const TMemoryMapCommon::UnknownFileName = "Unknown_file_name";

static inline i64 DownToGranularity(i64 offset) throw () {
    return offset & ~((i64)(GRANULARITY - 1));
}

// maybe we should move this function to another .cpp file to avoid unwanted optimization?
static int PrechargeImpl(TFile f, void* data, size_t dataSize, size_t off, size_t size) {
    if (off > dataSize) {
        assert(false);
        return 0;
    }
    size_t endOff = (size == (size_t)-1 ? dataSize : off + size);
    if (endOff > dataSize) {
        assert(false);
        endOff = dataSize;
    }
    size = endOff - off;
    if (dataSize == 0 || size == 0)
        return 0;

    const char *c = (char*)data + off, *e = c + size;
    int n = 0;
#ifdef _freebsd_
    if (off % PAGE_SIZE) {
        off = off / PAGE_SIZE * PAGE_SIZE; // align on PAGE_SIZE
        size = endOff - off;
        c = (char*)data + off;
    }

    madvise((void*)c, e - c, MADV_WILLNEED);
    const size_t rdSize1 = 64 << 20, rdSize2 = 4 << 20;
    const size_t rdSize = size > rdSize2 * 32? rdSize1 : rdSize2;
    TArrayHolder<char> pages(new char[(rdSize + PAGE_SIZE - 1) / PAGE_SIZE]);
    TBuffer buf(Min(rdSize, size));
    ui32 nbufs = 0, nread = 0;
    for (size_t r = 0; r < size; r += rdSize) {
        bool needRead = true;
        size_t toRead = Min(rdSize, size - r);
        if (mincore(c + r, toRead, pages.Get()) != -1)
            needRead = memchr(pages.Get(), 0, (toRead + PAGE_SIZE - 1) / PAGE_SIZE) != 0;
        if (needRead)
            f.Pread(buf.Data(), toRead, off + r);
        madvise((void*)(c + r), toRead, MADV_WILLNEED);
        for (const char *d = c; d < c + r; d += 512)
            n += *d;
        nbufs++;
        nread += needRead;
    }
    //warnx("precharge: read %u/%u (blk %" PRISZT ")", nread, nbufs, rdSize);
    return 0;
#else
    UNUSED(f);
#endif
    for (; c < e; c += 512)
        n += *c;
    return n; // prevent loop optimization
}

class TMemoryMap::TImpl: public TRefCounted<TImpl, TAtomicCounter> {
    public:
        inline void CreateMapping(const char* fileName) {
            UNUSED(fileName); // shut up warning when not (_win_ && _unix_mmap_64_)

#if defined (_win_)
            Mapping_ = NULL;
            if (Length_) {
                Mapping_ = CreateFileMapping(File_.GetHandle(), NULL,
                    (Mode_ & oAccessMask) == TFileMap::oRdOnly ? PAGE_READONLY : PAGE_READWRITE,
                    (DWORD)(Length_ >> 32), (DWORD)(Length_ & 0xFFFFFFFF), NULL);
                if (Mapping_ == NULL) {
                    ythrow yexception() << "Can't create file mapping of '" << fileName << "': " << LastSystemErrorText();
                }
            }
#elif defined(_unix_mmap_64_)
            if (!(Mode_ & oNotGreedy)) {
                PtrStart_ = mmap((caddr_t)NULL, Length_,
                    ((Mode_ & oAccessMask) == oRdOnly) ? PROT_READ : PROT_READ|PROT_WRITE,
                    MAP_SHARED | MAP_NOCORE, File_.GetHandle(), 0);
                if ((MAP_FAILED == PtrStart_) && Length_)
                    ythrow yexception() << "Can't map " << (unsigned long)Length_ << " bytes of file '" << fileName << "' at offset 0: " << LastSystemErrorText();
            } else
                PtrStart_ = 0;
#endif
        }

        inline TImpl(FILE* f, OMode om, const char* dbgName)
            : File_(Duplicate(f))
            , Length_(File_.GetLength())
            , Mode_(om)
        {
            if (!File_.IsOpen()) {
                ythrow yexception() << "TMemoryMap: FILE '" << dbgName << "' is not open, " << LastSystemErrorText();
            }

            CreateMapping(dbgName);
        }

        inline TImpl(const Stroka& name, OMode om)
            : File_(name, (om & oRdWr) ? OpenExisting | RdWr : OpenExisting | RdOnly)
            , Length_(File_.GetLength())
            , Mode_(om)
        {
            if (!File_.IsOpen()) {
                ythrow yexception() << "TMemoryMap: TFile '" << name << "' is not open, " << LastSystemErrorText();
            }

            CreateMapping(~name);
        }

        inline TImpl(const Stroka& name, i64 length, OMode om)
            : File_(name, (om & oRdWr) ? OpenExisting | RdWr : OpenExisting | RdOnly)
            , Length_(length)
            , Mode_(om)
        {
            if (!File_.IsOpen()) {
                ythrow yexception() << "TMemoryMap: TFile '" << name << "': is not open, " << LastSystemErrorText();
            }

            if (File_.GetLength() < Length_) {
                File_.Resize(Length_);
            }

            CreateMapping(~name);
        }

        inline TImpl(const TFile& file, OMode om, const char* dbgName)
            : File_(file)
            , Length_(File_.GetLength())
            , Mode_(om)
        {
            const Stroka& fileName = File_.GetName();
            const char* const name = fileName.Empty() ? dbgName : ~fileName;
            if (!File_.IsOpen()) {
                ythrow yexception() << "TMemoryMap: TFile '" << name << "' is not open, " << LastSystemErrorText();
            }

            CreateMapping(name);
        }

        inline bool IsOpen() const throw () {
            return File_.IsOpen()
#if defined (_win_)
                && Mapping_ != NULL
#endif
                ;
        }

        inline TMapResult Map(i64 offset, size_t size, const char* dbgName) {
            assert(File_.IsOpen());

            const Stroka& fileName = File_.GetName();
            const char* const name = fileName.Empty() ? dbgName : ~fileName;

            if (offset > Length_) {
                ythrow yexception() << "Can't map something at offset " << offset << " of '" << name << "' with length " << Length_;
            }

            if (offset + (i64)size > Length_) {
                ythrow yexception() << "Can't map " << (unsigned long)size << " bytes at offset " << offset << " of '" << name << "': " << LastSystemErrorText();
            }

            TMapResult result;

            i64 base = DownToGranularity(offset);
            result.Head = (i32)(offset - base);
            size += result.Head;

#if defined (_win_)
            result.Ptr = MapViewOfFile(Mapping_, (Mode_ & oAccessMask) == oRdOnly
                ? FILE_MAP_READ : FILE_MAP_WRITE, HI_32(base), LO_32(base), size);
#else
# if defined(_unix_mmap_64_)
            if (Mode_ & oNotGreedy) {
# endif
                result.Ptr = mmap((caddr_t)NULL, size, (Mode_ & oAccessMask) == oRdOnly
                    ? PROT_READ : PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NOCORE,
                    File_.GetHandle(), base);

                if (result.Ptr == (char *)(-1)) {
                    result.Ptr = 0;
                }
# if defined(_unix_mmap_64_)
            } else {
                result.Ptr = PtrStart_ ? static_cast<caddr_t>(PtrStart_) + base : NULL;
            }
# endif
#endif
            if (result.Ptr != 0 || size == 0) { // allow map of size 0
                result.Size = size;
            } else {
                ythrow yexception() << "Can't map " << (unsigned long)size << " bytes at offset " << offset << " of '" << name << "': " << LastSystemErrorText();
            }

            if (Mode_ & oPrecharge) {
                volatile int p = PrechargeImpl(File_, result.Ptr, result.Size, 0, result.Size);
                UNUSED(p);
            }

            return result;
        }

#if defined (_win_)
        inline bool Unmap(void* ptr, size_t) {
            return ::UnmapViewOfFile(ptr) != FALSE;
        }
#else
        inline bool Unmap(void* ptr, size_t size) {
#if defined(_unix_mmap_64_)
            if (Mode_ & oNotGreedy)
#endif
                return size == 0 || ::munmap(static_cast<caddr_t>(ptr), size) == 0;
#if defined(_unix_mmap_64_)
            else
                return true;
#endif
        }
#endif

        void SetSequential() {
#if defined(_unix_mmap_64_)
            if (!(Mode_ & oNotGreedy) && Length_ && madvise(PtrStart_, Length_, MADV_SEQUENTIAL))
                ythrow yexception() << "error setting sequential: " << LastSystemErrorText();
#endif
        }

        void Evict(void* ptr, size_t len) {
#if defined(_freebsd_)
            if (madvise(ptr, len, MADV_FREE))
                ythrow yexception() << "error evecting {" << ptr << ", " << len << "} with MADV_FREE: " << LastSystemErrorText();
#elif defined(_unix_)
            if (madvise(ptr, len, MADV_DONTNEED))
                ythrow yexception() << "error evecting {" << ptr << ", " << len << "} with MADV_DONTNEED: " << LastSystemErrorText();
#else
            UNUSED(ptr);
            UNUSED(len);
#endif
        }

        void Evict() {
#if defined(_unix_mmap_64_)
            Evict(PtrStart_, Length_);
#endif
        }

        inline ~TImpl() throw () {
#if defined (_win_)
            if (Mapping_) {
                ::CloseHandle(Mapping_); // != FALSE
                Mapping_ = NULL;
            }
#elif defined(_unix_mmap_64_)
            if (PtrStart_) {
                munmap((caddr_t)PtrStart_, Length_);
            }
#endif
        }

        inline i64 Length() const throw () {
            return Length_;
        }

        inline TFile GetFile() const throw () {
            return File_;
        }

    private:
        TFile File_;
        i64 Length_;
        OMode Mode_;

#if defined (_win_)
        void* Mapping_;
#elif defined(_unix_mmap_64_)
        void* PtrStart_;
#endif
};

TMemoryMap::TMemoryMap(const Stroka& name, OMode om)
    : Impl_(new TImpl(name, om))
{
}

TMemoryMap::TMemoryMap(const Stroka& name, i64 length, OMode om)
    : Impl_(new TImpl(name, length, om))
{
}

TMemoryMap::TMemoryMap(FILE* f, OMode om, const char* dbgName)
    : Impl_(new TImpl(f, om, dbgName))
{
}

TMemoryMap::TMemoryMap(const TFile& file, OMode om, const char* dbgName)
    : Impl_(new TImpl(file, om, dbgName))
{
}

TMemoryMap::~TMemoryMap() throw () {
}

TMemoryMap::TMapResult TMemoryMap::Map(i64 offset, size_t size, const char* dbgName) {
    return Impl_->Map(offset, size, dbgName);
}

bool TMemoryMap::Unmap(void* ptr, size_t size) {
    return Impl_->Unmap(ptr, size);
}

bool TMemoryMap::Unmap(TMapResult region) {
    return Unmap(region.Ptr, region.Size);
}

void TMemoryMap::SetSequential() {
    Impl_->SetSequential();
}

void TMemoryMap::Evict(void* ptr, size_t len) {
    Impl_->Evict(ptr, len);
}

void TMemoryMap::Evict() {
    Impl_->Evict();
}

i64 TMemoryMap::Length() const throw () {
    return Impl_->Length();
}

bool TMemoryMap::IsOpen() const throw () {
    return Impl_->IsOpen();
}

TFile TMemoryMap::GetFile() const throw () {
    return Impl_->GetFile();
}

TFileMap::TFileMap(const TMemoryMap& map) throw ()
    : Map_(map)
{
}

TFileMap::TFileMap(const Stroka& name, OMode om)
    : Map_(name, om)
{
}

TFileMap::TFileMap(const Stroka& name, i64 length, OMode om)
    : Map_(name, length, om)
{
}

TFileMap::TFileMap(FILE* f, OMode om)
    : Map_(f, om)
{
}

TFileMap::TFileMap(const TFile& file, OMode om)
    : Map_(file, om)
{
}

TFileMap::TFileMap(const TFileMap& fm) throw()
    : Map_(fm.Map_)
{
}

void TFileMap::Flush(void* ptr, size_t size, bool sync) {
    YASSERT(ptr >= Ptr());
    YASSERT(static_cast<char*>(ptr) + size <= static_cast<char*>(Ptr()) + MappedSize());

    if (!Region_.IsMapped()) {
        return;
    }

#if defined (_win_)
    if (sync) {
        FlushViewOfFile(ptr, size);
    }
#else
    msync(ptr, size, sync ? MS_SYNC : MS_ASYNC);
#endif
}

void TFileMap::Map(i64 offset, size_t size, const char* dbgName) {
    Unmap();
    Region_ = Map_.Map(offset, size, dbgName);
}

void TFileMap::Unmap() {
    if (!Region_.IsMapped()) {
        return;
    }

    if (Map_.Unmap(Region_)) {
        Region_.Reset();
    } else {
        ythrow yexception() << "can't unmap file";
    }
}

TFileMap::~TFileMap() throw () {
    try {
        Unmap();
    } catch (...) {
    }
}

int TFileMap::Precharge(size_t pos, size_t size) const {
    return PrechargeImpl(GetFile(), Ptr(), MappedSize(), pos, size);
}

/******************************** MappedFile ********************************/

TMappedFile::TMappedFile(TFileMap* map, const char* dbgName)
{
    Map_ = map;
    i64 len = Map_->Length();
    if (HI_32(len) != 0 && sizeof(size_t) <= sizeof(ui32))
        ythrow yexception() << "File '" << dbgName << "' mapping error: " << len << " too large";

    Map_->Map(0, static_cast<size_t>(len), dbgName);
}

TMappedFile::TMappedFile(const TFile& file, TFileMap::OMode om, const char* dbgName) : Map_(0)
{
    init(file, om, dbgName);
}

int TMappedFile::precharge(size_t off, size_t size) const {
    if (!Map_)
        return 0;

    return Map_->Precharge(off, size);
}

void TMappedFile::init(const Stroka& name, size_t length, TFileMap::OMode om)
{
    THolder<TFileMap> map(new TFileMap(name, length, om));
    TMappedFile newFile(map.Get(), ~name);
    map.Release();
    newFile.swap(*this);
    newFile.term();
}

void TMappedFile::init(const TFile& file, TFileMap::OMode om, const char* dbgName)
{
    THolder<TFileMap> map(new TFileMap(file, om));
    TMappedFile newFile(map.Get(), dbgName);
    map.Release();
    newFile.swap(*this);
    newFile.term();
}

void TMappedFile::init(const Stroka& name, TFileMap::OMode om)
{
    THolder<TFileMap> map(new TFileMap(name, om));
    TMappedFile newFile(map.Get(), ~name);
    map.Release();
    newFile.swap(*this);
    newFile.term();
}

void TMappedFile::flush()
{
    Map_->Flush();
}

TMappedAllocation::TMappedAllocation(size_t size, bool shared, void* addr)
  : Ptr_(NULL)
  , Size_(0)
  , Shared_(shared)
#if defined(_win_)
  , Mapping_(NULL)
#endif
{
    if (size != 0)
        Alloc(size, addr);
}

void* TMappedAllocation::Alloc(size_t size, void* addr)
{
    assert(Ptr_ == NULL);
#if defined(_win_)
    (void)addr;
    Mapping_ = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, size ? size : 1, NULL);
    Ptr_ = MapViewOfFile(Mapping_, FILE_MAP_WRITE, 0, 0, size ? size : 1);
#else
    Ptr_ = mmap(addr, size, PROT_READ | PROT_WRITE, (Shared_ ? MAP_SHARED : MAP_PRIVATE) | MAP_ANON, -1, 0);

    if (Ptr_ == (void*)MAP_FAILED) {
        Ptr_ = NULL;
    }
#endif
    if (Ptr_ != NULL)
        Size_ = size;
    return Ptr_;
}

void TMappedAllocation::Dealloc()
{
    if (Ptr_ == NULL)
        return;
#if defined(_win_)
    UnmapViewOfFile(Ptr_);
    CloseHandle(Mapping_);
    Mapping_ = NULL;
#else
    munmap((caddr_t)Ptr_, Size_);
#endif
    Ptr_ = NULL;
    Size_ = 0;
}

void TMappedAllocation::swap(TMappedAllocation &with)
{
    DoSwap(Ptr_, with.Ptr_);
    DoSwap(Size_, with.Size_);
#if defined(_win_)
    DoSwap(Mapping_, with.Mapping_);
#endif
}
