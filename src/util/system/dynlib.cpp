#include "dynlib.h"

#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

#ifdef _win32_
    #include "winint.h"

    #define DLLOPEN(path, flags) LoadLibrary(path)
    #define DLLCLOSE(hndl)       FreeLibrary(hndl)
    #define DLLSYM(hndl, name)   GetProcAddress(hndl, name)
#else
    #include <dlfcn.h>

    #ifndef RTLD_GLOBAL
        #define RTLD_GLOBAL (0)
    #endif

    typedef void* HINSTANCE;

    #define DLLOPEN(path, flags) dlopen(path, flags)
    #define DLLCLOSE(hndl)       dlclose(hndl)
    #define DLLSYM(hndl, name)   dlsym(hndl, name)
#endif

inline Stroka DLLERR() {
#ifdef _unix_
    return dlerror();
#endif

#ifdef _win32_
    char* msg = 0;
    DWORD cnt = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&msg, 0, NULL);
    if (!msg)
        return "DLLERR() unknown error";
    while (cnt && isspace(msg[cnt-1]))
        cnt--;
    Stroka err(msg, 0, cnt);
    LocalFree(msg);
    return err;
#endif
}

class TDynamicLibrary::TImpl {
    private:
        inline TImpl(const char* path, int flags)
            : Module(DLLOPEN(path, flags))
            , Unloadable(true)
        {
            (void)flags;

            if (!Module) {
                ythrow yexception() <<  ~DLLERR();
            }
        }

        class TCreateMutex : public TMutex
        {
        };

    public:
        static inline TImpl* SafeCreate(const char* path, int flags) {
            TGuard<TCreateMutex> g(Singleton<TCreateMutex>());
            return new TImpl(path, flags);
        }

        inline ~TImpl() throw () {
            if (Module && Unloadable) {
                DLLCLOSE(Module);
            }
        }

        inline void* SymOptional(const char* name) throw () {
            return (void*)DLLSYM(Module, name);
        }

        inline void* Sym(const char* name) {
            void* symbol = SymOptional(name);

            if (symbol == 0) {
                ythrow yexception() <<  ~DLLERR();
            }

            return symbol;
        }

        inline void SetUnloadable(bool unloadable) {
            Unloadable = unloadable;
        }

    private:
        HINSTANCE Module;
        bool Unloadable;
};

TDynamicLibrary::TDynamicLibrary() throw () {
}

TDynamicLibrary::TDynamicLibrary(const Stroka& path, int flags) {
    Open(~path, flags);
}

TDynamicLibrary::~TDynamicLibrary() throw () {
}

void TDynamicLibrary::Open(const char* path, int flags) {
    Impl_.Reset(TImpl::SafeCreate(path, flags));
}

void TDynamicLibrary::Close() throw () {
    Impl_.Destroy();
}

void* TDynamicLibrary::SymOptional(const char* name) throw () {
    if (!IsLoaded()) {
        return 0;
    }

    return Impl_->SymOptional(name);
}

void* TDynamicLibrary::Sym(const char* name) {
    if (!IsLoaded()) {
        ythrow yexception() << "library not loaded";
    }

    return Impl_->Sym(name);
}

bool TDynamicLibrary::IsLoaded() const throw () {
    return (bool)Impl_.Get();
}

void TDynamicLibrary::SetUnloadable(bool unloadable) {
    Impl_->SetUnloadable(unloadable);
}
