#pragma once

#include <util/stream/ios.h>
#include <util/memory/blob.h>
#include <util/generic/ptr.h>

class TArchiveWriter {
    public:
        TArchiveWriter(TOutputStream* out, bool compress = true);
        ~TArchiveWriter() throw ();

        void Flush();
        void Finish();
        void Add(const Stroka& key, TInputStream* src);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};

class TArchiveReader {
    public:
        TArchiveReader(const TBlob& data);
        ~TArchiveReader() throw ();

        size_t Count() const throw ();
        Stroka KeyByIndex(size_t n) const;
        bool Has(const Stroka& key) const;
        TAutoPtr<TInputStream> ObjectByKey(const Stroka& key) const;
        TBlob ObjectBlobByKey(const Stroka& key) const;
        TBlob BlobByKey(const Stroka& key) const;
        bool Compressed() const;

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};
