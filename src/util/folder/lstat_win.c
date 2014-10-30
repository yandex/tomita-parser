#include <util/system/defaults.h>

#ifdef _win_
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "lstat_win.h"

int lstat(const char * fileName, struct stat* fileStat) {
    HANDLE findHandle;
    WIN32_FIND_DATA findBuf;
    int result;
    result = stat(fileName, fileStat);
    if (result == 0) {
        findHandle = FindFirstFile(fileName, &findBuf);
        if (findBuf.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
           (findBuf.dwReserved0 & IO_REPARSE_TAG_MOUNT_POINT
#if _MSC_VER > 1500
           || findBuf.dwReserved0 & IO_REPARSE_TAG_SYMLINK
#endif
           )) {
            fileStat->st_mode = fileStat->st_mode & ~_S_IFMT | _S_IFLNK;
        }
        FindClose(findHandle);

    }
    return result;
}

#endif //_win_
