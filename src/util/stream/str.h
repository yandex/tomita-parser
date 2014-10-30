#pragma once

#include "base.h"

#include <util/generic/stroka.h>
#include <util/generic/noncopyable.h>

/// @addtogroup Streams_Strings
/// @{

/// Поток ввода для чтения данных из строки.
class TStringInput: public TInputStream {
    public:
        inline TStringInput(const Stroka& s) throw ()
            : S_(s)
            , Pos_(0)
        {
        }

        virtual ~TStringInput() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        const Stroka& S_;
        size_t Pos_;

    friend class TStringStream;
};

/// Поток вывода для записи данных в строку.
class TStringOutput: public TOutputStream, private TNonCopyable {
    public:
        inline TStringOutput(Stroka& s) throw ()
            : S_(s)
        {
        }

        virtual ~TStringOutput() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);

    private:
        Stroka& S_;
};

/// Поток ввода/вывода для чтения/записи данных в строку.
class TStringStream: public Stroka
                   , public TStringInput
                   , public TStringOutput {
    public:
        inline TStringStream()
            : TStringInput(Str())
            , TStringOutput(Str())
        {
        }

        inline TStringStream(const TStringStream& other)
            : Stroka(other)
            , TStringInput(Str())
            , TStringOutput(Str())
        {
        }

        inline TStringStream& operator=(const TStringStream& other) {
            // All references remain alive, we need to change position only
            Stroka& self = *this;
            self = other;
            Pos_ = other.Pos_;
            return *this;
        }

        virtual ~TStringStream() throw ();

        /*
         * compat(with STL) methods
         */
        inline Stroka& Str() throw () {
            return *this;
        }

        inline const Stroka& Str() const throw () {
            return *this;
        }
};

/// @}
