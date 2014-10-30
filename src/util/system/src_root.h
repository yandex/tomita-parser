#pragma once

namespace NPrivate {
    struct TStaticBuf {
        inline TStaticBuf(const char* data, unsigned len) throw ()
            : Data(data)
            , Len(len)
        {
        }

        template <class T>
        inline T As() const throw () {
            return T(Data, Len);
        }

        template <class T>
        inline operator T () const throw () {
            return this->As<T>();
        }

        const char* Data;
        unsigned Len;
    };

    #define STATIC_BUF(x) ::NPrivate::TStaticBuf(x, sizeof(x) - 1)

    TStaticBuf StripSourceRoot(const TStaticBuf& cur, const TStaticBuf& f) throw ();

    //$(SRC_ROOT)/prj/blah.cpp -> prj/blah.cpp
    inline TStaticBuf StripSourceRoot(const TStaticBuf& f) throw () {
        return StripSourceRoot(STATIC_BUF(__FILE__), f);
    }
}

//should be used instead of __FILE__
#define __SOURCE_FILE__ ::NPrivate::StripSourceRoot(STATIC_BUF(__FILE__))
