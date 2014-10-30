#pragma once

#include "mem.h"
#include "zerocopy.h"

class TWalkInput: public TInputStream, public IZeroCopyInput {
    public:
        inline TWalkInput(IZeroCopyInput* walker) throw ()
            : Walker_(walker)
        {
        }

        virtual ~TWalkInput() throw () {
        }

    private:
        virtual size_t DoRead(void* buf, size_t len) {
            const size_t ret = Mi_.Read(buf, len);

            if (ret) {
                return ret;
            }

            Mi_.Fill(Walker_);

            return Mi_.Read(buf, len);
        }

        virtual bool DoNext(const void** ptr, size_t* len) {
            return Mi_.Next(ptr, len) || Walker_->Next(ptr, len);
        }

    private:
        IZeroCopyInput* Walker_;
        TMemoryInput Mi_;
};
