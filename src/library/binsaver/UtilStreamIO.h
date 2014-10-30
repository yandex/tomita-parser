#pragma once
#include "BinSaver.h"

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/file.h>


class TYaStreamInput : public IBinaryStream
{
    TInputStream &Stream;

    int Write(const void*, int)
    {
        YASSERT(0);
        return 0;
    }
    int Read(void *userBuffer, int size)
    {
        return (int)Stream.Read(userBuffer, (size_t) size);
    }
    bool IsValid() const { return true; }
    bool IsFailed() const { return false; }
public:
    TYaStreamInput(TInputStream &stream)
        : Stream(stream) {
    }
};

template<class T>
inline void SerializeFromStream(TInputStream &stream, T &c)
{
    TYaStreamInput f(stream);
    {
        IBinSaver bs(f, true);
        bs.Add(1, &c);
    }
}

template<class T>
inline void SerializeFromFile(const Stroka &fileName, T &c)
{
    TIFStream in(fileName);
    SerializeFromStream(in, c);
}


class TYaStreamOutput : public IBinaryStream
{
    TOutputStream &Stream;

    int Write(const void* what, int size)
    {
        Stream.Write(what, (size_t) size);
        return size;
    }
    int Read(void*, int)
    {
        YASSERT(0);
        return 0;
    }
    bool IsValid() const { return true; }
    bool IsFailed() const { return false; }
public:
    TYaStreamOutput(TOutputStream &stream)
        : Stream(stream) {
    }
};

template<class T>
inline void SerializeToStream(TOutputStream &stream, T &c)
{
    TYaStreamOutput f(stream);
    {
        IBinSaver bs(f, false);
        bs.Add(1, &c);
    }
}

template<class T>
inline void SerializeToFile(const Stroka &fileName, T &c)
{
    TOFStream out(fileName);
    SerializeToStream(out, c);
}


