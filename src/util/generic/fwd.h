#pragma once

#include "stlfwd.h"

#include <util/system/defaults.h>

//misc
class TBuffer;
class Stroka;
class Wtroka;

template <typename TChar, typename TTraits>
class TStringBufImpl;

//intrusive containers
template <class T>
class TIntrusiveList;

template <class T, class D>
class TIntrusiveListWithAutoDelete;

template <class T>
class TIntrusiveSList;

template <class T, class C>
class TAvlTree;

template <class TValue, class TCmp>
class TRbTree;

//containers
template <typename T, size_t N>
class yarray;

template <class T, class A>
class ydeque;

template <class Key, class T, class HashFcn, class EqualKey, class Alloc>
class yhash;

template <class Value, class HashFcn, class EqualKey, class Alloc>
class yhash_set;

template <class T, class A>
class ylist;

template <class K, class V, class Less, class A>
class ymap;

template <class K, class V, class Less, class A>
class ymultimap;

template <class K, class L, class A>
class yset;

template <class K, class L, class A>
class ymultiset;

template <class T, class S>
class ystack;

template <class T, class A>
class yvector;

template <class T1, class T2>
struct TPair;

template <size_t BitCount, typename TChunkType>
class TBitMap;

//autopointers
template <class T, class D>
class TAutoPtr;

template <class T, class D>
class THolder;

template <class T, class C, class D>
class TRefCounted;

template <class T, class Ops>
class TIntrusivePtr;

template <class T, class Ops>
class TSimpleIntrusivePtr;

template <class T, class C, class D>
class TSharedPtr;

template <class T, class D>
class TLinkedPtr;

template <class T, class C, class D>
class TCopyPtr;
