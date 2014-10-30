#include "factory.h"

#include "bzip2.h"
#include "holder.h"
#include "file.h"
#include "ios.h"
#include "lz.h"
#include "str.h"
#include "zlib.h"

#ifdef _win_ // isatty
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {
    template <class T>
    class TWrapper {
    public:
        template <class C>
        inline TWrapper(const C& c)
            : Slave_(c)
        {
        }

    protected:
        T Slave_;
    };

    class TZLibHelper: public TZLibDecompress {
    public:
        inline TZLibHelper(IZeroCopyInput* in)
            : TZLibDecompress(in)
        {
        }
    };

    template <class T, class TDecoder>
    class TCompressed: public TWrapper<T>, public TDecoder {
    public:
        template <class C>
        inline TCompressed(const C& c)
            : TWrapper<T>(c)
            , TDecoder(&this->Slave_)
        {
        }

        virtual ~TCompressed() throw () {
        }
    };

    class TStringMultiInput: private Stroka, private THolder<TInputStream>, public TMultiInput {
    public:
        TStringMultiInput(const Stroka& head, TInputStream* tail)
            : Stroka(head)
            , THolder<TInputStream>(new TStringInput(*this))
            , TMultiInput(Get(), tail)
        {
        }

        virtual ~TStringMultiInput() throw () {
        }
    };
}

TAutoPtr<TInputStream> OpenInput(const Stroka& url) {
    if (!url || url == STRINGBUF("-")) {
        if (isatty(0))
            return new TFileInput(Duplicate(0));
        return new TBufferedFileInput(Duplicate(0));
    } else {
        if (url.has_suffix(".gz")) {
            return new TCompressed<TBufferedFileInput, TZLibHelper>(url);
        }

        if (url.has_suffix(".bz2")) {
            return new TCompressed<TBufferedFileInput, TBZipDecompress>(url);
        }

        return new TBufferedFileInput(url);
    }
}

TAutoPtr<TOutputStream> OpenOutput(const Stroka& url) {
    if (!url || url == STRINGBUF("-")) {
        return new TBufferedFileOutput(Duplicate(1));
    } else {
        return new TBufferedFileOutput(url);
    }
}


TAutoPtr<TInputStream> OpenMaybeCompressedInput(TInputStream* input) {
    const size_t MAX_SIGNATURE_SIZE = 4;
    char buffer[MAX_SIGNATURE_SIZE];

    Stroka header(buffer, input->Load(buffer, MAX_SIGNATURE_SIZE));

    // any lz
    TAutoPtr<TInputStream> lz = TryOpenLzDecompressor(header, input);
    if (lz.Get()) {
        return lz;
    }

    THolder<TInputStream> multi(new TStringMultiInput(header, input));

    // gzip
    const TStringBuf GZIP = STRINGBUF("\x1F\x8B");
    const TStringBuf ZLIB = STRINGBUF("\x78\x9C");
    if (header.has_prefix(GZIP) || header.has_prefix(ZLIB)) {
        return new TInputHolder<TZLibDecompress>(multi.Release());
    }

    // bzip2
    const TStringBuf BZIP2 = STRINGBUF("BZ");
    if (header.has_prefix(BZIP2)) {
        return new TInputHolder<TBZipDecompress>(multi.Release());
    }

    return multi.Release();
}
