#pragma once

#include <util/system/fstat.h>
#include <util/system/yassert.h>
#include <util/system/platform.h>

#include <util/generic/ptr.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

struct TPathSplit;

class TFsPath {
    struct TSplit;

    Stroka Path_;

    /// caches
    mutable Stroka RealPath_;
    mutable TSimpleIntrusivePtr<TSplit> Split_;

    TFsPath(const Stroka& path, const Stroka& realPath);
public:
    TFsPath();

    TFsPath(const Stroka& path);
    TFsPath(const TStringBuf& path);
    TFsPath(const char* path);

    inline void CheckDefined() const {
        if (!IsDefined())
            ythrow TIoException() << "must be defined";
    }

    inline bool IsDefined() const {
        return Path_.length() > 0;
    }
    inline const Stroka& operator~() const {
        return Path_;
    }
    inline operator const Stroka&() const {
        return Path_;
    }
    inline operator const char*() const {
        return Path_.c_str();
    }
    inline bool operator==(const TFsPath& that) const {
        return Path_ == that.Path_;
    }
    inline bool operator!=(const TFsPath& that) const {
        return Path_ != that.Path_;
    }

    TFsPath& operator/=(const TFsPath& that);
    inline TFsPath operator/(const TFsPath& that) const {
        TFsPath ret(*this);
        return ret /= that;
    }
    // these operators are needed to support addition of empty path (that cannot be represented as TFsPath)
    TFsPath& operator/=(const Stroka& that) {
        if (that.empty())
            return *this;
        return *this /= TFsPath(that);
    }
    inline TFsPath operator/(const Stroka& that) const {
        TFsPath ret(*this);
        return ret /= that;
    }
    TFsPath& operator/=(const char* that) {
        if (!that || !*that)
            return *this;
        return *this /= TFsPath(that);
    }
    inline TFsPath operator/(const char* that) const {
        TFsPath ret(*this);
        return ret /= that;
    }

    const TPathSplit& PathSplit() const;

    TFsPath& Fix();

    inline const Stroka& GetPath() const {
        return Path_;
    }

    /// last component of path, or "/" if root
    Stroka GetName() const;

    /**
     * "a.b.tmp" -> "tmp"
     * "a.tmp"   -> "tmp"
     * ".tmp"    -> ""
     */
    Stroka GetExtension() const;

    bool IsAbsolute() const;
    bool IsRelative() const;

    bool IsSubpathOf(const TFsPath& that) const;
    bool IsContainerOf(const TFsPath& that) const {
        return that.IsSubpathOf(*this);
    }

    TFsPath RelativeTo(const TFsPath& root) const; //must be subpath of root
    TFsPath RelativePath(const TFsPath& root) const; //..; for relative paths 1st component must be the same
    /**
     * Never fails. Returns this if already a root.
     */
    TFsPath Parent() const;

    Stroka Basename() const {
        return GetName();
    }
    Stroka Dirname() const {
        return Parent();
    }

    TFsPath Child(const Stroka& name) const;

    void MkDir() const;
    void MkDirs() const;

    // XXX: rewrite to return iterator
    void List(yvector<TFsPath>& children) const;
    void ListNames(yvector<Stroka>& children) const;

    // fails to delete non-empty directory
    void DeleteIfExists() const;
    // delete recursively. Does nothing if not exists
    void ForceDelete() const;

    // XXX: ino

    bool Stat(TFileStat& stat) const {
        stat = TFileStat(~Path_);
        return stat.Mode;
    }
    bool Exists() const;
    /// false if not exists
    bool IsDirectory() const;
    /// false if not exists
    bool IsFile() const;
    /// false if not exists
    bool IsSymlink() const;
    /// throw TIoException if not exists
    void CheckExists() const;

    void RenameTo(const Stroka& newPath) const;
    void RenameTo(const char* newPath) const;
    void RenameTo(const TFsPath& newFile) const;

    void Touch() const;

    TFsPath RealPath() const;
    TFsPath RealLocation() const;
    TFsPath ReadLink() const;

    /// always absolute
    static TFsPath Cwd();

    inline void Swap(TFsPath& p) throw () {
        DoSwap(Path_, p.Path_);
        DoSwap(RealPath_, p.RealPath_);
        Split_.Swap(p.Split_);
    }

private:
    void InitSplit() const;
    TSplit& GetSplit() const;
};

inline TFsPath operator/(const Stroka& s, const TFsPath& p) {
    return TFsPath(s) / p;
}

Stroka JoinFsPaths(const TStringBuf& first, const TStringBuf& second);
