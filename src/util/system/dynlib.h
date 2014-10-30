#pragma once

#include "defaults.h"

#include <util/generic/ptr.h>
#include <util/generic/stroka.h>

#define GET_FUNC(dll, name)          FUNC_##name((dll).Sym(#name))
#define GET_FUNC_OPTIONAL(dll, name) FUNC_##name((dll).SymOptional(#name))


#ifdef _win32_
    #define DEFAULT_DLLOPEN_FLAGS 0
#else
    #include <dlfcn.h>

    #ifndef RTLD_GLOBAL
        #define RTLD_GLOBAL (0)
    #endif

    #define DEFAULT_DLLOPEN_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#endif


class TDynamicLibrary {
    public:
        TDynamicLibrary() throw ();
        TDynamicLibrary(const Stroka& path, int flags = DEFAULT_DLLOPEN_FLAGS);
        ~TDynamicLibrary() throw ();

        void Open(const char* path, int flags = DEFAULT_DLLOPEN_FLAGS);
        void Close() throw ();
        void* SymOptional(const char* name) throw ();
        void* Sym(const char* name);
        bool IsLoaded() const throw ();
        void SetUnloadable(bool unloadable); // Set to false to avoid unloading on destructor

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

// a wrapper for a symbol
template <class TLib>
class TExternalSymbol {
private:
    TLib            *PLib;
    TDynamicLibrary *DLib;
    Stroka           lname;
    Stroka           vname;
public:
    TExternalSymbol() throw() {
        PLib = 0;
        DLib = 0;
    }
    TExternalSymbol(const TExternalSymbol& es) {
        PLib = 0;
        DLib = 0;
        if (es.IsDynamic())
            Open(~es.LibName(), ~es.VtblName());
        else if (es.IsStatic())
            SetSym(es.Symbol());
    }
    TExternalSymbol& operator=(const TExternalSymbol& es) {
        if (this != &es) {
            Close();
            if (es.IsDynamic())
                Open(~es.LibName(), ~es.VtblName());
            else if (es.IsStatic())
                SetSym(es.Symbol());
        }
        return *this;
    }
    ~TExternalSymbol() throw() {
        delete DLib;
    }
    // set the symbol from dynamic source
    void Open(const char* lib_name, const char* vtbl_name) {
        if (DLib != 0 || PLib != 0)
            return;
        try {
            DLib = new TDynamicLibrary();
            DLib->Open(lib_name);
            PLib = (TLib *)DLib->Sym(vtbl_name);
        } catch (...) {
            delete DLib;
            DLib = 0;
            throw;
        }
        lname = lib_name;
        vname = vtbl_name;
    }
    // set the symbol from static source
    void SetSym(TLib* pl) throw () {
        if (DLib == 0 && PLib == 0)
            PLib = pl;
    }
    void Close() throw () {
        delete DLib;
        DLib = 0;
        PLib = 0;
        lname.remove();
        vname.remove();
    }
    TLib* Symbol() const throw () {
        return PLib;
    }
    const Stroka& LibName() const throw () {
        return lname;
    }
    const Stroka& VtblName() const throw () {
        return vname;
    }
    bool IsStatic() const throw () {
        return DLib == 0 && PLib != 0;
    }
    bool IsDynamic() const throw () {
        return DLib && DLib->IsLoaded() && PLib != 0;
    }
};
