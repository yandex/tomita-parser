#ifndef PIRE_COMPAT_H_INCLUDED
#define PIRE_COMPAT_H_INCLUDED

#include <bitset>
#include <algorithm>
#include <iterator>
#include <functional>
#include <utility>
#include <memory>

#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/deque.h>
#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/generic/utility.h>
#include <util/generic/algorithm.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/vector.h>


typedef Stroka ystring;
#define ybitset NStl::bitset
#define ypair NStl::pair
#define ymake_pair NStl::make_pair
#define yauto_ptr NStl::auto_ptr
#define ybinary_function NStl::binary_function
#define ymax NStl::max
#define ymin NStl::min

namespace Pire {

	template<class Iter, class T>
	void Fill(Iter begin, Iter end, T t) { NStl::fill(begin, end, t); }

	class Error: public yexception {
	public:
		Error(const char* msg)    { *this << msg; }
		Error(const ystring& msg) { *this << msg; }
	};

	typedef TOutputStream yostream;
	typedef TInputStream yistream;

	template<class Iter>
	ystring Join(Iter begin, Iter end, const ystring& separator) { return JoinStroku(begin, end, separator); }
}

#endif
