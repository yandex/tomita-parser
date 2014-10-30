#include "fs.h"
#include "defaults.h"

#if defined(_win_)
#   include "fs_win.h"
#else
#   include <unistd.h>
#endif

#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/charset/wide.h>

int NFs::Remove(const char* name) {
#if defined(_win_)
    return WinRemove(name);
#else
    return ::remove(name);
#endif
}

int NFs::Rename(const char* oldName, const char* newName) {
#if defined(_win_)
    return (MoveFileEx(oldName, newName, MOVEFILE_REPLACE_EXISTING) != 0 ? 0 : -1);
#else
    return ::rename(oldName, newName);
#endif
}

void NFs::HardLinkOrCopy(const char* from, const char* to) {
    bool done = false;
#if defined(_win_)
    done = (CreateHardLink(to, from, NULL) != 0);
#elif defined(_unix_)
    done = (link(from, to) == 0);
#endif
    if (!done)
        Copy(from, to);
}

bool NFs::SymLink(const char* targetName, const char* linkName) {
#if defined(_win_)
    return WinSymLink(targetName, linkName);
#elif defined(_unix_)
    return 0 == symlink(targetName, linkName);
#endif
}

Stroka NFs::ReadLink(const char* path) {
#if defined(_win_)
    return WinReadLink(path);
#elif defined(_unix_)
    TTempBuf buf;
    while (true) {
        ssize_t r = readlink(path, buf.Data(), buf.Size());
        if (r < 0)
            ythrow yexception() << "can't read link " << path;
        if (r < (ssize_t)buf.Size())
            return Stroka(buf.Data(), r);
        buf = TTempBuf(buf.Size() * 2);
    }
#endif
}

void NFs::Cat(const char* dstName, const char* srcName) {
    TFileInput src(srcName);
    TFileOutput dst(TFile(dstName, ForAppend | WrOnly | Seq));

    TransferData(&src, &dst);
}

void NFs::Copy(const char* oldName, const char* newName) {
    TFileInput src(oldName);
    TFileOutput dst(TFile(newName, CreateAlways | WrOnly | Seq));

    TransferData(&src, &dst);
}
