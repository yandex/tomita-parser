#include "fstat.h"
#include "file.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <util/datetime/systime.h>

#if defined(_win_)
#   include "fs_win.h"

#   ifdef _S_IFLNK
#       undef _S_IFLNK
#   endif
#   define _S_IFLNK 0x80000000

ui32 GetFileMode(DWORD fileAttributes) {
    ui32 mode = 0;
    if (fileAttributes == 0xFFFFFFFF)
        return mode;
    if (fileAttributes & FILE_ATTRIBUTE_DEVICE)
        mode |= _S_IFCHR;
    if (fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        mode |= _S_IFLNK; // todo: was undefined by the moment of writing this code
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        mode |= _S_IFDIR;
    if (fileAttributes & (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE))
        mode |= _S_IFREG;
    return mode;
}

#   define S_ISDIR(st_mode) (st_mode & _S_IFDIR)
#   define S_ISREG(st_mode) (st_mode & _S_IFREG)
#   define S_ISLNK(st_mode) (st_mode & _S_IFLNK)

typedef BY_HANDLE_FILE_INFORMATION TSystemFStat;

#else

typedef struct stat TSystemFStat;

#endif

static void MakeStat(TFileStat& st, const TSystemFStat& fs) {
#ifdef _unix_
    st.Mode = fs.st_mode;
    st.NLinks = fs.st_nlink;
    st.Uid = fs.st_uid;
    st.Gid = fs.st_gid;
    st.Size = fs.st_size;
    st.ATime = fs.st_atime;
    st.MTime = fs.st_mtime;
    st.CTime = fs.st_ctime;
#else
    timeval tv;
    FileTimeToTimeval(&fs.ftCreationTime, &tv);
    st.CTime = tv.tv_sec;
    FileTimeToTimeval(&fs.ftLastAccessTime, &tv);
    st.ATime = tv.tv_sec;
    FileTimeToTimeval(&fs.ftLastWriteTime, &tv);
    st.MTime = tv.tv_sec;
    st.NLinks = fs.nNumberOfLinks;
    st.Mode = GetFileMode(fs.dwFileAttributes);
    st.Uid = 0;
    st.Gid = 0;
    st.Size = ((ui64)fs.nFileSizeHigh << 32) | fs.nFileSizeLow;
#endif
}

static bool GetStatByHandle(TSystemFStat& fs, FHANDLE f) {
#ifdef _win_
    return GetFileInformationByHandle(f, &fs);
#else
    return !fstat(f, &fs);
#endif
}

static bool GetStatByName(TSystemFStat& fs, const char* name, bool nofollow) {
#ifdef _win_
    TFileHandle h = CreateFileWithUtf8Name(name, FILE_READ_ATTRIBUTES|FILE_READ_EA, FILE_SHARE_READ|FILE_SHARE_WRITE,
                               OPEN_EXISTING,
                           (nofollow ? FILE_FLAG_OPEN_REPARSE_POINT : 0) | FILE_FLAG_BACKUP_SEMANTICS);
    return GetStatByHandle(fs, h);
#else
    return !(nofollow ? lstat : stat)(name, &fs);
#endif
}

TFileStat::TFileStat()
    : Mode()
    , Uid()
    , Gid()
    , NLinks()
    , Size()
    , ATime()
    , MTime()
    , CTime()
{
}

TFileStat::TFileStat(const TFile& f)
{
    *this = TFileStat(f.GetHandle());
}

TFileStat::TFileStat(FHANDLE f)
{
    TSystemFStat st;
    if (GetStatByHandle(st, f)) {
        MakeStat(*this, st);
    } else {
        *this = TFileStat();
    }
}

TFileStat::TFileStat(const char* f, bool nofollow)
{
    TSystemFStat st;
    if (GetStatByName(st, f, nofollow)) {
        MakeStat(*this, st);
    } else {
        *this = TFileStat();
    }
}

bool TFileStat::IsFile() const {
    return S_ISREG(Mode);
}

bool TFileStat::IsDir() const {
    return S_ISDIR(Mode);
}

bool TFileStat::IsSymlink() const {
    return S_ISLNK(Mode);
}

i64 GetFileLength(FHANDLE fd) {
#if defined (_win_)
    LARGE_INTEGER pos;
    if (!::GetFileSizeEx(fd, &pos))
        return -1L;
    return pos.QuadPart;
#elif defined (_unix_)
    struct stat statbuf;
    if (::fstat(fd, &statbuf) != 0)
        return -1L;
    return statbuf.st_size;
#else
#   error unsupported platform
#endif
}
