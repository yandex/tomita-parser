#pragma once

#include "pair.h"
#include "vector.h"
#include "mapfindptr.h"

#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/memory/alloc.h>
#include <util/str_stl.h>
#include <util/generic/yexception.h>

#include <stlport/memory>
#include <stlport/algorithm>
#include <stlport/functional>

#include <cstdlib>

#define KT template <class key_type_>
#define VT template <class value_type_>

struct TSelect1st {
    template <class TPair>
    inline const typename TPair::first_type& operator()(const TPair& x) const {
        return x.first;
    }
};

struct TSelect2nd {
    template <class TPair>
    inline const typename TPair::second_type& operator()(const TPair& x) const {
        return x.second;
    }
};

struct TIdentity {
    template <class T>
    inline const T& operator()(const T& x) const throw () {
        return x;
    }
};

template <class Value>
struct __yhashtable_node {
  __yhashtable_node* next;
  Value val;
  __yhashtable_node& operator=(const __yhashtable_node&);
};

template <class Value, class Key, class HashFcn,
          class ExtractKey, class EqualKey, class Alloc>
class yhashtable;

template <class Key, class T, class HashFcn,
          class EqualKey, typename size_type_f>
class sthash;

template <class Value>
struct __yhashtable_iterator;

template <class Value>
struct __yhashtable_const_iterator;

template <class Value>
struct __yhashtable_iterator {
  typedef __yhashtable_iterator<Value>        iterator;
  typedef __yhashtable_const_iterator<Value>  const_iterator;
  typedef __yhashtable_node<Value>            node;

  typedef NStl::forward_iterator_tag iterator_category;
  typedef Value value_type;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef Value& reference;
  typedef Value* pointer;

  node* cur;

  explicit __yhashtable_iterator(node* n) : cur(n) {} /*y*/
  __yhashtable_iterator() {}
  reference operator*() const { return cur->val; }
  pointer operator->() const { return &(operator*()); }
  iterator& operator++();
  iterator operator++(int);
  bool operator==(const iterator& it) const { return cur == it.cur; }
  bool operator!=(const iterator& it) const { return cur != it.cur; }
  bool IsEnd() const { return !cur; }
  FORCED_INLINE explicit operator bool() const throw () { return cur != 0; }
};

class TStlSaver;

template <class Value>
struct __yhashtable_const_iterator {
  typedef __yhashtable_iterator<Value>        iterator;
  typedef __yhashtable_const_iterator<Value>  const_iterator;
  typedef __yhashtable_node<Value>            node;

  typedef NStl::forward_iterator_tag iterator_category;
  typedef Value value_type;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef const Value& reference;
  typedef const Value* pointer;

  const node* cur;

  explicit __yhashtable_const_iterator(const node* n) : cur(n) {}
  __yhashtable_const_iterator() {}
  __yhashtable_const_iterator(const iterator& it) : cur(it.cur) {}
  reference operator*() const { return cur->val; }
  pointer operator->() const { return &(operator*()); }
  const_iterator& operator++();
  const_iterator operator++(int);
  bool operator==(const const_iterator& it) const { return cur == it.cur; }
  bool operator!=(const const_iterator& it) const { return cur != it.cur; }
  bool IsEnd() const { return !cur; }
  FORCED_INLINE explicit operator bool() const throw () { return cur != 0; }
};

// Note: assumes long is at least 32 bits.
enum { __y_num_primes = 31 };

static const unsigned long __y_prime_list[__y_num_primes] =
{
  7ul,          17ul,         29ul,
  53ul,         97ul,         193ul,       389ul,       769ul,
  1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
  49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
  1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
  50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
  1610612741ul, 3221225473ul, 4294967291ul
};

template <class Value, class Key, class HashFcn,
          class ExtractKey, class EqualKey,
          class Alloc >
class yhashtable {
public:
  typedef Key key_type;
  typedef Value value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

  typedef size_t            size_type;
  typedef ptrdiff_t         difference_type;
  typedef value_type*       pointer;
  typedef const value_type* const_pointer;
  typedef value_type&       reference;
  typedef const value_type& const_reference;

  hasher hash_funct() const { return hash; }
  key_equal key_eq() const { return equals; }

private:
  hasher hash;
  key_equal equals;
  ExtractKey get_key;

  typedef __yhashtable_node<Value> node;

public:
#define REBIND(A) typedef typename Alloc::template rebind<A>::other
  REBIND(node) node_allocator_type;
  REBIND(node*) nodep_allocator_type;
#undef REBIND
  node_allocator_type &GetNodeAllocator() { return node_allocator; }
  const node_allocator_type &GetNodeAllocator() const {
    return node_allocator;
  }
private:
  node_allocator_type node_allocator;
  node* get_node() { return node_allocator.allocate(1); }
  void put_node(node* p) { node_allocator.deallocate(p, 1); }
  yvector<node*,nodep_allocator_type> buckets;
  size_type num_elements;

public:
  typedef __yhashtable_iterator<Value>        iterator;
  typedef __yhashtable_const_iterator<Value>  const_iterator;
  typedef node**                              insert_ctx;

  friend struct __yhashtable_iterator<Value>;
  friend struct __yhashtable_const_iterator<Value>;

public:
  yhashtable(size_type n,
            const HashFcn&    hf,
            const EqualKey&   eql,
            const ExtractKey& ext)
    : hash(hf), equals(eql), get_key(ext), num_elements(0)
  {
    initialize_buckets(n);
  }

  yhashtable(size_type n,
            const HashFcn&    hf,
            const EqualKey&   eql)
    : hash(hf), equals(eql), get_key(ExtractKey()), num_elements(0)
  {
    initialize_buckets(n);
  }

  template <class TAllocParam>
  yhashtable(size_type n, const HashFcn& hf, const EqualKey& eql, TAllocParam* allocParam)
      : hash(hf)
      , equals(eql)
      , get_key(ExtractKey())
      , node_allocator(allocParam)
      , buckets(allocParam)
      , num_elements(0)
  {
      initialize_buckets(n);
  }

  yhashtable(const yhashtable& ht)
    : hash(ht.hash)
    , equals(ht.equals)
    , get_key(ht.get_key)
    , node_allocator(ht.node_allocator)
    , buckets(ht.buckets.get_allocator())
    , num_elements(0)
  {
    copy_from(ht);
  }

  yhashtable& operator= (const yhashtable& ht)
  {
    if (&ht != this) {
      basic_clear();
      hash = ht.hash;
      equals = ht.equals;
      get_key = ht.get_key;
      copy_from(ht);
    }
    return *this;
  }

  size_t operator+ () const {
    return size();
  }

  ~yhashtable() { basic_clear(); }

  size_type size() const { return num_elements; }
  size_type max_size() const { return size_type(-1); }
  bool empty() const { return size() == 0; }

  void swap(yhashtable& ht)
  {
    NStl::swap(hash, ht.hash);
    NStl::swap(equals, ht.equals);
    NStl::swap(get_key, ht.get_key);
    NStl::swap(node_allocator, ht.node_allocator);
    buckets.swap(ht.buckets);
    NStl::swap(num_elements, ht.num_elements);
  }

  iterator begin()
  {
    for (size_type n = 0; n + 1 < buckets.size(); ++n) /*y*/
      if (buckets[n])
        return iterator(buckets[n]); /*y*/
    return end();
  }

  iterator end() { return iterator(0); } /*y*/

  const_iterator begin() const
  {
    for (size_type n = 0; n < buckets.size() - 1; ++n) /*y*/
      if (buckets[n])
        return const_iterator(buckets[n]); /*y*/
    return end();
  }

  const_iterator end() const { return const_iterator(0); } /*y*/

public:

  size_type bucket_count() const { return buckets.size() - 1; } /*y*/

#if 0
  size_type max_bucket_count() const
    { return __stl_prime_list_y[__stl_num_primes - 1]; }
#endif

  size_type elems_in_bucket(size_type bucket) const
  {
    size_type result = 0;
    if (const node* cur = buckets[bucket])
      for (; !((uintptr_t)cur & 1); cur = cur->next)
        result += 1;
    return result;
  }

  VT TPair<iterator, bool> insert_unique(const value_type_& obj)
  {
    resize(num_elements + 1);
    return insert_unique_noresize(obj);
  }

  VT iterator insert_equal(const value_type_& obj)
  {
    resize(num_elements + 1);
    return insert_equal_noresize(obj);
  }

  VT iterator insert_direct(const value_type_& obj, insert_ctx ins)
  {
    bool resized = resize(num_elements + 1);
    node* tmp = new_node(obj);
    if (resized) {
      find_i(get_key(tmp->val), ins);
    }
    tmp->next = *ins? *ins : (node*)((uintptr_t)(ins + 1) | 1);
    *ins = tmp;
    ++num_elements;
    return iterator(tmp);
  }

  VT TPair<iterator, bool> insert_unique_noresize(const value_type_& obj);
  VT iterator insert_equal_noresize(const value_type_& obj);

  template <class InputIterator>
  void insert_unique(InputIterator f, InputIterator l)
  {
    insert_unique(f, l, typename NStl::iterator_traits<InputIterator>::iterator_category());
  }

  template <class InputIterator>
  void insert_equal(InputIterator f, InputIterator l)
  {
    insert_equal(f, l, typename NStl::iterator_traits<InputIterator>::iterator_category());
  }

  template <class InputIterator>
  void insert_unique(InputIterator f, InputIterator l, NStl::input_iterator_tag)
  {
    for (; f != l; ++f)
      insert_unique(*f);
  }

  template <class InputIterator>
  void insert_equal(InputIterator f, InputIterator l, NStl::input_iterator_tag)
  {
    for (; f != l; ++f)
      insert_equal(*f);
  }

  template <class ForwardIterator>
  void insert_unique(ForwardIterator f, ForwardIterator l, NStl::forward_iterator_tag)
  {
    difference_type n = NStl::distance(f, l);

    resize(num_elements + n);
    for (; n > 0; --n, ++f)
      insert_unique_noresize(*f);
  }

  template <class ForwardIterator>
  void insert_equal(ForwardIterator f, ForwardIterator l, NStl::forward_iterator_tag)
  {
    difference_type n = NStl::distance(f, l);

    resize(num_elements + n);
    for (; n > 0; --n, ++f)
      insert_equal_noresize(*f);
  }

  VT reference find_or_insert(const value_type_& v);


  KT iterator find(const key_type_& key)
  {
    size_type n = bkt_num_key(key);
    node* first;
    for (first = buckets[n];
        first && !equals(get_key(first->val), key);
        first = ((uintptr_t)first->next & 1)? 0 : first->next) /*y*/
    {}
    return iterator(first); /*y*/
  }

  KT const_iterator find(const key_type_& key) const
  {
    size_type n = bkt_num_key(key);
    const node* first;
    for (first = buckets[n];
        first && !equals(get_key(first->val), key);
        first = ((uintptr_t)first->next & 1)? 0 : first->next) /*y*/
    {}
    return const_iterator(first); /*y*/
  }

  KT iterator find_i(const key_type_& key, insert_ctx &ins);

  KT size_type count(const key_type_& key) const
  {
    const size_type n = bkt_num_key(key);
    size_type result = 0;

    if (const node* cur = buckets[n])
      for (; !((uintptr_t)cur & 1); cur = cur->next)
        if (equals(get_key(cur->val), key))
          ++result;
    return result;
  }

  KT TPair<iterator, iterator> equal_range(const key_type_& key);
  KT TPair<const_iterator, const_iterator> equal_range(const key_type_& key) const;

  KT size_type erase(const key_type_& key);
  KT size_type erase_one(const key_type_& key);
  void erase(const iterator& it);
  void erase(iterator first, iterator last);

  void erase(const const_iterator& it);
  void erase(const_iterator first, const_iterator last);

  bool need_resize(size_type num_elements_hint) const {
    const size_type old_n = buckets.size() - 1;
    return num_elements_hint > old_n && next_size(num_elements_hint) > old_n;
  }

  bool resize(size_type num_elements_hint);
  void basic_clear();

  // implemented in save_stl.h
  template<class KeySaver>
  int save_for_st(TStlSaver &saver, KeySaver &ks, sthash<int, int, THash<int>, TEqualTo<int>, typename KeySaver::TSizeType>* stHash = NULL) const;

  void clear(size_type downsize) {
    basic_clear();
    if (downsize + 1 < buckets.size()) {
      downsize = next_size(downsize);
      if (downsize + 1 < buckets.size()) {
        buckets.resize(downsize + 1);
        buckets[downsize] = (node*) 1;
      }
    }
  }

  void clear() {
    if (num_elements)
      clear((num_elements * 2 + buckets.size()) / 3);
  }

private:
  size_type next_size(size_type n) const {
    const unsigned long* __first = __y_prime_list;
    const unsigned long* __last = __y_prime_list + (int)__y_num_primes;
    const unsigned long* pos = NStl::lower_bound(__first, __last, n);
    return pos == __last ? *(__last - 1) : *pos;
  }

  void initialize_buckets(size_type n)
  {
    const size_type n_buckets = next_size(n); /*y*/
    buckets.reserve(n_buckets + 1);
    buckets.insert(buckets.end(), n_buckets + 1, (node*) 0); /*y*/
    buckets[n_buckets] = (node*) 1;/*y*/
    num_elements = 0;
  }

  KT size_type bkt_num_key(const key_type_& key) const
  {
    return bkt_num_key(key, buckets.size() - 1);
  }

  VT size_type bkt_num(const value_type_& obj) const
  {
    return bkt_num_key(get_key(obj));
  }

  KT size_type bkt_num_key(const key_type_& key, size_t n) const
  {
    return hash(key) % n;
  }

  VT size_type bkt_num(const value_type_& obj, size_t n) const
  {
    return bkt_num_key(get_key(obj), n);
  }

  VT node* new_node(const value_type_& obj)
  {
    node* n = get_node();
    n->next = (node*) 1; /*y*/ // just for a case
    try {
      new (static_cast<void*> (&n->val)) Value(obj);
    } catch(...){
        put_node(n);
        throw;
    }
    return n;
  }

  void delete_node(node* n)
  {
    n->val.~Value();
    //n->next = (node*) 0xDeadBeeful;
    put_node(n);
  }

  void erase_bucket(const size_type n, node* first, node* last);
  void erase_bucket(const size_type n, node* last);

  void copy_from(const yhashtable& ht);

};

template <class V>
__yhashtable_iterator<V>&
__yhashtable_iterator<V>::operator++()/*y*/
{
    YASSERT(cur);
    cur = cur->next;
    if ((uintptr_t)cur & 1) {
        node **bucket = (node**)((uintptr_t)cur & ~1);
        while (*bucket == 0)
            bucket++;
        YASSERT(*bucket != 0);
        cur = (node*)((uintptr_t)*bucket & ~1);
    }
    return *this;
}

template <class V>
inline __yhashtable_iterator<V>
__yhashtable_iterator<V>::operator++(int)
{
  iterator tmp = *this;
  ++*this;
  return tmp;
}

template <class V>
__yhashtable_const_iterator<V>&
__yhashtable_const_iterator<V>::operator++() /*y*/
{
    YASSERT(cur);
    cur = cur->next;
    if ((uintptr_t)cur & 1) {
        node **bucket = (node**)((uintptr_t)cur & ~1);
        while (*bucket == 0)
            bucket++;
        YASSERT(*bucket != 0);
        cur = (node*)((uintptr_t)*bucket & ~1);
    }
    return *this;
}

template <class V>
inline __yhashtable_const_iterator<V>
__yhashtable_const_iterator<V>::operator++(int)
{
  const_iterator tmp = *this;
  ++*this;
  return tmp;
}

template <class V, class K, class HF, class Ex, class Eq, class A>
VT TPair<typename yhashtable<V, K, HF, Ex, Eq, A>::iterator, bool>
yhashtable<V, K, HF, Ex, Eq, A>::insert_unique_noresize(const value_type_& obj)
{
  const size_type n = bkt_num(obj);
  node* first = buckets[n];

  if (first) /*y*/
    for (node* cur = first; !((uintptr_t)cur & 1); cur = cur->next) /*y*/
      if (equals(get_key(cur->val), get_key(obj)))
          return TPair<iterator, bool>(iterator(cur), false); /*y*/

  node* tmp = new_node(obj);
  tmp->next = first? first : (node*)((uintptr_t)&buckets[n + 1] | 1); /*y*/
  buckets[n] = tmp;
  ++num_elements;
  return TPair<iterator, bool>(iterator(tmp), true); /*y*/
}

template <class V, class K, class HF, class Ex, class Eq, class A>
VT __yhashtable_iterator<V>
yhashtable<V, K, HF, Ex, Eq, A>::insert_equal_noresize(const value_type_& obj)
{
  const size_type n = bkt_num(obj);
  node* first = buckets[n];

  if (first) /*y*/
    for (node* cur = first; !((uintptr_t)cur & 1); cur = cur->next) /*y*/
      if (equals(get_key(cur->val), get_key(obj))) {
        node* tmp = new_node(obj);
        tmp->next = cur->next;
        cur->next = tmp;
        ++num_elements;
        return iterator(tmp); /*y*/
      }

  node* tmp = new_node(obj);
  tmp->next = first? first : (node*)((uintptr_t)&buckets[n + 1] | 1); /*y*/
  buckets[n] = tmp;
  ++num_elements;
  return iterator(tmp); /*y*/
}

template <class V, class K, class HF, class Ex, class Eq, class A>
VT
typename yhashtable<V, K, HF, Ex, Eq, A>::reference
yhashtable<V, K, HF, Ex, Eq, A>::find_or_insert(const value_type_& v)
{
  resize(num_elements + 1);

  size_type n = bkt_num_key(get_key(v));
  node* first = buckets[n];

  if (first) /*y*/
    for (node* cur = first; !((uintptr_t)cur & 1); cur = cur->next) /*y*/
      if (equals(get_key(cur->val), get_key(v)))
        return cur->val;

  node* tmp = new_node(v);
  tmp->next = first? first : (node*)((uintptr_t)&buckets[n + 1] | 1); /*y*/
  buckets[n] = tmp;
  ++num_elements;
  return tmp->val;
}

template <class V, class K, class HF, class Ex, class Eq, class A>
KT __yhashtable_iterator<V>
yhashtable<V, K, HF, Ex, Eq, A>::find_i(const key_type_& key, /*typename yhashtable<V, K, HF, Ex, Eq, A>::*/insert_ctx &ins)
{
  size_type n = bkt_num_key(key);
  ins = &buckets[n];
  node* first = buckets[n];

  if (first) /*y*/
    for (node* cur = first; !((uintptr_t)cur & 1); cur = cur->next) /*y*/
      if (equals(get_key(cur->val), key))
          return iterator(cur); /*y*/
  return end();
}

template <class V, class K, class HF, class Ex, class Eq, class A>
KT TPair<__yhashtable_iterator<V>, __yhashtable_iterator<V> >
yhashtable<V, K, HF, Ex, Eq, A>::equal_range(const key_type_& key)
{
  typedef TPair<iterator, iterator> pii;
  const size_type n = bkt_num_key(key);
  node* first = buckets[n];

  if (first) /*y*/
    for (; !((uintptr_t)first & 1); first = first->next) { /*y*/
      if (equals(get_key(first->val), key)) {
        for (node* cur = first->next; !((uintptr_t)cur & 1); cur = cur->next)
          if (!equals(get_key(cur->val), key))
            return pii(iterator(first), iterator(cur)); /*y*/
        for (size_type m = n + 1; m < buckets.size() - 1; ++m) /*y*/
          if (buckets[m])
            return pii(iterator(first), /*y*/
                       iterator(buckets[m])); /*y*/
        return pii(iterator(first), end()); /*y*/
      }
    }
  return pii(end(), end());
}

template <class V, class K, class HF, class Ex, class Eq, class A>
KT TPair<__yhashtable_const_iterator<V>, __yhashtable_const_iterator<V> >
yhashtable<V, K, HF, Ex, Eq, A>::equal_range(const key_type_& key) const
{
  typedef TPair<const_iterator, const_iterator> pii;
  const size_type n = bkt_num_key(key);
  const node* first = buckets[n];

  if (first) /*y*/
    for (; !((uintptr_t)first & 1); first = first->next) { /*y*/
      if (equals(get_key(first->val), key)) {
        for (const node* cur = first->next; !((uintptr_t)cur & 1); cur = cur->next)
          if (!equals(get_key(cur->val), key))
            return pii(const_iterator(first), /*y*/
                       const_iterator(cur)); /*y*/
        for (size_type m = n + 1; m < buckets.size() - 1; ++m) /*y*/
          if (buckets[m])
            return pii(const_iterator(first /*y*/),
                       const_iterator(buckets[m] /*y*/));
        return pii(const_iterator(first /*y*/), end());
      }
    }
  return pii(end(), end());
}

template <class V, class K, class HF, class Ex, class Eq, class A>
KT typename yhashtable<V, K, HF, Ex, Eq, A>::size_type
yhashtable<V, K, HF, Ex, Eq, A>::erase(const key_type_& key)
{
  const size_type n = bkt_num_key(key);
  node* first = buckets[n];
  size_type erased = 0;

  if (first) {
    node* cur = first;
    node* next = cur->next;
    while (!((uintptr_t)next & 1)) { /*y*/
      if (equals(get_key(next->val), key)) {
        cur->next = next->next;
        ++erased;
        --num_elements;
        delete_node(next);
        next = cur->next;
      } else {
        cur = next;
        next = cur->next;
      }
    }
    if (equals(get_key(first->val), key)) {
      buckets[n] = ((uintptr_t)first->next & 1)? 0 : first->next; /*y*/
      ++erased;
      --num_elements;
      delete_node(first);
    }
  }
  return erased;
}

template <class V, class K, class HF, class Ex, class Eq, class A>
KT typename yhashtable<V, K, HF, Ex, Eq, A>::size_type
yhashtable<V, K, HF, Ex, Eq, A>::erase_one(const key_type_& key)
{
  const size_type n = bkt_num_key(key);
  node* first = buckets[n];

  if (first) {
    node* cur = first;
    node* next = cur->next;
    while (!((uintptr_t)next & 1)) { /*y*/
      if (equals(get_key(next->val), key)) {
        cur->next = next->next;
        --num_elements;
        delete_node(next);
        return 1;
      } else {
        cur = next;
        next = cur->next;
      }
    }
    if (equals(get_key(first->val), key)) {
      buckets[n] = ((uintptr_t)first->next & 1)? 0 : first->next; /*y*/
      --num_elements;
      delete_node(first);
      return 1;
    }
  }
  return 0;
}

template <class V, class K, class HF, class Ex, class Eq, class A>
void yhashtable<V, K, HF, Ex, Eq, A>::erase(const iterator& it)
{
  if (node* const p = it.cur) {
    const size_type n = bkt_num(p->val);
    node* cur = buckets[n];

    if (cur == p) {
      buckets[n] = ((uintptr_t)cur->next & 1)? 0 : cur->next; /*y*/
      delete_node(cur);
      --num_elements;
    } else {
      node* next = cur->next;
      while (!((uintptr_t)next & 1)) {
        if (next == p) {
          cur->next = next->next;
          delete_node(next);
          --num_elements;
          break;
        } else {
          cur = next;
          next = cur->next;
        }
      }
    }
  }
}

template <class V, class K, class HF, class Ex, class Eq, class A>
void yhashtable<V, K, HF, Ex, Eq, A>::erase(iterator first, iterator last)
{
  size_type f_bucket = first.cur ? bkt_num(first.cur->val) : buckets.size() - 1; /*y*/
  size_type l_bucket = last.cur ? bkt_num(last.cur->val) : buckets.size() - 1; /*y*/

  if (first.cur == last.cur)
    return;
  else if (f_bucket == l_bucket)
    erase_bucket(f_bucket, first.cur, last.cur);
  else {
    erase_bucket(f_bucket, first.cur, 0);
    for (size_type n = f_bucket + 1; n < l_bucket; ++n)
      erase_bucket(n, 0);
    if (l_bucket != buckets.size() - 1) /*y*/
      erase_bucket(l_bucket, last.cur);
  }
}

template <class V, class K, class HF, class Ex, class Eq, class A>
inline void
yhashtable<V, K, HF, Ex, Eq, A>::erase(const_iterator first,
                                      const_iterator last)
{
  erase(iterator(const_cast<node*>(first.cur)),
        iterator(const_cast<node*>(last.cur)));
}

template <class V, class K, class HF, class Ex, class Eq, class A>
inline void
yhashtable<V, K, HF, Ex, Eq, A>::erase(const const_iterator& it)
{
  erase(iterator(const_cast<node*>(it.cur)));
}

template <class V, class K, class HF, class Ex, class Eq, class A>
bool yhashtable<V, K, HF, Ex, Eq, A>::resize(size_type num_elements_hint)
{
  const size_type old_n = buckets.size() - 1; /*y*/
  if (num_elements_hint > old_n) {
    const size_type n = next_size(num_elements_hint);
    if (n > old_n) {
      yvector<node*, nodep_allocator_type> tmp(buckets.get_allocator());
      tmp.resize(n + 1, (node*)0); /*y*/
      tmp[n] = (node*) 1;
#         ifdef __STL_USE_EXCEPTIONS
      try {
#         endif /* __STL_USE_EXCEPTIONS */
        for (size_type bucket = 0; bucket < old_n; ++bucket) {
          node* first = buckets[bucket];
          while (first) {
            size_type new_bucket = bkt_num(first->val, n);
            node* next = first->next;
            buckets[bucket] = ((uintptr_t)next & 1)? 0 : next; /*y*/
            next = tmp[new_bucket];
            first->next = next? next : (node*)((uintptr_t)&(tmp[new_bucket + 1]) | 1);  /*y*/
            tmp[new_bucket] = first;
            first = buckets[bucket];
          }
        }
        buckets.swap(tmp);

        return true;
#         ifdef __STL_USE_EXCEPTIONS
      } catch(...) {
        for (size_type bucket = 0; bucket < tmp.size() - 1; ++bucket) {
          while (tmp[bucket]) {
            node* next = tmp[bucket]->next;
            delete_node(tmp[bucket]);
            tmp[bucket] = ((uintptr_t)next & 1)? 0 : next /*y*/;
          }
        }
        throw;
      }
#         endif /* __STL_USE_EXCEPTIONS */
    }
  }

  return false;
}

template <class V, class K, class HF, class Ex, class Eq, class A>
void yhashtable<V, K, HF, Ex, Eq, A>::erase_bucket(const size_type n,
                                                  node* first, node* last)
{
  node* cur = buckets[n];
  if (cur == first)
    erase_bucket(n, last);
  else {
    node* next;
    for (next = cur->next; next != first; cur = next, next = cur->next)
      ;
    while (next != last) { /*y; 3.1*/
      cur->next = next->next;
      delete_node(next);
      next = ((uintptr_t)cur->next & 1)? 0 : cur->next; /*y*/
      --num_elements;
    }
  }
}

template <class V, class K, class HF, class Ex, class Eq, class A>
void
yhashtable<V, K, HF, Ex, Eq, A>::erase_bucket(const size_type n, node* last)
{
  node* cur = buckets[n];
  while (cur != last) {
    node* next = cur->next;
    delete_node(cur);
    cur = ((uintptr_t)next & 1)? 0 : next; /*y*/
    buckets[n] = cur;
    --num_elements;
  }
}

template <class V, class K, class HF, class Ex, class Eq, class A>
void yhashtable<V, K, HF, Ex, Eq, A>::basic_clear()
{
  if (!num_elements) {
      return;
  }

  node** first = buckets.begin();
  node** last = buckets.end() - 1;
  for (; first < last; ++first) {
    node* cur = *first;
    if (cur) { /*y*/
      while (!((uintptr_t)cur & 1)) { /*y*/
        node* next = cur->next;
        delete_node(cur);
        cur = next;
      }
      *first = 0;
    }
  }
  num_elements = 0;
}

template <class V, class K, class HF, class Ex, class Eq, class A>
void yhashtable<V, K, HF, Ex, Eq, A>::copy_from(const yhashtable& ht)
{
  buckets.clear();
  buckets.reserve(ht.buckets.size());
  buckets.insert(buckets.end(), ht.buckets.size(), (node*) 0);
  buckets.back() = (node*) 1;
#   ifdef __STL_USE_EXCEPTIONS
  try {
#   endif /* __STL_USE_EXCEPTIONS */
    for (size_type i = 0; i < ht.buckets.size() - 1; ++i) { /*y*/
      if (const node* cur = ht.buckets[i]) {
        node* copy = new_node(cur->val);
        buckets[i] = copy;

        for (node* next = cur->next; !((uintptr_t)next & 1); cur = next, next = cur->next) {
          copy->next = new_node(next->val);
          copy = copy->next;
        }
        copy->next = (node*)((uintptr_t)&buckets[i + 1] | 1); /*y*/
      }
    }
    num_elements = ht.num_elements;
#   ifdef __STL_USE_EXCEPTIONS
  } catch(...) {
    basic_clear();
    throw;
  }
#   endif /* __STL_USE_EXCEPTIONS */
}

/* end __SGI_STL_HASHTABLE_H */

#undef KT
#undef VT

template <class Key, class T, class HashFcn = THash<Key>,
          class EqualKey = TEqualTo<Key>,
          class Alloc = DEFAULT_ALLOCATOR(T)>
class yhash: public TMapOps<T, yhash<Key, T, HashFcn, EqualKey, Alloc> >
{
private:
  typedef yhashtable<TPair<const Key, T>, Key, HashFcn, TSelect1st, EqualKey, Alloc > ht;
  ht rep;

public:
  typedef typename ht::key_type key_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;
  typedef typename ht::key_equal key_equal;
  typedef typename ht::node_allocator_type node_allocator_type;
  typedef T mapped_type;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::iterator iterator;
  typedef typename ht::const_iterator const_iterator;
  typedef typename ht::insert_ctx insert_ctx;

  hasher hash_funct() const { return rep.hash_funct(); }
  key_equal key_eq() const { return rep.key_eq(); }

public:
  yhash() : rep(7, hasher(), key_equal()) {}
  template <class TAllocParam>
  explicit yhash(TAllocParam* allocParam)
    : rep(7, hasher(), key_equal(), allocParam)
  {
  }
  explicit yhash(size_type n) : rep(n, hasher(), key_equal()) {}
  yhash(size_type n, const hasher& hf) : rep(n, hf, key_equal()) {}
  yhash(size_type n, const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql)
  {
  }
  template <class TAllocParam>
  explicit yhash(size_type n, TAllocParam* allocParam)
    : rep(n, hasher(), key_equal(), allocParam)
  {
  }
  template <class InputIterator>
  yhash(InputIterator f, InputIterator l)
    : rep(7, hasher(), key_equal()) { rep.insert_unique(f, l); }
  template <class InputIterator>
  yhash(InputIterator f, InputIterator l, size_type n)
    : rep(n, hasher(), key_equal()) { rep.insert_unique(f, l); }
  template <class InputIterator>
  yhash(InputIterator f, InputIterator l, size_type n,
           const hasher& hf)
    : rep(n, hf, key_equal()) { rep.insert_unique(f, l); }
  template <class InputIterator>
  yhash(InputIterator f, InputIterator l, size_type n,
           const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) { rep.insert_unique(f, l); }

public:
  size_type size() const { return rep.size(); }
  int ysize() const { return (int)rep.size(); }
  size_t operator+() const { return +rep; }
  size_type max_size() const { return rep.max_size(); }
  bool empty() const { return rep.empty(); }
  void swap(yhash& hs) { rep.swap(hs.rep); }

  iterator begin() { return rep.begin(); }
  iterator end() { return rep.end(); }
  const_iterator begin() const { return rep.begin(); }
  const_iterator end() const { return rep.end(); }
  const_iterator cbegin() const { return rep.begin(); }
  const_iterator cend() const { return rep.end(); }

public:
  TPair<iterator, bool> insert(const value_type& obj)
    { return rep.insert_unique(obj); }
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l) { rep.insert_unique(f,l); }
  TPair<iterator, bool> insert_noresize(const value_type& obj)
    { return rep.insert_unique_noresize(obj); }
  template <class TheObj>
  iterator insert_direct(const TheObj& obj, const insert_ctx &ins)
    { return rep.insert_direct(obj, ins); }

  template <class TheKey>
  iterator find(const TheKey& key) { return rep.find(key); }

  template <class TheKey>
  const_iterator find(const TheKey& key) const { return rep.find(key); }

  template <class TheKey>
  iterator find(const TheKey& key, insert_ctx &ins) { return rep.find_i(key, ins); }

  template <class TheKey>
  bool has(const TheKey& key) const { return rep.find(key) != rep.end(); }
  bool has(const key_type& key) const { return rep.find(key) != rep.end(); }

  template <class TheKey>
  bool has(const TheKey& key, insert_ctx &ins) { return rep.find_i(key, ins) != rep.end(); }

  template <class TKey>
  T& operator[](const TKey& key)
  {
    insert_ctx ctx = 0;
    iterator it = find(key, ctx);

    if (it != end()) {
        return it->second;
    }

#if defined(_MSC_VER)
    return insert_direct(MakePair(key, T()), ctx)->second;
#else
    return insert_direct(TPair<const TKey&, const T&>(key, T()), ctx)->second;
#endif
  }

  template <class TheKey>
  const T& at(const TheKey& key) const {
    const_iterator it = find(key);

    if (it == end()) {
      ThrowRangeError("hash");
    }

    return it->second;
  }

  template <class TheKey>
  T& at(const TheKey& key) {
    iterator it = find(key);

    if (it == end()) {
      ThrowRangeError("hash");
    }

    return it->second;
  }

  template <class TKey>
  size_type count(const TKey& key) const { return rep.count(key); }

  template <class TKey>
  TPair<iterator, iterator> equal_range(const TKey& key)
    { return rep.equal_range(key); }

  template <class TKey>
  TPair<const_iterator, const_iterator> equal_range(const TKey& key) const
    { return rep.equal_range(key); }

  template <class TKey>
  size_type erase(const TKey& key) {return rep.erase_one(key); }

  void erase(iterator it) { rep.erase(it); }
  void erase(iterator f, iterator l) { rep.erase(f, l); }
  void clear() { rep.clear(); }
  void clear(size_t downsize_hint) { rep.clear(downsize_hint); }
  void basic_clear() { rep.basic_clear(); }

  // if (stHash != NULL) bucket_count() must be equal to stHash->bucket_count()
  template<class KeySaver>
  int save_for_st(TStlSaver &saver, KeySaver &ks, sthash<int, int, THash<int>, TEqualTo<int>, typename KeySaver::TSizeType>* stHash = NULL) const {
      return rep.template save_for_st<KeySaver>(saver, ks, stHash);
  }

public:
  void resize(size_type hint) { rep.resize(hint); }
  size_type bucket_count() const { return rep.bucket_count(); }
  size_type max_bucket_count() const { return rep.max_bucket_count(); }
  size_type elems_in_bucket(size_type n) const
    { return rep.elems_in_bucket(n); }
  node_allocator_type &GetNodeAllocator() { return rep.GetNodeAllocator(); }
  const node_allocator_type &GetNodeAllocator() const {
    return rep.GetNodeAllocator();
  }
};

template <class Key, class T, class HashFcn, class EqualKey, class Alloc>
inline bool operator==(const yhash<Key, T, HashFcn, EqualKey, Alloc>& hm1,
                       const yhash<Key, T, HashFcn, EqualKey, Alloc>& hm2)
{
  return hm1.rep == hm2.rep;
}

template <class Key, class T, class HashFcn = THash<Key>,
          class EqualKey = TEqualTo<Key>,
          class Alloc = DEFAULT_ALLOCATOR(T)>
class yhash_mm
{
private:
  typedef yhashtable<TPair<const Key, T>, Key, HashFcn, TSelect1st, EqualKey, Alloc > ht;
  ht rep;

public:
  typedef typename ht::key_type key_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;
  typedef typename ht::key_equal key_equal;
  typedef T mapped_type;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::iterator iterator;
  typedef typename ht::const_iterator const_iterator;

  hasher hash_funct() const { return rep.hash_funct(); }
  key_equal key_eq() const { return rep.key_eq(); }

public:
  yhash_mm() : rep(7, hasher(), key_equal()) {}
  template<typename TAllocParam>
  explicit yhash_mm(TAllocParam* allocParam)
    : rep(7, hasher(), key_equal(), allocParam)
  {
  }
  explicit yhash_mm(size_type n) : rep(n, hasher(), key_equal()) {}
  yhash_mm(size_type n, const hasher& hf) : rep(n, hf, key_equal()) {}
  yhash_mm(size_type n, const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) {}

  template <class InputIterator>
  yhash_mm(InputIterator f, InputIterator l)
    : rep(7, hasher(), key_equal()) { rep.insert_equal(f, l); }
  template <class InputIterator>
  yhash_mm(InputIterator f, InputIterator l, size_type n)
    : rep(n, hasher(), key_equal()) { rep.insert_equal(f, l); }
  template <class InputIterator>
  yhash_mm(InputIterator f, InputIterator l, size_type n,
                const hasher& hf)
    : rep(n, hf, key_equal()) { rep.insert_equal(f, l); }
  template <class InputIterator>
  yhash_mm(InputIterator f, InputIterator l, size_type n,
                const hasher& hf, const key_equal& eql)
    : rep(n, hf, eql) { rep.insert_equal(f, l); }

public:
  size_type size() const { return rep.size(); }
  int ysize() const { return (int)rep.size(); }
  size_type max_size() const { return rep.max_size(); }
  bool empty() const { return rep.empty(); }
  void swap(yhash_mm& hs) { rep.swap(hs.rep); }

  iterator begin() { return rep.begin(); }
  iterator end() { return rep.end(); }
  const_iterator begin() const { return rep.begin(); }
  const_iterator end() const { return rep.end(); }
  const_iterator cbegin() const { return rep.begin(); }
  const_iterator cend() const { return rep.end(); }

public:
  iterator insert(const value_type& obj) { return rep.insert_equal(obj); }
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l) { rep.insert_equal(f,l); }
  iterator insert_noresize(const value_type& obj)
    { return rep.insert_equal_noresize(obj); }

  template <class TKey>
  iterator find(const TKey& key) { return rep.find(key); }

  template <class TKey>
  const_iterator find(const TKey& key) const { return rep.find(key); }

  template <class TKey>
  size_type count(const TKey& key) const { return rep.count(key); }

  template <class TKey>
  TPair<iterator, iterator> equal_range(const TKey& key)
    { return rep.equal_range(key); }

  template <class TKey>
  TPair<const_iterator, const_iterator> equal_range(const TKey& key) const
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
};

template <class Key, class T, class HF, class EqKey, class Alloc>
inline bool operator==(const yhash_mm<Key, T, HF, EqKey, Alloc>& hm1,
                       const yhash_mm<Key, T, HF, EqKey, Alloc>& hm2)
{
  return hm1.rep == hm2.rep;
}

#define yhash_map yhash
#define yhash_multimap yhash_mm
