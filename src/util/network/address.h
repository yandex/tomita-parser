#pragma once

#include "ip.h"
#include "socket.h"

#include <util/generic/ptr.h>
#include <util/generic/stroka.h>

namespace NAddr {
    class IRemoteAddr {
    public:
        virtual ~IRemoteAddr() {
        }

        virtual const sockaddr* Addr() const = 0;
        virtual socklen_t Len() const = 0;
    };

    typedef TAutoPtr<IRemoteAddr> IRemoteAddrPtr;
    typedef TSharedPtr<NAddr::IRemoteAddr, TAtomicCounter> IRemoteAddrRef;

    IRemoteAddrPtr GetSockAddr(SOCKET s);
    void PrintHost(TOutputStream& out, const IRemoteAddr& addr);

    Stroka PrintHost(const IRemoteAddr& addr);
    Stroka PrintHostAndPort(const IRemoteAddr& addr);

    bool IsLoopback(const IRemoteAddr& addr);
    bool IsSame(const IRemoteAddr& lhs, const IRemoteAddr& rhs);

    socklen_t SockAddrLength(const sockaddr* addr);

    //for accept, recvfrom - see LenPtr()
    class TOpaqueAddr: public IRemoteAddr {
    public:
        inline TOpaqueAddr() throw ()
            : L_(sizeof(S_))
        {
            Zero(S_);
        }

        inline TOpaqueAddr(const IRemoteAddr* addr) throw () {
            Assign(addr->Addr(), addr->Len());
        }

        inline TOpaqueAddr(const sockaddr* addr) {
            Assign(addr, SockAddrLength(addr));
        }

        virtual const sockaddr* Addr() const {
            return MutableAddr();
        }

        virtual socklen_t Len() const {
            return L_;
        }

        inline sockaddr* MutableAddr() const throw () {
            return (sockaddr*)&S_;
        }

        inline socklen_t* LenPtr() throw () {
            return &L_;
        }

    private:
        inline void Assign(const sockaddr* addr, socklen_t len) throw () {
            L_ = len;
            memcpy(MutableAddr(), addr, L_);
        }

    private:
        sockaddr_storage S_;
        socklen_t L_;
    };

    //for TNetworkAddress
    class TAddrInfo: public IRemoteAddr {
    public:
        inline TAddrInfo(const addrinfo* ai) throw ()
            : AI_(ai)
        {
        }

        virtual const sockaddr* Addr() const {
            return AI_->ai_addr;
        }

        virtual socklen_t Len() const {
            return (socklen_t)AI_->ai_addrlen;
        }

    private:
        const addrinfo* const AI_;
    };

    //compat, for TIpAddress
    class TIPv4Addr: public IRemoteAddr {
    public:
        inline TIPv4Addr(const TIpAddress& addr) throw ()
            : A_(addr)
        {
        }

        virtual const sockaddr* Addr() const {
            return A_;
        }

        virtual socklen_t Len() const {
            return A_;
        }

    private:
        const TIpAddress A_;
    };

    //same, for ipv6 addresses
    class TIPv6Addr: public IRemoteAddr {
    public:
        inline TIPv6Addr(const sockaddr_in6& a) throw ()
            : A_(a)
        {
        }

        virtual const sockaddr* Addr() const {
            return (sockaddr*)&A_;
        }

        virtual socklen_t Len() const {
            return sizeof(A_);
        }

    private:
        const sockaddr_in6 A_;
    };
}
