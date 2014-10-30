#include "mutex.h"
#include "oldfile.h"

#include <util/memory/tempbuf.h>

#ifdef _win_
#   include "winint.h"
#endif

namespace Os {
    File::File(const Stroka& name, unsigned access, unsigned permission)
        : m_Handle(FileNull)
        , m_Flags(SHOULD_CLOSE)
    {
        Open(~name, access, permission);
    }

    bool File::Open(const char *name, unsigned access, unsigned permission) {
        if (IsOpen())
            return false;
        m_Handle = ::open(name, access, permission);
        m_Flags = SHOULD_CLOSE;
        m_Name = name;
        return IsOpen();
    }

    bool File::Close() {
        if (IsOpen() && ::close(m_Handle) == 0) {
            m_Handle = FileNull;
            m_Name.clear();
            return true;
        } else
            return false;
    }

    void File::CloseEx() {
        if (IsOpen()) {
            if (::close(m_Handle) != 0) {
                m_Handle = FileNull;
                ythrow yexception() << "Os::File::CloseEx(" <<  ~m_Name << ") - can't close: " << LastSystemErrorText();
            }
            m_Handle = FileNull;
            m_Name.clear();
        }
    }

#if defined (_win_)

#ifndef USE_OVERLAPPED_IO
#   define USE_OVERLAPPED_IO
#endif

#ifndef USE_OVERLAPPED_IO
    static TMutex pread_tm;
#endif

    i32 File::Pread(void *buffer, ui32 count, i64 offset) const {
#ifdef USE_OVERLAPPED_IO
        OVERLAPPED io;
        Zero(io);
        DWORD bytesRead = 0;
        io.Offset = (ui32)offset;
        io.OffsetHigh = (ui32)(offset >> 32);
        BOOL bRead = ::ReadFile(GetOsfHandle(), buffer, count, &bytesRead, &io);
        return bRead ? bytesRead : -1;
#else
        TGuard<TMutex> g(pread_tm);
        ::_lseeki64(m_Handle, offset, SEEK_SET);
        return ::read(m_Handle, buffer, count);

#endif
    }
    i32 File::Pwrite(const void *buffer, ui32 count, i64 offset) const {
#ifdef USE_OVERLAPPED_IO
        OVERLAPPED io;
        Zero(io);
        DWORD bytesWritten = 0;
        io.Offset = (ui32)offset;
        io.OffsetHigh = (ui32)(offset >> 32);
        BOOL bWritten = ::WriteFile(GetOsfHandle(), buffer, count, &bytesWritten, &io);
        return bWritten ? bytesWritten : -1;
#else
        TGuard<TMutex> g(pread_tm);
        ::_lseeki64(m_Handle, offset, SEEK_SET);
        return ::write(m_Handle, buffer, count);
#endif
    }

#endif

    void File::CopyEx(File& file) {
        TTempBuf buffer;
        size_t really_read;
        while ((really_read = file.ReadExEx(buffer.Data(), buffer.Size())) > 0)
            WriteEx(buffer.Data(), (ui32)really_read);
    }

    i64 File::Length(const char *name) {
#ifdef _win_
        WIN32_FIND_DATA fData;
        HANDLE h = FindFirstFileA(name, &fData);
        if (h == INVALID_HANDLE_VALUE)
            return -1;
        FindClose(h);
        return (((i64)fData.nFileSizeHigh) * (i64(MAXDWORD)+1)) + (i64)fData.nFileSizeLow;
#elif defined(_unix_)
        struct stat buf;
        int r = stat(name, &buf);
        if (r == -1)
            return -1;
        return (i64)buf.st_size;
#endif
    }
}
