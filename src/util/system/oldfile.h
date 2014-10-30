#pragma once

#include "defaults.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <cstdlib>

#include <fcntl.h>

#if defined(_win_)
    #include <io.h>
    #include <dos.h>
    #include <share.h>
    #include <direct.h>
    #include <util/folder/lstat_win.h>
#else
    #include <unistd.h>
#endif

#include <util/generic/stroka.h>
#include <util/generic/ptr.h>
#include <util/generic/ylimits.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>

#include "fs.h"
#include "yassert.h"

#if !defined (_win_) && !defined (_cygwin_)
    #define O_BINARY 0
    #define O_TEXT 0
#endif

#if defined (_win_)
    #define S_ISDIR(st_mode) ((st_mode & _S_IFMT) == _S_IFDIR)
#endif

class Stroka;

namespace Os {
    typedef void* Handle;

    struct FileRange {
        i64 Offset;
        ui32 Length;
    };

    class File {
    public:
        typedef struct FileRange Range; // allow to write Os::File::Range

        class Lock;
        class Path;

        enum {
            Invalid = -1
        };

        enum {
            FileNull = -1
        };

        enum OpenMode {
            // open behavior
            oOpen       = 0,                // 0000000, // (.)
            oTrunc      = O_TRUNC,          // 0000001, // ( )
            oCreatExcl  = O_CREAT | O_EXCL,   // 0000002, // ( )
            oCreat      = O_CREAT,          // 0000003, // ( )
            oCreatTrunc = O_CREAT | O_TRUNC,  // 0000004, // ( )
#if defined (_win_)
            oNonBlock   = 0,
#else
            oNonBlock   = O_NONBLOCK,
#endif
            oOpenExisting     = oOpen,
            oTruncExisting    = oTrunc,
            oCreatNew         = oCreatExcl,
            oCreatAllways     = oCreatTrunc,
            oOpenAllways      = oCreat,

            // generic access
            oRdOnly    = O_RDONLY,          // 0000000, // (.)
            oWrOnly    = O_WRONLY,          // 0000010, // ( )
            oRdWr      = O_RDWR,            // 0000020, // ( )

            // additional _open_osf_handle
            oBinary   = O_BINARY,           // 0000000, // (.)
            oText     = O_TEXT,             // 0000100, // ( )
            oAppend   = O_APPEND,           // 0000200, // [ ]

            // win32 optimization
#if defined(_win_)
            oSeq    = O_SEQUENTIAL,         // 0010000, // [ ]
            oRandom = O_RANDOM,             // 0020000, // [ ]
#else
            oSeq    = 0,
            oRandom = 0,
#endif

            // unix optimization
#if defined (_freebsd_)
            /*
             * we do not use this in linux because of strange alignment requirements on r/w buffer
             */
            oDirect = O_DIRECT,
#else
            oDirect = 0,
#endif

            // other flags
#if defined (_win_)
            oTemp   = O_TEMPORARY,          // 0100000, // [ ] // remove on close
#endif
        };

        enum AccessAttr {
            // access() flags
            a_OK   = 00,
            aX_OK  = 01,
            aW_OK  = 02,
            aR_OK  = 04,
            aRW_OK = 06,

            // read/write permission
#if !defined (_win_)
            aX_Other = S_IXOTH, // 0001, // [x]
            aW_Other = S_IWOTH, // 0002, // [ ]
            aR_Other = S_IROTH, // 0004, // [x]
            aX_Group = S_IXGRP, // 0010, // [x]
            aW_Group = S_IWGRP, // 0020, // [x]
            aR_Group = S_IRGRP, // 0040, // [x]
            aX_User  = S_IXUSR, // 0100, // [x]
            aW_User  = S_IWUSR, // 0200, // [x]
            aR_User  = S_IRUSR, // 0400, // [x]
            aX = aX_User|aX_Group|aX_Other,
            aW = aW_User|aW_Group,
            aR = aR_User|aR_Group|aR_Other,
#else
            aR = S_IREAD,
            aW = S_IWRITE,
#endif
            aRW = aR | aW,
        };

        enum SeekDir {
            sSet = SEEK_SET, // 00, // (.)
            sCur = SEEK_CUR, // 01, // ( )
            sEnd = SEEK_END, // 02, // ( )
        };

#if 1
        enum SYSFILE_FLAGS {
            ReadOnly    = oRdOnly,
            ReadWrite   = oRdWr,
            WriteOnly   = oWrOnly,
            Append      = oAppend,
            Binary      = oBinary,
            Text        = oText,
            OpenOld     = oOpen,      //==oOpenExisting
            TruncOld    = oTrunc,     //==oTruncExisting
            CreateNew   = oCreatExcl, //==oCreatNew
            OpenAll     = oCreat,     //==oOpenAllways
            CreateAll   = oCreatTrunc,//==oCreatAllways
            PermRead    = aR,
            PermWrite   = aW,
#if !defined(_win_)
            PermExec    = aX,
#endif
            PermRdWr    = aRW,
        };
#endif

        File();
        File(int handle, bool shouldClose = false);
        File(const File &file);
        void operator=(const File &src);
        File(const char* name, unsigned openMode = oBinary | oRdOnly, unsigned accessRigts = aRW);
        File(const Stroka& name, unsigned openMode = oBinary | oRdOnly, unsigned accessRigts = aRW);
        ~File();

        const Stroka& GetName() const;
        int GetHandle() const;
        void SetHandle(int handle);
#if defined (_win_)
        Handle GetOsfHandle() const;
#endif

        bool Open(const char *name, unsigned openMode=oBinary|oRdOnly, unsigned accessRigts = aRW);
        void OpenEx(const char *name, unsigned openMode=oBinary|oRdOnly, unsigned accessRigts = aRW);
        bool Close();
        void CloseEx();

        bool IsOpen() const;

        i64 Position() const;
        i64 Length() const;
        i64 Seek(i64 offset, enum SeekDir = sSet);
        i64 SeekEx(i64 offset, enum SeekDir = sSet);
        i64 SeekToBegin();
        i64 SeekToEnd();
        int Truncate(i64 offset);

        bool Flush();

        i32 Read(void * buffer, ui32 numBytes);
        void ReadEx(void * buffer, ui32 numBytes);
        size_t ReadExEx(void* buffer, size_t numBytes);

        i32 Write(const void * buffer, ui32 numBytes);
        void WriteEx(const void * buffer, ui32 numBytes);
        void WriteExEx(const void* buffer, size_t numBytes);

        i32 Pread(void* buffer, ui32 count, i64 offset) const;
        size_t PreadExEx(void* buffer, size_t count, ui64 offset) const;

        i32 Pwrite(const void* buffer, ui32 count, i64 offset) const;

        // Добавить содержимое с текущей позиции до конца файла file в текущую позицию this.
        // Файлы file и this должны быть открыты.
        void CopyEx(File& file);

        static int Access(const char* name, int accessRights = aR_OK);

        static void Copy(const char* oldName, const char* newName) {
            NFs::Copy(oldName, newName);
        }

        static void Cat(const char* dstName, const char* srcName) {
            NFs::Cat(dstName, srcName);
        }

        static int Move(const char* oldName, const char* newName);

        // -1 if not exists
        static i64 Length(const char *name);

    protected:
        enum {SHOULD_CLOSE=1};
        int m_Handle;
        int m_Flags;
        Stroka m_Name;
    };

    inline File::File()
        : m_Handle(FileNull)
        , m_Flags(0) // not SHOULD_CLOSE !!
    {
    }

    inline File::File(int handle, bool shouldClose /* = false*/)
        : m_Handle(handle)
        , m_Flags(shouldClose ? SHOULD_CLOSE : 0)
    {
    }

    inline File::File(const Os::File &file)
        : m_Handle(file.m_Handle == Invalid ? file.m_Handle : ::dup(file.m_Handle))
        , m_Flags(SHOULD_CLOSE)
        , m_Name(file.m_Name)
    {
    }

    inline File::File(const char *name, unsigned access, unsigned permission)
        : m_Handle(FileNull)
        , m_Flags(SHOULD_CLOSE)
    {
        Open(name, access, permission);
    }

    inline void File::OpenEx(const char *name, unsigned openMode, unsigned accessRigts) {
        YASSERT(!IsOpen());
        if (!Open(name, openMode, accessRigts))
            ythrow yexception() << "can't open \'" <<  name << "\' with mode " <<  openMode << ": " << LastSystemErrorText();
    }

    inline bool File::IsOpen() const {
        return m_Handle > FileNull;
    }

    inline File::~File() {
        if (IsOpen() && (m_Flags & SHOULD_CLOSE))
            ::close(m_Handle);
    }

    inline int File::GetHandle() const {
        return m_Handle;
    }

    inline const Stroka& File::GetName() const {
        return m_Name;
    }

    inline void File::SetHandle(int handle) {
        if (m_Handle == handle)
            return;
        if (IsOpen() && (m_Flags & SHOULD_CLOSE))
            ::close(m_Handle);
        m_Handle = handle;
        m_Flags = 0; // not SHOULD_CLOSE !!!
    }

    inline void File::operator=(const Os::File &src) {
        SetHandle(src.m_Handle);
        m_Name = src.m_Name;
    }

#if defined (_win_)
    inline Handle File::GetOsfHandle() const  {
        return (void*)::_get_osfhandle(m_Handle);
    }
#endif

    inline i64 File::Position() const {
#if defined (_win_) && defined(_MSC_VER)
        return ::_telli64(m_Handle);
#elif defined(_sun_)
        return ::llseek(m_Handle, 0, SEEK_CUR);
#else
        return ::lseek(m_Handle, 0, SEEK_CUR);
#endif
    }

    inline int File::Truncate(i64 offset) {
        return ftruncate(m_Handle, (off_t)offset);
    }

    inline i64 File::Length() const {
#if defined (_win_) && defined(_MSC_VER)
        if (!IsOpen())
            return -1L;
        return ::_filelengthi64(m_Handle);
#else
        struct stat statbuf;
        if (::fstat(m_Handle, &statbuf) != 0)
            return -1L;
        return statbuf.st_size;
#endif
    }

    inline i64 File::Seek(i64 offset, enum SeekDir origin) {
#if defined (_win_) && defined (_MSC_VER)
        return ::_lseeki64(m_Handle, (__int64)offset, origin);
#elif defined(_sun_)
        return ::llseek(m_Handle, (offset_t)offset, origin);
#else
        return ::lseek(m_Handle, (off_t)offset, origin);
#endif
}

    inline i64 File::SeekEx(i64 offset, enum SeekDir origin) {
        i64 pos = Seek(offset, origin);
        if (pos == -1L)
            ythrow yexception() << "can't seek " <<  offset << " bytes: " << LastSystemErrorText() << "\n";
        return pos;
    }

    inline i64 File::SeekToBegin() {
        return Seek(0, sSet);
    }

    inline i64 File::SeekToEnd() {
        return Seek(0, sEnd);
    }

    inline bool File::Flush() {
#if defined(_win_)
        return ::_commit(m_Handle) == 0; // OS_success
#elif defined(_32_) || defined(_64_)
        return ::fsync(m_Handle) == 0;
#else
        return ::close(::dup(m_Handle)) == 0;
#endif
    }

    inline i32 File::Read(void * buffer, ui32 numBytes) {
#if defined(_32_) || defined(_64_)
        return ::read(m_Handle, buffer, numBytes);
#endif
    }

    inline i32 File::Write(const void *buffer, ui32 numBytes) {
#if defined(_32_) || defined(_64_)
        return ::write(m_Handle, buffer, numBytes);
#endif
    }

    inline void File::ReadEx(void * buffer, ui32 numBytes) {
        if (Read(buffer, numBytes) != i32(numBytes))
            ythrow yexception() << "can't read " <<  numBytes << " bytes from file " <<  ~m_Name << ": " << LastSystemErrorText() << "\n";
    }

    inline size_t File::ReadExEx(void* bufferIn, size_t numBytes) {
        char* buf = (char*)bufferIn;
        const size_t max_to_read = (size_t)Max<i32>();

        while (numBytes) {
            const i32 to_read     = (i32)Min(max_to_read, numBytes);
            const i32 really_read = Read(buf, to_read);

            if (really_read < 0) {
                ythrow yexception() << "can not read data from file " <<  ~m_Name << " (" << LastSystemErrorText() << ")";
            }

            if (really_read == 0) {
                /*
                 * file exausted
                 */

                break;
            }

            buf += really_read;
            numBytes -= really_read;
        }

        return buf - (char*)bufferIn;
    }

    inline void File::WriteEx(const void * buffer, ui32 numBytes) {
        if (Write(buffer, numBytes) != i32(numBytes))
            ythrow yexception() << "can't write " <<  numBytes << " bytes to file " <<  ~m_Name << ": " << LastSystemErrorText() << "\n";
    }

    inline void File::WriteExEx(const void* buffer, size_t numBytes) {
        const size_t max_portion = Max<i32>();
        const char* buf = (const char*)buffer;

        while (numBytes) {
            const ui32 to_write = (ui32)Min(max_portion, numBytes);

            WriteEx(buf, to_write);

            buf += to_write;
            numBytes -= to_write;
        }
    }

#if (defined(_32_) || defined(_64_)) && !defined(_win_)
    inline i32 File::Pread(void *buffer, ui32 count, i64 offset) const {
        return ::pread(m_Handle, buffer, count, offset);
    }
    inline i32 File::Pwrite(const void *buffer, ui32 count, i64 offset) const {
        return ::pwrite(m_Handle, buffer, count, offset);
    }
#endif

    inline size_t File::PreadExEx(void* bufferIn, size_t numBytes, ui64 offset) const {
        char* buf = (char*)bufferIn;
        const size_t max_to_read = (size_t)Max<i32>();

        while (numBytes) {
            const i32 to_read     = (i32)Min(max_to_read, numBytes);
            const i32 really_read = Pread(buf, to_read, offset);

            if (really_read < 0) {
                ythrow yexception() << "can not read data from file " <<  ~m_Name << " (" << LastSystemErrorText() << ")";
            }

            if (really_read == 0) {
                /*
                 * file exausted
                 */

                break;
            }

            buf += really_read;
            offset += really_read;
            numBytes -= really_read;
        }

        return buf - (char*)bufferIn;
    }
}

typedef Os::File TOldOsFile;

#define BIGBUF      (20u * BUFSIZ)
#define MAXBUF     (0x40000)
#define _1Kb_       (1024u)
#define F_open   (TOldOsFile::oBinary|TOldOsFile::oRdOnly)
