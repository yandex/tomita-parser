#include "lz.h"
#include "file.h"
#include "lzma.h"
#include "zlib.h"
#include "bzip2.h"
#include "buffered.h"
#include "stack_input_file.h"
#include "stack_output_file.h"

#include <util/generic/map.h>
#include <util/generic/stroka.h>

namespace Nydx {
namespace Nio {

/*
 * When creating stream stacks, we need to ensure that buffering layers
 * are added as needed, so that expensive operations are buffered. For
 * instance, file IO is expensive; compression operations benefitting from
 * larger input data (a block) can also be expensive.
 *
 * To add a type, derive the IType interface and map the file extension
 * to an instance of the derived type in TExtensionMap.
 */

namespace {

struct IType {
    virtual ~IType() {}
    // return true if top is buffered
    virtual bool Push(TStreamStackInput&) = 0;
    virtual bool Push(TStreamStackOutput&) = 0;
};

struct TTypeGZ
    : public IType
{
    virtual bool Push(TStreamStackInput& stack) {
        stack.Push(new TZLibDecompress(stack));
        return false;
    }
    virtual bool Push(TStreamStackOutput& stack) {
        stack.Push(new TZLibCompress(stack, ZLib::GZip));
        return false;
    }
};

struct TTypeBZ2
    : public IType
{
    virtual bool Push(TStreamStackInput& stack) {
        stack.Push(new TBZipDecompress(stack));
        return false;
    }
    virtual bool Push(TStreamStackOutput& stack) {
        stack.Push(new TBZipCompress(stack));
        return false;
    }
};

struct TTypeLZMA
    : public IType
{
    virtual bool Push(TStreamStackInput& stack) {
        stack.Push(new TLzmaDecompress(stack));
        return false;
    }
    virtual bool Push(TStreamStackOutput& stack) {
        stack.Push(new TBufferedOutput(stack));
        stack.Push(new TLzmaCompress(stack));
        return false;
    }
};

struct TTypeLZO
    : public IType
{
    virtual bool Push(TStreamStackInput& stack) {
        stack.Push(new TBufferedInput(stack));
        stack.Push(new TLzoDecompress(stack));
        return false;
    }
    virtual bool Push(TStreamStackOutput& stack) {
        stack.Push(new TBufferedOutput(stack));
        stack.Push(new TLzoCompress(stack));
        return false;
    }
};

struct TExtensionMap
{
    TExtensionMap() {
        Set("gz", new TTypeGZ);
        Set("bz2", new TTypeBZ2);
        Set("lzo", new TTypeLZO);
        Set("lzma", new TTypeLZMA);
    }
    typedef TAutoPtr<IType> TDVal;
    typedef ymap<TStringBuf, TDVal> TDMap;
    void Set(const TStringBuf& key, const TDVal& val) {
        Map_.insert(TDMap::value_type(key, val));
    }
    template <typename TPStream> inline bool Push(TStreamStackT<TPStream>& stack, TStringBuf name) const
    {
        TStringBuf ext = name.RNextTok('.');
        TDMap::const_iterator it = Map_.find(ext);
        return Map_.end() != it && it->second->Push(stack);
    }
    TDMap Map_;
};

// singleton
static TExtensionMap ExtensionMap;

}

void TStreamStackInputFileFactory
    ::DoOpen(TStreamStackInput& stack, const TStringBuf& file)
{
    if (file.empty())
        stack.Push(Cin);
    else {
        // this is unbuffered
        stack.Push(new TFileInput(Stroka(file)));
        if (!ExtensionMap.Push(stack, file))
            stack.Push(new TBufferedInput(stack));
    }
}

void TStreamStackOutputFileFactory
    ::DoOpen(TStreamStackOutput& stack, const TStringBuf& file)
{
    if (file.empty())
        stack.Push(Cout);
    else {
        // this is unbuffered
        stack.Push(new TFileOutput(Stroka(file)));
        if (!ExtensionMap.Push(stack, file))
            stack.Push(new TBufferedOutput(stack));
    }
}

}
}
