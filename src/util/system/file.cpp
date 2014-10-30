#include "file.h"
#include "flock.h"

#include <util/string/util.h>
#include <util/string/cast.h>
#include <util/random/random.h>
#include <util/generic/stroka.h>
#include <util/generic/ylimits.h>
#include <util/generic/yexception.h>
#include <util/system/fstat.h>
#include <util/system/sysstat.h>
#include <util/datetime/base.h>

#include <errno.h>

#if defined(_unix_)
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
#elif defined(_win_)
    #include "winint.h"
    #include "fs_win.h"
    #include <io.h>
#endif

#if defined(_linux_)
    #define HAVE_POSIX_FADVISE 1
#elif defined(__FreeBSD__) && !defined(WITH_VALGRIND)
    #include <sys/param.h>
    #define HAVE_POSIX_FADVISE (__FreeBSD_version >= 900501)
#else
    #define HAVE_POSIX_FADVISE 0
#endif

#if HAVE_POSIX_FADVISE
    #include <dlfcn.h>
    #include <util/generic/singleton.h>

    typedef int (*TPosixFadvisePtr) (int, off_t, off_t, int);

    namespace {
        struct TPosixFadvise {
            TPosixFadvisePtr posix_fadvise;

            TPosixFadvise() {
                posix_fadvise = (TPosixFadvisePtr)dlsym(RTLD_NEXT, "posix_fadvise");

#if defined(_musl_)
                if (!posix_fadvise) {
                    posix_fadvise = ::posix_fadvise;
                }
#endif
            }
        };
    }
#endif

// Maximum amount of bytes to be read or written via single system call.
// Some libraries fail when it is greater than max int.
// FreeBSD has performance loss when it is greater than 1GB.
const size_t MaxPortion = size_t(1 << 30);

TFileHandle::TFileHandle(const Stroka& fName, ui32 oMode) throw () {
    ui32 fcMode = 0;
    OpenMode createMode = (OpenMode)(oMode & MaskCreation);
    if ((oMode & MaskRW) == 0)
        oMode |= RdWr;
    if ((oMode & AMask) == 0)
        oMode |= ARW;

#ifdef _win_

    switch (createMode) {
        case OpenExisting:
            fcMode = OPEN_EXISTING;
            break;
        case TruncExisting:
            fcMode = TRUNCATE_EXISTING;
            break;
        case OpenAlways:
            fcMode = OPEN_ALWAYS;
            break;
        case CreateNew:
            fcMode = CREATE_NEW;
            break;
        case CreateAlways:
            fcMode = CREATE_ALWAYS;
            break;
        default:
            abort();
            break;
    }

    ui32 faMode = 0;
    if (oMode & RdOnly)
        faMode |= GENERIC_READ;
    if (oMode & WrOnly)
        faMode |= GENERIC_WRITE;
    if (oMode & ForAppend) {
        faMode |= GENERIC_WRITE;
        faMode |= FILE_APPEND_DATA;
        faMode &= ~FILE_WRITE_DATA;
    }

    ui32 shMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    ui32 attrMode = FILE_ATTRIBUTE_NORMAL;
    if ((createMode == OpenExisting || createMode == OpenAlways) &&
       ((oMode & AMask) == (oMode & AR)))
        attrMode |= FILE_ATTRIBUTE_READONLY;
    if (oMode & Seq)
        attrMode |= FILE_FLAG_SEQUENTIAL_SCAN;
    if (oMode & Temp)
        attrMode |= FILE_ATTRIBUTE_TEMPORARY; // we use TTempFile instead of FILE_FLAG_DELETE_ON_CLOSE
    if (oMode & Transient)
        attrMode |= FILE_FLAG_DELETE_ON_CLOSE;
    if ((oMode & Direct) && (oMode & WrOnly)) // WrOnly or RdWr
        attrMode |= /*FILE_FLAG_NO_BUFFERING |*/ FILE_FLAG_WRITE_THROUGH;

    Fd_ = CreateFileWithUtf8Name(fName, faMode, shMode, fcMode, attrMode);

    if ((oMode & ForAppend) && (Fd_ != INVALID_FHANDLE))
        ::SetFilePointer(Fd_, 0, 0, FILE_END);

#elif defined(_unix_)

    switch (createMode) {
        case OpenExisting:
            fcMode = 0;
            break;
        case TruncExisting:
            fcMode = O_TRUNC;
            break;
        case OpenAlways:
            fcMode = O_CREAT;
            break;
        case CreateNew:
            fcMode = O_CREAT | O_EXCL;
            break;
        case CreateAlways:
            fcMode = O_CREAT | O_TRUNC;
            break;
        default:
            abort();
            break;
    }

    if ((oMode & RdOnly) && (oMode & WrOnly))
        fcMode |= O_RDWR;
    else if (oMode & RdOnly)
        fcMode |= O_RDONLY;
    else if (oMode & WrOnly)
        fcMode |= O_WRONLY;

    if (oMode & ForAppend) {
        fcMode |= O_APPEND;
    }

    /* I don't now about this for unix...
    if (oMode & Temp) {
    }
    */
    if (oMode & Direct) {
#if defined (_freebsd_)
        /*
         * we do not use this in linux because of strange alignment requirements on r/w buffer
         */
        fcMode |= O_DIRECT;
#endif
    }

#if defined(_linux_)
    fcMode |= O_LARGEFILE;
#endif

    ui32 permMode = 0;
    if (oMode & AXOther)
        permMode |= S_IXOTH;
    if (oMode & AWOther)
        permMode |= S_IWOTH;
    if (oMode & AROther)
        permMode |= S_IROTH;
    if (oMode & AXGroup)
        permMode |= S_IXGRP;
    if (oMode & AWGroup)
        permMode |= S_IWGRP;
    if (oMode & ARGroup)
        permMode |= S_IRGRP;
    if (oMode & AXUser)
        permMode |= S_IXUSR;
    if (oMode & AWUser)
        permMode |= S_IWUSR;
    if (oMode & ARUser)
        permMode |= S_IRUSR;

    Fd_ = ::open(~fName, fcMode, permMode);

#if HAVE_POSIX_FADVISE
    TPosixFadvisePtr posixFadvise = Singleton<TPosixFadvise>()->posix_fadvise;
    if (posixFadvise != NULL && Fd_ >= 0) {
        if(oMode & NoReuse) {
            posixFadvise(Fd_, 0, 0, POSIX_FADV_NOREUSE);
        }
        if(oMode & Seq) {
            posixFadvise(Fd_, 0, 0, POSIX_FADV_SEQUENTIAL);
        }
    }
#endif

    //temp file
    if (Fd_ >= 0 && oMode & Transient) {
        unlink(~fName);
    }
#else
#   error unsupported platform
#endif
}

bool TFileHandle::Close() throw () {
    bool isOk = true;
    if (Fd_ != INVALID_FHANDLE)
#ifdef _win_
        isOk = (::CloseHandle(Fd_) != 0);
        if (!isOk) {
            VERIFY(GetLastError() != ERROR_INVALID_HANDLE,
                    "must not quietly close invalid handle");
        }
#elif defined(_unix_)
        isOk = (::close(Fd_) == 0 || errno == EINTR);
        if (!isOk) {
            // Do not quietly close bad descriptor,
            // because often it means double close
            // that is disasterous
            VERIFY(errno != EBADF, "must not quietly close bad descriptor: fd=%d", int(Fd_));
        }
#else
#       error unsupported platform
#endif
    Fd_ = INVALID_FHANDLE;
    return isOk;
}

static inline i64 DoSeek(FHANDLE h, i64 offset, SeekDir origin) throw () {
    if (h == INVALID_FHANDLE)
        return -1L;
#if defined (_win_)
    static ui32 dir[] = {FILE_BEGIN, FILE_CURRENT, FILE_END};
    LARGE_INTEGER pos;
    pos.QuadPart = offset;
    pos.LowPart = ::SetFilePointer(h, pos.LowPart, &pos.HighPart, dir[origin]);
    if (pos.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        pos.QuadPart = -1;
    return pos.QuadPart;
#elif defined(_unix_)
    static int dir[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    #if defined(_sun_)
        return ::llseek(h, (offset_t)offset, dir[origin]);
    #else
        return ::lseek(h, (off_t)offset, dir[origin]);
    #endif
#else
#   error unsupported platform
#endif
}

i64 TFileHandle::GetPosition() const throw () {
    return DoSeek(Fd_, 0, sCur);
}

i64 TFileHandle::Seek(i64 offset, SeekDir origin) throw () {
    return DoSeek(Fd_, offset, origin);
}

i64 TFileHandle::GetLength() const throw () {
    // XXX: returns error code, but does not set errno
    if (!IsOpen())
        return -1L;
    return GetFileLength(Fd_);
}

bool TFileHandle::Resize(i64 length) throw () {
    if (!IsOpen())
        return false;
    i64 currentLength = GetLength();
    if (length == currentLength)
        return true;
#if defined (_win_)
    i64 currentPosition = GetPosition();
    if (currentPosition == -1L)
        return false;
    Seek(length, sSet);
    if (!::SetEndOfFile(Fd_))
        return false;
    if (currentPosition < length)
        Seek(currentPosition, sSet);
    return true;
#elif defined (_unix_)
    return (0 == ftruncate(Fd_, (off_t)length));
#else
#   error unsupported platform
#endif
}

bool TFileHandle::Reserve(i64 length) throw() {
    if (!IsOpen())
        return false;
    i64 currentLength = GetLength();
    if (length <= currentLength)
        return true;
    if (!Resize(length))
        return false;
#if defined (_win_)
    if (!::SetFileValidData(Fd_, length)) {
        Resize(currentLength);
        return false;
    }
#elif defined (_unix_)
    // No way to implement this under FreeBSD. Just do nothing
#else
#   error unsupported platform
#endif
    return true;
}

bool TFileHandle::Flush() throw () {
    if (!IsOpen())
        return false;
#if defined(_win_)
    bool ok = ::FlushFileBuffers(Fd_) != 0;
    if (!ok) {
        // FlushFileBuffers fails if hFile is a handle to the console output.
        // That is because the console output is not buffered.
        // The function returns FALSE, and GetLastError returns ERROR_INVALID_HANDLE.
        if (GetLastError() == ERROR_INVALID_HANDLE)
            ok = true;
    }
    return ok;
#elif defined(_unix_)
    return ::fsync(Fd_) == 0;
#else
#   error unsupported platform
#endif
}

bool TFileHandle::FlushData() throw () {
#if defined(_linux_)
    if (!IsOpen()) {
        return false;
    }

    return ::fdatasync(Fd_) == 0;
#else
    return Flush();
#endif
}

i32 TFileHandle::Read(void* buffer, ui32 byteCount) throw () {
    if (!IsOpen())
        return -1;
#if defined(_win_)
    DWORD bytesRead = 0;
    if (::ReadFile(Fd_, buffer, byteCount, &bytesRead, NULL))
        return bytesRead;
    return -1;
#elif defined(_unix_)
    i32 ret;
    while ((ret = ::read(Fd_, buffer, byteCount)) == -1 && errno == EINTR) {
    }
    return ret;
#else
#   error unsupported platform
#endif
}

i32 TFileHandle::Write(const void* buffer, ui32 byteCount) throw () {
    if (!IsOpen())
        return -1;
#if defined(_win_)
    DWORD bytesWritten = 0;
    if (::WriteFile(Fd_, buffer, byteCount, &bytesWritten, NULL))
        return bytesWritten;
    return -1;
#elif defined(_unix_)
    i32 ret;
    while ((ret = ::write(Fd_, buffer, byteCount)) == -1 && errno == EINTR) {
    }
    return ret;
#else
#   error unsupported platform
#endif
}

i32 TFileHandle::Pread(void* buffer, ui32 byteCount, i64 offset) const throw () {
#if defined(_win_)
    OVERLAPPED io;
    Zero(io);
    DWORD bytesRead = 0;
    io.Offset = (ui32)offset;
    io.OffsetHigh = (ui32)(offset >> 32);
    if (::ReadFile(Fd_, buffer, byteCount, &bytesRead, &io))
        return bytesRead;
    return -1;
#elif defined(_unix_)
    i32 ret;
    while ((ret = ::pread(Fd_, buffer, byteCount, offset)) == -1 && errno == EINTR) {
    }
    return ret;
#else
#   error unsupported platform
#endif
}

i32 TFileHandle::Pwrite(const void* buffer, ui32 byteCount, i64 offset) const throw () {
#if defined(_win_)
    OVERLAPPED io;
    Zero(io);
    DWORD bytesWritten = 0;
    io.Offset = (ui32)offset;
    io.OffsetHigh = (ui32)(offset >> 32);
    if (::WriteFile(Fd_, buffer, byteCount, &bytesWritten, &io))
        return bytesWritten;
    return -1;
#elif defined(_unix_)
    i32 ret;
    while ((ret = ::pwrite(Fd_, buffer, byteCount, offset)) == -1 && errno == EINTR) {
    }
    return ret;
#else
#   error unsupported platform
#endif
}

FHANDLE TFileHandle::Duplicate() const throw () {
    if (!IsOpen())
        return INVALID_FHANDLE;
#if defined(_win_)
    FHANDLE dupHandle;
    if (!::DuplicateHandle(GetCurrentProcess(), Fd_, GetCurrentProcess(), &dupHandle, 0, TRUE, DUPLICATE_SAME_ACCESS))
        return INVALID_FHANDLE;
    return dupHandle;
#elif defined(_unix_)
    return ::dup(Fd_);
#else
#   error unsupported platform
#endif
}

bool TFileHandle::LinkTo(const TFileHandle& fh) const throw () {
#if defined(_unix_)
    return dup2(fh.Fd_, Fd_) != -1;
#elif defined(_win_)
    TFileHandle nh(fh.Duplicate());

    if (!nh.IsOpen()) {
        return false;
    }

    //not thread-safe
    nh.Swap(*const_cast<TFileHandle*>(this));

    return true;
#else
    #error unsupported
#endif
}

int TFileHandle::Flock(int op) throw () {
    return ::Flock(Fd_, op);
}

Stroka DecodeOpenMode(ui32 mode0) {
    ui32 mode = mode0;

    Stroka r;

#define F(flag) \
    if ((mode & flag) == flag) { \
        mode &= ~flag; \
        if (r) \
            r += "|"; \
        r += #flag; \
    }

    F(RdWr)
    F(CreateNew)
    F(AX)
    F(AR)
    F(AW)
    F(ARW)

    F(TruncExisting)
    F(CreateAlways)
    F(RdOnly)
    F(WrOnly)
    F(Seq)
    F(Direct)
    F(Temp)
    F(MaskMisc)
    F(ForAppend)
    F(Transient)
    F(AXOther)
    F(AWOther)
    F(AROther)
    F(AXGroup)
    F(AWGroup)
    F(ARGroup)
    F(AXUser)
    F(AWUser)
    F(ARUser)

#undef F

    if (mode != 0) {
        if (r)
            r += "|";
        r += Sprintf("0x%lX", long(mode));
    }

    if (!r)
        r = "0";

    return r;
};

class TFile::TImpl: public TRefCounted<TImpl, TAtomicCounter> {
    public:
        inline TImpl(FHANDLE fd, const Stroka& fname = Stroka())
            : Handle_(fd)
            , FileName_(fname)
        {
        }

        inline TImpl(const Stroka& fName, ui32 oMode)
            : Handle_(fName, oMode)
            , FileName_(fName)
        {
            if (!Handle_.IsOpen()) {
                char mode[32];
                sprintf(mode, "0x%lX", long(oMode));
                ythrow TFileError() << "can't open " << fName.Quote() << " with mode " << DecodeOpenMode(oMode) << " (" << mode << ")";
            }
        }

        inline ~TImpl() throw () {
        }

        inline void Close() {
            if (!Handle_.Close())
                ythrow TFileError() << "can't close " << FileName_.Quote();
        }

        const Stroka& GetName() const throw () {
            return FileName_;
        }

        void SetName(const Stroka& newName) {
            FileName_ = newName;
        }

        const TFileHandle& GetHandle() const throw () {
            return Handle_;
        }

        i64 Seek(i64 offset, SeekDir origin) {
            i64 pos = Handle_.Seek(offset, origin);
            if (pos == -1L)
                ythrow TFileError() << "can't seek " << offset << " bytes in " << FileName_.Quote();
            return pos;
        }

        void Resize(i64 length) {
            if (!Handle_.Resize(length))
                ythrow TFileError() << "can't resize " << FileName_.Quote() << " to size " << length;
        }

        void Reserve(i64 length) {
            if (!Handle_.Reserve(length))
                ythrow TFileError() << "can't reserve " << length << " for file " << FileName_.Quote();
        }

        void Flush() {
            if (!Handle_.Flush())
                ythrow TFileError() << "can't flush " << FileName_.Quote();
        }

        void FlushData() {
            if (!Handle_.FlushData())
                ythrow TFileError() << "can't flush data " << FileName_.Quote();
        }

        TFile Duplicate() const {
            TFileHandle dupH(Handle_.Duplicate());
            if (!dupH.IsOpen())
                ythrow TFileError() << "can't duplicate the handle of " << FileName_.Quote();
            TFile res(dupH);
            dupH.Release();
            return res;
        }

        size_t Read(void* bufferIn, size_t numBytes) {
            ui8* buf = (ui8*)bufferIn;

            while (numBytes) {
                const i32 toRead = (i32)Min(MaxPortion, numBytes);
                const i32 reallyRead = Handle_.Read(buf, toRead);

                if (reallyRead < 0)
                    ythrow TFileError() << "can not read data from " << FileName_.Quote();

                if (reallyRead == 0) // file exausted
                    break;

                buf += reallyRead;
                numBytes -= reallyRead;
            }

            return buf - (ui8*)bufferIn;
        }

        void Load(void* buf, size_t len) {
            if (Read(buf, len) != len)
                ythrow TFileError() << "can't read " << len << " bytes from " << FileName_.Quote();
        }

        void Write(const void* buffer, size_t numBytes) {
            const ui8* buf = (const ui8*)buffer;

            while (numBytes) {
                const i32 toWrite = (i32)Min(MaxPortion, numBytes);
                const i32 reallyWritten = Handle_.Write(buf, toWrite);

                if (reallyWritten < 0)
                    ythrow TFileError() << "can't write " << toWrite << " bytes to " << FileName_.Quote();

                buf += reallyWritten;
                numBytes -= reallyWritten;
            }
        }

        size_t Pread(void* bufferIn, size_t numBytes, i64 offset) const {
            ui8* buf = (ui8*)bufferIn;

            while (numBytes) {
                const i32 toRead = (i32)Min(MaxPortion, numBytes);
                const i32 reallyRead = Handle_.Pread(buf, toRead, offset);

                if (reallyRead < 0)
                    ythrow TFileError() << "can not read data from " << FileName_.Quote();

                if (reallyRead == 0) // file exausted
                    break;

                buf += reallyRead;
                offset += reallyRead;
                numBytes -= reallyRead;
            }

            return buf - (ui8*)bufferIn;
        }

        void Pload(void* buf, size_t len, i64 offset) const {
            if (Pread(buf, len, offset) != len)
                ythrow TFileError() << "can't read " << len << " bytes at offset " << offset << " from " << FileName_.Quote();
        }

        void Pwrite(const void* buffer, size_t numBytes, i64 offset) const {
            const ui8* buf = (const ui8*)buffer;

            while (numBytes) {
                const i32 toWrite = (i32)Min(MaxPortion, numBytes);
                const i32 reallyWritten = Handle_.Pwrite(buf, toWrite, offset);

                if (reallyWritten < 0)
                    ythrow TFileError() << "can't write " << toWrite << " bytes to " << FileName_.Quote();

                buf += reallyWritten;
                offset += reallyWritten;
                numBytes -= reallyWritten;
            }
        }

        void Flock(int op) {
            if (0 != Handle_.Flock(op))
                ythrow TFileError() << "can't flock " << FileName_.Quote();
        }

    private:
        TFileHandle Handle_;
        Stroka FileName_;
};

TFile::TFile()
    : Impl_(new TImpl(INVALID_FHANDLE))
{
}

TFile::TFile(FHANDLE fd)
    : Impl_(new TImpl(fd))
{
}

TFile::TFile(FHANDLE fd, const Stroka& name)
    : Impl_(new TImpl(fd, name))
{
}

TFile::TFile(const Stroka& fName, ui32 oMode)
    : Impl_(new TImpl(fName, oMode))
{
}

TFile::~TFile() throw () {
}

void TFile::Close() {
    Impl_->Close();
}

const Stroka& TFile::GetName() const throw () {
    return Impl_->GetName();
}

i64 TFile::GetPosition() const throw () {
    return Impl_->GetHandle().GetPosition();
}

i64 TFile::GetLength() const throw () {
    return Impl_->GetHandle().GetLength();
}

bool TFile::IsOpen() const throw () {
    return Impl_->GetHandle().IsOpen();
}

FHANDLE TFile::GetHandle() const throw() {
    return Impl_->GetHandle();
}

i64 TFile::Seek(i64 offset, SeekDir origin) {
    return Impl_->Seek(offset, origin);
}

void TFile::Resize(i64 length) {
    Impl_->Resize(length);
}

void TFile::Reserve(i64 length) {
    Impl_->Reserve(length);
}

void TFile::Flush() {
    Impl_->Flush();
}

void TFile::FlushData() {
    Impl_->FlushData();
}

TFile TFile::Duplicate() const {
    TFile res = Impl_->Duplicate();
    res.Impl_->SetName(Impl_->GetName());
    return res;
}

size_t TFile::Read(void* buf, size_t len) {
    return Impl_->Read(buf, len);
}

void TFile::Load(void* buf, size_t len) {
    Impl_->Load(buf, len);
}

void TFile::Write(const void* buf, size_t len) {
    Impl_->Write(buf, len);
}

size_t TFile::Pread(void* buf, size_t len, i64 offset) const {
    return Impl_->Pread(buf, len, offset);
}

void TFile::Pload(void* buf, size_t len, i64 offset) const {
    Impl_->Pload(buf, len, offset);
}

void TFile::Pwrite(const void* buf, size_t len, i64 offset) const {
    Impl_->Pwrite(buf, len, offset);
}

void TFile::Flock(int op) {
    Impl_->Flock(op);
}

void TFile::LinkTo(const TFile& f) const {
    if (!Impl_->GetHandle().LinkTo(f.Impl_->GetHandle())) {
        ythrow TFileError() << "can not link fd(" << GetName() << " -> " << f.GetName() << ")";
    }
}

TFile TFile::Temporary(const Stroka& prefix) {
    //TODO - handle impossible case of name collision
    return TFile(prefix + ToString(MicroSeconds()) + "-" + ToString(RandomNumber<ui64>()), CreateNew | RdWr | Seq | Temp | Transient);
}

TFile Duplicate(FILE* f) {
    return Duplicate(fileno(f));
}

TFile Duplicate(int fd) {
#if defined (_win_)
    int id_dup = ::dup(fd);
    if (id_dup == -1) {
    // this thow very useful for windows, when descriptors number are over maximum
        ythrow TFileError() << "can not duplicate file descriptor " << LastSystemError() << Endl;
    }
    return TFile((FHANDLE)::_get_osfhandle(id_dup));
#elif defined (_unix_)
    return TFile(::dup(fd));
#else
#   error unsupported platform
#endif
}
