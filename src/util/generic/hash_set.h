#pragma once

#include "hash.h"
#include "pair.h"

#undef value_type

template <class Value, class HashFcn = THash<Value>,
          class EqualKey = TEqualTo<Value>,
          class Alloc = DEFAULT_ALLOCATOR(Value) >
class yhash_set
{
private:
  typedef yhashtable<Value, Value, HashFcn, ::TIdentity, EqualKey, Alloc> ht;
  ht rep;

  typedef typename ht::iterator mutable_iterator;

public:
  typedef typename ht::key_type key_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;
  typedef typename ht::key_equal key_equal;
  typedef typename ht::node_allocator_type node_allocator_type;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::const_pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::const_reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::const_iterator iterator;
  typedef typename ht::const_iterator const_iterator;
  typedef typename ht::insert_ctx insert_ctx;

  hasher hash_funct() const { return rep.hash_funct(); }
  key_equal key_eq() const { return rep.key_eq(); }

public:
  yhash_set() : rep(7, hasher(), key_equal()) {}
  template <class TT>
  explicit yhash_set(TT* allocParam) : rep(7, hasher(), key_equal(), allocParam) {}
  explicit yhash_set(size_type n) : rep(n, hasher(), key_equal()) {}
  yhash_set(size_type n, const hasher& hf) : rep(n, hf, key_equal()) {}
  yhash_set(size_type n, const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) {}

  template <class InputIterator>
  yhash_set(InputIterator f, InputIterator l)
    : rep(7, hasher(), key_equal()) { rep.insert_unique(f, l); }
  template <class InputIterator>
  yhash_set(InputIterator f, InputIterator l, size_type n)
    : rep(n, hasher(), key_equal()) { rep.insert_unique(f, l); }
  template <class InputIterator>
  yhash_set(InputIterator f, InputIterator l, size_type n,
           const hasher& hf)
    : rep(n, hf, key_equal()) { rep.insert_unique(f, l); }
  template <class InputIterator>
  yhash_set(InputIterator f, InputIterator l, size_type n,
           const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) { rep.insert_unique(f, l); }

public:
  size_type size() const { return rep.size(); }
  size_type max_size() const { return rep.max_size(); }
  bool empty() const { return rep.empty(); }
  void swap(yhash_set& hs) { rep.swap(hs.rep); }

//  friend bool operator==(const yhash_set<Value,HashFcn,EqualKey,Alloc>&,
//                         const yhash_set<Value,HashFcn,EqualKey,Alloc>&);

  iterator begin() const { return rep.begin(); }
  iterator end() const { return rep.end(); }
  iterator cbegin() const { return rep.begin(); }
  iterator cend() const { return rep.end(); }

public:
  TPair<iterator, bool> insert(const value_type& obj)
    {
      TPair<mutable_iterator, bool> p = rep.insert_unique(obj);
      return TPair<iterator, bool>(p.first, p.second);
    }
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l) { rep.insert_unique(f,l); }
  TPair<iterator, bool> insert_noresize(const value_type& obj)
    {
      TPair<mutable_iterator, bool> p = rep.insert_unique_noresize(obj);
      return TPair<iterator, bool>(p.first, p.second);
    }
  template<class TheObj>
  iterator insert_direct(const TheObj& obj, const insert_ctx &ins)
    { return rep.insert_direct(obj, ins); }

  template<class TheKey>
  iterator find(const TheKey& key) const { return rep.find(key); }
  template<class TheKey>
  iterator find(const TheKey& key, insert_ctx &ins) { return rep.find_i(key, ins); }
  template<class TheKey>
  bool has(const TheKey& key) const { return rep.find(key) != rep.end(); }
  template<class TheKey>
  bool has(const TheKey& key, insert_ctx &ins) { return rep.find_i(key, ins) != rep.end(); }

  template <class TKey>
  size_type count(const TKey& key) const { return rep.count(key); }

  template <class TKey>
  TPair<iterator, iterator> equal_range(const TKey& key) const
    { return rep.equal_range(key); }

  size_type erase(const key_type& key) {return rep.erase(key); }
  void erase(iterator it) { rep.erase(it); }
  void erase(iterator f, iterator l) { rep.erase(f, l); }
  void clear() { rep.clear(); }
  void clear(size_t downsize_hint) { rep.clear(downsize_hint); }
  void basic_clear() { rep.basic_clear(); }

  template<class KeySaver>
  int save_for_st(TStlSaver &saver, KeySaver &ks) const { return rep.template save_for_st<KeySaver>(saver, ks); }

public:
  void resize(size_type hint) { rep.resize(hint); }
  size_type bucket_count() const { return rep.bucket_count(); }
  size_type max_bucket_count() const { return rep.max_bucket_count(); }
  size_type elems_in_bucket(size_type n) const
    { return rep.elems_in_bucket(n); }
  node_allocator_type &GetNodeAllocator() { return rep.GetNodeAllocator(); }
};

template <class Value, class HashFcn, class EqualKey, class Alloc>
inline bool operator==(const yhash_set<Value, HashFcn, EqualKey, Alloc>& hs1,
                       const yhash_set<Value, HashFcn, EqualKey, Alloc>& hs2)
{
  return hs1.rep == hs2.rep;
}

template <class Value, class HashFcn = THash<Value>,
          class EqualKey = TEqualTo<Value>,
          class Alloc = DEFAULT_ALLOCATOR(Value)>
class yhash_multiset
{
private:
  typedef yhashtable<Value, Value, HashFcn, ::TIdentity, EqualKey, Alloc> ht;
  ht rep;

public:
  typedef typename ht::key_type key_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;
  typedef typename ht::key_equal key_equal;
  typedef typename ht::node_allocator_type node_allocator_type;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::const_pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::const_reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::const_iterator iterator;
  typedef typename ht::const_iterator const_iterator;

  hasher hash_funct() const { return rep.hash_funct(); }
  key_equal key_eq() const { return rep.key_eq(); }

public:
  yhash_multiset() : rep(7, hasher(), key_equal()) {}
  explicit yhash_multiset(size_type n) : rep(n, hasher(), key_equal()) {}
  yhash_multiset(size_type n, const hasher& hf) : rep(n, hf, key_equal()) {}
  yhash_multiset(size_type n, const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) {}

  template <class InputIterator>
  yhash_multiset(InputIterator f, InputIterator l)
    : rep(7, hasher(), key_equal()) { rep.insert_equal(f, l); }
  template <class InputIterator>
  yhash_multiset(InputIterator f, InputIterator l, size_type n)
    : rep(n, hasher(), key_equal()) { rep.insert_equal(f, l); }
  template <class InputIterator>
  yhash_multiset(InputIterator f, InputIterator l, size_type n,
                const hasher& hf)
    : rep(n, hf, key_equal()) { rep.insert_equal(f, l); }
  template <class InputIterator>
  yhash_multiset(InputIterator f, InputIterator l, size_type n,
                const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) { rep.insert_equal(f, l); }

public:
  size_type size() const { return rep.size(); }
  size_type max_size() const { return rep.max_size(); }
  bool empty() const { return rep.empty(); }
  void swap(yhash_multiset& hs) { rep.swap(hs.rep); }
//  friend bool operator==(const yhash_multiset<Value,HashFcn,EqualKey,Alloc>&,
//                         const yhash_multiset<Value,HashFcn,EqualKey,Alloc>&);

  iterator begin() const { return rep.begin(); }
  iterator end() const { return rep.end(); }
  iterator cbegin() const { return rep.begin(); }
  iterator cend() const { return rep.end(); }

public:
  iterator insert(const value_type& obj) { return rep.insert_equal(obj); }
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l) { rep.insert_equal(f,l); }
  iterator insert_noresize(const value_type& obj)
    { return rep.insert_equal_noresize(obj); }

  template <class TKey>
  iterator find(const TKey& key) const { return rep.find(key); }

  template <class TKey>
  size_type count(const TKey& key) const { return rep.count(key); }

  template <class TKey>
  TPair<iterator, iterator> equal_range(const TKey& key) const
    { return rep.equal_range(key); }

  size_type erase(const key_type& key) {return rep.erase(key); }
  void erase(iterator it) { rep.erase(it); }
  void erase(iterator f, iterator l) { rep.erase(f, l); }
  void clear() { rep.clear(); }
  void clear(size_t downsize_hint) { rep.clear(downsize_hint); }
  void basic_clear() { rep.basic_clear(); }

public:
  void resize(size_type hint) { rep.resize(hint); }
  size_type bucket_count() const { return rep.bucket_count(); }
  size_type max_bucket_count() const { return rep.max_bucket_count(); }
  size_type elems_in_bucket(size_type n) const
    { return rep.elems_in_bucket(n); }
  node_allocator_type &GetNodeAllocator() { return rep.GetNodeAllocator(); }
};

template <class Val, class HashFcn, class EqualKey, class Alloc>
inline bool operator==(const yhash_multiset<Val, HashFcn, EqualKey, Alloc>& hs1,
                       const yhash_multiset<Val, HashFcn, EqualKey, Alloc>& hs2)
{
  return hs1.rep == hs2.rep;
}
