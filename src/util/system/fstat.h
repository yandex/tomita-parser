#pragma once

#include "fhandle.h"

class TFile;

struct TFileStat {
    ui32 Mode;        /* protection */
    ui32 Uid;          /* user ID of owner */
    ui32 Gid;          /* group ID of owner */

    ui64 NLinks;      /* number of hard links */
    ui64 Size;         /* total size, in bytes */

    time_t ATime;       /* time of last access */
    time_t MTime;       /* time of last modification */
    time_t CTime;       /* time of last status change */

public:
    TFileStat();

    bool IsFile() const;
    bool IsDir() const;
    bool IsSymlink() const;

    explicit TFileStat(const TFile&);
    explicit TFileStat(FHANDLE);
    TFileStat(const char* fName, bool nofollow = false);
};

i64 GetFileLength(FHANDLE fd);
