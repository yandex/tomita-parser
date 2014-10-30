#pragma once

#include "socket.h"
#include "hostip.h"

#include <util/system/error.h>
#include <util/system/byteorder.h>
#include <util/generic/stroka.h>
#include <util/generic/yexception.h>

/// IPv4 address in network format
typedef ui32 TIpHost;

/// Port number in host format
typedef ui16 TIpPort;

/*
 * ipStr is in 'ddd.ddd.ddd.ddd' format
 * returns IPv4 address in inet format
 */
static inline TIpHost IpFromString(const char* ipStr) {
    in_addr ia;

    if (inet_aton(ipStr, &ia) == 0) {
        ythrow TSystemError() << "Failed to convert (" <<  ipStr << ") to ip address";
    }

    return (ui32)ia.s_addr;
}

static inline char* IpToString(TIpHost ip, char* buf, size_t len) {
    if (!inet_ntop(AF_INET, (void*)&ip, buf, (socklen_t)len)) {
        ythrow TSystemError() << "Failed to get ip address string";
    }

    return buf;
}

static inline Stroka IpToString(TIpHost ip) {
    char buf[INET_ADDRSTRLEN];

    return Stroka(IpToString(ip, buf, sizeof(buf)));
}

static inline TIpHost ResolveHost(const char* data, size_t len) {
    TIpHost ret;
    const Stroka s(data, len);

    if (NResolver::GetHostIP(~s, &ret) != 0) {
        ythrow TSystemError(NResolver::GetDnsError()) << "can not resolve(" << s << ")";
    }

    return HostToInet(ret);
}

/// socket address
struct TIpAddress: public sockaddr_in {
    inline TIpAddress() throw () {
        Clear();
    }

    inline TIpAddress(const sockaddr_in& addr) throw ()
        : sockaddr_in(addr)
    {
    }

    inline TIpAddress(TIpHost ip, TIpPort port) throw () {
        Set(ip, port);
    }

    template <typename D, typename C, typename T>
    inline TIpAddress(const TStringBase<D, C, T>& ip, TIpPort port) {
        Set(ResolveHost(~ip, +ip), port);
    }

    inline TIpAddress(const char* ip, TIpPort port) {
        Set(ResolveHost(ip, strlen(ip)), port);
    }

    inline operator sockaddr* () const throw () {
        return (sockaddr*)(sockaddr_in*)this;
    }

    inline operator socklen_t* () const throw () {
        tmp = sizeof(sockaddr_in);

        return (socklen_t*)&tmp;
    }

    inline operator socklen_t () const throw () {
        tmp = sizeof(sockaddr_in);

        return tmp;
    }

    inline void Clear() throw () {
        Zero((sockaddr_in&)(*this));
    }

    inline void Set(TIpHost ip, TIpPort port) throw () {
        Clear();

        sin_family = AF_INET;
        sin_addr.s_addr = ip;
        sin_port = HostToInet(port);
    }

    inline TIpHost Host() const throw () {
        return sin_addr.s_addr;
    }

    inline TIpPort Port() const throw () {
        return InetToHost(sin_port);
    }

    mutable socklen_t tmp;
};
