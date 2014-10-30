#pragma once

// Stroka is case-sensitive
// stroka is case-insensitive

#include <cstddef>
#include <cstring>

#include <util/cpp11.h>

#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <util/system/atomic.h>
#include <util/system/defaults.h>

#include "utility.h"
#include "reinterpretcast.h"
#include "stlfwd.h"
#include "chartraits.h"
#include "bitops.h"

struct CodePage;
extern const CodePage& csYandex;

void ThrowLengthError(const char* descr);
void ThrowRangeError(const char* descr);

namespace NDetail {
extern void const* STRING_DATA_NULL;

//! represents string data shared between instances of string objects
struct TStringData {
    TAtomic Refs;
    size_t BufLen; //!< maximum number of characters that this data can fit
    size_t Length; //!< actual string data length
};

template <typename TCharType>
struct TStringDataTraits {
    typedef TStringData TData;

    enum {
        Overhead = sizeof(TData) + sizeof(TCharType) // + null terminated symbol
    };

    static const size_t MaxSize = (size_t(-1) - Overhead) / sizeof(TCharType);

    static size_t CalcAllocationSize(size_t len) {
        return len * sizeof(TCharType) + Overhead;
    }

    static TData* GetData(TCharType* p) {
        return ((TData*)(void*)p) - 1;
    }

    static TCharType* GetChars(TData* data) {
        return (TCharType*)(void*)(data + 1);
    }

    static TCharType* GetNull() {
        return (TCharType*)STRING_DATA_NULL;
    }
};

//! allocates new string data that fits not less than @c newLen of characters
//! @note the string is null-terminated at the @c oldLen position and so it
//!       must be filled with characters after allocation
//! @throw std::length_error
template <typename TCharType>
TCharType* Allocate(size_t oldLen, size_t newLen, TStringData* oldData = 0);

void Deallocate(void* data);

} // namespace NDetail

template <typename TDerived, typename TCharType, typename TTraits>
class TStringBase;

//! helper class
template < typename TCharType, typename TTraits = TCharTraits<TCharType> >
struct TFixedString {
    TFixedString()
        : Start(nullptr)
        , Length(0)
    {
    }

    template <typename T>
    TFixedString(const TStringBase<T, TCharType, TTraits>& s)
        : Start(s.c_str())
        , Length(s.size())
    {
    }

    template <typename T, typename A>
    TFixedString(const NStl::basic_string<TCharType, T, A>& s)
        : Start(s.c_str())
        , Length(s.size())
    {
    }

    TFixedString(const TCharType* s)
        : Start(s)
        , Length(s ? TTraits::GetLength(s) : 0)
    {
    }

    TFixedString(const TCharType* s, size_t n)
        : Start(s)
        , Length(n)
    {
    }

    TFixedString(const TCharType* begin, const TCharType* end)
        : Start(begin)
        , Length(end - begin)
    {
    }

    inline TFixedString SubString(size_t pos, size_t n) const throw () {
        pos = Min(pos, Length);
        n = Min(n, Length - pos);
        return TFixedString(Start + pos, n);
    }

    const TCharType* Start;
    size_t Length;
};

template < typename TDerived, typename TCharType, typename TTraitsType = TCharTraits<TCharType> >
class TStringBase {
public:
    typedef TCharType TChar;
    typedef TTraitsType TTraits;
    typedef TStringBase<TDerived, TChar, TTraits> TSelf;
    typedef ::TFixedString<TChar, TTraits> TFixedString;

    typedef size_t size_type;
    static const size_t npos = size_t(-1);

    static size_t max_size() throw () {
        return NDetail::TStringDataTraits<TCharType>::MaxSize;
    }

    static size_t hashVal(const TCharType* s, size_t n) throw () {
        return TTraits::GetHash(s, n);
    }

    typedef const TCharType* const_iterator;

    static inline size_t StrLen(const TCharType* s) throw () {
        return s ? TTraits::GetLength(s) : 0;
    }

    template <class T, class A>
    inline operator NStl::basic_string<TCharType, T, A> () const {
        return NStl::basic_string<TCharType, T, A>(Ptr(), Len());
    }

    /**
      * @param ret must be pointer to character inside the string, or zero
      * @return offset from string beginning (in chars), or npos on zero input
      **/
    inline size_t off(const TCharType* ret) const throw () {
        return ret ? (size_t)(ret - Ptr()) : npos;
    }

    inline size_t IterOff(const_iterator it) const throw () {
        return begin() <= it && end() > it ? size_t(it - begin()) : npos;
    }

    inline const TCharType* c_str() const throw () {
        return Ptr();
    }

    inline const TCharType* operator~ () const throw () {
        return Ptr();
    }

    inline const_iterator begin() const throw () {
        return Ptr();
    }

    inline const_iterator end() const throw () {
        return Ptr() + size();
    }

    inline TCharType back() const throw () {
        if (empty())
            return 0;
        return Ptr()[Len() - 1];
    }

    inline size_t size() const throw () {
        return Len();
    }

    inline size_t operator+ () const throw () {
        return Len();
    }

    inline size_t hash() const throw () {
        return hashVal(Ptr(), size());
    }

    inline bool is_null() const throw () {
        return *Ptr() == 0;
    }

    inline bool empty() const throw () {
        return Len() == 0;
    }

    inline explicit operator bool () const throw () {
        return !empty();
    }

public: // style-guide compliant methods
    const TCharType* Data() const throw () {
        return Ptr();
    }

    size_t Size() const throw () {
        return Len();
    }

    bool Empty() const throw () {
        return 0 == Len();
    }

public:
    // ~~~ Comparison ~~~ : FAMILY0(int, compare)
    static int compare(const TSelf& s1, const TSelf& s2) throw () {
        return TTraits::Compare(s1.Ptr(), s1.Len(), s2.Ptr(), s2.Len());
    }

    static int compare(const TCharType* p, const TSelf& s2) throw () {
        return TTraits::Compare(p, StrLen(p), s2.Ptr(), s2.Len());
    }

    static int compare(const TSelf& s1, const TCharType* p) throw () {
        return TTraits::Compare(s1.Ptr(), s1.Len(), p, StrLen(p));
    }

    static int compare(const TFixedString& s1, const TFixedString& s2) throw () {
        return TTraits::Compare(s1.Start, s1.Length, s2.Start, s2.Length);
    }

    template <class T>
    inline int compare(const T& t) const throw () {
        return compare(*this, t);
    }

    template <class T>
    inline int compare(size_t p, size_t n, const T& t) const throw () {
        return compare(TFixedString(*this).SubString(p, n), TFixedString(t));
    }

    template <class T>
    inline int compare(size_t p, size_t n, const T& t, size_t p1, size_t n1) const throw () {
        return compare(TFixedString(*this).SubString(p, n), TFixedString(t).SubString(p1, n1));
    }

    template <class T>
    inline int compare(size_t p, size_t n, const T& t, size_t n1) const throw () {
        return compare(TFixedString(*this).SubString(p, n), TFixedString(t).SubString(0, n1));
    }

    static inline bool is_prefix(const TCharType* ptr, size_t len, const TCharType* p, size_t l) throw () {
        return (len <= l) && (0 == TTraits::Compare(ptr, len, p, len));
    }

    inline bool is_prefix(const TCharType* s, size_t n) const throw () {
        return is_prefix(Ptr(), Len(), s, n);
    }

    inline bool is_prefix(const TFixedString& s) const throw () {
        return is_prefix(s.Start, s.Length);
    }

    static inline bool is_suffix(const TCharType* ptr, size_t len, const TCharType* p, size_t slen) throw () {
        return (len <= slen) && (0 == TTraits::Compare(ptr, len, p + slen - len, len));
    }

    inline bool is_suffix(const TCharType* s, size_t n) const throw () {
        return is_suffix(Ptr(), Len(), s, n);
    }

    inline bool is_suffix(const TFixedString& s) const throw () {
        return is_suffix(s.Start, s.Length);
    }

    inline bool has_prefix(const TCharType* s, size_t n) const throw () {
        return is_prefix(s, n, Ptr(), Len());
    }

    inline bool has_prefix(const TFixedString& s) const throw () {
        return has_prefix(s.Start, s.Length);
    }

    inline bool has_suffix(const TCharType* s, size_t n) const throw () {
        return is_suffix(s, n, Ptr(), Len());
    }

    inline bool has_suffix(const TFixedString& s) const throw () {
        return has_suffix(s.Start, s.Length);
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator== (const TSelf& s1, const TStringBase<TDerived2, TChar, TTraits2>& s2) throw () {
        return s1.Len() == s2.size() && compare(s1, s2) == 0;
    }

    friend bool operator== (const TSelf& s, const TCharType *pc) throw () {
        return compare(s, pc) == 0;
    }

    friend bool operator== (const TCharType* pc, const TSelf& s) throw () {
        return compare(pc, s) == 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator!= (const TSelf& s1, const TStringBase<TDerived2, TChar, TTraits2>& s2) throw () {
        return s1.Len() != s2.size() || compare(s1, s2) != 0;
    }

    friend bool operator!= (const TSelf& s, const TCharType* pc) throw () {
        return compare(s, pc) != 0;
    }

    friend bool operator!= (const TCharType* pc, const TSelf& s) throw () {
        return compare(pc, s) != 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator< (const TSelf& s1, const TStringBase<TDerived2, TChar, TTraits2>& s2) throw () {
        return compare(s1, s2) < 0;
    }

    friend bool operator< (const TSelf& s, const TCharType* pc) throw () {
        return compare(s, pc) < 0;
    }

    friend bool operator< (const TCharType* pc, const TSelf& s) throw () {
        return compare(pc, s) < 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator<= (const TSelf& s1, const TStringBase<TDerived2, TChar, TTraits2>& s2) throw () {
        return compare(s1, s2) <= 0;
    }

    friend bool operator<= (const TSelf& s, const TCharType* pc) throw () {
        return compare(s, pc) <= 0;
    }

    friend bool operator<= (const TCharType* pc, const TSelf& s) throw () {
        return compare(pc, s) <= 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator> (const TSelf& s1, const TStringBase<TDerived2, TChar, TTraits2>& s2) throw () {
        return compare(s1, s2) > 0;
    }

    friend bool operator> (const TSelf& s, const TCharType* pc) throw () {
        return compare(s, pc) > 0;
    }

    friend bool operator> (const TCharType* pc, const TSelf& s) throw () {
        return compare(pc, s) > 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator>= (const TSelf& s1, const TStringBase<TDerived2, TChar, TTraits2>& s2) throw () {
        return compare(s1, s2) >= 0;
    }

    friend bool operator>= (const TSelf& s, const TCharType* pc) throw () {
        return compare(s, pc) >= 0;
    }

    friend bool operator>= (const TCharType* pc, const TSelf& s) throw () {
        return compare(pc, s) >= 0;
    }

    // ~~ Read access ~~
    inline TCharType at(size_t pos) const throw () {
        if (EXPECT_TRUE(pos < Len())) {
            return (Ptr())[pos];
        }
        return 0;
    }

    inline TCharType operator[](size_t pos) const throw () {
        return at(pos);
    }

    //~~~~Search~~~~
    /// return npos if not found
    inline size_t find(const TFixedString& s, size_t pos = 0) const throw () {
        return GenericFind<TTraits::Find>(s.Start, s.Length, pos);
    }

    inline size_t find(TCharType c, size_t pos = 0) const throw () {
        if (pos >= Len()) {
            return npos;
        }
        return off(TTraits::Find(Ptr() + pos, c, Len() - pos));
    }

    inline size_t rfind(TCharType c) const throw () {
        return off(TTraits::RFind(Ptr(), c, Len()));
    }

    inline size_t rfind(TCharType c, size_t pos) const throw () {
        if (pos > Len())
            pos = Len();
        return off(TTraits::RFind(Ptr(), c, pos));
    }

    inline size_t rfind(const TFixedString& str, size_t pos = npos) const {
        return off(TTraits::RFind(Ptr(), Len(), str.Start, str.Length, pos));
    }

    //~~~~Character Set Search~~~
    inline size_t find_first_of(TCharType c) const throw () {
        return find(c);
    }

    inline size_t find_first_of(const TFixedString& set, size_t pos = 0) const throw () {
        return GenericFind<TTraits::FindFirstOf>(set.Start, set.Length, pos);
    }

    inline size_t find_first_not_of(const TFixedString& set, size_t pos = 0) const throw () {
        return GenericFind<TTraits::FindFirstNotOf>(set.Start, set.Length, pos);
    }

    inline size_t find_last_of(TCharType c, size_t pos = npos) const throw () {
        return find_last_of(&c, pos, 1);
    }

    inline size_t find_last_of(const TFixedString& set, size_t pos = npos) const throw () {
        return find_last_of(set.Start, pos, set.Length);
    }

    inline size_t find_last_of(const TCharType* set, size_t pos, size_t n) const throw () {
        ssize_t startpos = pos >= size() ? static_cast<ssize_t>(size()) - 1 : static_cast<ssize_t>(pos);

        for (ssize_t i = startpos; i >= 0; --i) {
            const TCharType c = Ptr()[i];
            for (const TCharType* p = set; p < set + n; ++p) {
                if (c == *p) {
                    return static_cast<size_t>(i);
                }
            }
        }
        return npos;
    }

    inline size_t copy(TCharType* pc, size_t n, size_t pos) const {
        if (pos > Len()) {
            ThrowRangeError("TStringBase::copy");
        }
        return CopyImpl(pc, n, pos);
    }

    inline size_t copy(TCharType* pc, size_t n) const {
        return CopyImpl(pc, n, 0);
    }

    inline size_t strcpy(TCharType* pc, size_t n) const throw () {
        if (n) {
            n = copy(pc, n - 1);
            pc[n] = 0;
        }
        return n;
    }

    inline TDerived copy() const {
        return TDerived(Ptr(), Len());
    }

    // ~~~ Partial copy ~~~~
    TDerived substr(size_t pos, size_t n = npos) const {
        return TDerived(*This(), pos, n);
    }

private:
    typedef const TCharType* (*GenericFinder)(const TCharType*, size_t, const TCharType*, size_t);

    template<GenericFinder finder>
    inline size_t GenericFind(const TCharType *s, size_t n, size_t pos = npos) const throw() {
        if (pos >= Len()) {
            return npos;
        }
        return off(finder(Ptr() + pos, Len() - pos, s, n));
    }

    inline const TCharType* Ptr() const throw () {
        return This()->data();
    }

    inline size_t Len() const throw () {
        return This()->length();
    }

    inline const TDerived* This() const throw () {
        return static_cast<const TDerived*>(this);
    }

    inline size_t CopyImpl(TCharType* pc, size_t n, size_t pos) const throw () {
        const size_t toCopy = Min(Len() - pos, n);
        TTraits::Copy(pc, Ptr() + pos, toCopy);
        return toCopy;
    }
};

template <typename TDerived, typename TCharType, typename TTraitsType>
const size_t TStringBase<TDerived, TCharType, TTraitsType>::npos;

template <typename TDerived, typename TCharType, typename TTraits>
class TStringImpl: public TStringBase<TDerived, TCharType, TTraits> {
public:
    typedef TStringImpl<TDerived, TCharType, TTraits> TSelf;
    typedef TStringBase<TDerived, TCharType, TTraits> TBase;
    typedef NDetail::TStringDataTraits<TCharType> TDataTraits;
    typedef typename TDataTraits::TData TData;
    typedef typename TBase::TFixedString TFixedString;

    typedef TCharType TdChar;
    typedef TCharType char_type;
    typedef TCharType value_type;
    typedef TTraits traits_type;

    typedef TCharType* iterator;
    typedef typename TBase::const_iterator const_iterator;

private:
    TDerived* This() {
        return static_cast<TDerived*>(this);
    }

    const TDerived* This() const {
        return static_cast<const TDerived*>(this);
    }

protected:

    //! allocates new string data that fits not less than the specified number of characters
    //! @param len      number of characters
    //! @throw std::length_error
    static TCharType* Allocate(size_t len, TData* oldData = 0) {
        return Allocate(len, len, oldData);
    }

    static TCharType* Allocate(size_t oldLen, size_t newLen, TData* oldData) {
        return NDetail::Allocate<TCharType>(oldLen, newLen, oldData);
    }

    // ~~~ Data member ~~~
    TCharType* p;

    // ~~~ Core functions ~~~
    inline void Ref() throw () {
        if (p != TDataTraits::GetNull()) {
            AtomicIncrement(GetData()->Refs);
        }
    }

    inline void UnRef() throw () {
        if (p != TDataTraits::GetNull()) {
            // IsNonShared() check is a common case optimization
            if (IsNonShared() || AtomicDecrement(GetData()->Refs) == 0) {
                NDetail::Deallocate(GetData());
            }
        }
    }

    inline TData* GetData() const throw () {
        return TDataTraits::GetData(p);
    }

    void Relink(TCharType* tmp) {
        UnRef();
        p = tmp;
    }

    bool CloneIfShared() {
        if (IsNonShared())
            return false;
        Clone();
        return true;
    }

    //! makes a distinct copy of itself
    //! @throw std::length_error
    void Clone() {
        const size_t len = length();
        Relink(TTraits::Copy(Allocate(len), p, len));
    }

    void TruncNonShared(size_t n) {
        GetData()->Length = n;
        p[n] = 0;
    }

    void ResizeNonShared(size_t n) {
        if (capacity() < n) {
            p = Allocate(n, GetData());
        } else {
            TruncNonShared(n);
        }
    }

public:
    inline size_t length() const throw () {
        return GetData()->Length;
    }

    inline const TCharType* data() const throw () {
        return p;
    }

    // ~~~ STL compatible method to obtain data pointer ~~~
    iterator begin() {
        CloneIfShared();
        return p;
    }

    iterator vend() {
        CloneIfShared();
        return p + length();
    }

    using TBase::begin;   //!< const_iterator TStringBase::begin() const
    using TBase::end;     //!< const_iterator TStringBase::end() const

    inline size_t reserve() const throw () {
        return GetData()->BufLen;
    }

    inline size_t capacity() const throw () {
        return reserve();
    }

    bool IsNonShared() const {
        return 1 == AtomicGet(GetData()->Refs);
    }

    // ~~~ Size and capacity ~~~
    TDerived& resize(size_t n, TCharType c = ' ') { // remove or append
        const size_t len = length();
        if (n > len) {
            ReserveAndResize(n);
            TTraits::Assign(p + len, n - len, c);
            return *This();
        }
        return remove(n);
    }

    // ~~~ Constructor ~~~ : FAMILY0(,TStringImpl)
    inline TStringImpl()
        : p(TDataTraits::GetNull())
    {
    }

    inline TStringImpl(const TDerived& s)
        : p(s.p)
    {
        Ref();
    }

    template <typename T, typename A>
    inline TStringImpl(const NStl::basic_string<TCharType, T, A>& s)
        : p(TDataTraits::GetNull())
    {
        AssignNoAlias(s.data(), s.length());
    }

    //! @throw std::length_error
    TStringImpl(const TDerived& s, size_t pos, size_t n) {
        size_t len = s.length();
        pos = Min(pos, len);
        n = Min(n, len - pos);
        p = Allocate(n);
        TTraits::Copy(p, s.p + pos, n);
    }

    //! @throw std::length_error
    TStringImpl(const TCharType* pc) {
        const size_t len = TBase::StrLen(pc);
        p = Allocate(len);
        TTraits::Copy(p, pc, len);
    }

    //! @throw std::length_error
    TStringImpl(const TCharType* pc, size_t n) {
        p = Allocate(n);
        TTraits::Copy(p, pc, n);
    }

    //! @throw std::length_error
    TStringImpl(const TCharType* pc, size_t pos, size_t n) {
        p = Allocate(n);
        TTraits::Copy(p, pc + pos, n);
    }

    explicit TStringImpl(TCharType c) {
        p = Allocate(1);
        p[0] = c;
    }

    //! @throw std::length_error
    explicit TStringImpl(size_t n, TCharType c = ' ') {
        p = Allocate(n);
        TTraits::Assign(p, n, c);
    }

    //! @throw std::length_error
    //! @todo the string will be null terminated at @c n but not filled with characters
    explicit TStringImpl(int n) {
        p = Allocate(n);
    }

    TStringImpl(const TCharType* b, const TCharType* e) {
        p = Allocate(e - b);
        TTraits::Copy(p, b, e - b);
    }

    TStringImpl(const TFixedString & s)
        : p(Allocate(s.Length))
    {
        if (0 != s.Length)
            TTraits::Copy(p, s.Start, s.Length);
    }

private:
    template<typename... R>
    static size_t SumLength(const TFixedString& s1, const R&... r) {
        return s1.Length + SumLength(r...);
    }

    static size_t SumLength() {
        return 0;
    }

    template<typename... R>
    static void CopyAll(TCharType* p, const TFixedString& s, const R&... r) {
        TTraits::Copy(p, s.Start, s.Length);
        CopyAll(p + s.Length, r...);
    }

    static void CopyAll(TCharType*) {
    }

public:

    // deprecated, use Join instead
    explicit TStringImpl(const TFixedString& s1, const TFixedString& s2)
        : TStringImpl(Join(s1, s2))
    {
    }

    explicit TStringImpl(const TFixedString& s1, const TFixedString& s2, const TFixedString& s3)
        : TStringImpl(Join(s1, s2, s3))
    {
    }

    // ~~~ Destructor ~~~
    inline ~TStringImpl() throw () {
        UnRef();
    }

    inline void clear() throw () {
        if (IsNonShared()) {
            TruncNonShared(0);
            return;
        }

        Relink(TDataTraits::GetNull());
    }

    template <typename... R>
    static inline TDerived Join(const R&... r) {
        TDerived s;
        s.p = Allocate(SumLength(r...));
        CopyAll(s.p, r...);
        return s;
    }

    // ~~~ Assignment ~~~ : FAMILY0(TStringImpl&, assign);
    TDerived& assign(const TDerived& s) {
        TDerived(s).swap(*This());

        return *This();
    }

    TDerived& assign(const TDerived& s, size_t pos, size_t n) {
        return assign(TDerived(s, pos, n));
    }

    TDerived& assign(const TCharType* pc) {
        return assign(pc, TBase::StrLen(pc));
    }

    TDerived& assign(const TCharType* pc, size_t len) {
        if (EXPECT_TRUE(IsNonShared() && (pc + len <= TBase::begin() || pc >= TBase::end()))) {
            ResizeNonShared(len);
            TTraits::Copy(p, pc, len);
        } else if (IsNonShared() && pc == data() && capacity() >= len) {
            TruncNonShared(len);
        } else {
            Relink(TTraits::Copy(Allocate(len), pc, len));
        }

        return *This();
    }

    TDerived& assign(const TCharType* begin, const TCharType* end) {
        return assign(begin, end - begin);
    }

    TDerived& assign(const TCharType* pc, size_t pos, size_t n) {
        return assign(pc + pos, n);
    }

    inline TDerived& AssignNoAlias(const TCharType* pc, size_t len) {
        if (IsNonShared()) {
            ResizeNonShared(len);
        } else {
            Relink(Allocate(len));
        }

        TTraits::Copy(p, pc, len);

        return *This();
    }

    inline TDerived& AssignNoAlias(const TCharType* b, const TCharType* e) {
        return AssignNoAlias(b, e - b);
    }

    TDerived& assign(const TFixedString& s) {
        return assign(s.Start, s.Length);
    }

    TDerived& assign(const TFixedString& s, size_t spos, size_t sn = TBase::npos) {
        return assign(s.SubString(spos, sn));
    }

    TDerived& AssignNoAlias(const TFixedString& s) {
        return AssignNoAlias(s.Start, s.Length);
    }

    TDerived& AssignNoAlias(const TFixedString& s, size_t spos, size_t sn = TBase::npos) {
        return AssignNoAlias(s.SubString(spos, sn));
    }

    TDerived& operator=(const TDerived& s) {
        return assign(s);
    }

    TDerived& operator=(const TFixedString& s) {
        return assign(s);
    }

    TDerived& operator=(const TCharType* s) {
        return assign(s);
    }

    inline void reserve(size_t len) {
        if (IsNonShared()) {
            if (capacity() < len) {
                p = Allocate(length(), len, GetData());
            }
        } else {
            Relink(TTraits::Copy(Allocate(length(), len, (TData*)0), p, length()));
        }
    }

    // ~~~ Appending ~~~ : FAMILY0(TStringImpl&, append);
    inline TDerived& append(size_t count, TCharType ch) {
        while (count--) {
            append(ch);
        }
        return *This();
    }

    inline TDerived& append(const TDerived& s) {
        if (&s != This()) {
            return AppendNoAlias(~s, +s);
        }
        return append(~s, +s);
    }

    inline TDerived& append(const TDerived& s, size_t pos, size_t n) {
        return append(~s, pos, n, +s);
    }

    inline TDerived& append(const TCharType* pc) {
        return append(pc, TBase::StrLen(pc));
    }

    inline TDerived& append(TCharType c) {
        const size_t olen = length();

        ReserveAndResize(olen + 1);
        *(p + olen) = c;

        return *This();
    }

    inline TDerived& append(const TCharType* begin, const TCharType* end) {
        return append(begin, end - begin);
    }

    inline TDerived& append(const TCharType* pc, size_t len) {
        if (pc + len <= TBase::begin() || pc >= TBase::end()) {
            return AppendNoAlias(pc, len);
        }

        return append(pc, 0, len, len);
    }

    inline void ReserveAndResize(size_t len) {
        if (IsNonShared()) {
            ResizeNonShared(len);
        } else {
            Relink(TTraits::Copy(Allocate(len), p, Min(len, length())));
        }
    }

    inline TDerived& AppendNoAlias(const TCharType* pc, size_t len) {
        const size_t olen = length();
        const size_t nlen = olen + len;
        ReserveAndResize(nlen);
        TTraits::Copy(p + olen, pc, len);
        return *This();
    }

    TDerived& AppendNoAlias(const TFixedString& s) {
        return AppendNoAlias(s.Start, s.Length);
    }

    TDerived& AppendNoAlias(const TFixedString& s, size_t spos, size_t sn = TBase::npos) {
        return AppendNoAlias(s.SubString(spos, sn));
    }

    TDerived& append(const TFixedString& s) {
        return append(s.Start, s.Length);
    }

    TDerived& append(const TFixedString& s, size_t spos, size_t sn = TBase::npos) {
        return append(s.SubString(spos, sn));
    }

    inline TDerived& append(const TCharType* pc, size_t pos, size_t n, size_t pc_len = TBase::npos) {
        return replace(length(), 0, pc, pos, n, pc_len);
    }

    inline void push_back(TCharType c) {
        append(c);
    }

    template <class T>
    TDerived& operator+= (const T& s) {
        return append(s);
    }

    template <class T>
    friend TDerived operator* (const TDerived& s, T count) {
        TDerived result;

        for (T i = 0; i < count; ++i) {
            result += s;
        }

        return result;
    }

    template <class T>
    TDerived& operator*= (T count) {
        TDerived temp;

        for (T i = 0; i < count; ++i) {
            temp += *This();
        }

        swap(temp);

        return *This();
    }

    // "s1 + s2 + s3" expressions optimization:
    // 1. Pass first argument by value and let compiler to copy it for us, or
    // elide copying if this is possible.
    // 2. I haven't heard about compiler which is smart enough to elide copying
    // and perform return value optimization of the same variable, so we're
    // performing lightweight swap operation and returning new variable, which
    // allows compiler to apply RVO.
    friend TDerived operator+ (TDerived s1, const TDerived& s2) {
        s1 += s2;
        TDerived result;
        result.swap(s1);
        return result;
    }

    friend TDerived operator+ (TDerived s1, const TFixedString& s2) {
        s1 += s2;
        TDerived result;
        result.swap(s1);
        return result;
    }

    friend TDerived operator+ (TDerived s1, const TCharType* s2) {
        s1 += s2;
        TDerived result;
        result.swap(s1);
        return result;
    }

    friend TDerived operator+ (TDerived s1, TCharType s2) {
        s1 += s2;
        TDerived result;
        result.swap(s1);
        return result;
    }

    friend TDerived operator+ (const TFixedString& s1, const TDerived& s2) {
        return TDerived(s1, s2);
    }

    friend TDerived operator+ (const TCharType* s1, const TDerived& s2) {
        return TDerived(s1, s2);
    }

    // ~~~ Prepending ~~~ : FAMILY0(TDerived&, prepend);
    TDerived& prepend(const TDerived& s) {
        return replace(0, 0, s.p, 0, TBase::npos, s.length());
    }

    TDerived& prepend(const TDerived& s, size_t pos, size_t n) {
        return replace(0, 0, s.p, pos, n, s.length());
    }

    TDerived& prepend(const TCharType* pc) {
        return replace(0, 0, pc);
    }

    TDerived& prepend(size_t n, TCharType c) {
        return insert(size_t(0), n, c);
    }

    TDerived& prepend(TCharType c) {
        return replace(0, 0, &c, 0, 1, 1);
    }

    TDerived& prepend(const TFixedString& s, size_t spos = 0, size_t sn = TBase::npos) {
        return insert(0, s, spos, sn);
    }

    // ~~~ Insertion ~~~ : FAMILY1(TDerived&, insert, size_t pos);
    TDerived& insert(size_t pos, const TDerived& s) {
        return replace(pos, 0, s.p, 0, TBase::npos, s.length());
    }

    TDerived& insert(size_t pos, const TDerived& s, size_t pos1, size_t n1) {
        return replace(pos, 0, s.p, pos1, n1, s.length());
    }

    TDerived& insert(size_t pos, const TCharType* pc) {
        return replace(pos, 0, pc);
    }

    TDerived& insert(size_t pos, const TCharType* pc, size_t len) {
        return insert(pos, TFixedString(pc, len));
    }

    TDerived& insert(const_iterator p, const_iterator b, const_iterator e) {
        return insert(this->off(p), b, e - b);
    }

    TDerived& insert(size_t pos, size_t n, TCharType c) {
        if (n == 1)
            return replace(pos, 0, &c, 0, 1, 1);
        else
            return insert(pos, TDerived(n, c));
    }

    TDerived& insert(const_iterator p, size_t len, TCharType ch) {
        return this->insert(this->off(p), len, ch);
    }

    TDerived& insert(const_iterator p, TCharType ch) {
        return this->insert(p, 1, ch);
    }

    TDerived& insert(size_t pos, const TFixedString& s, size_t spos = 0, size_t sn = TBase::npos) {
        return replace(pos, 0, s, spos, sn);
    }

    // ~~~ Removing ~~~
    TDerived& remove(size_t pos, size_t n) {
        return replace(pos, n, TDataTraits::GetNull(), 0, 0, 0);
    }

    TDerived& remove(size_t pos = 0) {
        if (pos < length()) {
            CloneIfShared();
            TruncNonShared(pos);
        }

        return *This();
    }

    TDerived& erase(size_t pos = 0, size_t n = TBase::npos) {
        return remove(pos, n);
    }

    TDerived& erase(const_iterator b, const_iterator e) {
        return erase(this->off(b), e - b);
    }

    TDerived& erase(const_iterator i) {
        return erase(i, i + 1);
    }

    TDerived& pop_back() {
        YASSERT(!this->empty());
        return erase(this->length() - 1, 1);
    }

    // ~~~ replacement ~~~ : FAMILY2(TDerived&, replace, size_t pos, size_t n);
    TDerived& replace(size_t pos, size_t n, const TDerived& s) {
        return replace(pos, n, s.p, 0, TBase::npos, s.length());
    }

    TDerived& replace(size_t pos, size_t n, const TDerived& s, size_t pos1, size_t n1) {
        return replace(pos, n, s.p, pos1, n1, s.length());
    }

    TDerived& replace(size_t pos, size_t n, const TCharType* pc) {
        return replace(pos, n, TFixedString(pc));
    }

    TDerived & replace(size_t pos, size_t n, const TCharType * s, size_t len) {
        return replace(pos, n, s, 0, len, len);
    }

    TDerived & replace(size_t pos, size_t n, const TCharType * s, size_t spos, size_t sn) {
        return replace(pos, n, s, spos, sn, sn);
    }

    TDerived& replace(size_t pos, size_t n1, size_t n2, TCharType c) {
        if (n2 == 1)
            return replace(pos, n1, &c, 0, 1, 1);
        else
            return replace(pos, n1, TDerived(n2, c));
    }

    TDerived & replace(size_t pos, size_t n, const TFixedString& s, size_t spos = 0, size_t sn = TBase::npos) {
        return replace(pos, n, s.Start, spos, sn, s.Length);
    }

    // ~~~ main driver: should be protected (in the future)
    TDerived& replace(size_t pos, size_t del, const TCharType *pc, size_t pos1, size_t ins, size_t len1) {
        size_t len = length();
        // 'pc' can point to a single character that is not null terminated, so in this case TTraits::GetLength must not be called
        len1 = pc ? (len1 == TBase::npos ? (ins == TBase::npos ? TTraits::GetLength(pc) : TTraits::GetLength(pc, ins + pos1)) : len1) : 0;

        pos = Min(pos, len);
        pos1 = Min(pos1, len1);

        del = Min(del, len - pos);
        ins = Min(ins, len1 - pos1);

        if (len - del > this->max_size() - ins) { // len-del+ins -- overflow
            ThrowLengthError("TStringImpl::replace");
        }

        size_t total = len - del + ins;

        if (!total) {
            clear();
            return *This();
        }

        size_t rem = len - del - pos;
        if (!IsNonShared() || (pc && (pc >= p && pc < p + len))) {
            // malloc
            // 1. alias
            // 2. overlapped
            TCharType* temp = Allocate(total);
            TTraits::Copy(temp, p, pos);
            TTraits::Copy(temp + pos, pc + pos1, ins);
            TTraits::Copy(temp + pos + ins, p + pos + del, rem);
            Relink(temp);
        } else if (reserve() < total) {
            // realloc (increasing)
            // 3. not enough room
            p = Allocate(total, GetData());
            TTraits::Move(p + pos + ins, p + pos + del, rem);
            TTraits::Copy(p + pos, pc + pos1, ins);
        } else {
            // 1. not alias
            // 2. not overlapped
            // 3. enough room
            // 4. not too much room
            TTraits::Move(p + pos + ins, p + pos + del, rem);
            TTraits::Copy(p + pos, pc + pos1, ins);
            //GetData()->SetLength(total);
            TruncNonShared(total);
        }
        return *This();
    }

    // ~~ Initial Capacity ~~
    static size_t initial_capacity(size_t = 0) {
        return 0;
    }

    static size_t get_initial_capacity() {
        return 0;
    }

    // ~~ ReSize Increment ~~
    static size_t resize_increment(size_t = 32) {
        return 32;
    }

    static size_t get_resize_increment() {
        return 32;
    }

    // ~~~ Reversion ~~~~
    void reverse() {
        CloneIfShared();
        TTraits::Reverse(p, length());
    }

    void swap(TDerived& s) throw () {
        DoSwap(p, s.p);
    }

    /// Return string suitable for debug printing (like Python's repr()).
    /// Format of the string is unspecified and may be changed over time.
    TDerived Quote() const {
        extern TDerived EscapeC(const TDerived&);

        return TDerived() + '"' + EscapeC(*This()) + '"';
    }
};

class Stroka: public TStringImpl< Stroka, char, TCharTraits<char> > {
public:
    typedef TCharTraits<char> TTraits;
    typedef TStringImpl<Stroka, char, TTraits> TImpl;
    typedef TImpl::TFixedString TFixedString;

    Stroka() {
    }

    Stroka(const Stroka& s)
        : TImpl(s)
    {
    }

    Stroka(Stroka&& s) noexcept {
        swap(s);
    }

    template <typename T, typename A>
    explicit Stroka(const NStl::basic_string<char, T, A>& s)
        : TImpl(s)
    {
    }

    Stroka(const Stroka& s, size_t pos, size_t n)
        : TImpl(s, pos, n)
    {
    }

    Stroka(const char* pc)
        : TImpl(pc)
    {
    }

    Stroka(const char* pc, size_t n)
        : TImpl(pc, n)
    {
    }

    Stroka(const char* pc, size_t pos, size_t n)
        : TImpl(pc, pos, n)
    {
    }

    explicit Stroka(char c)
        : TImpl(c)
    {
    }

    explicit Stroka(size_t n, char c = ' ')
        : TImpl(n, c)
    {
    }

    //! @todo the string will be null terminated at @c n but not filled with characters
    explicit Stroka(int n)
        : TImpl(n)
    {
    }

    Stroka(const char* b, const char* e)
        : TImpl(b, e)
    {
    }

    explicit Stroka(const TFixedString& s)
        : TImpl(s)
    {
    }

public:

    explicit Stroka(const TFixedString& s1, const TFixedString& s2)
        : TImpl(s1, s2)
    {
    }

    explicit Stroka(const TFixedString& s1, const TFixedString& s2, const TFixedString& s3)
        : TImpl(s1, s2, s3)
    {
    }

    Stroka& operator=(const Stroka& s) {
        return assign(s);
    }

    Stroka& operator=(Stroka&& s) noexcept {
        swap(s);

        return *this;
    }

    Stroka& operator=(const TFixedString& s) {
        return assign(s);
    }

    Stroka& operator=(const TdChar* s) {
        return assign(s);
    }

protected:
    static bool read_token_part(TIStream& is, char* buf, size_t bufsize, char);
    static bool read_line_part(TIStream& is, char* buf, size_t bufsize, char delim);
    TIStream& build(TIStream& is, bool(*f)(TIStream&, char*, size_t, char), size_t& total, char delim = 0);

public:
    // @{
    /**
     * Modifies the case of the string, depending on the operation.
     * @return false if no changes have been made
     */
    bool to_lower(size_t pos = 0, size_t n = TBase::npos, const CodePage& cp = csYandex);
    bool to_upper(size_t pos = 0, size_t n = TBase::npos, const CodePage& cp = csYandex);
    bool to_title(const CodePage& cp = csYandex);
    // @}

    //~~~~Input~~~~
    // ~~ until Delimiter ~~
    TIStream& read_to_delim(TIStream& is, char delim = '\n') {
        size_t n = 0;
        return build(is, read_line_part, n, delim);
    }

    friend TIStream& getline(TIStream& is, Stroka& s, char c = '\n') {
        return s.read_to_delim(is, c);
    }

    // ~~ until WhiteSpace
    TIStream& read_token(TIStream& is) {
        size_t n = 0;
        return build(is, read_token_part, n);
    }

    friend TIStream& operator >> (TIStream& is, Stroka& s) {
        return s.read_token(is);
    }

    // ~~ until EOS ~~
    TIStream& read_string(TIStream& is) {
        return read_to_delim(is, 0);
    }

    friend Stroka to_lower(const Stroka& s, const CodePage& cp = csYandex) {
        Stroka ret(s);
        ret.to_lower(0, TBase::npos, cp);
        return ret;
    }

    friend Stroka to_upper(const Stroka& s, const CodePage& cp = csYandex) {
        Stroka ret(s);
        ret.to_upper(0, TBase::npos, cp);
        return ret;
    }

    friend Stroka to_title(const Stroka& s, const CodePage& cp = csYandex) {
        Stroka ret(s);
        ret.to_title(cp);
        return ret;
    }

    static int cmp(const char* s1, size_t n1, const char* s2, size_t n2) {
        return TTraits::Compare(s1, n1, s2, n2);
    }
};

class Wtroka: public TStringImpl< Wtroka, wchar16, TCharTraits<wchar16> > {
public:
    typedef TCharTraits<wchar16> TTraits;
    typedef TStringImpl<Wtroka, wchar16, TTraits> TImpl;
    typedef TImpl::TFixedString TFixedString;

    Wtroka() {
    }

    Wtroka(Wtroka&& s) noexcept {
        swap(s);
    }

    Wtroka(const Wtroka& s)
        : TImpl(s)
    {
    }

    template <typename T, typename A>
    explicit Wtroka(const NStl::basic_string<wchar16, T, A>& s)
        : TImpl(s)
    {
    }

    Wtroka(const Wtroka& s, size_t pos, size_t n)
        : TImpl(s, pos, n)
    {
    }

    Wtroka(const wchar16* pc)
        : TImpl(pc)
    {
    }

    Wtroka(const wchar16* pc, size_t n)
        : TImpl(pc, n)
    {
    }

    Wtroka(const wchar16* pc, size_t pos, size_t n)
        : TImpl(pc, pos, n)
    {
    }

    explicit Wtroka(wchar16 c)
        : TImpl(c)
    {
    }

    explicit Wtroka(size_t n, wchar16 c)
        : TImpl(n, c)
    {
    }

    //! @todo the string will be null terminated at @c n but not filled with characters
    explicit Wtroka(int n)
        : TImpl(n)
    {
    }

    Wtroka(const wchar16* b, const wchar16* e)
        : TImpl(b, e)
    {
    }

    explicit Wtroka(const TFixedString& s)
        : TImpl(s)
    {
    }

    explicit Wtroka(const TFixedString& s1, const TFixedString& s2)
        : TImpl(s1, s2)
    {
    }

    explicit Wtroka(const TFixedString& s1, const TFixedString& s2, const TFixedString& s3)
        : TImpl(s1, s2, s3)
    {
    }

    static Wtroka FromUtf8(const ::TFixedString<char>& s) {
        return Wtroka().AppendUtf8(s);
    }

    static Wtroka FromAscii(const ::TFixedString<char>& s) {
        return Wtroka().AppendAscii(s);
    }

    Wtroka& AssignUtf8(const ::TFixedString<char>& s) {
        clear();
        return AppendUtf8(s);
    }

    Wtroka& AssignAscii(const ::TFixedString<char>& s) {
        clear();
        return AppendAscii(s);
    }

    Wtroka& AppendUtf8(const ::TFixedString<char>& s);
    Wtroka& AppendAscii(const ::TFixedString<char>& s);

    Wtroka& operator=(const Wtroka& s) {
        return assign(s);
    }

    Wtroka& operator=(Wtroka&& s) noexcept {
        swap(s);

        return *this;
    }

    Wtroka& operator=(const TFixedString& s) {
        return assign(s);
    }

    Wtroka& operator=(const TdChar* s) {
        return assign(s);
    }

    // ~~~ Reversion ~~~~
    void reverse();

    // @{
    /**
     * Modifies the case of the string, depending on the operation.
     * @return false if no changes have been made
     */
    bool to_lower(size_t pos = 0, size_t n = TBase::npos);
    bool to_upper(size_t pos = 0, size_t n = TBase::npos);
    bool to_title();
    // @}

    friend Wtroka to_lower(const Wtroka& s) {
        Wtroka ret(s);
        ret.to_lower();
        return ret;
    }

    friend Wtroka to_upper(const Wtroka& s) {
        Wtroka ret(s);
        ret.to_upper();
        return ret;
    }

    friend Wtroka to_title(const Wtroka& s) {
        Wtroka ret(s);
        ret.to_title();
        return ret;
    }
};

TOStream& operator << (TOStream&, const Stroka&);

// Same as Stroka but uses CASE INSENSITIVE comparator and hash. Use with care.
class stroka : public Stroka {
public:
    stroka() {
    }

    stroka(const Stroka& s)
        : Stroka(s)
    {
    }

    stroka(const Stroka& s, size_t pos, size_t n)
        : Stroka(s, pos, n)
    {
    }

    stroka(const char* pc)
        : Stroka(pc)
    {
    }

    stroka(const char* pc, size_t n)
        : Stroka(pc, n)
    {
    }

    stroka(const char* pc, size_t pos, size_t n)
        : Stroka(pc, pos, n)
    {
    }

    explicit stroka(char c)
        : Stroka(c)
    {
    }

    explicit stroka(size_t n, char c = ' ')
        : Stroka(n, c)
    {
    }

    stroka(const char* b, const char* e)
        : Stroka(b, e)
    {
    }

    explicit stroka(const TFixedString& s)
        : Stroka(s)
    {
    }

    // ~~~ Comparison ~~~ : FAMILY0(int, compare)
    static int compare(const stroka &s1, const stroka &s2, const CodePage& cp = csYandex);
    static int compare(const char* p, const stroka &s2, const CodePage& cp = csYandex);
    static int compare(const stroka &s1, const char* p, const CodePage& cp = csYandex);
    static int compare(const TFixedString &p1, const TFixedString &p2, const CodePage& cp = csYandex);

    static bool is_prefix(const TFixedString &what, const TFixedString &of, const CodePage& cp = csYandex);
    static bool is_suffix(const TFixedString &what, const TFixedString &of, const CodePage& cp = csYandex);

    bool is_prefix(const TFixedString &s, const CodePage& cp = csYandex) const {
        return is_prefix(*this, s, cp);
    }

    bool is_suffix(const TFixedString &s, const CodePage& cp = csYandex) const {
        return is_suffix(*this, s, cp);
    }

    bool has_prefix(const TFixedString &s, const CodePage& cp = csYandex) const {
        return is_prefix(s, *this, cp);
    }

    bool has_suffix(const TFixedString &s, const CodePage& cp = csYandex) const {
        return is_suffix(s, *this, cp);
    }

    friend bool operator == (const stroka &s1, const stroka &s2) {
        return stroka::compare(s1, s2) == 0;
    }

    friend bool operator == (const stroka &s, const char *pc) {
        return stroka::compare(s, pc) == 0;
    }

    friend bool operator == (const char *pc, const stroka &s) {
        return stroka::compare(pc, s) == 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator == (const stroka &s, const TStringBase<TDerived2, TChar, TTraits2> &pc) {
        return stroka::compare(s, pc) == 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator == (const TStringBase<TDerived2, TChar, TTraits2> &pc, const stroka &s) {
        return stroka::compare(pc, s) == 0;
    }

    friend bool operator != (const stroka &s1, const stroka &s2) {
        return stroka::compare(s1, s2) != 0;
    }

    friend bool operator != (const stroka &s, const char *pc) {
        return stroka::compare(s, pc) != 0;
    }

    friend bool operator != (const char *pc, const stroka &s) {
        return stroka::compare(pc, s) != 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator != (const stroka &s, const TStringBase<TDerived2, TChar, TTraits2> &pc) {
        return stroka::compare(s, pc) != 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator != (const TStringBase<TDerived2, TChar, TTraits2> &pc, const stroka &s) {
        return stroka::compare(pc, s) != 0;
    }

    friend bool operator < (const stroka &s1, const stroka &s2) {
        return stroka::compare(s1, s2) < 0;
    }

    friend bool operator < (const stroka &s, const char *pc) {
        return stroka::compare(s, pc) < 0;
    }

    friend bool operator < (const char *pc, const stroka &s) {
        return stroka::compare(pc, s) < 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator < (const stroka &s, const TStringBase<TDerived2, TChar, TTraits2> &pc) {
        return stroka::compare(s, pc) < 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator < (const TStringBase<TDerived2, TChar, TTraits2> &pc, const stroka &s) {
        return stroka::compare(pc, s) < 0;
    }

    friend bool operator <= (const stroka &s1, const stroka &s2) {
        return stroka::compare(s1, s2) <= 0;
    }

    friend bool operator <= (const stroka &s, const char *pc) {
        return stroka::compare(s, pc) <= 0;
    }

    friend bool operator <= (const char *pc, const stroka &s) {
        return stroka::compare(pc, s) <= 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator <= (const stroka &s, const TStringBase<TDerived2, TChar, TTraits2> &pc) {
        return stroka::compare(s, pc) <= 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator <= (const TStringBase<TDerived2, TChar, TTraits2> &pc, const stroka &s) {
        return stroka::compare(pc, s) <= 0;
    }

    friend bool operator > (const stroka &s1, const stroka &s2) {
        return stroka::compare(s1, s2) > 0;
    }

    friend bool operator > (const stroka &s, const char *pc) {
        return stroka::compare(s, pc) > 0;
    }

    friend bool operator > (const char *pc, const stroka &s) {
        return stroka::compare(pc, s) > 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator > (const stroka &s, const TStringBase<TDerived2, TChar, TTraits2> &pc) throw () {
        return stroka::compare(s, pc) > 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator > (const TStringBase<TDerived2, TChar, TTraits2> &pc, const stroka &s) throw () {
        return stroka::compare(pc, s) > 0;
    }

    friend bool operator >= (const stroka &s1, const stroka &s2) {
        return stroka::compare(s1, s2) >= 0;
    }

    friend bool operator >= (const stroka &s, const char *pc) {
        return stroka::compare(s, pc) >= 0;
    }

    friend bool operator >= (const char *pc, const stroka &s) {
        return stroka::compare(pc, s) >= 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator >= (const stroka &s, const TStringBase<TDerived2, TChar, TTraits2> &pc) {
        return stroka::compare(s, pc) >= 0;
    }

    template <typename TDerived2, typename TTraits2>
    friend bool operator >= (const TStringBase<TDerived2, TChar, TTraits2> &pc, const stroka &s) {
        return stroka::compare(pc, s) >= 0;
    }

    static size_t hashVal(const char* pc, size_t len, const CodePage& cp = csYandex);

    size_t hash() const {
        return stroka::hashVal(p, length());
    }
};

#ifdef DONT_USE_CODEPAGE
inline int stroka::compare(const stroka &s1, const stroka &s2) {
    return stricmp(s1.p, s2.p);
}
inline int stroka::compare(const char* p, const stroka &s2) {
    return stricmp(p, s2.p);
}
inline int stroka::compare(const stroka &s1, const char* p) {
    return stricmp(s1.p, p);
}
inline int stroka::compare(const TFixedString &p1, const TFixedString &p2) {
    int rv = strnicmp(p1.Start, p2.Start, Min(p1.Length, p2.Length));
    return rv? rv : p1.Length < p2.Length? -1 : p1.Length == p2.Length? 0 : 1;
}
inline bool stroka::is_prefix(const TFixedString &what, const TFixedString &of) const {
    size_t len = what.Length;
    return len <= of.Length && strnicmp(what.Start, of.Start, len) == 0;
}
#endif
