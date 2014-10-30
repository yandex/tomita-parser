#pragma once

#include <util/system/defaults.h>

template <class T, unsigned Align = sizeof(T)>
class TUnalignedMemoryIterator {
    public:
        inline TUnalignedMemoryIterator(const void* buf, size_t len)
            : C_((const unsigned char*)buf)
            , E_(C_ + len)
            , L_(E_ - (len % Align))
        {
        }

        inline bool AtEnd() const throw () {
            return C_ == L_;
        }

        inline T Cur() const throw () {
#if defined(_x86_64_) || defined(_i386_) || defined(_arm_)
            return *(T*)C_;
#else
            #error todo
#endif
        }

        inline T Next() throw () {
            T ret(Cur());

            C_ += sizeof(T);

            return ret;
        }

        inline const unsigned char* Last() const throw () {
            return C_;
        }

        inline size_t Left() const throw () {
            return E_ - C_;
        }

    private:
        const unsigned char* C_;
        const unsigned char* E_;
        const unsigned char* L_;
};
