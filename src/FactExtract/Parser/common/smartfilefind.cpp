#include <util/generic/map.h>

#include "smartfilefind.h"
#include <util/stream/file.h>


void CSmartFileFind::Reset()
{
    m_FoundFiles.clear();
}

size_t  CSmartFileFind::GetFoundFilesCount()
{
    return m_FoundFiles.size();
}

const Stroka& CSmartFileFind::GetFoundFilePath(size_t i)
{
    YASSERT(i < GetFoundFilesCount());
    return m_FoundFiles[i].first;
}

const SFileInfo& CSmartFileFind::GetFoundFileInfo(size_t i)
{
    YASSERT(i < GetFoundFilesCount());
    return m_FoundFiles[i].second;
}

void CSmartFileFind::FindFiles(Stroka strPath, bool bFillFileInfo /*= false*/)
{
    m_bFillFileInfo = bFillFileInfo;

    if (!PathHelper::Exists(strPath))
        PathHelper::AppendSlash(strPath);

    yvector<Stroka> results;
    CGlob::FindFilesRecursive(strPath, results);
    for (yvector<Stroka>::const_iterator item = results.begin(); item != results.end(); ++item) {
        TPair<Stroka, SFileInfo> f(*item, SFileInfo());
        if (m_bFillFileInfo)
            FillFileInfo(*item, f.second);
        m_FoundFiles.push_back(f);
    }
}

void CSmartFileFind::FillFileInfo(const Stroka& filename, SFileInfo& FileInfo)
{
    FileInfo.m_LastWriteTime = PathHelper::GetMTime(filename);
}

/* --------------------------- CGlob methods --------------------------- */

const TRegExMatch& CGlob::CachedRegEx(const TStringBuf& FilePattern)
{
    typedef ymap<Stroka, TRegExMatch, TLess<TStringBuf> > TRegexCache;
    static TRegexCache cache;
    TRegexCache::iterator item = cache.find(FilePattern);
    if (item == cache.end()) {
        TRegExMatch match(CGlob::ShellPatternToRegex(FilePattern).c_str());
        TPair<TRegexCache::iterator, bool> tmp = cache.insert(MakePair(ToString(FilePattern), match));
        item = tmp.first;
    }
    return item->second;
}

void CGlob::Find(const TStringBuf& mask, yvector<Stroka>& results)
{
    // CAUTION: this version of glob will find both files and folders textually matching the mask.
    // But if the mask ends with a slash it will look only for folders
    // Mask '*.*' does not guarantee to find only files (folder can have dots in name too)

    if (CGlob::IsFixedPath(mask)) {
        if (PathHelper::Exists(mask))
            results.push_back(ToString(mask));
        return;
    }
    TStringBuf dirname, basename;
    PathHelper::Split(mask, dirname, basename);
    yvector<Stroka> dirs;
    if (CGlob::IsFixedPath(dirname))
        dirs.push_back(ToString(dirname));
    else
        CGlob::Find(dirname, dirs);

    void (*func)(const TStringBuf&, const TStringBuf&, yvector<Stroka>&);
    if (CGlob::IsFixedPath(basename))
        func = CGlob::FindInFolderFixed;
    else
        func = CGlob::FindInFolderPattern;

    for (yvector<Stroka>::const_iterator it = dirs.begin(); it != dirs.end(); ++it)
        func(*it, basename, results);
}

void CGlob::FindFilesRecursive(const TStringBuf& mask, yvector<Stroka>& results)
{
    // Splitting mask in two parts: dir to start from and filemask. Both could contain wildcards.
    // If mask is fixed (no wildcards) and it is a directory - filemask will be "*"
    // Otherwise the last part of mask will be used as mask and the rest - as base directory
    // One exception: if mask is fixed path to existing file - only this file is returned in results.
    if (CGlob::IsFixedPath(mask)) {
        if (PathHelper::Exists(mask) && !PathHelper::IsDir(mask))
            results.push_back(ToString(mask));
        else
            CGlob::FindFilesRecursive(mask, "*", results);
    } else {
        TStringBuf head, tail;
        PathHelper::Split(mask, head, tail);
        CGlob::FindFilesRecursive(head, tail.empty() ? "*" : tail, results);
    }
}

void CGlob::FindFilesRecursiveByRegex(const TStringBuf& folder, const TRegExMatch& pattern, yvector<Stroka>& results)
{
    yvector<Stroka> items;
    CGlob::Find(PathHelper::Join(folder, "*"), items); //list of all content in folder
    for (yvector<Stroka>::const_iterator item = items.begin(); item != items.end(); ++item) {
        if (PathHelper::IsDir(*item) && (!PathHelper::IsDots(*item)))
            CGlob::FindFilesRecursiveByRegex(*item, pattern, results);
        else {
            TStringBuf head, tail;
            PathHelper::Split(*item, head, tail);
            if (CGlob::IsMatch(tail, pattern))
                results.push_back(*item);
        }
    }
}

void CGlob::FindInFolderPattern(const TStringBuf& path, const TStringBuf& filemask, yvector<Stroka>& results)
{
    if (!path.empty() && !PathHelper::IsDir(path))
        return;
    TFileEntitiesList names(TFileEntitiesList::EM_FILES_DIRS);

    //TFileEntitiedList could not cope with tailing dir-slashes in dirpath - so remove
    TStringBuf cor = path;
    while (!cor.empty() && PathHelper::IsDirSeparator(cor.back()))
        cor.Chop(1);

    if (cor.empty())
        cor = ".";

    names.Fill(ToString(cor), "", "", 1);
    CGlob::FilterFileList(path, filemask, names, results);
}

void CGlob::FilterFileList(const TStringBuf& path, const TStringBuf& pattern, TFileEntitiesList& filelist,
                           yvector<Stroka>& results)
{
    const TRegExMatch& repattern = CGlob::GetRegEx(pattern);
    for (const char* name = filelist.Next(); name != NULL; name = filelist.Next()) {
        if (CGlob::IsMatch(name, repattern))
            results.push_back(PathHelper::Join(path, name));
    }
}

void CGlob::FindInFolderFixed(const TStringBuf& path, const TStringBuf& filename, yvector<Stroka>& results)
{
    if (filename == "") {
        if (PathHelper::IsDir(path))
            results.push_back(ToString(path));
    } else {
        Stroka fullname = PathHelper::Join(path, filename);
        if (PathHelper::Exists(fullname))
            results.push_back(fullname);
    }
}

Stroka CGlob::ShellPatternToRegex(const TStringBuf& pattern)
{
    Stroka res;
    res.reserve(pattern.size());

    int i = 0;
    int n = pattern.size();
    while (i < n) {
        char c = pattern[i];
        i += 1;
        if (c == '*')
            res += ".*";
        else if (c == '?')
            res += '.';
        else if (c == '[') {
            int j = i;
            if (j < n && pattern[j] == '!')
                j += 1;
            if (j < n && pattern[j] == ']')
                j += 1;
            while (j < n && pattern[j] != ']')
                j += 1;
            if (j >= n)
                res += "\\[";
            else {
                res += "[";
                if (pattern[i] == '!') {
                    res += '^';
                    i += 1;
                } else if (pattern[i] == '^')
                    res += '\\';
                while (i < j) {
                    if (pattern[i] == '\\')
                        res += "\\\\";
                    else
                        res += pattern[i];
                    i += 1;
                }
                res += ']';
                i = j+1;
            }
        } else {
            //escape all non-alphanum characters (non-latin letters too!)
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                res += c;
            else {
                if (c == '\0')
                    res += "\\000";
                else {
                    res += '\\';
                    res += c;
                }
            }
        }
    }
    return Stroka("^") + res + "$";
}

