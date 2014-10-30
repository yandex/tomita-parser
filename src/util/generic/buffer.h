#pragma once

#include "utility.h"
#include "noncopyable.h"

#include <util/system/align.h>
#include <util/system/yassert.h>

#include <cstring>

class Stroka;

class TBuffer {
    public:
        typedef char* TIterator;
        typedef const char* TConstIterator;

        TBuffer(size_t len = 0);
        TBuffer(const Stroka& s);
        TBuffer(const char* buf, size_t len);

        TBuffer(const TBuffer& b)
            : Data_(0)
            , Len_(0)
            , Pos_(0)
        {
            *this = b;
        }

        TBuffer& operator= (const TBuffer& b) {
            Assign(b.Data(), b.Size());
            return *this;
        }

        ~TBuffer() throw ();

        inline void Clear() throw () {
            Pos_ = 0;
        }

        inline void EraseBack(size_t n) throw () {
            YASSERT(n <= Pos_);
            Pos_ -= n;
        }

        inline void Reset() throw () {
            TBuffer().Swap(*this);
        }

        inline void Assign(const char* data, size_t len) {
            Clear();
            Append(data, len);
        }

        inline void Assign(const char* b, const char* e) {
            Assign(b, e - b);
        }

        inline char* Data() throw () {
            return Data_;
        }

        inline const char* Data() const throw () {
            return Data_;
        }

        inline char* Pos() throw () {
            return Data_ + Pos_;
        }

        inline const char* Pos() const throw () {
            return Data_ + Pos_;
        }

        /// Used space in bytes (do not mix with Capacity!)
        inline size_t Size() const throw () {
            return Pos_;
        }

        inline bool Empty() const throw () {
            return !Size();
        }

        inline explicit operator bool () const throw () {
            return Size();
        }

        inline size_t Avail() const throw () {
            return Len_ - Pos_;
        }

        inline void Append(const char* buf, size_t len) {
            if (len > Avail()) {
                Reserve(Pos_ + len);
            }

            YASSERT(len <= Avail());

            memcpy(Data() + Pos_, buf, len);
            Pos_ += len;

            YASSERT(Pos_ <= Len_);
        }

        inline void Append(const char* b, const char* e) {
            Append(b, e - b);
        }

        inline void Append(char ch) {
            if (Len_ == Pos_) {
                Reserve(Len_ + 1);
            }

            *(Data() + Pos_++) = ch;
        }

        // Method is useful when first messages from buffer are processed, and
        // the last message in buffer is incomplete, so we need to move partial
        // message to the begin of the buffer and continue filling the buffer
        // from the network.
        inline void ChopHead(size_t count) {
            YASSERT(count <= Pos_);

            if (count == 0) {
                return;
            } else if (count == Pos_) {
                Pos_ = 0;
            } else {
                size_t newSize = Pos_ - count;
                memmove(Data_, Data_ + count, newSize);
                Pos_ = newSize;
            }
        }

        inline void Proceed(size_t pos) {
            Resize(pos);
        }

        inline void Advance(size_t len) {
            Proceed(Pos_ + len);
        }

        inline void Reserve(size_t len) {
            if (len > Len_) {
                DoReserve(len);
            }
        }

        inline void ShrinkToFit() {
            if (Pos_ < Len_) {
                Realloc(Pos_);
            }
        }

        inline void Resize(size_t len) {
            Reserve(len);
            Pos_ = len;
        }

        inline size_t Capacity() const throw () {
            return Len_;
        }

        inline void AlignUp(size_t align, char fillChar = '\0') {
            size_t diff = ::AlignUp(Pos_, align) - Pos_;
            while (diff-- > 0)
                Append(fillChar);
        }

        /*
         * some helpers...
         */
        inline char* operator~ () throw () {
            return Data();
        }

        inline const char* operator~ () const throw () {
            return Data();
        }

        inline size_t operator+ () const throw () {
            return Size();
        }

        inline void Swap(TBuffer& r) throw () {
            DoSwap(Data_, r.Data_);
            DoSwap(Len_, r.Len_);
            DoSwap(Pos_, r.Pos_);
        }

        /*
         * after this call buffer becomes empty
         */
        void AsString(Stroka& s);

        /*
         * iterator-like interface
         */
        inline TIterator Begin() throw () {
            return Data();
        }

        inline TIterator End() throw () {
            return Begin() + Size();
        }

        inline TConstIterator Begin() const throw () {
            return Data();
        }

        inline TConstIterator End() const throw () {
            return Begin() + Size();
        }

    private:
        void DoReserve(size_t len);
        void Realloc(size_t len);

    private:
        char* Data_;
        size_t Len_;
        size_t Pos_;
};
