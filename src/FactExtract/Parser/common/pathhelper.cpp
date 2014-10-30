#include <util/system/maxlen.h>
#include "pathhelper.h"

#ifdef _win_
const char PathHelper::DirSeparator = '\\';
#else
const char PathHelper::DirSeparator = '/';
#endif

/* --------------------------- PathHelper methods --------------------------- */

Stroka& PathHelper::AppendSlashInplace(Stroka& path)
{
    if (path.empty())
        path = Stroka(".") + PathHelper::DirSeparator;  /* TODO: replace . with curdir? */
    else if (!PathHelper::IsDirSeparator(path.back()))
        path += PathHelper::DirSeparator;
    return path;
}

void PathHelper::Split(const TStringBuf& path, TStringBuf& dirname, TStringBuf& basename)
{
    int bstart = path.size();        // basename starting index
    int dstart = (bstart >= 2 && path[1] == ':') ? 2 : 0;  // dirname starting index (drive letter is excluded)
    while (bstart > dstart && !PathHelper::IsDirSeparator(path[bstart-1]))
        bstart -= 1;
    int dstop = bstart - 1;        // last char index of dirname (should not be a slash, unless all dirname is slashes)
    while (dstop >= dstart && PathHelper::IsDirSeparator(path[dstop]))
        dstop -= 1;
    dirname  = path.SubStr(0, (dstop < dstart) ? bstart : dstop + 1);
    basename = path.SubStr(bstart);
}

Stroka PathHelper::Join(const TStringBuf& dirname, const TStringBuf& basename)
{
    // basename should NOT start with drive letter ("C:\for\example")
    if (dirname.empty())
        return ToString(basename);

    Stroka res(~dirname, +dirname);
    bool base_has_sep = !basename.empty() && PathHelper::IsDirSeparator(basename[0]);
    //ensure there is a separator between joined parts
    if (PathHelper::IsDirSeparator(res.back())) {
        if (base_has_sep)
            res.append(~basename + 1, +basename - 1);
        else
            res.append(~basename, +basename);
    } else {
        if (!base_has_sep)
            res.append(PathHelper::DirSeparator);
        res.append(~basename, +basename);
    }
    return res;
}

void PathHelper::ReplaceExtension(Stroka& filename, const TStringBuf& ext)
{
    size_t i = filename.find_last_of('.'); // найти последнее расширение
    if (i != Stroka::npos)
        filename.erase(i + 1);
    else
        filename.append('.');
    filename.AppendNoAlias(~ext, +ext);
}

TStringBuf PathHelper::DirName(const TStringBuf& path)
{
    TStringBuf res, tmp;
    PathHelper::Split(path, res, tmp);
    return res;
}

Stroka PathHelper::DirNameWithSlash(const TStringBuf& path)
{
    TStringBuf dir, base;
    PathHelper::Split(path, dir, base);
    Stroka res(~dir, +dir);
    return PathHelper::AppendSlashInplace(res);
}

TStringBuf PathHelper::BaseName(const TStringBuf& path)
{
    TStringBuf res, tmp;
    PathHelper::Split(path, tmp, res);
    return res;
}

bool PathHelper::MakePath(const TStringBuf& path)
{
    Stroka buffer;
    const char* path_cstr = MakeCStr(path, buffer);

    if (path.empty() || isexist(path_cstr))
        return false;
    TStringBuf base, dir;
    PathHelper::Split(path, base, dir);
    if (!dir.empty())
        MakePath(base);
    if (!IsDots(dir))
        MakeDirIfNotExist(path_cstr);

    return true;
}

CTime PathHelper::GetMTime(const TStringBuf& path)
{
    Stroka buffer;
    const char* path_cstr = MakeCStr(path, buffer);
    return CTime(TFileStat(path_cstr).MTime);
}

