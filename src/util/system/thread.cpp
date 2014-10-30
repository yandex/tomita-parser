#include "tls.h"
#include "thread.h"
#include "thread.i"

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/pair.h>
#include <util/generic/ymath.h>
#include <util/generic/ylimits.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>

#include <cstdio>
#include <cstdlib>

namespace {
    typedef TThread::TParams TParams;
    typedef TThread::TId TId;

    inline void SetThrName(const TParams& p) {
        try {
            if (p.Name) {
                TThread::CurrentThreadSetName(~p.Name);
            }
        } catch (...) {
            //just ignore it
        }
    }

    inline size_t StackSize(const TParams& p) throw () {
        if (p.StackSize) {
            return FastClp2(p.StackSize);
        }

        return 0;
    }

#if defined(_win_)
    class TWinThread {
        struct TMyParams: public TParams, public TThrRefBase {
            inline TMyParams(const TParams& p)
                : TParams(p)
                , Result(0)
            {
            }

            void* Result;
        };

        typedef TIntrusivePtr<TMyParams> TParamsRef;

    public:
        inline TWinThread(const TParams& params)
            : P_(new TMyParams(params))
            , Handle(0)
#if _WIN32_WINNT < 0x0502
            , ThreadId(0)
#endif
        {
        }

        inline bool Running() const throw () {
            return Handle != 0;
        }

        inline TId SystemThreadId() const throw () {
#if _WIN32_WINNT < 0x0502
            return (TId)ThreadId;
#else
            return (TId)GetThreadId(Handle);
#endif
        }

        inline void* Join() {
            ::WaitForSingleObject(Handle, INFINITE);
            ::CloseHandle(Handle);

            return P_->Result;
        }

        inline void Detach() {
            ::CloseHandle(Handle);
        }

        static ui32 __stdcall Proxy(void* ptr) {
            NTls::TCleaner cleaner;

            (void)cleaner;

            {
                TParamsRef p((TMyParams*)(ptr));

                //drop counter, gotten in Start()
                p->UnRef();

                SetThrName(*p);
                p->Result = p->Proc(p->Data);
            }

            return 0;
        }

        inline void Start() {
#if _WIN32_WINNT < 0x0502
            Handle = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, (unsigned)StackSize(*P_), Proxy, (void*)P_.Get(), 0, &ThreadId));
#else
            Handle = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, (unsigned)StackSize(*P_), Proxy, (void*)P_.Get(), 0, NULL));
#endif

            if (!Handle) {
                ythrow yexception() << "failed to create a thread";
            }

            //do not do this, kids, at home
            P_->Ref();
        }

    private:
        TParamsRef P_;
        HANDLE Handle;
#if _WIN32_WINNT < 0x0502
        ui32 ThreadId;
#endif
    };

    typedef TWinThread TThreadBase;
#else
    //unix

#define PCHECK(x, y) {const int err_ = x; if (err_) {ythrow TSystemError(err_) << STRINGBUF(y);}}

    class TPosixThread {
    public:
        inline TPosixThread(const TParams& params)
            : P_(new TParams(params))
            , H_(0)
        {
            STATIC_ASSERT(sizeof(H_) == sizeof(TId));
        }

        inline TId SystemThreadId() const throw () {
            return (TId)H_;
        }

        inline void* Join() {
            void* tec = 0;
            PCHECK(pthread_join(H_, &tec), "can not join thread");

            return tec;
        }

        inline void Detach() {
            PCHECK(pthread_detach(H_), "can not detach thread");
        }

        inline bool Running() const throw () {
            return (bool)H_;
        }

        inline void Start() {
            pthread_attr_t* pattrs = 0;
            pthread_attr_t attrs;

            if (P_->StackSize > 0) {
                Zero(attrs);

                pthread_attr_init(&attrs);

#if (!defined(__FreeBSD__) || (__FreeBSD__ >= 5))
                pattrs = &attrs;
                pthread_attr_setstacksize(pattrs, StackSize(*P_));
#endif
            }

            {
                TParams *holdP = P_.Release();
                int err = pthread_create(&H_, pattrs, ThreadProxy, holdP);
                if (err) {
                    P_.Reset(holdP);
                    PCHECK(err, "failed to create thread");
                }
            }
        }

    private:
        static void* ThreadProxy(void* arg) {
            THolder<TParams> p((TParams*)arg);

            SetThrName(*p);

            return p->Proc(p->Data);
        }

    private:
        THolder<TParams> P_;
        pthread_t H_;
    };

#undef PCHECK

    typedef TPosixThread TThreadBase;
#endif

    template <class T>
    static inline typename T::TValueType* Impl(T& t, const char* op, bool check = true) {
        if (!t) {
            ythrow yexception() << "can not " << op << " dead thread";
        }

        if (t->Running() != check) {
            static const char* msg[] = {"running", "not running"};

            ythrow yexception() << "can not " << op << " " << msg[check] << " thread";
        }

        return t.Get();
    }
}

class TThread::TImpl: public TThreadBase {
public:
    inline TImpl(const TParams& params)
        : TThreadBase(params)
    {
    }

    inline TId Id() const throw () {
        return ThreadIdHashFunction(SystemThreadId());
    }
};

TThread::TThread(const TParams& p)
    : Impl_(new TImpl(p))
{
}

TThread::TThread(TThreadProc threadProc, void* param)
    : Impl_(new TImpl(TParams(threadProc, param)))
{
}

TThread::~TThread() throw () {
    Join();
}

void TThread::Start() {
    Impl(Impl_, "start", false)->Start();
}

void* TThread::Join() {
    if (Running()) {
        void* ret = Impl_->Join();

        Impl_.Destroy();

        return ret;
    }

    return 0;
}

void TThread::Detach() {
    if (Running()) {
        Impl_->Detach();
        Impl_.Destroy();
    }
}

bool TThread::Running() const throw () {
    return Impl_ && Impl_->Running();
}

TThread::TId TThread::Id() const throw () {
    if (Running()) {
        return Impl_->Id();
    }

    return ImpossibleThreadId();
}

TThread::TId TThread::CurrentThreadId() throw () {
    return SystemCurrentThreadId();
}

TThread::TId TThread::ImpossibleThreadId() throw () {
    return Max<TThread::TId>();
}

namespace {
    template <class T>
    static void* ThreadProcWrapper(void* param) throw () {
        return reinterpret_cast<T*>(param)->ThreadProc();
    }
}

TSimpleThread::TSimpleThread(size_t stackSize)
    : TThread(TParams(ThreadProcWrapper<TSimpleThread>, reinterpret_cast<void*>(this), stackSize))
{
}

#if defined(_MSC_VER)
// This beautiful piece of code is borrowed from
// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

//
// Usage: WindowsCurrentSetThreadName (-1, "MainThread");
//
#include <windows.h>
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void WindowsCurrentSetThreadName( DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
#endif

void TThread::CurrentThreadSetName(const char* name) {
    (void)name;

#if defined(_freebsd_)
    pthread_t thread = pthread_self();
    pthread_set_name_np(thread, name);
#elif defined(_linux_)
    VERIFY(prctl(PR_SET_NAME, name, 0, 0, 0) == 0, "pctl failed: %s", strerror(errno));
#elif defined(_darwin_)
    VERIFY(pthread_setname_np(name) == 0, "pthread_setname_np failed: %s", strerror(errno));
#elif defined(_MSC_VER)
    WindowsCurrentSetThreadName(DWORD(-1), name);
#else
    // no idea
#endif // OS
}
