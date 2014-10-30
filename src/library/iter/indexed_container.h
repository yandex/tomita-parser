#pragma once

namespace NIter {

// Allows iteration over any collection of known size and random access operator[].
template <class T, class TIndexedContainer, bool fixSize>
class TIndexedContainerIterator {
public:
    typedef T TValue;

    TIndexedContainerIterator()
        : Source(NULL)
        , FixedSize(0)
        , CurIndex(0)
    {
    }

    TIndexedContainerIterator(TIndexedContainer& source)
        : Source(&source)
        , FixedSize(source.size())
        , CurIndex(0)
    {
    }

    inline TIndexedContainerIterator(TIndexedContainer& source, size_t size)
        : Source(&source)
        , FixedSize(size)
        , CurIndex(0)
    {
    }

    inline bool Ok() const {
        return Source != NULL && CurIndex < GetSize();
    }

    inline void operator++() {
        ++CurIndex;
    }

    inline T* operator->() const {
        return &((*Source)[CurIndex]);
    }

    inline T& operator*() const {
        return (*Source)[CurIndex];
    }
private:
    size_t GetSize() const {
        if (fixSize)
            return FixedSize;
        else
            return Source->size();
    }

    TIndexedContainer* Source;
    size_t FixedSize, CurIndex;
};

} // namespace NIter
