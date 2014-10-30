#include "dirut.h"
#include "iterator.h"
#include "filelist.h"

#include <util/system/fs.h>
#include <util/system/maxlen.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>

#ifdef _win32_
#   define stat _stat
#endif

void SlashFolderLocal(Stroka &folder)
{
    if (!folder)
        return;
#ifdef _win32_
    size_t pos;
    while ((pos = folder.find('/')) != Stroka::npos)
        folder.replace(pos, 1, LOCSLASH_S);
#endif
    if (folder[+folder-1] != LOCSLASH_C)
        folder.append(LOCSLASH_S);
}

#ifndef _win32_

bool correctpath(Stroka &folder) {
    return resolvepath(folder, "/");
}

bool resolvepath(Stroka &folder, const Stroka &home)
{
    YASSERT(home && home.at(0) == '/');
    if (!folder) {
        return false;
    }
    // may be from windows
    char *ptr = folder.begin();
    while ((ptr = strchr(ptr, '\\')) != 0)
        *ptr = '/';

    if (folder.at(0) == '~') {
        if (folder.length() == 1 || folder.at(1) == '/') {
            folder = GetHomeDir() + (~folder + 1);
        } else {
            char* buf = (char*)alloca(folder.length()+1);
            strcpy(buf, ~folder + 1);
            char* p = strchr(buf, '/');
            if (p)
                *p++ = 0;
            passwd* pw = getpwnam(buf);
            if (pw) {
                folder = pw->pw_dir;
                folder += "/";
                if (p)
                    folder += p;
            } else {
                return false; // unknown user
            }
        }
    }
    int len = folder.length() + home.length() + 1;
    char* path = (char*)alloca(len);
    if (folder.at(0) != '/') {
        strcpy(path, ~home);
        strcpy(strrchr(path, '/')+1, ~folder); // the last char must be '/' if it's a dir
    } else {
        strcpy(path, ~folder);
    }
    len = strlen(path)+1;
    // grabbed from url.cpp
    char *newpath = (char*)alloca(len+2);
    const char **pp = (const char**)alloca(len*sizeof(char*));
    int i = 0;
    for (char* s = path; s;) {
        pp[i++] = s;
        s = strchr(s, '/');
        if (s)
            *s++ = 0;
    }

    for (int j = 1; j < i;) {
        const char*& p = pp[j];
        if (strcmp(p, ".") == 0 || strcmp(p,"") == 0) {
            if (j == i-1) {
                p = "";
                break;
            } else {
                memmove(pp+j, pp+j+1, (i-j-1)*sizeof(p));
                i--;
            }
        } else if (strcmp(p, "..") == 0) {
            if (j == i-1) {
                if (j == 1) {
                    p = "";
                } else {
                    i--;
                    pp[j-1] = "";
                }
                break;
            } else {
                if (j == 1) {
                    memmove(pp+j, pp+j+1, (i-j-1)*sizeof(p));
                    i--;
                } else {
                    memmove(pp+j-1, pp+j+1, (i-j-1)*sizeof(p));
                    i-=2;
                    j--;
                }
            }
        } else
            j++;
    }

    char* s = newpath;
    for (int k = 0; k < i; k++) {
        s = strchr(strcpy(s, pp[k]), 0);
        *s++ = '/';
    }
    *(--s) = 0;
    folder = newpath;
    return true;
}

#else

typedef enum {dt_empty, dt_error, dt_up, dt_dir} dir_type;
// precondition:  *ptr != '\\' || *ptr == 0 (cause dt_error)
// postcondition: *ptr != '\\'
template<typename T>
static int next_dir(T*& ptr)
{
    int has_blank = 0;
    int has_dot = 0;
    int has_letter = 0;
    int has_ctrl = 0;

    while (*ptr && *ptr != '\\') {
        int c = (unsigned char)*ptr++;
        switch (c) {
        case ' ':
            has_blank++;
            break;
        case '.':
            has_dot++;
            break;
        case '/':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            has_ctrl++;
            break;
        default:
            if (c == 127 || c < ' ')
                has_ctrl++;
            else
                has_letter++;
        }
    }
    if (*ptr)
        ptr++;
    if (has_ctrl)
        return dt_error;
    if (has_letter)
        return dt_dir;
    if (has_dot && has_blank)
        return dt_error;
    if (has_dot == 1)
        return dt_empty;
    if (has_dot == 2)
        return dt_up;
    return dt_error;
}

typedef enum
{dk_noflags = 0, dk_unc=1, dk_hasdrive=2, dk_fromroot=4, dk_error=8} disk_type;
// root slash (if any) - part of disk
template<typename T>
static int skip_disk(T*& ptr)
{
    int result = dk_noflags;
    if (!*ptr)
        return result;
    if (ptr[0] == '\\' && ptr[1] == '\\') {
        result |= dk_unc|dk_fromroot;
        ptr += 2;
        if (next_dir(ptr) != dt_dir)
            return dk_error;         // has no host name
        if (next_dir(ptr) != dt_dir)
            return dk_error;         // has no share name
    } else {
        if (*ptr && *(ptr+1) == ':') {
            result |= dk_hasdrive;
            ptr += 2;
        }
        if (*ptr == '\\' || *ptr == '/') {
            ptr++;
            result |= dk_fromroot;
        }
    }
    return result;
}

int correctpath(char *cpath, const char *path)
{
    if (!path || !*path)
    {
        *cpath = 0;
        return 1;
    }
    char *ptr = (char*)path;
    char *cptr = cpath;
    int counter = 0;
    while (*ptr) {
        char c = *ptr;
        if (c == '/')
            c = '\\';
        if (c == '\\')
            counter++;
        else
            counter = 0;
        if (counter <= 1) {
            *cptr = c;
            cptr++;
        }
        ptr++;
    }
    *cptr = 0;
    // replace '/' by '\'
    int dk = skip_disk(cpath);

    if (dk == dk_error)
        return 0;

    char *ptr1 = ptr = cpath;
    int level = 0;
    while (*ptr)
    {
        switch (next_dir(ptr))
        {
        case dt_dir:
            level++;
            break;
        case dt_empty:
            memmove(ptr1, ptr, strlen(ptr)+1);
            ptr = ptr1;
            break;
        case dt_up:
            level--;
            if (level >= 0) {
                *--ptr1 = 0;
                ptr1 = strrchr(cpath, '\\');
                if (!ptr1)
                    ptr1 = cpath;
                else
                    ptr1++;
                memmove(ptr1, ptr, strlen(ptr)+1);
                ptr = ptr1;
                break;
            } else if (level == -1 && (dk & dk_hasdrive)) {
                if (*ptr && *(ptr+1) == ':' && *(cpath-2) == ':') {
                    memmove(cpath-3, ptr, strlen(ptr)+1);
                    return 1;
                }
            }
            if (dk&dk_fromroot)
                return 0;
            break;
        case dt_error:
        default:
            return 0;
        }
        ptr1 = ptr;
    }

    if ((ptr > cpath || ptr == cpath && dk&dk_unc) && *(ptr-1) == '\\')
        *(ptr-1) = 0;
    return 1;
}

static inline int normchar(unsigned char c)
{
    return (c < 'a' || c >  'z') ? c : c-32;
}

static inline char *strslashcat(char *a, const char *b)
{
    size_t len = strlen(a);
    if (len && a[len-1] != '\\')
        a[len++] = '\\';
    strcpy(a+len, b);
    return a;
}

int resolvepath(char *apath, const char *rpath, const char *cpath)
{
    const char *redisk = rpath;
    if (!rpath || !*rpath)
        return 0;
    int rdt = skip_disk(redisk);
    if (rdt == dk_error)
        return 0;
    if (rdt&dk_unc || rdt&dk_hasdrive && rdt&dk_fromroot) {
        return correctpath(apath, rpath);
    }

    const char *cedisk = cpath;
    if (!cpath || !*cpath)
        return 0;
    int cdt = skip_disk(cedisk);
    if (cdt == dk_error)
        return 0;

    char *tpath = (char *) alloca(strlen(rpath)+strlen(cpath)+3);

    // rdt&dk_hasdrive && !rdt&dk_fromroot
    if (rdt&dk_hasdrive) {
        if (!(cdt&dk_fromroot))
            return 0;
        if (cdt&dk_hasdrive && normchar(*rpath) != normchar(*cpath))
            return 0;
        memcpy(tpath, rpath, 2);
        memcpy(tpath + 2, cedisk, strlen(cedisk) + 1);
        strslashcat(tpath, redisk);

    // !rdt&dk_hasdrive && rdt&dk_fromroot
    } else if (rdt&dk_fromroot) {
        if (!(cdt&dk_hasdrive) && !(cdt&dk_unc))
            return 0;
        memcpy(tpath, cpath, cedisk-cpath);
        tpath[cedisk-cpath] = 0;
        strslashcat(tpath, redisk);

    // !rdt&dk_hasdrive && !rdt&dk_fromroot
    } else {
        if (!(cdt&dk_fromroot) || !(cdt&dk_hasdrive) && !(cdt&dk_unc))
            return 0;
        strslashcat(strcpy(tpath, cpath), redisk);
    }

    return correctpath(apath, tpath);
}

bool correctpath(Stroka &filename)
{
    char *ptr = (char*)alloca(+filename+2);
    if (correctpath(ptr, ~filename)) {
        filename = ptr;
        return true;
    }
    return false;
}

bool resolvepath(Stroka &folder, const Stroka &home)
{
    char *ptr = (char*)alloca(+folder + 3 + +home);
    if (resolvepath(ptr, ~folder, ~home)) {
        folder = ptr;
        return true;
    }
    return false;
}

#endif // !defined _win32_

char GetDirectorySeparator()
{
    return LOCSLASH_C;
}

const char* GetDirectorySeparatorS()
{
    return LOCSLASH_S;
}

void RemoveDirWithContents(Stroka dirName) {
    SlashFolderLocal(dirName);

    TDirIterator dir(dirName);

    for (TDirIterator::TIterator it = dir.Begin(); it != dir.End(); ++it) {
        switch (it->fts_info) {
            case FTS_F:
            case FTS_DEFAULT:
            case FTS_DP:
            case FTS_SL:
            case FTS_SLNONE:
                if (NFs::Remove(it->fts_path))
                    ythrow TSystemError() << "error while removing " << it->fts_path;
                break;
        }
    }
}

int mkpath(char *path, int mode)
{
    struct stat sb;
    char *slash;
    int done = 0;

    slash = path;

    while (!done) {
        slash += strspn(slash, LOCSLASH_S);
        slash += strcspn(slash, LOCSLASH_S);

        done = (*slash == '\0');
        *slash = '\0';

        if (stat(path, &sb)) {
            if (errno != ENOENT || (Mkdir(path, mode) &&
                errno != EEXIST)) {
                return (-1);
            }
        } else if (!(S_IFDIR & sb.st_mode)) {
            return (-1);
        }

        if (!done)
            *slash = LOCSLASH_C;
    }

    return (0);
}

// Implementation of realpath in FreeBSD (version 9.0 and less) and GetFullPathName in Windows
// did not require last component of the file name to exist (other implementations will fail
// if it does not). Use RealLocation if that behaviour is required.
Stroka RealPath(const Stroka& path) {
    TTempBuf result;
    YASSERT(result.Size() > MAX_PATH); //TMP_BUF_LEN > MAX_PATH
#ifdef _win_
    if (GetFullPathName(~path, result.Size(), result.Data(), 0) == 0)
#else
    if (realpath(~path, result.Data()) == 0)
#endif
        ythrow TFileError() << "RealPath failed \"" << path << "\"";
    return result.Data();
}

Stroka RealLocation(const Stroka& path) {
    if (isexist(~path))
        return RealPath(path);
    Stroka dirpath = GetDirName(path);
    if (isexist(~dirpath))
        return  RealPath(dirpath) + GetDirectorySeparatorS() + GetFileNameComponent(~path);
    ythrow TFileError() << "RealLocation failed \"" << path << "\"";
}

Stroka GetCwd() {
    TTempBuf result;
#ifdef _win_
    int r = GetCurrentDirectory(result.Size(), result.Data());
    if (r == 0)
        throw TIoSystemError() << "failed to GetCurrentDirectory";
    return Stroka(result.Data(), r);
#else
    char* r = getcwd(result.Data(), result.Size());
    if (r == 0)
        throw TIoSystemError() << "failed to getcwd";
    return Stroka(result.Data());
#endif
}

int MakeTempDir(char path[/*FILENAME_MAX*/], const char* prefix) {
    int ret;

    Stroka sysTmp;

#ifdef _win32_
    if (!prefix || *prefix == '/') {
#else
    if (!prefix) {
#endif
        sysTmp = GetSystemTempDir();
        prefix = ~sysTmp;
    }

    if ((ret = ResolvePath(prefix, NULL, path, 1)) != 0)
        return ret;
    if ((ret = isdir(path)) != 0)
        return ret;
    if ((strlcat(path, "tmpXXXXXX", FILENAME_MAX) > FILENAME_MAX - 100))
        return EINVAL;
    if (!(mkdtemp(path)))
        return errno ? errno : EINVAL;
    strcat(path, LOCSLASH_S);
    return 0;
}

int RemoveTempDir(const char* dirName) {
    DIR *dir;
#ifndef DT_DIR
    struct stat sbp;
#endif
    if ((dir = opendir(dirName)) == 0)
        return errno ? errno : ENOENT;

    int ret;
    dirent ent, *pent;
    char path[FILENAME_MAX], *ptr;
    size_t len = strlcpy(path, dirName, FILENAME_MAX);

    if (path[len-1] != LOCSLASH_C) {
        path[len] = LOCSLASH_C;
        path[++len] = 0;
    }
    ptr = path + len;
    while ((ret = readdir_r(dir, &ent, &pent)) == 0 && pent == &ent) {
        if (!strcmp(ent.d_name, ".") || !strcmp(ent.d_name, ".."))
            continue;
#ifdef DT_DIR
        if (ent.d_type == DT_DIR)
#else
        lstat(ent.d_name, &sbp);
        if (S_ISREG(sbp.st_mode))
#endif
        {
            ret = ENOTEMPTY;
            break;
        }
        strcpy(ptr, ent.d_name);
        if (unlink(path)) {
            ret = errno ? errno : ENOENT;
            break;
        }
    }
    closedir(dir);
    if (ret)
        return ret;
    *ptr = 0;
    if (rmdir(path))
        return errno ? errno : ENOTEMPTY;
    return 0;
}

int isdir(const char *path) // OBSOLETE use IsDir instead
{
    if (!path || !*path)
        return ENOENT;
#ifdef _win32_
    size_t len = strlen(path);
    const char *ptr = path;
    if (*ptr == '\\' && *(ptr+1) == '\\') {
        ptr = strchr(ptr+2, '\\');
        if (ptr)
            ptr = strchr(ptr+1, '\\');
        if (ptr)
            ptr++;
        else
            ptr = path + len;
    } else {
        if (*(ptr+1) == ':')
            ptr += 2;
        if (*ptr == '\\')
            ptr++;
    }
    char *npath = (char*)alloca(len+2);
    if (ptr > path+3 && !*ptr && *(ptr-1) != '\\') {
        strcpy(npath, path);
        npath[len++] = '\\';
        npath[len] = 0;
        path = npath;
    } else if (path[len-1] == '\\' && len > size_t(ptr-path)) {
        strcpy(npath, path);
        npath[len-1] = 0;
        path = npath;
    }
    //struct stat sb;
    //return stat(path, &sb) != -1 && (sb.st_mode&S_IFDIR) != 0 ? 0 : ENOENT;
    ui32 attr = ::GetFileAttributesA(path);
    return attr != 0xFFFFFFFF && (attr & FILE_ATTRIBUTE_DIRECTORY) ? 0 : ENOENT;
#else
    struct stat sb;
    if (stat(path, &sb) == -1)
        return errno;
    return (sb.st_mode&S_IFDIR)? 0 : ENOENT;
#endif
}

bool IsDir(const Stroka& path) {
    return isdir(path.c_str()) == 0;
}

bool isexist(const char *path) {
    if (EXPECT_FALSE(!path)) {
        return false;
    }
#ifdef _win32_
    return ::GetFileAttributesA(path) != 0xFFFFFFFF;
#else
    return access(path, F_OK) == 0;
#endif
}

bool PathExists(const Stroka& path) {
    VERIFY(path.find('\0') == Stroka::npos);
    return isexist(~path);
}

void CheckIsExist(const char *path) {
    if (!isexist(path)) {
        ythrow yexception() << "File or dir " <<  path << " does not exist";
    }
}

Stroka GetHomeDir() {
    Stroka s(getenv("HOME"));
    if (!s) {
#ifndef _win32_
        passwd* pw = 0;
        s = getenv("USER");
        if (s)
            pw = getpwnam(~s);
        else
            pw = getpwuid(getuid());
        if (pw)
            s = pw->pw_dir;
        else
#endif
        {
            char* cur_dir = getcwd(0,0);
            s = cur_dir;
            free(cur_dir);
        }
    }
    return s;
}

void MakeDirIfNotExist(const char *path, int mode) {
    if (Mkdir(path, mode)) {
        if (errno != EEXIST) {
            ythrow TSystemError() << "failed to create directory " << path;
        } else {
            ClearLastSystemError();
        }
    }
}

void MakePathIfNotExist(const char *path, int mode) {
    const size_t size = strlen(path) + 1;
    TTempBuf tmp(size);
    memcpy(tmp.Data(), path, size);
    if (mkpath(tmp.Data(), mode)) {
        if (errno != EEXIST)
            ythrow TSystemError() << "failed to create directory " << path;
        else {
            struct stat sb;
            if (!stat(path, &sb) && S_IFDIR & sb.st_mode)
                ClearLastSystemError();
            else
                ythrow yexception() << path << " already exists and is not a directory";
        }
    }
}

const char* GetFileNameComponent(const char* f) {
    const char* p = strrchr(f, LOCSLASH_C);
#ifdef _win_
    // "/" is also valid char separator on Windows
    const char* p2 = strrchr(f, '/');
    if (p2 > p)
        p = p2;
#endif

    if (p) {
        return p + 1;
    }

    return f;
}

Stroka GetSystemTempDir() {
#ifdef _win_
    char buffer[1024];
    DWORD size = GetTempPath(1024, buffer);
    if (!size) {
        ythrow TSystemError() << "failed to get system temporary directory";
    }
    return Stroka(buffer, size);
#else
    const char* var = "TMPDIR";
    const char* def = "/tmp";
    const char* r = getenv(var);
    const char* result = r ? r : def;
    return result[0] == '/' ? result : ResolveDir(result);
#endif
}

Stroka ResolveDir(const char* path) {
    return ResolvePath(path, true);
}

Stroka GetDirName(const Stroka& path) {
#ifdef _win_
// May be mixed style of filename ('/' and '\')
    Stroka path_loc = path;
    correctpath(path_loc);
#else
    const Stroka& path_loc = path;
#endif
    size_t lastSlash = path_loc.find_last_of(LOCSLASH_C);
    if (lastSlash == Stroka::npos) {
        return ".";
    } else {
        return path_loc.substr(0, lastSlash);
    }
}

#ifdef _win32_

char* realpath(const char* pathname, char resolved_path[MAXPATHLEN]) {
    // partial implementation: no path existence check
    return _fullpath(resolved_path, pathname, MAXPATHLEN - 1);
}

#endif

Stroka GetBaseName(const Stroka& path) {
    size_t lastSlash = path.find_last_of(LOCSLASH_C);
    return lastSlash == Stroka::npos ? path : path.substr(lastSlash + 1);
}

int ResolvePath(const char* rel, const char* abs, char res[/*MAXPATHLEN*/], bool isdir) {
    char t[MAXPATHLEN*2+3];
    size_t len;

    *res = 0;
    if (!rel || !*rel)
        return EINVAL;
    if (*rel != LOCSLASH_C && abs && *abs == LOCSLASH_C) {
        len = strlcpy(t, abs, sizeof(t));
        if (len >= sizeof(t)-3)
            return EINVAL;
        if (t[len-1] != LOCSLASH_C)
            t[len++] = LOCSLASH_C;
        len += strlcpy(t+len, rel, sizeof(t)-len);
    } else {
        len = strlcpy(t, rel, sizeof(t));
    }
    if (len >= sizeof(t)-3)
        return EINVAL;
    if (isdir && t[len-1] != LOCSLASH_C) {
            t[len++] = LOCSLASH_C;
            t[len] = 0;
    }
    if (!realpath(t, res)) {
        if (!isdir) {
            if (realpath(~GetDirName(t), res)) {
                strcpy(res + strlen(res), ~GetBaseName(t));
                return 0;
            }
        }
        return errno ? errno : ENOENT;
    }
    if (isdir) {
        len = strlen(res);
        if (res[len-1] != LOCSLASH_C) {
            res[len++]  = LOCSLASH_C;
            res[len] = 0;
        }
    }
    return 0;
}

Stroka ResolvePath(const char* path, bool isDir) {
    char buf[PATH_MAX];
    if (ResolvePath(path, NULL, buf, isDir))
        ythrow yexception() << "cannot resolve path: \"" <<  path << "\"";
    return buf;
}

void ChDir(const Stroka& path) {
#ifdef _win_
    bool ok = SetCurrentDirectory(~path);
#else
    bool ok = !chdir(~path);
#endif
    if (!ok)
        ythrow TSystemError() << "failed to change directory to " << path.Quote();
}

Stroka StripFileComponent(const Stroka& fileName)
{
    Stroka dir = IsDir(fileName) ? fileName : GetDirName(fileName);
    if (!dir.empty() && dir.back() != GetDirectorySeparator()) {
        dir.append(GetDirectorySeparator());
    }
    return dir;
}
