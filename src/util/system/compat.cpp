#include "compat.h"
#include "defaults.h"
#include "progname.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>

#ifdef _win_
    #include <util/system/winint.h>
    #include <io.h>
#endif

#ifndef HAVE_NATIVE_GETPROGNAME
const char* getprogname() {
    return ~GetProgramName();
}
#endif

extern "C" {

#if defined (_MSC_VER)
#if _MSC_VER < 1400
int vsnprintf(char *str, size_t size, const char *format, va_list argptr)
{
  int i = _vsnprintf(str, size-1, format, argptr);
  if (i < 0) {
    str[size - 1] = '\x00';
    i = (int)size;
  } else if (i < (int)size) {
    str[i] = '\x00'; //Bug in MS library - sometimes it happens. Never trust MS :(
  }
  return i;
}
#endif

int snprintf(char *str, size_t size, const char *format, ...)
{
  va_list  argptr;
  va_start(argptr, format);
  int i = _vsnprintf(str, size-1, format, argptr);
  va_end(argptr);

  if (i < 0) {
    str[size - 1] = '\x00';
    i = (int)size;
  } else if (i < (int)size) {
    str[i] = '\x00'; //Bug in MS library - sometimes it happens. Never trust MS :(
  }
  return i;
}

int vasprintf(char **ret, const char *fmt, va_list params) {
    int n = 16, k;
    char *buf = (char*)malloc(n);
    while (((k = vsnprintf(buf, n, fmt, params)) >= n) || (k < 0)) {
        n = 3 * n / 2;
        buf = (char*)realloc(buf, n);
        if (!buf) { *ret = 0; return -1; }
        buf[n-1] = '+';
    }
    *ret = buf;
    return k;
}

int asprintf(char **ret, const char *fmt, ...) {
    va_list params;
    int n = 16, k;
    char *buf = (char*)malloc(n);
    va_start(params, fmt);
    while (((k = vsnprintf(buf, n, fmt, params)) >= n) || (k < 0)) {
        n = 3 * n / 2;
        buf = (char*)realloc(buf, n);
        if (!buf) { va_end(params); *ret = 0; return -1; }
        va_end(params);
        va_start(params, fmt);
    }
    *ret = buf;
    va_end(params);
    return k;
}

#elif 0 // !defined (HAVE_SNPRINTF)

int vsnprintf(char *str, size_t size, const char *format, va_list argptr)
{
  vsprintf(str, format, argptr);
  assert(strlen(str) < size);
  return strlen(str);
}

int snprintf(char *str, size_t size, const char *format, ...)
{
  va_list  argptr;
  va_start(argptr, format);
  vsprintf(str, format, argptr);
  va_end(argptr);
  assert(strlen(str) < size);
  return strlen(str);
}

#endif

#if !defined (_MSC_VER)
char *strrev(char *s) {
   char *d  = s;
   char *r = strchr(s,0);
   for (--r; d < r; ++d,--r) {
      char tmp;
      tmp = *d;
      *d = *r;
      *r = tmp;
   }
   return s;
}

char *strupr(char *s) {
   char* d;
   for (d  = s;*d; ++d)
      *d = (char)toupper((int)*d);
   return s;
}

char *strlwr(char *s) {
   char* d;
   for (d  = s;*d; ++d)
      *d = (char)tolower((int)*d);
   return s;
}

#endif

#if !defined(__linux__) && (!defined(__FreeBSD__) || __FreeBSD__ <= 4)
char *stpcpy(char *dst, const char *src) {
    size_t len = strlen(src);
    return ((char*)memcpy(dst, src, len+1)) + len;
}
#endif

#if defined (_MSC_VER) || defined(_sun_)
/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char* strsep(register char **stringp, register const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;

    if ((s = *stringp) == NULL)
        return NULL;
    for (tok = s;;)
    {
        c = *s++;
        spanp = delim;
        do
        {
            if ((sc = *spanp++) == c)
            {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return tok;
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}
#endif

} // extern "C"

#ifdef _win_
#include <fcntl.h>
int ftruncate(int fd, i64 length)
{
    return _chsize_s(fd, length);
}
int truncate(const char* name, i64 length)
{
    int fd = ::_open(name, _O_WRONLY);
    int ret = ftruncate(fd, length);
    ::close(fd);
    return ret;
}
#endif

