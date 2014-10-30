#pragma once

/// This code should not be used directly unless you really understand what you do.
/// If you need threads, use thread pool functionality in <util/thread/pool.h>
/// @see SystemThreadPool()

#include <util/generic/ptr.h>
#include <util/generic/stroka.h>

#include "defaults.h"

class TThread {
    public:
        typedef void* (*TThreadProc)(void*);
        typedef size_t TId;

        struct TParams {
            TThreadProc Proc;
            void* Data;
            size_t StackSize;
            Stroka Name;

            inline TParams()
                : Proc(0)
                , Data(0)
                , StackSize(0)
            {
            }

            inline TParams(TThreadProc proc, void* data)
                : Proc(proc)
                , Data(data)
                , StackSize(0)
            {
            }

            inline TParams(TThreadProc proc, void* data, size_t stackSize)
                : Proc(proc)
                , Data(data)
                , StackSize(stackSize)
            {
            }

            inline TParams& SetName(const Stroka& name) throw () {
                Name = name;

                return *this;
            }

            inline TParams& SetStackSize(size_t size) throw () {
                StackSize = size;

                return *this;
            }
        };

        TThread(const TParams& params);
        TThread(TThreadProc threadProc, void* param);

        ~TThread() throw ();

        void Start();

        void* Join();
        void Detach();
        bool Running() const throw ();
        TId Id() const throw ();

        static TId ImpossibleThreadId() throw ();
        static TId CurrentThreadId() throw ();
        /// content of `name` parameter is copied
        static void CurrentThreadSetName(const char* name);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

class TSimpleThread: public TThread {
public:
    TSimpleThread(size_t stackSize = 0);

    virtual ~TSimpleThread() throw () {
    }

    virtual void* ThreadProc() throw () = 0;
};
