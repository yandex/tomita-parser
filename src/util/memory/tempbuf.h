#pragma once

#include <util/system/defaults.h>
#include <util/generic/ptr.h>

/*
 * This is really fast buffer for temporary data.
 * For small sizes it works almost fast as pure alloca()
 * (by using perthreaded list of free blocks),
 * for big sizes it works as fast as malloc()/operator new()/...
 */
class TTempBuf {
    public:
        /*
         * we do not want many friends for this class :)
         */
        class TImpl;

        TTempBuf();
        TTempBuf(size_t len);
        TTempBuf(const TTempBuf& b) throw ();
        ~TTempBuf() throw ();

        TTempBuf& operator= (const TTempBuf& b) throw ();

        char* Data() throw ();
        const char* Data() const throw ();
        char* Current() throw ();
        const char* Current() const throw ();
        size_t Size() const throw ();
        size_t Filled() const throw ();
        size_t Left() const throw ();
        void Reset() throw ();
        void SetPos(size_t off);
        void Proceed(size_t off);
        void Append(const void* data, size_t len);

    private:
        TIntrusivePtr<TImpl> Impl_;
};

template <typename T>
class TTempArray: private TTempBuf {
    private:
        static T* TypedPointer(char* pointer) throw () {
            return reinterpret_cast<T*>(pointer);
        }
        static const T* TypedPointer(const char* pointer) throw () {
            return  reinterpret_cast<const T*>(pointer);
        }
        static size_t RawSize(size_t size) throw () {
            return size * sizeof(T);
        }
        static size_t TypedSize(size_t size) throw () {
            return size / sizeof(T);
        }

    public:
        TTempArray() {
        }

        TTempArray(size_t len)
            : TTempBuf(RawSize(len))
        {
        }

        T* Data() throw () {
            return TypedPointer(TTempBuf::Data());
        }

        const T* Data() const throw () {
            return TypedPointer(TTempBuf::Data());
        }

        T* Current() throw () {
            return TypedPointer(TTempBuf::Current());
        }

        const T* Current() const throw () {
            return TypedPointer(TTempBuf::Current());
        }

        size_t Size() const throw () {
            return TypedSize(TTempBuf::Size());
        }
        size_t Filled() const throw () {
            return TypedSize(TTempBuf::Filled());
        }

        void Proceed(size_t off) {
            TTempBuf::Proceed(RawSize(off));
        }
};
