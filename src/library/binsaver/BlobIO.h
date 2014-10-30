#pragma once

#include "BinSaver.h"
#include "BufferedIO.h"

#include <util/memory/blob.h>

class TYaBlobStream : public IBinaryStream
{
    TBlob Blob;
    int Pos;

    int Write(const void*, int)
    {
        YASSERT(0);
        return 0;
    }
    int Read(void *userBuffer, int size)
    {
        if (size == 0)
            return 0;
        int res = Min((int)Blob.Length() - Pos, size);
        if (res)
            memcpy(userBuffer, ((const char*)Blob.Data()) + Pos, res);
        Pos += res;
        return res;
    }
    bool IsValid() const { return true; }
    bool IsFailed() const { return false; }
public:
    TYaBlobStream(const TBlob &blob) : Blob(blob), Pos(0) {}
};


template<class T>
inline void SerializeBlob(const TBlob &data, T &c)
{
    TYaBlobStream f(data);
    {
        IBinSaver bs(f, true);
        bs.Add(1, &c);
    }
}
