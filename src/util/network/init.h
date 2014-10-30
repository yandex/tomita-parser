#pragma once

#include <util/system/error.h>

#if defined(_unix_)
    #include <fcntl.h>
    #include <netdb.h>
    #include <time.h>
    #include <unistd.h>

    #include <sys/uio.h>
    #include <sys/time.h>
    #include <sys/poll.h>
    #include <sys/types.h>
    #include <sys/socket.h>

    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>

    typedef int SOCKET;

    #define closesocket(s)           close(s)
    #define SOCKET_ERROR             -1
    #define INVALID_SOCKET           -1
    #define WSAGetLastError()        errno
#elif defined(_win_)
    #include <util/system/winint.h>
    #include <io.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

    typedef ULONG nfds_t;

    #undef Yield

    struct sockaddr_un {
        short sun_family;
        char sun_path[108];
    };

    #define PF_LOCAL AF_UNIX
    #define NETDB_INTERNAL -1
    #define NETDB_SUCCESS 0

    #ifndef MSG_NOSIGNAL
    #   define MSG_NOSIGNAL 0
    #endif
#endif

void InitNetworkSubSystem();

static struct TNetworkInitializer {
    inline TNetworkInitializer() {
        InitNetworkSubSystem();
    }
} NetworkInitializerObject;
