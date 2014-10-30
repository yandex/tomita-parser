#pragma once

#include "abc.h"
#include <library/unicode_normalization/normalization.h>
#include <util/generic/ptr.h>

namespace NLemmerAux {
    template<size_t staticBufferSize>
    class TBuffer : TNonCopyable {
    private:
        TChar BufferStatic[staticBufferSize];
        THolder<TChar, TDeleteArray> BufferDynamic;
        size_t BufferSize;
        TChar* Buffer;
    public:
        TBuffer()
            : BufferSize(staticBufferSize)
            , Buffer(BufferStatic)
        {}
        TChar* operator()(size_t size) {
            if (size > BufferSize) {
                size_t newBufferSize = BufferSize * 2;
                if (newBufferSize < size)
                    newBufferSize = size;
                THolder<TChar> newBuffer(new TChar[newBufferSize]);
                memcpy(newBuffer.Get(), Buffer, BufferSize * sizeof(TChar));
                BufferDynamic.Reset(newBuffer.Release());
                BufferSize = newBufferSize;
                Buffer = BufferDynamic.Get();
            }
            return Buffer;
        }
        const TChar* GetPtr() const {
            return Buffer;
        }
        size_t GetSize() const {
            return BufferSize;
        }
    };

    class TLimitedString {
    private:
        TChar* const Buffer;
        const size_t MaxLength;
        size_t Len;
        bool Valid;
    public:
        TLimitedString(TChar* buf, size_t maxLen)
            : Buffer(buf)
            , MaxLength(maxLen)
            , Len(0)
            , Valid(true)
        {}
        void push_back(TChar c) {
            if (Len < MaxLength)
                Buffer[Len++] = c;
            else
                Valid = false;
        }
        size_t Length() const {
            return Len;
        }
        const TChar* GetStr() const {
            return Buffer;
        }
        bool IsValid() const {
            return Valid;
        }
    };

    template <class T>
    inline void Decompose(const TChar* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFKD>(str, len, out);
    }

    // uses softer denormalization than Decompose method, for example ¹ does not decomposes to No, see http://www.unicode.org/reports/tr15/
    template <class T>
    inline void SoftDecompose(const TChar* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFD>(str, len, out);
    }

    inline NLemmer::NDetail::TTransdRet SoftDecompose(const TChar* str, size_t len, TChar* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        SoftDecompose(str, len, out);
        return NLemmer::NDetail::TTransdRet (out.Length(), len, out.IsValid(), true);
    }

    inline NLemmer::NDetail::TTransdRet Decompose(const TChar* str, size_t len, TChar* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        Decompose(str, len, out);
        return NLemmer::NDetail::TTransdRet (out.Length(), len, out.IsValid(), true);
    }

    inline Wtroka Decompose(const TChar* str, size_t len) {
        Wtroka out;
        out.reserve(len);
        Decompose(str, len, out);
        return out;
    }

    inline Wtroka SoftDecompose(const TChar* str, size_t len) {
        Wtroka out;
        out.reserve(len);
        SoftDecompose(str, len, out);
        return out;
    }

    template <class T>
    inline void Compose(const TChar* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFKC>(str, len, out);
    }

    // uses softer normalization than Compose method, see http://www.unicode.org/reports/tr15/
    template <class T>
    inline void SoftCompose(const TChar* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFC>(str, len, out);
    }

    inline NLemmer::NDetail::TTransdRet Compose(const TChar* str, size_t len, TChar* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        Compose(str, len, out);
        return NLemmer::NDetail::TTransdRet (out.Length(), len, out.IsValid(), true);
    }
    inline NLemmer::NDetail::TTransdRet SoftCompose(const TChar* str, size_t len, TChar* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        SoftCompose(str, len, out);
        return NLemmer::NDetail::TTransdRet (out.Length(), len, out.IsValid(), true);
    }
}
