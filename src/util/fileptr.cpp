#include "fgood.h"

#include <util/system/fstat.h>

#ifdef _win32_
#   include <io.h>
#endif

i64 TFILEPtr::length() const {
#ifdef _win32_
    FHANDLE fd = (FHANDLE)_get_osfhandle(fileno(m_file));
#else
    FHANDLE fd = fileno(m_file);
#endif
    i64 rv = GetFileLength(fd);
    if (rv < 0)
        ythrow yexception() << "TFILEPtr::length() " <<  ~Name << ": " << LastSystemErrorText();
    return rv;
}
