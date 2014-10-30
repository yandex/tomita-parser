#pragma once

#include <util/generic/fwd.h>
#include <util/generic/buffer.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/generic/array.h>
#include <util/generic/queue.h>
#include <util/stream/base.h>

#include <climits>

template <typename T>
class TSerializeTypeTraits {
public:
    /*
     *  pointer types cannot be serialized as POD-type
     */
    enum {
        IsSerializablePod = TTypeTraits<T>::IsPod && !TTypeTraits<T>::IsPointer
    };
};

struct TSerializeException: public yexception {
};

struct TLoadEOF: public TSerializeException {
};

template <class T>
static inline void Save(TOutputStream* out, const T& t);

template <class T>
static inline void SaveArray(TOutputStream* out, const T* t, size_t len);

template <class T>
static inline void Load(TInputStream* in, T& t);

template <class T>
static inline void LoadArray(TInputStream* in, T* t, size_t len);

template <class T, class TStorage>
static inline void Load(TInputStream* in, T& t, TStorage& pool);

template <class T, class TStorage>
static inline void LoadArray(TInputStream* in, T* t, size_t len, TStorage& pool);

template <class T>
static inline void SavePodType(TOutputStream* rh, const T& t) {
    rh->Write(&t, sizeof(T));
}

template <class T>
static inline void LoadPodType(TInputStream* rh, T& t) {
    const size_t res = rh->Load(&t, sizeof(T));

    if (res != sizeof(T)) {
        ythrow TLoadEOF() << "can not load pod type(" << sizeof(T) << ", " << res << " bytes)";
    }
}

template <class T>
static inline void SavePodArray(TOutputStream* rh, const T* arr, size_t count) {
    rh->Write(arr, sizeof(T) * count);
}

template <class T>
static inline void LoadPodArray(TInputStream* rh, T* arr, size_t count) {
    const size_t len = sizeof(T) * count;
    const size_t res = rh->Load(arr, len);

    if (res != len) {
        ythrow TLoadEOF() << "can not load pod array(" << len << ", " << res << " bytes)";
    }
}

template <class It>
static inline void SaveIterRange(TOutputStream* rh, It b, It e) {
    while (b != e) {
        ::Save(rh, *b++);
    }
}

template <class It>
static inline void LoadIterRange(TInputStream* rh, It b, It e) {
    while (b != e) {
        ::Load(rh, *b++);
    }
}

template <class It, class TStorage>
static inline void LoadIterRange(TInputStream* rh, It b, It e, TStorage& pool) {
    while (b != e) {
        ::Load(rh, *b++, pool);
    }
}

template <class T, bool isPod>
struct TSerializerTakingIntoAccountThePodType {
    static inline void Save(TOutputStream* out, const T& t) {
        ::SavePodType(out, t);
    }

    static inline void Load(TInputStream* in, T& t) {
        ::LoadPodType(in, t);
    }

    template <class TStorage>
    static inline void Load(TInputStream* in, T& t, TStorage& /*pool*/) {
        ::LoadPodType(in, t);
    }

    static inline void SaveArray(TOutputStream* out, const T* t, size_t len) {
        ::SavePodArray(out, t, len);
    }

    static inline void LoadArray(TInputStream* in, T* t, size_t len) {
        ::LoadPodArray(in, t, len);
    }
};

template <class T, bool newStyle>
struct TSerializerMethodSelector {
    static inline void Save(TOutputStream* out, const T& t) {
        //assume Save clause do not change t
        (const_cast<T&>(t)).SaveLoad(out);
    }

    static inline void Load(TInputStream* in, T& t) {
        t.SaveLoad(in);
    }

    template <class TStorage>
    static inline void Load(TInputStream* in, T& t, TStorage& pool) {
        t.SaveLoad(in, pool);
    }
};

template <class T>
struct TSerializerMethodSelector<T, false> {
    static inline void Save(TOutputStream* out, const T& t) {
        t.Save(out);
    }

    static inline void Load(TInputStream* in, T& t) {
        t.Load(in);
    }

    template <class TStorage>
    static inline void Load(TInputStream* in, T& t, TStorage& pool) {
        t.Load(in, pool);
    }
};

namespace NHasSaveLoad {
    HAS_MEMBER(SaveLoad)
}

template <class T>
struct TSerializerTakingIntoAccountThePodType<T, false>: public TSerializerMethodSelector<T, NHasSaveLoad::THasSaveLoad<T>::Result> {
    static inline void SaveArray(TOutputStream* out, const T* t, size_t len) {
        ::SaveIterRange(out, t, t + len);
    }

    static inline void LoadArray(TInputStream* in, T* t, size_t len) {
        ::LoadIterRange(in, t, t + len);
    }

    template <class TStorage>
    static inline void LoadArray(TInputStream* in, T* t, size_t len, TStorage& pool) {
        ::LoadIterRange(in, t, t + len, pool);
    }
};

template <class It, bool isPtr>
struct TRangeSerialize {
    static inline void Save(TOutputStream* rh, It b, It e) {
        SaveArray(rh, b, e - b);
    }

    static inline void Load(TInputStream* rh, It b, It e) {
        LoadArray(rh, b, e - b);
    }

    template <class TStorage>
    static inline void Load(TInputStream* rh, It b, It e, TStorage& pool) {
        LoadArray(rh, b, e - b, pool);
    }
};

template <class It>
struct TRangeSerialize<It, false> {
    static inline void Save(TOutputStream* rh, It b, It e) {
        SaveIterRange(rh, b, e);
    }

    static inline void Load(TInputStream* rh, It b, It e) {
        LoadIterRange(rh, b, e);
    }

    template <class TStorage>
    static inline void Load(TInputStream* rh, It b, It e, TStorage& pool) {
        LoadIterRange(rh, b, e, pool);
    }
};

template <class It>
static inline void SaveRange(TOutputStream* rh, It b, It e) {
    TRangeSerialize<It, TTypeTraits<It>::IsPointer>::Save(rh, b, e);
}

template <class It>
static inline void LoadRange(TInputStream* rh, It b, It e) {
    TRangeSerialize<It, TTypeTraits<It>::IsPointer>::Load(rh, b, e);
}

template <class It, class TStorage>
static inline void LoadRange(TInputStream* rh, It b, It e, TStorage& pool) {
    TRangeSerialize<It, TTypeTraits<It>::IsPointer>::Load(rh, b, e, pool);
}

template <class T>
class TSerializer: public TSerializerTakingIntoAccountThePodType<T, TSerializeTypeTraits<T>::IsSerializablePod> {
};

template <class T>
class TArraySerializer: public TSerializerTakingIntoAccountThePodType<T, TSerializeTypeTraits<T>::IsSerializablePod> {
};

template <class T>
static inline void Save(TOutputStream* out, const T& t) {
    TSerializer<T>::Save(out, t);
}

template <class T>
static inline void SaveArray(TOutputStream* out, const T* t, size_t len) {
    TArraySerializer<T>::SaveArray(out, t, len);
}

template <class T>
static inline void Load(TInputStream* in, T& t) {
    TSerializer<T>::Load(in, t);
}

template <class T>
static inline void LoadArray(TInputStream* in, T* t, size_t len) {
    TArraySerializer<T>::LoadArray(in, t, len);
}

template <class T, class TStorage>
static inline void Load(TInputStream* in, T& t, TStorage& pool) {
    TSerializer<T>::Load(in, t, pool);
}

template <class T, class TStorage>
static inline void LoadArray(TInputStream* in, T* t, size_t len, TStorage& pool) {
    TArraySerializer<T>::LoadArray(in, t, len, pool);
}

static inline void SaveSize(TOutputStream* rh, size_t len) {
    ::Save(rh, (ui32)len);
}

static inline size_t LoadSize(TInputStream* rh) {
    ui32 s;

    ::Load(rh, s);
    return s;
}

template <class C>
static inline void LoadSizeAndResize(TInputStream* rh, C& c) {
    c.resize(LoadSize(rh));
}

template <class TStorage>
static inline char* AllocateFromPool(TStorage& pool, size_t len) {
    return static_cast<char*>(pool.Allocate(len));
}

template <>
class TSerializer<const char*> {
    public:
        static inline void Save(TOutputStream* rh, const TStringBuf& s) {
            ::SaveSize(rh, +s);
            ::SavePodArray(rh, ~s, +s);
        }

        template <class C, class TStorage>
        static inline void Load(TInputStream* rh, C*& s, TStorage& pool) {
            const size_t len = LoadSize(rh);

            char* res = AllocateFromPool(pool, len + 1);
            ::LoadPodArray(rh, res, len);
            res[len] = 0;
            s = res;
        }
};

template <class TVec>
class TVectorSerializer {
        typedef typename TVec::iterator TIter;
    public:
        static inline void Save(TOutputStream* rh, const TVec& v) {
            ::SaveSize(rh, +v);
            ::SaveRange(rh, v.begin(), v.end());
        }

        static inline void Load(TInputStream* rh, TVec& v) {
            ::LoadSizeAndResize(rh, v);
            TIter b = v.begin(); TIter e = (TIter)v.end();
            ::LoadRange(rh, b, e);
        }

        template <class TStorage>
        static inline void Load(TInputStream* rh, TVec& v, TStorage& pool) {
            ::LoadSizeAndResize(rh, v);
            TIter b = v.begin(); TIter e = (TIter)v.end();
            ::LoadRange(rh, b, e, pool);
        }
};

template <class T, class A>
class TSerializer< yvector<T, A> >: public TVectorSerializer< yvector<T, A> > {
};

template <>
class TSerializer<Stroka>: public TVectorSerializer<Stroka> {
};

template <>
class TSerializer<Wtroka>: public TVectorSerializer<Wtroka> {
};

template <class T, class A>
class TSerializer< ydeque<T, A> >: public TVectorSerializer< ydeque<T, A> > {
};

template <class TArray>
class TYArraySerializer {
    public:
        static inline void Save(TOutputStream* rh, const TArray& a) {
            ::SaveArray(rh, ~a, +a);
        }

        static inline void Load(TInputStream* rh, TArray& a) {
            ::LoadArray(rh, ~a, +a);
        }
};

template <class T, size_t N>
class TSerializer< yarray<T, N> >: public TYArraySerializer< yarray<T, N> > {
};

template <class A, class B>
class TSerializer< NStl::pair<A, B> > {
        typedef NStl::pair<A, B> TPair;
    public:
        static inline void Save(TOutputStream* rh, const TPair& p) {
            ::Save(rh, p.first);
            ::Save(rh, p.second);
        }

        static inline void Load(TInputStream* rh, TPair& p) {
            ::Load(rh, p.first);
            ::Load(rh, p.second);
        }

        template<class TStorage>
        static inline void Load(TInputStream* rh, TPair& p, TStorage& pool) {
            ::Load(rh, p.first, pool);
            ::Load(rh, p.second, pool);
        }
};

template <class A, class B>
class TSerializer< TPair<A, B> > : public TSerializer< NStl::pair<A, B> > {
};

template <>
class TSerializer<TBuffer> {
    public:
        static inline void Save(TOutputStream* rh, const TBuffer& buf) {
            ::SaveSize(rh, buf.Size());
            ::SavePodArray(rh, buf.Data(), buf.Size());
        }

        static inline void Load(TInputStream* rh, TBuffer& buf) {
            const size_t s = ::LoadSize(rh);
            buf.Resize(s);
            ::LoadPodArray(rh, buf.Data(), buf.Size());
        }
};

template <class TSet, class TValue, bool sorted>
class TSetSerializerInserter {
public:
    inline TSetSerializerInserter(TSet& s)
        : S_(s)
    {
        S_.clear();
    }

    inline void Insert(const TValue& v) {
        S_.insert(v);
    }

private:
    TSet& S_;
};

template <class TSet, class TValue>
class TSetSerializerInserter<TSet, TValue, true> {
public:
    inline TSetSerializerInserter(TSet& s)
        : S_(s)
    {
        S_.clear();
        P_ = S_.begin();
    }

    inline void Insert(const TValue& v) {
        P_ = S_.insert(P_, v);
    }

private:
    TSet& S_;
    typename TSet::iterator P_;
};

template <class TSet, class TValue, bool sorted>
class TSetSerializerBase {
    public:
        static inline void Save(TOutputStream* rh, const TSet& s) {
            ::SaveSize(rh, s.size());
            ::SaveRange(rh, s.begin(), s.end());
        }

        static inline void Load(TInputStream* rh, TSet& s) {
            TSetSerializerInserter<TSet, TValue, sorted> ins(s);
            const size_t cnt = ::LoadSize(rh);

            TValue v;
            for (size_t i = 0; i != cnt; ++i) {
                ::Load(rh, v);
                ins.Insert(v);
            }
        }

        template <class TStorage>
        static inline void Load(TInputStream* rh, TSet& s, TStorage& pool) {
            TSetSerializerInserter<TSet, TValue, sorted> ins(s);
            const size_t cnt = ::LoadSize(rh);

            TValue v;
            for (size_t i = 0; i != cnt; ++i) {
                ::Load(rh, v, pool);
                ins.Insert(v);
            }
        }
};

template <class TMap, bool sorted = false>
struct TMapSerializer: public TSetSerializerBase< TMap, TPair<typename TMap::key_type, typename TMap::mapped_type>, sorted > {
};

template <class TSet, bool sorted = false>
struct TSetSerializer: public TSetSerializerBase<TSet, typename TSet::value_type, sorted> {
};

template <class T1, class T2, class T3, class T4>
class TSerializer< ymap<T1, T2, T3, T4> >: public TMapSerializer< ymap<T1, T2, T3, T4>, true > {
};

template <class T1, class T2, class T3, class T4, class T5>
class TSerializer< yhash<T1, T2, T3, T4, T5> >: public TMapSerializer< yhash<T1, T2, T3, T4, T5>, false > {
};

template <class K, class C, class A>
class TSerializer< yset<K, C, A> >: public TSetSerializer< yset<K, C, A>, true > {
};

template <class T1, class T2, class T3, class T4>
class TSerializer< yhash_set<T1, T2, T3, T4> >: public TSetSerializer< yhash_set<T1, T2, T3, T4>, false > {
};

template <class T1, class T2>
class TSerializer<yqueue<T1, T2> > {
public:
    static inline void Save(TOutputStream* rh, const yqueue<T1, T2>& v) {
        ::Save(rh, v.Container());
    }
    static inline void Load(TInputStream* in, yqueue<T1, T2>& t) {
        ::Load(in, t.Container());
    }
};

template <class T1, class T2, class T3>
class TSerializer<ypriority_queue<T1, T2, T3> > {
public:
    static inline void Save(TOutputStream* rh, const ypriority_queue<T1, T2, T3>& v) {
        ::Save(rh, v.Container());
    }
    static inline void Load(TInputStream* in, ypriority_queue<T1, T2, T3>& t) {
        ::Load(in, t.Container());
    }
};

template <class T>
static inline void SaveLoad(TOutputStream* out, const T& t) {
    ::Save(out, t);
}

template <class T>
static inline void SaveLoad(TInputStream* in, T& t) {
    ::Load(in, t);
}

template <typename S>
static inline void SaveMany(S*) {
}

template <typename S, typename T, typename... R>
static inline void SaveMany(S* s, const T& t, const R& ... r) {
    ::Save(s, t);
    ::SaveMany(s, r...);
}

template <typename S>
static inline void LoadMany(S*) {
}

template <typename S, typename T, typename... R>
static inline void LoadMany(S* s, T& t, R& ... r) {
    ::Load(s, t);
    ::LoadMany(s, r...);
}

#define SAVELOAD_DEFINE(...) \
    inline void Save(TOutputStream* s) const { \
        ::SaveMany(s, __VA_ARGS__);             \
    } \
 \
    inline void Load(TInputStream* s) { \
        ::LoadMany(s, __VA_ARGS__);             \
    }
