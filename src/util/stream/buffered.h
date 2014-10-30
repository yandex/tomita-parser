#pragma once

#include "base.h"

#include <util/generic/forward.h>
#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>
#include <util/generic/typehelpers.h>

/// @addtogroup Streams_Buffered
/// @{

/// Создание буферизованного потока ввода из небуферизованного.
class TBufferedInput: public TInputStream, public IZeroCopyInput {
    public:
        TBufferedInput(TInputStream* slave, size_t buflen = 8192);
        virtual ~TBufferedInput() throw ();

        /*
         * does not clear data already buffered
         */
        void Reset(TInputStream* slave);

    protected:
        virtual size_t DoRead(void* buf, size_t len);
        virtual bool DoReadTo(Stroka& st, char ch);
        virtual bool DoNext(const void** ptr, size_t* len);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/// Создание буферизованного потока вывода из небуферизованного.
class TBufferedOutput: public TOutputStream {
    public:
        TBufferedOutput(TOutputStream* slave, size_t buflen = 8192);
        virtual ~TBufferedOutput() throw ();

        /*
         * propagate Flush() && Finish() events to slave stream
         */
        inline void SetPropagateMode(bool propagate) throw () {
            SetFlushPropagateMode(propagate);
            SetFinishPropagateMode(propagate);
        }

        void SetFlushPropagateMode(bool propagate) throw ();
        void SetFinishPropagateMode(bool propagate) throw ();

    protected:
        virtual void DoWrite(const void* data, size_t len);
        virtual void DoFlush();
        virtual void DoFinish();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

/*
 * helper class
 */
template <class T>
struct TBufferedStreamSelector {
    class TMyBufferedOutput: public TBufferedOutput {
        public:
            inline TMyBufferedOutput(TOutputStream* slave, size_t buflen)
                : TBufferedOutput(slave, buflen)
            {
                SetFinishPropagateMode(true);
            }
    };

    enum {
        IsDerivedFromInputStream = TIsDerived<TInputStream, T>::Result
    };

    enum {
        IsDerivedFromOutputStream = TIsDerived<TOutputStream, T>::Result
    };

    typedef typename TSelectType<IsDerivedFromInputStream, TBufferedInput, TMyBufferedOutput>::TResult TResult;
};

template <class TSlave>
class TBufferedSlave {
    public:
        template <class... R>
        inline TBufferedSlave(R&&... r)
            : Slave_(ForwardArg<R>(r)...)
        {
        }

        inline TSlave& Slave() throw () {
            return Slave_;
        }

        inline const TSlave& Slave() const throw () {
            return Slave_;
        }

    private:
        TSlave Slave_;
};

/*
 * useful class for simple buffered input/output streams creation
 */
/// Создание буферизованных потоков ввода и вывода из небуферизованных.
/// @details Примеры использования:
/// @li TBuffered<TFileInput> file_input(1024, "/path/to/file");
/// @li TBuffered<TFileOutput> file_output(1024, "/path/to/file");
/// @n где 1024 - размер буфера.
template <class TSlave>
class TBuffered: public TBufferedSlave<TSlave>,
                 public TBufferedStreamSelector<TSlave>::TResult {
        typedef TBufferedSlave<TSlave> TSlaveBase;
        typedef typename TBufferedStreamSelector<TSlave>::TResult TBase;
    public:
        inline TBuffered(size_t b)
            : TSlaveBase()
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1>
        inline TBuffered(size_t b, T1& t1)
            : TSlaveBase(t1)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1>
        inline TBuffered(size_t b, const T1& t1)
            : TSlaveBase(t1)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1, class T2>
        inline TBuffered(size_t b, const T1& t1, const T2& t2)
            : TSlaveBase(t1, t2)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1, class T2, class T3>
        inline TBuffered(size_t b, const T1& t1, const T2& t2, const T3& t3)
            : TSlaveBase(t1, t2, t3)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1, class T2, class T3, class T4>
        inline TBuffered(size_t b, const T1& t1, const T2& t2, const T3& t3, const T4& t4)
            : TSlaveBase(t1, t2, t3, t4)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1, class T2, class T3, class T4, class T5>
        inline TBuffered(size_t b, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
            : TSlaveBase(t1, t2, t3, t4, t5)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1, class T2, class T3, class T4, class T5, class T6>
        inline TBuffered(size_t b, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6)
            : TSlaveBase(t1, t2, t3, t4, t5, t6)
            , TBase(&TSlaveBase::Slave(), b)
        {
        }

        template <class T1, class T2>
        inline TBuffered(size_t buflen, const T1& t1, T2& t2)
            : TSlaveBase(t1, t2)
            , TBase(&TSlaveBase::Slave(), buflen)
        {
        }

        template <class T1, class T2>
        inline TBuffered(size_t buflen, T1& t1, const T2& t2)
            : TSlaveBase(t1, t2)
            , TBase(&TSlaveBase::Slave(), buflen)
        {
        }

        template <class T1, class T2>
        inline TBuffered(size_t buflen, T1& t1, T2& t2)
            : TSlaveBase(t1, t2)
            , TBase(&TSlaveBase::Slave(), buflen)
        {
        }
};

/// @}
