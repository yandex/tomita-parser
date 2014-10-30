#include <util/system/defaults.h>

#ifdef _win_

#include <stdio.h>
#include "dirent_win.h"

void __cdecl _dosmaperr(unsigned long);

struct DIR *opendir(const char *dirname) {
    struct DIR *dir = (struct DIR*)malloc(sizeof(struct DIR));
    int len = strlen(dirname);
    //Remove trailing slashes
    while (len && (dirname[len-1] == '\\' || dirname[len-1] == '/')) {
        --len;
    }
    dir->fff_templ = (char*)malloc(len + 5);
    memcpy(dir->fff_templ, dirname, len);
    memcpy(dir->fff_templ + len, "\\*.*", 5);
    dir->sh = FindFirstFile(dir->fff_templ, &dir->wfd);
    dir->file_no = 0;
    dir->readdir_buf = 0;
    if (!dir->sh) {
        _dosmaperr(GetLastError());
        free(dir);
        return 0;
    }
    return dir;
}

int closedir(struct DIR *dir) {
    FindClose(dir->sh);
    free(dir->fff_templ);
    free(dir->readdir_buf);
    free(dir);
    return 0;
}

int readdir_r(struct DIR *dir, struct dirent *entry, struct dirent **result) {
    if (!FindNextFile(dir->sh, &dir->wfd)) {
        int err = GetLastError();
        *result = 0;
        if (err == ERROR_NO_MORE_FILES) {
            SetLastError(0);
            return 0;
        } else {
            return err;
        }
    }
    entry->d_fileno = dir->file_no++;
    entry->d_reclen = sizeof(struct dirent);
    if (dir->wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
        (dir->wfd.dwReserved0 & IO_REPARSE_TAG_MOUNT_POINT
#if _MSC_VER > 1500
        || dir->wfd.dwReserved0 & IO_REPARSE_TAG_SYMLINK
#endif
        )) {
        entry->d_type = DT_LNK;
    } else if ( dir->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        entry->d_type = DT_DIR;
    } else {
        entry->d_type = DT_REG;
    }
    entry->d_namlen = (ui8)strlen(dir->wfd.cFileName);
    strcpy(entry->d_name, dir->wfd.cFileName);
    *result = entry;
    return 0;
}

struct dirent *readdir(struct DIR *dir) {
    struct dirent *res;
    if (!dir->readdir_buf)
        dir->readdir_buf = (struct dirent*)malloc(sizeof(struct dirent));
    readdir_r(dir, dir->readdir_buf, &res);
    return res;
}

void rewinddir(struct DIR *dir) {
    FindClose(dir->sh);
    dir->sh = FindFirstFile(dir->fff_templ, &dir->wfd);
    dir->file_no = 0;
}

#endif //_win_
