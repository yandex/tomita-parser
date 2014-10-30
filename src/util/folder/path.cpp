#include "path.h"
#include "pathsplit.h"

#include <util/system/fs.h>
#include <util/system/file.h>
#include <util/system/sysstat.h>
#include <util/system/platform.h>
#include <util/folder/dirut.h>
#include <util/generic/yexception.h>

struct TFsPath::TSplit: public TRefCounted<TSplit, TAtomicCounter>, public TPathSplit {
    inline TSplit(const TStringBuf& path)
        : TPathSplit(path)
    {
    }
};

bool TFsPath::IsSubpathOf(const TFsPath& that) const {
    const TSplit& split = GetSplit();
    const TSplit& rsplit = that.GetSplit();

    if (rsplit.IsAbsolute != split.IsAbsolute)
        return false;

    if (rsplit.Drive != split.Drive)
        return false;

    if (+rsplit >= +split)
        return false;

    return NStl::equal(rsplit.begin(), rsplit.end(), split.begin());
}

TFsPath TFsPath::RelativeTo(const TFsPath& root) const {
    TSplit split = GetSplit();
    const TSplit& rsplit = root.GetSplit();

    if (split.Reconstruct() == rsplit.Reconstruct())
        return TFsPath();

    if (!this->IsSubpathOf(root))
        ythrow TIoException() << "path " << *this << " is not subpath of " << root;

    split.erase(split.begin(), split.begin() + rsplit.size());
    split.IsAbsolute = false;

    return TFsPath(split.Reconstruct());
}

TFsPath TFsPath::RelativePath(const TFsPath& root) const {
    TSplit split = GetSplit();
    const TSplit& rsplit = root.GetSplit();
    size_t cnt = 0;

    while (+split > cnt && +rsplit > cnt && split[cnt] == rsplit[cnt])
        cnt++;
    bool absboth = split.IsAbsolute && rsplit.IsAbsolute;
    if (cnt == 0 && !absboth)
        ythrow TIoException() << "No common parts in " << *this << " and " << root;
    Stroka r;
    for (size_t i = 0; i < +rsplit - cnt; i++)
        r += i == 0 ? ".." : "/..";
    for (size_t i  = cnt; i < +split; i++) {
        r += (i == 0 || i == cnt && +rsplit - cnt == 0? "" : "/");
        r += split[i];
    }
    return +r ? TFsPath(r) : TFsPath() ;
}

TFsPath TFsPath::Parent() const {
    TSplit split = GetSplit();
    if (+split)
        split.pop_back();
    if (!+split && !split.IsAbsolute)
        return TFsPath(".");
    return TFsPath(split.Reconstruct());
}

TFsPath& TFsPath::operator/=(const TFsPath& that) {
    TSplit split = GetSplit();
    const TSplit& rsplit = that.GetSplit();
    if (that.IsDefined() && that.GetPath() != ".") {
        if (!that.IsRelative())
            ythrow TIoException() << "path should be relative: " << that.GetPath();

        split.insert(split.end(), rsplit.begin(), rsplit.end());
        *this = TFsPath(split.Reconstruct());
    }
    return *this;
}

TFsPath& TFsPath::Fix() {
    // just normalize via reconstruction
    TFsPath(GetSplit().Reconstruct()).Swap(*this);

    return *this;
}

Stroka TFsPath::GetName() const {
    TSplit& split = GetSplit();

    if (split.size() > 0) {
        if (split.back() != "..") {
            return Stroka(split.back());
        } else {
            // cannot just drop last component, because path itself may be a symlink
            return RealPath().GetName();
        }
    } else {
        if (split.IsAbsolute) {
            return split.Reconstruct();
        } else {
            return Cwd().GetName();
        }
    }
}

Stroka TFsPath::GetExtension() const {
    return Stroka(GetSplit().Extension());
}

bool TFsPath::IsAbsolute() const {
    return GetSplit().IsAbsolute;
}

bool TFsPath::IsRelative() const {
    return !IsAbsolute();
}

void TFsPath::InitSplit() const {
    CheckDefined();
    Split_ = new TSplit(Path_);
}

TFsPath::TSplit& TFsPath::GetSplit() const {
    // XXX: race condition here
    if (!Split_)
        InitSplit();
    return *Split_;
}

TFsPath::TFsPath() {
}

TFsPath::TFsPath(const Stroka& path)
    : Path_(path)
{
    CheckDefined();
}

TFsPath::TFsPath(const TStringBuf& path)
    : Path_(ToString(path))
{
    CheckDefined();
}

TFsPath::TFsPath(const Stroka& path, const Stroka& realPath)
    : Path_(path)
    , RealPath_(realPath)
{
    CheckDefined();
    YASSERT(RealPath_.length() > 0);
}

TFsPath::TFsPath(const char* path)
    : Path_(path)
{
    CheckDefined();
}

TFsPath TFsPath::Child(const Stroka& name) const {
    CheckDefined();
    if (!name)
        ythrow TIoException() << "child name must not be empty";
    return *this / name;
}

struct TClosedir {
    static void Destroy(DIR* dir) {
        if (dir)
            if (0 != closedir(dir))
                ythrow TIoSystemError() << "failed to closedir";
    }
};

void TFsPath::ListNames(yvector<Stroka>& children) const {
    CheckDefined();
    THolder<DIR, TClosedir> dir(opendir(*this));
    if (!dir)
        ythrow TIoSystemError() << "failed to opendir " << Path_;
    for (;;) {
        struct dirent de;
        struct dirent* ok;
        int r = readdir_r(dir.Get(), &de, &ok);
        if (r != 0)
            ythrow TIoSystemError() << "failed to readdir " << Path_;
        if (ok == NULL)
            return;
        Stroka name(de.d_name);
        if (name == "." || name == "..")
            continue;
        children.push_back(name);
    }
}

void TFsPath::List(yvector<TFsPath>& files) const {
    yvector<Stroka> names;
    ListNames(names);
    for (yvector<Stroka>::iterator i = names.begin(); i != names.end(); ++i) {
        files.push_back(Child(*i));
    }
}

void TFsPath::RenameTo(const Stroka& newPath) const {
    CheckDefined();
    if (!newPath)
        ythrow TIoException() << "bad new file name";
    int rv = NFs::Rename(~Path_, ~newPath);
    if (rv < 0)
        ythrow TIoSystemError() << "failed to rename " << Path_ << " to " << newPath;
}

void TFsPath::RenameTo(const char* newPath) const {
    RenameTo(Stroka(newPath));
}

void TFsPath::RenameTo(const TFsPath& newPath) const {
    RenameTo(newPath.GetPath());
}

void TFsPath::Touch() const {
    CheckDefined();
    if (!TFile(*this, OpenAlways).IsOpen()) {
        ythrow TIoException() << "failed to touch " << *this;
    }
}

// XXX: move implementation to util/somewhere.
TFsPath TFsPath::RealPath() const {
    CheckDefined();
    if (RealPath_)
        return RealPath_;
    RealPath_ = ::RealPath(*this);
    return TFsPath(RealPath_, RealPath_);
}

TFsPath TFsPath::RealLocation() const {
    CheckDefined();
    if (RealPath_)
        return RealPath_;
    RealPath_ = ::RealLocation(*this);
    return TFsPath(RealPath_, RealPath_);
}


TFsPath TFsPath::ReadLink() const {
    CheckDefined();

    if (!IsSymlink())
        ythrow TIoException() << "not a symlink " << *this;

    return NFs::ReadLink(~GetPath());
}

bool TFsPath::Exists() const {
    CheckDefined();
    return isexist(*this);
}

void TFsPath::CheckExists() const {
    if (!Exists()) {
        ythrow TIoException() << "path does not exist " << Path_;
    }
}

bool TFsPath::IsDirectory() const {
    CheckDefined();
    return TFileStat(~GetPath()).IsDir();
}

bool TFsPath::IsFile() const {
    CheckDefined();
    return TFileStat(~GetPath()).IsFile();
}

bool TFsPath::IsSymlink() const {
    CheckDefined();
    return TFileStat(~GetPath(), true).IsSymlink();
}

void TFsPath::DeleteIfExists() const {
    CheckDefined();
    ::unlink(*this);
    ::rmdir(*this);
    if (Exists()) {
        ythrow TIoException() << "failed to delete " << Path_;
    }
}

void TFsPath::MkDir() const {
    CheckDefined();
    int r = Mkdir(*this, MODE0777);
    if (r != 0)
        ythrow TIoSystemError() << "could not create directory " << Path_;
}

void TFsPath::MkDirs() const {
    // TODO: must check if it is a directory
    if (!Exists()) {
        Parent().MkDirs();
        MkDir();
    }
}

void TFsPath::ForceDelete() const {
    CheckDefined();
    if (IsDirectory()) {
        yvector<TFsPath> children;
        List(children);
        for (yvector<TFsPath>::iterator i = children.begin(); i != children.end(); ++i) {
            i->ForceDelete();
        }
    }
    DeleteIfExists();
}

TFsPath TFsPath::Cwd() {
    return TFsPath(::GetCwd());
}

const TPathSplit& TFsPath::PathSplit() const {
    return GetSplit();
}

template<>
void Out<TFsPath>(TOutputStream& os, const TFsPath& f) {
    os << f.GetPath();
}

Stroka JoinFsPaths(const TStringBuf& first, const TStringBuf& second) {
    TFsPath fsPath(first);
    fsPath /= second;
    return fsPath.GetPath();
}
