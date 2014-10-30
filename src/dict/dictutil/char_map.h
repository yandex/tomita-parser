#pragma once

#include <vector>
#include <util/system/defaults.h>

template<typename T>
class TWCharMap {
    DECLARE_NOCOPY(TWCharMap);

// Constructor/Destructor
public:
    TWCharMap(const T& value = T())
      : Memory()
      , DefValue(value)
    {
        T* nullBlock = NewBlock();
        std::fill_n(Blocks, ARRAY_SIZE(Blocks), nullBlock);
    }

    ~TWCharMap() {
        for (int i = 0; i < (int)Memory.size(); ++i)
            delete[] Memory[i];
        Memory.clear();
    }

// Methods/Operators
public:
    T& at(wchar16 ch) {
        T* block = Blocks[ch >> 8];
        if (block == Memory[0]) // it's nullBlock?
            block = Blocks[ch >> 8] = NewBlock();
        return block[ch & 0xff];
    }

    const T& operator[](wchar16 ch) const {
        return Blocks[ch >> 8][ch & 0xff];
    }

// Helper methods
private:
    T* NewBlock() {
        T* block = new T[256]();
        Memory.push_back(block);
        if (DefValue != T())
            std::fill_n(block, 256, DefValue);
        return block;
    }

// Fields
private:
    T* Blocks[256];
    std::vector<T*> Memory;
    const T DefValue;
};
