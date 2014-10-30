#pragma once

#include <util/generic/yexception.h>
#include <functional>

class CWordsPair
{
public:
    CWordsPair()
        : First(-1)
        , Last(-1)
    {
    }

    CWordsPair(int v)
        : First(v)
        , Last(v)
    {
        YASSERT(v >= -1);
    }

    CWordsPair(int first, int last)
        : First(first)
        , Last(last)
    {
        YASSERT((first == -1 && last == -1) || (first != -1 && last != -1 && first <= last));
    }

    virtual ~CWordsPair() {
    }

    void SetPair(int first, int last) {
        YASSERT((first == -1 && last == -1) || (first != -1 && last != -1 && first <= last));
        First = first;
        Last = last;
    }

    void SetPair(const CWordsPair& p) {
        SetPair(p.First, p.Last);
    }

    virtual void SetFirstWord(int first) {
        YASSERT(first != -1);
        YASSERT(Last >= first);
        First = first;
    }

    virtual void SetLastWord(int last) {
        YASSERT(last != -1);
        YASSERT(last >= First);
        Last = last;
    }

    virtual void reset() {
        First = -1;
        Last = -1;
    }

    virtual CWordsPair* CloneAsWordsPair();

    int FirstWord() const {
        return First;
    }

    int LastWord() const {
        return Last;
    }

    size_t Size() const {
        return Last - First + 1;
    }

    virtual bool IsValid() const {
        return First != -1 && Last != -1;
    }

    // Primitive operations with pairs

    bool operator < (const CWordsPair& p) const {
        // This is very strange ordering (it is not defined for non-inclusive intersections!).
        return (p.Includes(*this) || p.FromRight(*this)) && !(*this == p);

        // This ordering is better, but it does not work well for some reason (multiple tests from fdo fail with it)
        // return Last < p.Last || (Last == p.Last && First < p.First);
    }

    bool operator == (const CWordsPair& p) const {
        return First == p.First && Last  == p.Last;
    }

    bool Includes(const CWordsPair& p) const {
        return First <= p.First
            && Last  >= p.Last;
    }

    bool Contains(int w) const {
        return First <= w && Last >= w;
    }

    bool FromLeft(const CWordsPair& p) const {
        return First < p.First
            && Last < p.First;
    }

    bool FromRight(const CWordsPair& p) const {
        return First > p.Last
            && Last  > p.Last;
    }

    bool Intersects(const CWordsPair& p) const {
        return Last >= p.First && p.Last >= First;
    }

protected:
    int First;
    int Last;
};

class SPeriodOrder : public std::binary_function<const CWordsPair*&, const CWordsPair*&, bool>
{
public:
    bool operator()(const CWordsPair* cl1, const CWordsPair* cl2) const
    {
        return *(cl1) < *(cl2);
    }
};

//простой лексикографический порядок на парах, которые могут пересекаться
//(SPeriodOrder на пересекающихся парах ведет себя неадекватно)
class SSimplePeriodOrder : public std::binary_function<const CWordsPair&, const CWordsPair&, bool>
{
public:
    bool operator()(const CWordsPair& cl1, const CWordsPair& cl2) const
    {
        if (cl1.FirstWord() < cl2.FirstWord())
            return true;
        return cl1.LastWord() < cl2.LastWord();
    }
};

