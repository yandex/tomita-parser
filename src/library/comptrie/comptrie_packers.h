#pragma once

#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/set.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/draft/bitutils.h>
// Data serialization strategy class.
// Default realization can pack only limited range of types, but you can pack any data other using your own strategy class.

template <class T>
class TNullPacker { // Very effective package class - pack any data into zero bytes :)
public:
    void UnpackLeaf(const char*, T& t) const {
        t = T();
    }

    void PackLeaf(char* , const T&, size_t) const {}

    size_t MeasureLeaf(const T&) const {
        return 0;
    }

    size_t SkipLeaf(const char*) const {
        return 0;
    }
};

template<typename T>
class TAsIsPacker { // this packer is not really a packer...
public:
    void UnpackLeaf(const char* p, T& t) const {
        memcpy(&t, p, sizeof(T));
    }
    void PackLeaf(char* buffer, const T& data, size_t computedSize) const {
        YASSERT(computedSize == sizeof(data));
        memcpy(buffer, &data, sizeof(T));
    }
    size_t MeasureLeaf(const T& data) const {
        UNUSED(data);
        return sizeof(T);
    }
    size_t SkipLeaf(const char* ) const {
        return sizeof(T);
    }
};

// Implementation

namespace NCompactTrie {
    template <class T>
    inline ui64 ConvertIntegral(const T& data);

    template <>
    inline ui64 ConvertIntegral(const i64& data) {
        if (data < 0) {
            return (static_cast<ui64>(-1 * data) << 1) | 1;
        } else {
            return static_cast<ui64>(data) << 1;
        }
    }

    namespace NImpl {
        template <class T, bool isSigned>
        struct TConvertImpl {
            static inline ui64 Convert(const T& data);
        };

        template <class T>
        struct TConvertImpl<T, true> {
            static inline ui64 Convert(const T& data) {
                return ConvertIntegral<i64>(static_cast<i64>(data));
            }
        };

        template <class T>
        struct TConvertImpl<T, false> {
            static inline ui64 Convert(const T& data) {
                return data;
            }
        };
    }

    template <class T>
    inline ui64 ConvertIntegral(const T& data) {
        return NImpl::TConvertImpl<T, TTypeTraits<T>::IsSignedInt>::Convert(data);
    }

    //---------------------------------
    // TIntegralPacker --- for integral types.

    template <class T>
    class TIntegralPacker { // can pack only integral types <= ui64
    public:
        void UnpackLeaf(const char* p, T& t) const;
        void PackLeaf(char* buffer, const T& data, size_t size) const;
        size_t MeasureLeaf(const T& data) const;
        size_t SkipLeaf(const char* p) const;
    };

    template <>
    inline void TIntegralPacker<ui64>::UnpackLeaf(const char* p, ui64& result) const {
        size_t taillen = 0;
        char ch;

        for (ch = *(p++); ch & (1 << 7) ; ch <<= 1)
            ++taillen;

        result = (ch & 0xFF) >> taillen;

        while (taillen--)
            result = ((result << 8) | (*(p++) & 0xFF));
    }

    template <>
    inline size_t TIntegralPacker<ui64>::MeasureLeaf(const ui64& val) const {
        const size_t MAX_SIZE = sizeof(ui64) + sizeof(ui64) / 8;

        ui64 value = val;
        size_t len = 1;

        value >>= 7;
        for (; value && len < MAX_SIZE ; value >>= 7)
            ++len;

        return len;
    }

    template <>
    inline void TIntegralPacker<ui64>::PackLeaf(char* buffer, const ui64& val, size_t len) const {
        ui64 value = val;
        int lenmask = 0;

        for (size_t i = len - 1; i; --i)  {
            buffer[i] = (char)(value & 0xFF);
            value >>= 8;
            lenmask = ((lenmask >> 1) | (1 << 7));
        }

        buffer[0] = (char)(lenmask | value);
    }

#define _X_4(X) X, X, X, X
#define _X_8(X) _X_4(X), _X_4(X)
#define _X_16(X) _X_8(X), _X_8(X)
#define _X_32(X) _X_16(X), _X_16(X)
#define _X_64(X) _X_32(X), _X_32(X)
#define _X_128(X) _X_64(X), _X_64(X)

    static const ui8 SkipTable[256] = { _X_128(1), _X_64(2), _X_32(3), _X_16(4), _X_8(5), _X_4(6), 7, 7, 8, 9};

#undef _X_4
#undef _X_8
#undef _X_16
#undef _X_32
#undef _X_64
#undef _X_128

    template <>
    inline size_t TIntegralPacker<ui64>::SkipLeaf(const char* p) const {
        return SkipTable[(ui8)*p];
    }

    namespace NImpl {
        template <class T, bool isSigned>
        struct TUnpackLeafImpl {
            inline void UnpackLeaf(const char* p, T& t) const;
        };
        template <class T>
        struct TUnpackLeafImpl<T, true> {
            inline void UnpackLeaf(const char* p, T& t) const {
                ui64 val;
                TIntegralPacker<ui64>().UnpackLeaf(p, val);
                if (val & 1) {
                    t = -1 * static_cast<T>(val >> 1);
                } else {
                    t = static_cast<T>(val >> 1);
                }
            }
        };
        template <class T>
        struct TUnpackLeafImpl<T, false> {
            inline void UnpackLeaf(const char* p, T& t) const {
                ui64 tmp;
                TIntegralPacker<ui64>().UnpackLeaf(p, tmp);
                t = static_cast<T>(tmp);
            }
        };
    }

    template <class T>
    inline void TIntegralPacker<T>::UnpackLeaf(const char* p, T& t) const {
        NImpl::TUnpackLeafImpl<T, TTypeTraits<T>::IsSignedInt>().UnpackLeaf(p, t);
    }

    template <class T>
    inline void TIntegralPacker<T>::PackLeaf(char* buffer, const T& data, size_t size) const {
        TIntegralPacker<ui64>().PackLeaf(buffer, ConvertIntegral<T>(data), size);
    }

    template <class T>
    inline size_t TIntegralPacker<T>::MeasureLeaf(const T& data) const {
        return TIntegralPacker<ui64>().MeasureLeaf(ConvertIntegral<T>(data));
    }

    template <class T>
    inline size_t TIntegralPacker<T>::SkipLeaf(const char* p) const {
        return TIntegralPacker<ui64>().SkipLeaf(p);
    }

    //-------------------------------------------
    // TFPPacker --- for float/double
    namespace NImpl {
        template <class TFloat, class TUInt>
        class TFPPackerBase {
        protected:
            typedef TIntegralPacker<TUInt> TPacker;

            union THelper {
                TFloat F;
                TUInt U;
            };

            TFloat FromUInt(TUInt u) const {
                THelper h;
                h.U = NBitUtils::ReverseBytes(u);
                return h.F;
            }

            TUInt ToUInt(TFloat f) const {
                THelper h;
                h.F = f;
                return NBitUtils::ReverseBytes(h.U);
            }
        public:
            void UnpackLeaf(const char* c, TFloat& t) const {
                TUInt u = 0;
                TPacker().UnpackLeaf(c, u);
                t = FromUInt(u);
            }

            void PackLeaf(char* c, const TFloat& t, size_t sz) const {
                TPacker().PackLeaf(c, ToUInt(t), sz);
            }

            size_t MeasureLeaf(const TFloat& t) const {
                return TPacker().MeasureLeaf(ToUInt(t));
            }

            size_t SkipLeaf(const char* c) const {
                return TPacker().SkipLeaf(c);
            }
        };
    }

    class TFloatPacker : public NImpl::TFPPackerBase<float, ui32> {
    };

    class TDoublePacker : public NImpl::TFPPackerBase<double, ui64> {
    };

    //-------------------------------------------
    // TStringPacker --- for Stroka/Wtroka and TStringBuf.

    template <class TString>
    class TStringPacker {
    public:
        void UnpackLeaf(const char* p, TString& t) const;
        void PackLeaf(char* buffer, const TString& data, size_t size) const;
        size_t MeasureLeaf(const TString& data) const;
        size_t SkipLeaf(const char* p) const;
    };

    template<class TString>
    inline void TStringPacker<TString>::UnpackLeaf(const char* buf, TString& t) const {
        size_t len;
        TIntegralPacker<size_t>().UnpackLeaf(buf, len);
        size_t start = TIntegralPacker<size_t>().SkipLeaf(buf);
        t = TString((const typename TString::char_type*)(buf + start), len);
    }

    template<class TString>
    inline void TStringPacker<TString>::PackLeaf(char* buf, const TString& str, size_t size) const {
        size_t len = str.size();
        size_t lenChar = len * sizeof(typename TString::char_type);
        size_t start = size - lenChar;
        TIntegralPacker<size_t>().PackLeaf(buf, len, TIntegralPacker<size_t>().MeasureLeaf(len));
        memcpy(buf + start, str.c_str(), lenChar);
    }

    template<class TString>
    inline size_t TStringPacker<TString>::MeasureLeaf(const TString& str) const {
        size_t len = str.size();
        return TIntegralPacker<size_t>().MeasureLeaf(len) + len * sizeof(typename TString::char_type);
    }

    template<class TString>
    inline size_t TStringPacker<TString>::SkipLeaf(const char* buf) const {
        size_t result = TIntegralPacker<size_t>().SkipLeaf(buf);
        {
            size_t len;
            TIntegralPacker<size_t>().UnpackLeaf(buf, len);
            result += len * sizeof(typename TString::char_type);
        }
        return result;
    }

    template<class T>
    class TPacker;

    // TContainerPacker --- for any container
    // Requirements to class C:
    //    - has method size() (returns size_t)
    //    - has subclass C::value_type
    //    - has subclass C::const_iterator
    //    - has methods begin() and end() (return C::const_iterator)
    //    - has method insert(C::const_iterator, const C::value_type&)
    //  Examples: yvector, ylist, yset
    //  Requirements to class EP: has methods as in any packer (UnpackLeaf, PackLeaf, MeasureLeaf, SkipLeaf) that
    //    are applicable to C::value_type

    template<typename T>
    struct TContainerInfo {
        enum {
            IsVector = 0
        };
    };

    template<typename T>
    struct TContainerInfo< std::vector<T> > {
        enum {
            IsVector = 1
        };
    };

    template<typename T>
    struct TContainerInfo< yvector<T> > {
        enum {
            IsVector = 1
        };
    };

    template<bool IsVector>
    class TContainerPackerHelper {
    };

    template<>
    class TContainerPackerHelper<false> {
    public:
        template<class Packer, class Container>
        static void UnpackLeaf(Packer& p, const char* buffer, Container& c)
        {
            p.UnpackLeafSimple(buffer,c);
        }
    };

    template<>
    class TContainerPackerHelper<true> {
    public:
        template<class Packer, class Container>
        static void UnpackLeaf(Packer& p, const char* buffer, Container& c)
        {
            p.UnpackLeafVector(buffer,c);
        }
    };

    template< class C, class EP = TPacker<typename C::value_type> >
    class TContainerPacker {
    private:
        typedef C TContainer;
        typedef EP TElementPacker;
        typedef typename TContainer::const_iterator TElementIterator;

        void UnpackLeafSimple(const char* buffer, TContainer& c) const;
        void UnpackLeafVector(const char* buffer, TContainer& c) const;

        friend class TContainerPackerHelper<TContainerInfo<C>::IsVector>;
    public:
        void UnpackLeaf(const char* buffer, TContainer& c) const {
            TContainerPackerHelper<TContainerInfo<C>::IsVector>::UnpackLeaf(*this, buffer, c);
        }
        void PackLeaf(char* buffer, const TContainer& data, size_t size) const;
        size_t MeasureLeaf(const TContainer& data) const;
        size_t SkipLeaf(const char* buffer) const;
    };

    template<class C, class EP>
    inline void TContainerPacker<C,EP>::UnpackLeafSimple(const char* buffer, C& result) const {
        size_t offset = TIntegralPacker<size_t>().SkipLeaf(buffer); // first value is the total size (not needed here)
        size_t len;
        TIntegralPacker<size_t>().UnpackLeaf(buffer + offset, len);
        offset += TIntegralPacker<size_t>().SkipLeaf(buffer + offset);

        result.clear();

        typename C::value_type value;
        for (size_t i = 0; i < len; i++) {
            TElementPacker().UnpackLeaf(buffer + offset, value);
            result.insert(result.end(), value);
            offset += TElementPacker().SkipLeaf(buffer + offset);
        }
    }

    template<class C, class EP>
    inline void TContainerPacker<C,EP>::UnpackLeafVector(const char* buffer, C& result) const {
        size_t offset = TIntegralPacker<size_t>().SkipLeaf(buffer); // first value is the total size (not needed here)
        size_t len;
        TIntegralPacker<size_t>().UnpackLeaf(buffer + offset, len);
        offset += TIntegralPacker<size_t>().SkipLeaf(buffer + offset);
        result.resize(len);

        for (size_t i = 0; i < len; i++) {
            TElementPacker().UnpackLeaf(buffer + offset, result[i]);
            offset += TElementPacker().SkipLeaf(buffer + offset);
        }
    }

    template<class C, class EP>
    inline void TContainerPacker<C, EP>::PackLeaf(char* buffer, const C& data, size_t size) const {
        size_t sizeOfSize = TIntegralPacker<size_t>().MeasureLeaf(size);
        TIntegralPacker<size_t>().PackLeaf(buffer, size, sizeOfSize);
        size_t len = data.size();
        size_t curSize = TIntegralPacker<size_t>().MeasureLeaf(len);
        TIntegralPacker<size_t>().PackLeaf(buffer + sizeOfSize, len, curSize);
        curSize += sizeOfSize;
        for (TElementIterator p = data.begin(); p != data.end(); p++) {
            size_t sizeChange = TElementPacker().MeasureLeaf(*p);
            TElementPacker().PackLeaf(buffer + curSize, *p, sizeChange);
            curSize += sizeChange;
        }
        YASSERT(curSize == size);
    }

    template<class C, class EP>
    inline size_t TContainerPacker<C, EP>::MeasureLeaf(const C& data) const {
        size_t curSize = TIntegralPacker<size_t>().MeasureLeaf(data.size());
        for (TElementIterator p = data.begin(); p != data.end(); p++)
            curSize += TElementPacker().MeasureLeaf(*p);
        size_t extraSize = TIntegralPacker<size_t>().MeasureLeaf(curSize);

        // Double measurement protects against sudden increases in extraSize,
        // e.g. when curSize is 127 and stays in one byte, but curSize + 1 requires two bytes.

        extraSize = TIntegralPacker<size_t>().MeasureLeaf(curSize + extraSize);
        YASSERT(extraSize == TIntegralPacker<size_t>().MeasureLeaf(curSize + extraSize));
        return curSize + extraSize;
    }

    template<class C, class EP>
    inline size_t TContainerPacker<C, EP>::SkipLeaf(const char* buffer) const {
        size_t value;
        TIntegralPacker<size_t>().UnpackLeaf(buffer, value);
        return value;
    }

    // TPairPacker --- for TPair<T1, T2> (any two types; can be nested)
    // TPacker<T1> and TPacker<T2> should be valid classes

    template<class T1, class T2, class TPacker1 = TPacker<T1>, class TPacker2 = TPacker<T2> >
    class TPairPacker {
    private:
        typedef TPair<T1, T2> TMyPair;
    public:
        void UnpackLeaf(const char* buffer, TMyPair& pair) const;
        void PackLeaf(char* buffer, const TMyPair& data, size_t size) const;
        size_t MeasureLeaf(const TMyPair& data) const;
        size_t SkipLeaf(const char* buffer) const;
    };


    template<class T1, class T2, class TPacker1, class TPacker2>
    inline void TPairPacker<T1, T2, TPacker1, TPacker2>::UnpackLeaf(const char* buffer, TPair<T1, T2>& pair) const {
        TPacker1().UnpackLeaf(buffer, pair.first);
        size_t size = TPacker1().SkipLeaf(buffer);
        TPacker2().UnpackLeaf(buffer + size, pair.second);
    }

    template<class T1, class T2, class TPacker1, class TPacker2>
    inline void TPairPacker<T1, T2, TPacker1, TPacker2>::PackLeaf(char* buffer, const TPair<T1, T2>& data, size_t size) const {
        size_t size1 = TPacker1().MeasureLeaf(data.first);
        TPacker1().PackLeaf(buffer, data.first, size1);
        size_t size2 = TPacker2().MeasureLeaf(data.second);
        TPacker2().PackLeaf(buffer + size1, data.second, size2);
        YASSERT(size == size1 + size2);
    }

    template<class T1, class T2, class TPacker1, class TPacker2>
    inline size_t TPairPacker<T1, T2, TPacker1, TPacker2>::MeasureLeaf(const TPair<T1, T2>& data) const {
        size_t size1 = TPacker1().MeasureLeaf(data.first);
        size_t size2 = TPacker2().MeasureLeaf(data.second);
        return size1 + size2;
    }

    template<class T1, class T2, class TPacker1, class TPacker2>
    inline size_t TPairPacker<T1, T2, TPacker1, TPacker2>::SkipLeaf(const char* buffer) const {
        size_t size1 = TPacker1().SkipLeaf(buffer);
        size_t size2 = TPacker2().SkipLeaf(buffer + size1);
        return size1 + size2;
    }

    //------------------------------------
    // TPacker --- the generic packer.

    template <class T, bool IsIntegral>
    class TPackerImpl;

    template <class T>
    class TPackerImpl<T, true> : public TIntegralPacker<T> {
    };
    // No implementation for non-integral types.

    template <class T>
    class TPacker : public TPackerImpl<T, TTypeTraits<T>::IsIntegral> {
    };

    template<>
    class TPacker<float> : public TAsIsPacker<float> {
    };

    template<>
    class TPacker<double> : public TAsIsPacker<double> {
    };

    template <>
    class TPacker<Stroka> : public TStringPacker<Stroka> {
    };

    template <>
    class TPacker<Wtroka> : public TStringPacker<Wtroka> {
    };

    template <>
    class TPacker<TStringBuf> : public TStringPacker<TStringBuf> {
    };

    template <>
    class TPacker<TWtringBuf> : public TStringPacker<TWtringBuf> {
    };

    template <class T>
    class TPacker<std::vector<T> > : public TContainerPacker<std::vector<T> > {
    };

    template <class T>
    class TPacker<yvector<T> > : public TContainerPacker<yvector<T> > {
    };

    template <class T>
    class TPacker<std::list<T> > : public TContainerPacker<std::list<T> > {
    };

    template <class T>
    class TPacker<ylist<T> > : public TContainerPacker<ylist<T> > {
    };

    template <class T>
    class TPacker<std::set<T> > : public TContainerPacker<std::set<T> > {
    };

    template <class T>
    class TPacker<yset<T> > : public TContainerPacker<yset<T> > {
    };

    template<class T1, class T2>
    class TPacker<TPair<T1, T2> > : public TPairPacker<T1, T2> {
    };


} // namespace NCompactTrie

template <class T>
class TCompactTriePacker {
public:
    void UnpackLeaf(const char* p, T& t) const {
        NCompactTrie::TPacker<T>().UnpackLeaf(p, t);
    }
    void PackLeaf(char* buffer, const T& data, size_t computedSize) const {
        NCompactTrie::TPacker<T>().PackLeaf(buffer, data, computedSize);
    }
    size_t MeasureLeaf(const T& data) const {
        return NCompactTrie::TPacker<T>().MeasureLeaf(data);
    }
    size_t SkipLeaf(const char* p) const // this function better be fast because it is very frequently used
    {
        return NCompactTrie::TPacker<T>().SkipLeaf(p);
    }
};
