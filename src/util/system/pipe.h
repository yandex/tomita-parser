#pragma once

#include <util/system/defaults.h>
#include <util/generic/ptr.h>
#include <util/network/pair.h>
#include <util/generic/noncopyable.h>

typedef SOCKET PIPEHANDLE;
#define INVALID_PIPEHANDLE INVALID_SOCKET

/// Pipe-like object: pipe on POSIX and socket on windows
class TPipeHandle: public TNonCopyable {
public:
    inline TPipeHandle() throw ()
        : Fd_(INVALID_PIPEHANDLE)
    {
    }

    inline TPipeHandle(PIPEHANDLE fd) throw ()
        : Fd_(fd)
    {
    }

    inline ~TPipeHandle() throw () {
        Close();
    }

    bool Close() throw ();

    inline PIPEHANDLE Release() throw () {
        PIPEHANDLE ret = Fd_;
        Fd_ = INVALID_PIPEHANDLE;
        return ret;
    }

    inline void Swap(TPipeHandle& r) throw () {
        DoSwap(Fd_, r.Fd_);
    }

    inline operator PIPEHANDLE() const throw () {
        return Fd_;
    }

    inline bool IsOpen() const throw () {
        return Fd_ != INVALID_PIPEHANDLE;
    }

    ssize_t Read(void* buffer, size_t byteCount) const throw ();
    ssize_t Write(const void* buffer, size_t byteCount) const throw ();

    static void Pipe(TPipeHandle& reader, TPipeHandle& writer);

private:
    PIPEHANDLE Fd_;
};

class TPipe {
public:
    TPipe();
    /// Takes ownership of handle, so closes it when the last holder of descriptor dies.
    explicit TPipe(PIPEHANDLE fd);
    ~TPipe() throw ();

    void Close();

    bool IsOpen() const throw ();
    PIPEHANDLE GetHandle() const throw ();

    size_t Read(void* buf, size_t len) const;
    size_t Write(const void* buf, size_t len) const;

    static void Pipe(TPipe& reader, TPipe& writer);

private:
    class TImpl;
    typedef TSimpleIntrusivePtr<TImpl> TImplRef;
    TImplRef Impl_;
};
