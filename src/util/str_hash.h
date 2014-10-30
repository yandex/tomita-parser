#pragma once

#include "str_map.h"

#include <util/system/yassert.h>
#include <util/memory/alloc.h>
#include <util/memory/tempbuf.h>

class TInputStream;
class TOutputStream;

template <class T, class Alloc = DEFAULT_ALLOCATOR(const char*)>
class Hash;

struct yvoid {
    yvoid() {}
};

template <typename T, class Alloc>
class Hash : public string_hash<T, ci_hash, ci_equal_to, Alloc> {
    typedef string_hash<T, ci_hash, ci_equal_to, Alloc> ci_string_hash;
protected:
    using ci_string_hash::pool;
public:
    typedef typename ci_string_hash::size_type size_type;
    typedef typename ci_string_hash::const_iterator const_iterator;
    typedef typename ci_string_hash::iterator iterator;
    typedef typename ci_string_hash::value_type value_type;
    using ci_string_hash::find;
    using ci_string_hash::begin;
    using ci_string_hash::end;
    using ci_string_hash::size;

    Hash()
      : ci_string_hash()
    {
    }
    explicit Hash(size_type size)
      : ci_string_hash(size, size * AVERAGEWORD_BUF)
    {
    }
    Hash(const char** strings, size_type size = 0, T* =0); // must end with NULL or "\0"
    virtual ~Hash();
    bool Has(const char *s, size_t len, T *pp=0) const;
    bool Has(const char *s, T *pp=0) const
    {
        const_iterator it;
        if ((it = find(s)) == end())
            return false;
        else if (pp)
            *pp = (*it).second;
        return true;

    }
    void Add(const char *s, T data)
    {
        // in fact it is the same insert_unique as in AddUnique.
        // it's impossible to have _FAST_ version of insert() in 'hash_map'

        // you have to use 'hash_mmap' to get the _kind_ of desired effect.
        // BUT still there will be "Checks" inside -
        // to make the same keys close to each other (see insert_equal())
        this->insert_copy(s, data);
    }
    bool AddUniq(const char *s, T data)
    {
        return this->insert_copy(s, data).second;
    }
    // new function to get rid of allocations completely! -- e.g. in constructors
    void AddPermanent(const char *s, T data)
    {
        this->insert(value_type(s, data));
    }
    T Detach(const char *s)
    {
        iterator it = find(s);
        if (it == end())
            return T();
        T data = (*it).second;
        this->erase(it);
        return data;
    }
    size_type NumEntries() const
    {
        return size();
    }
    bool ForEach(bool (*func)(const char* key, T data, void *cookie), void *cookie=0);
    void Resize(size_type size)
    {
        this->resize(size);
        // no pool resizing here.
    }
    virtual void Clear();
    char* Pool() {
        if (pool.Size() < 2 || pool.End()[-2] != '\0')
            pool.Append("\0", 1);
        return pool.Begin();
    }
};

template <class T, class Alloc>
Hash<T, Alloc>::Hash(const char** array, size_type size, T* data)
{
    // must end with NULL or "\0"
    YASSERT(data != 0);
    Resize(size);
    while (*array && **array)
        AddPermanent(*array++, *data++);
}

template <class T, class Alloc>
bool Hash<T, Alloc>::Has(const char *s, size_t len, T *pp) const
{
    TTempArray<char> buf(len + 1);
    char* const allocated = buf.Data();
    memcpy(allocated, s, len);
    allocated[len] = '\x00';
    return Has(allocated, pp);
}

template <class T, class Alloc>
Hash<T, Alloc>::~Hash()
{
    Clear();
}

template <class T, class Alloc>
void Hash<T, Alloc>::Clear()
{
    ci_string_hash::clear_hash(); // to make the key pool empty
}

template <class T, class Alloc>
bool Hash<T, Alloc>::ForEach(bool (*func)(const char *key, T data, void *cookie), void *cookie)
{
    for (const_iterator it = begin(); it != end(); ++it)
        if (!func((*it).first, (*it).second, cookie))
            return false;
    return true;
}

class HashSet : public Hash<yvoid> {
public:
    HashSet(const char** array, size_type size = 0);
    HashSet() : Hash<yvoid>() {}
    void Read(TInputStream* input);
    void Write(TOutputStream* output) const;
    void Add(const char *s)
    {
        // in fact it is the same insert_unique as in AddUnique.
        // it's impossible to have _FAST_ version of insert() in 'hash_map'

        // you have to use 'hash_mmap' to get the _kind_ of desired effect.
        // BUT still there will be "Checks" inside -
        // to make the same keys close to each other (see insert_equal())
        insert_copy(s, yvoid());
    }
    bool AddUniq(const char *s)
    {
        return insert_copy(s, yvoid()).second;
    }
    // new function to get rid of allocations completely! -- e.g. in constructors
    void AddPermanent(const char *s)
    {
        insert(value_type(s, yvoid()));
    }
};

template <class T, class HashFcn = THash<T>, class EqualKey = TEqualTo<T>, class Alloc = DEFAULT_ALLOCATOR(T)>
class TStaticHash: private yhash<T, T, HashFcn, EqualKey> {
private:
    typedef yhash<T, T, HashFcn, EqualKey> TBase;
public:
    TStaticHash(T arr[][2], size_t size) {
        TBase::resize(size);
        while (size) {
            TBase::insert(typename TBase::value_type(arr[0][0], arr[0][1]));
            arr++;
            size--;
        }
    }
    T operator[](const T& key) const { // !!! it is not lvalue nor it used to be
        typename TBase::const_iterator it = TBase::find(key);
        if (it == TBase::end())
            return NULL;
        return it->second;
    }
};

typedef TStaticHash<const char*, ci_hash, ci_equal_to> TStHash;
