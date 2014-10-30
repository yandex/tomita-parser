#include "sem.h"

#ifdef _win_
    #include <malloc.h>
#elif defined(_sun)
    #include <alloca.h>
#endif

#include <cerrno>
#include <cstring>

#ifdef _win_
    #include "winint.h"
#else
    #include <signal.h>
    #include <unistd.h>
    #include <semaphore.h>

    #define USE_SYSV_SEMAPHORES //unixoids declared the standard but not implemented it...
#endif

#ifdef USE_SYSV_SEMAPHORES
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>

    #if defined(_linux_) || defined(_sun_) || defined(_cygwin_)
        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short  *array;
        } arg;
    #else
        union semun arg;
    #endif
#endif

#include <util/digest/crc.h>
#include <util/string/cast.h>
#include <util/random/random.h>

namespace {
    class TSemaphoreImpl {
    private:
#ifdef _win_
        typedef HANDLE SEMHANDLE;
#else
    #ifdef USE_SYSV_SEMAPHORES
        typedef int SEMHANDLE;
    #else
        typedef sem_t* SEMHANDLE;
    #endif
#endif

        SEMHANDLE Handle;

    public:
        inline TSemaphoreImpl(const char* name, ui32 max_free_count)
            : Handle(0)
        {
#ifdef _win_
            char* key = (char*)name;
            if (name) {
                size_t len = strlen(name);
                key = (char*)alloca(len+1);
                strcpy(key, name);
                if (len > MAX_PATH)
                    *(key + MAX_PATH) = 0;
                char* p = key;
                while (*p) {
                    if (*p == '\\')
                        *p = '/';
                    p++;
                }
            }
            // non-blocking on init
            Handle = ::CreateSemaphore(0, max_free_count, max_free_count, key);
#else
    #ifdef USE_SYSV_SEMAPHORES
            key_t key = Crc<ui32>(name, strlen(name));
            Handle = semget(key, 0, 0); // try to open exist semaphore
            if (Handle == -1) { // create new semaphore
                Handle = semget(key, 1, 0666 | IPC_CREAT);
                if (Handle != -1) {
                    union semun arg;
                    arg.val = max_free_count;
                    semctl(Handle, 0, SETVAL, arg);
                } else {
                    ythrow TSystemError() << "can not init sempahore";
                }
            }
    #else
            Handle = sem_open(name, O_CREAT, 0666, max_free_count);
            if (Handle == SEM_FAILED) {
                ythrow TSystemError() << "can not init sempahore";
            }
    #endif
#endif
        }

        inline ~TSemaphoreImpl() throw () {
#ifdef _win_
            ::CloseHandle(Handle);
#else
    #ifdef USE_SYSV_SEMAPHORES
            // we DO NOT want 'semctl(Handle, 0, IPC_RMID)' for multiprocess tasks;
            //struct sembuf ops[] = {{0, 0, IPC_NOWAIT}};
            //if (semop(Handle, ops, 1) != 0) // close only if semaphore's value is zero
            //    semctl(Handle, 0, IPC_RMID);
    #else
            sem_close(Handle); // we DO NOT want sem_unlink(...)
    #endif
#endif
        }

        inline void Release() throw () {
#ifdef _win_
            ::ReleaseSemaphore(Handle, 1, 0);
#else
    #ifdef USE_SYSV_SEMAPHORES
            struct sembuf ops[] = {{0, 1, SEM_UNDO}};
            int ret = semop(Handle, ops, 1);
    #else
            int ret = sem_post(Handle);
    #endif
            VERIFY(ret == 0, "can not release semaphore");
#endif
        }

        //The UNIX semaphore object does not support a timed "wait", and
        //hence to maintain consistancy, for win32 case we use INFINITE or 0 timeout.
        inline void Acquire() throw () {
#ifdef _win_
            VERIFY(::WaitForSingleObject(Handle, INFINITE) == WAIT_OBJECT_0, "can not acquire semaphore");
#else
    #ifdef USE_SYSV_SEMAPHORES
            struct sembuf ops[] = {{0, -1, SEM_UNDO}};
            int ret = semop(Handle, ops, 1);
    #else
            int ret = sem_wait(Handle);
    #endif
            VERIFY(ret == 0, "can not acquire semaphore");
#endif
        }

        inline bool TryAcquire() throw () {
#ifdef _win_
            // zero-second time-out interval
            // WAIT_OBJECT_0: current free count > 0
            // WAIT_TIMEOUT:  current free count == 0
            return ::WaitForSingleObject(Handle, 0) == WAIT_OBJECT_0;
#else
    #ifdef USE_SYSV_SEMAPHORES
            struct sembuf ops[] = {{0, -1, SEM_UNDO|IPC_NOWAIT}};
            int ret = semop(Handle, ops, 1);
    #else
            int ret = sem_trywait(Handle);
    #endif
            return ret == 0;
#endif
        }
    };

#if defined(_unix_)
    struct TPosixSemaphore {
        inline TPosixSemaphore(ui32 maxFreeCount) {
            if (sem_init(&S_, 0, maxFreeCount)) {
                ythrow TSystemError() << "can not init semaphore";
            }
        }

        inline ~TPosixSemaphore() throw () {
            VERIFY(sem_destroy(&S_) == 0, "semaphore destroy failed");
        }

        inline void Acquire() throw () {
            VERIFY(sem_wait(&S_) == 0, "semaphore acquire failed");
        }

        inline void Release() throw () {
            VERIFY(sem_post(&S_) == 0, "semaphore release failed");
        }

        inline bool TryAcquire() throw () {
            if (sem_trywait(&S_)) {
                VERIFY(errno == EAGAIN, "semaphore try wait failed");

                return false;
            }

            return true;
        }

        sem_t S_;
    };
#endif
}

class TSemaphore::TImpl: public TSemaphoreImpl {
public:
    inline TImpl(const char* name, ui32 maxFreeCount)
        : TSemaphoreImpl(name, maxFreeCount)
    {
    }
};

TSemaphore::TSemaphore(const char* name, ui32 maxFreeCount)
    : Impl_(new TImpl(name, maxFreeCount))
{
}

TSemaphore::~TSemaphore() throw () {
}

void TSemaphore::Release() throw () {
    Impl_->Release();
}

void TSemaphore::Acquire() throw () {
    Impl_->Acquire();
}

bool TSemaphore::TryAcquire() throw () {
    return Impl_->TryAcquire();
}

#if defined(_unix_)
class TFastSemaphore::TImpl: public TPosixSemaphore {
public:
    inline TImpl(ui32 n)
        : TPosixSemaphore(n)
    {
    }
};
#else
class TFastSemaphore::TImpl: public Stroka, public TSemaphoreImpl {
public:
    inline TImpl(ui32 n)
        : Stroka(ToString(RandomNumber<ui64>()))
        , TSemaphoreImpl(c_str(), n)
    {
    }
};
#endif

TFastSemaphore::TFastSemaphore(ui32 maxFreeCount)
    : Impl_(new TImpl(maxFreeCount))
{
}

TFastSemaphore::~TFastSemaphore() throw () {
}

void TFastSemaphore::Release() throw () {
    Impl_->Release();
}

void TFastSemaphore::Acquire() throw () {
    Impl_->Acquire();
}

bool TFastSemaphore::TryAcquire() throw () {
    return Impl_->TryAcquire();
}
