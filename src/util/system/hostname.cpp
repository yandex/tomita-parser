#include <util/memory/tempbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

#if defined(_unix_)
    #include <unistd.h>
#endif

#if defined(_win_)
    #include <WinSock2.h>
#endif

#include "defaults.h"
#include "yassert.h"
#include "hostname.h"

namespace {
struct THostNameHolder {
    inline THostNameHolder() {
        TTempBuf hostNameBuf;

        if (gethostname(hostNameBuf.Data(), hostNameBuf.Size() - 1)) {
            ythrow TSystemError() << "can not get host name";
        }

        HostName = hostNameBuf.Data();
    }

    Stroka HostName;
};
}

const Stroka& HostName() {
    return (Singleton<THostNameHolder>())->HostName;
}

const char* GetHostName() {
    return ~HostName();
}
