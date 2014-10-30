#pragma once

#include "lz.h"

class TLzmaCompress: public TOutputStream {
    public:
        TLzmaCompress(TOutputStream* slave, size_t level = 7);
        virtual ~TLzmaCompress() throw ();

    private:
        virtual void DoWrite(const void* buf, size_t len);
        virtual void DoFinish();

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

class TLzmaDecompress: public TInputStream {
    public:
        TLzmaDecompress(TInputStream* slave);
        virtual ~TLzmaDecompress() throw ();

    private:
        virtual size_t DoRead(void* buf, size_t len);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};
