#pragma once

#include  <string>
#include  <vector>
#include  <map>
#include  <set>
#include  <bitset>

#include <util/system/defaults.h>
#include <util/ysaveload.h>
#include <util/generic/set.h>
#include <util/generic/stroka.h>

#include <library/lemmer/dictlib/grammarhelper.h>

/* Specialization of TSerializer for pointers - in order to save(load) pointed object, not address of pointer. */
template <class T>
class TSerializer<T*> {
    typedef T* TPointer;
public:
    static inline void Save(TOutputStream* out, const TPointer& t) {
        ::Save(out, *t);
    }

    static inline void Load(TInputStream* in, TPointer& t) {
        t = new T;
        ::Load(in, *t);
    }
};

template <class T>
class TArraySerializer<T*> {
    typedef T* TPointer;
public:
    static inline void SaveArray(TOutputStream* out, const TPointer* t, size_t len) {
        ::SaveIterRange(out, t, t + len);
    }

    static inline void LoadArray(TInputStream* in, TPointer* t, size_t len) {
        ::LoadIterRange(in, t, t + len);
    }
};

template <class T, class A>
class TSerializer< std::vector<T, A> >: public TVectorSerializer< std::vector<T, A> > {
};

template <class K, class T, class C, class A>
class TSerializer< std::map<K, T, C, A> >: public TMapSerializer< std::map<K, T, C, A> > {
};

template <class K, class C, class A>
class TSerializer< std::set<K, C, A> >: public TSetSerializer< std::set<K, C, A> > {
};

template <>
class TSerializer<std::string> {
public:
    static inline void Save(TOutputStream* buffer, const std::string& s)
    {
        const ui32 sLen = ui32(s.size());
        ::Save(buffer, sLen);
        ::SavePodArray(buffer, s.data(), sLen);
    }

    static inline void Load(TInputStream* buffer, std::string& s)
    {
        ui32 sLen;
        ::Load(buffer, sLen);

        std::string temp;
        temp.resize(sLen);

        ::LoadPodArray(buffer, temp.begin(), sLen);
        s.swap(temp);
    }
};

