#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/pair.h>

#include <util/system/defaults.h>
#include <util/folder/dirut.h>      //required for some cross-platform methods
#include <util/regexp/regexp.h>
#include <util/regexp/glob_compat.h>
#include <util/folder/filelist.h>

#include "timehelper.h"
#include "pathhelper.h"

struct SFileInfo
{
    SFileInfo(): m_LastWriteTime() {}

    CTime   m_LastWriteTime;
};

class CSmartFileFind
{
public:
    void FindFiles(Stroka strPath, bool bFillFileInfo = false);
    void Reset();

    size_t  GetFoundFilesCount();
    const Stroka& GetFoundFilePath(size_t i);
    const SFileInfo& GetFoundFileInfo(size_t i);

protected:
    void FillFileInfo(const Stroka& filename, SFileInfo& FileInfo);

protected:
    bool m_bFillFileInfo;
    yvector< TPair<Stroka, SFileInfo> > m_FoundFiles;
};

// helper class to CSmartFileFind (for win and unux) - adopted from Python "glob" implementation
class CGlob
{
public:
    static void Find(const TStringBuf& mask, yvector<Stroka>& results);  // classic glob
    //static void FindRecursive(const Stroka& folder, const Stroka& filemask, yvector<Stroka>& results);

    static void FindFilesRecursive(const TStringBuf& mask, yvector<Stroka>& results);
    static void FindFilesRecursive(const TStringBuf& folder, const TStringBuf& filemask,
                                   yvector<Stroka>& results);

private:
    static void FindFilesRecursiveByRegex(const TStringBuf& folder, const TRegExMatch& pattern,
                                          yvector<Stroka>& results);

    static void FindInFolderPattern(const TStringBuf& folder, const TStringBuf& filemask,
                                    yvector<Stroka>& results);
    static void FindInFolderFixed(const TStringBuf& folder, const TStringBuf& filename,
                                  yvector<Stroka>& results);

    static void FilterFileList(const TStringBuf& folder, const TStringBuf& pattern, TFileEntitiesList& filelist,
                               yvector<Stroka>& results);

    static bool IsFixedPath(const TStringBuf& path);
    static bool IsMatch(const TStringBuf& path, const TRegExMatch& pattern);

    static const TRegExMatch& GetRegEx(const TStringBuf& ShellPattern); // with normalization of ShellPattern
    static const TRegExMatch& CachedRegEx(const TStringBuf& pattern);    // without normalization, just what asked
    static Stroka ShellPatternToRegex(const TStringBuf& pattern);
};

/* ------- CGlob inlines definitions ------- */
inline bool CGlob::IsFixedPath(const TStringBuf& path)
{
    for (const char* a = path.begin(); a != path.end(); ++a)
        if (*a == '*' || *a == '?' || *a == '[')
            return false;
    return true;
}

inline bool CGlob::IsMatch(const TStringBuf& path, const TRegExMatch& pattern)
{
#ifdef _win_
    Stroka npath;
    return pattern.Match(PathHelper::NormalizeWin(path, npath).c_str());
#else
    return pattern.Match(*(path.c_str() + path.size()) == 0 ? path.c_str() : ToString(path).c_str());
#endif
}

inline const TRegExMatch& CGlob::GetRegEx(const TStringBuf& ShellPattern)
{
#ifdef _win_
    Stroka normpat;
    return CGlob::CachedRegEx(PathHelper::NormalizeWin(ShellPattern, normpat));
#else
    return CGlob::CachedRegEx(ShellPattern);
#endif
}

inline void CGlob::FindFilesRecursive(const TStringBuf& folder, const TStringBuf& filemask, yvector<Stroka>& results)
{
    CGlob::FindFilesRecursiveByRegex(folder, CGlob::GetRegEx(filemask.empty() ? "*" : filemask), results);
}
