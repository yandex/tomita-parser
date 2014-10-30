#pragma once

#include <util/system/defaults.h>
#include <util/folder/dirut.h>        //required for PathHelper::IsDir and some other cross-platform methods
#include <util/system/fstat.h>
#include <util/system/execpath.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>

#include "timehelper.h"


inline const char* MakeCStr(const TStringBuf& str, Stroka& buffer)
{
    if (*(str.data() + str.size()) == 0)
        return str.data();
    else {
        buffer.assign(~str, +str);
        return buffer.c_str();
    }
}

class PathHelper
{
public:
    static void CorrectSep(Stroka& result)
    {
        for (char* b = result.begin(); b != result.end(); ++b)
            if (*b == '/')
                *b = '\\';
    }

    static const char DirSeparator;
#ifdef _win_
    static Stroka NormalizeWin(const TStringBuf& path, Stroka& result)
    {
        result = ToString(path);
        CorrectSep(result);
        return result;
    }
#endif
    static Stroka& AppendSlashInplace(Stroka& path);
    static Stroka AppendSlash(const Stroka& path)
    {
        Stroka res = path;
        return PathHelper::AppendSlashInplace(res);
    }

    static bool IsDirSeparator(char ch)
    {
        return ch == '\\' || ch == '/';        //check both variants as either of them is acceptable on windows
    }

    static bool IsDots(const TStringBuf& path)
    {
        return path == "." || path == "..";
    }

    static void Split(const TStringBuf& path, TStringBuf& dirname, TStringBuf& basename);
    static Stroka Join(const TStringBuf& dirname, const TStringBuf& basename);
    static Stroka Join(const TStringBuf& dirname1, const TStringBuf& dirname2, const TStringBuf& basename)
    {
        return PathHelper::Join(PathHelper::Join(dirname1, dirname2), basename);
    }

    static Stroka Join(const TStringBuf& dirname1, const TStringBuf& dirname2, const TStringBuf& dirname3,
                       const TStringBuf& basename)
    {
        return PathHelper::Join(PathHelper::Join(PathHelper::Join(dirname1, dirname2), dirname3), basename);
    }

    static void ReplaceExtension(Stroka& filename, const TStringBuf& ext);

    static TStringBuf DirName(const TStringBuf& path);
    static Stroka DirNameWithSlash(const TStringBuf& path);

    static TStringBuf BaseName(const TStringBuf& path);

    static bool IsDir(const TStringBuf& dirname);
    static bool Exists(const TStringBuf& name)
    {
        Stroka buffer;
        const char* name_cstr = MakeCStr(name, buffer);
        return isexist(name_cstr);                      //using dirut.h implementation
    }

    // Check that all components from specified path exist or create missing folders.
    // Return true if some folders were created.
    static bool MakePath(const TStringBuf& path);

    static CTime GetMTime(const TStringBuf& path);

    // parser specific
    static Stroka GetMainPath() {
        Stroka str = GetExecPath();
        return ToString(DirName(DirName(str)));
    }

    static Stroka GetMainBinPath() {
        return Join(GetMainPath(), "bin", "");        //with trailing slash!
    }

    static Stroka GetDefaultDicPath() {
        return Join(GetMainPath(), "dic");
    }
};

/* ------- PathHelper inlines definitions ------- */

inline bool PathHelper::IsDir(const TStringBuf& dirname)
{
    Stroka buffer;
    const char* dirname_cstr = MakeCStr(dirname, buffer);
    return TFileStat(dirname_cstr).IsDir();
}
