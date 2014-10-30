#pragma once

#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/generic/map.h>
#include <util/generic/stroka.h>
#include <util/stream/mem.h>
#include <util/stream/length.h>
#include <util/memory/blob.h>
#include <util/ysaveload.h>
#include <util/stream/lz.h>
#include <util/stream/file.h>
#include <util/system/tempfile.h>
#include <util/random/random.h>

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/messagext.h>
#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/repeated_field.h>
#include <contrib/libs/protobuf/io/zero_copy_stream.h>
#include <contrib/libs/protobuf/io/coded_stream.h>

#include "tools.h"

namespace NGzt
{

// Defined in this file:
class TMemoryInputStreamAdaptor;
class TBlobCollection;


typedef NProtoBuf::Message TMessage;


// allows transferring data from TMemoryInput to NProtoBuf::io::ZeroCopyInputStream without intermediate bufferring.
class TMemoryInputStreamAdaptor: public NProtoBuf::io::ZeroCopyInputStream {
public:
    TMemoryInputStreamAdaptor(TMemoryInput* input)
        : Input(input)
        , LastData(NULL)
        , LastSize(0)
        , BackupSize(0)
        , TotalRead(0)
    {
    }

    ~TMemoryInputStreamAdaptor() {
        // return unread memory back to Input
        if (BackupSize > 0)
            Input->Reset(LastData + (LastSize - BackupSize), BackupSize + Input->Avail());
    }

    // implements ZeroCopyInputStream ----------------------------------
    bool Next(const void** data, int* size) {
        if (EXPECT_FALSE(BackupSize > 0)) {
            YASSERT(LastData != NULL && LastSize >= BackupSize);
            size_t rsize = CutSize(BackupSize);
            *data = LastData + (LastSize - BackupSize);
            *size = rsize;
            TotalRead += rsize;
            BackupSize -= rsize;
            return true;

        } else if (Input->Next(&LastData, &LastSize)) {
            size_t rsize = CutSize(LastSize);
            *data = LastData;
            *size = rsize;
            TotalRead += rsize;
            BackupSize = LastSize - rsize;          // in case when LastSize > Max<int>() do auto-backup
            return true;

        } else {
            LastData = NULL;
            LastSize = 0;
            return false;
        }
    }

    void BackUp(int count) {
        YASSERT(count >= 0 && BackupSize + count <= LastSize);
        BackupSize += count;
        TotalRead -= count;
    }

    bool Skip(int count) {
        int skipped = 0, size = 0;
        const void* data = NULL;
        while (skipped < count && Next(&data, &size))
            skipped += size;
        if (skipped < count)
            return false;
        if (skipped > count)
            BackUp(skipped - count);
        return true;
    }

    NProtoBuf::int64 ByteCount() const {
        return TotalRead;
    }

private:
    size_t CutSize(size_t sz) const {
        return Min<size_t>(sz, Max<int>());
    }

private:
    TMemoryInput* Input;
    const char* LastData;
    size_t LastSize, BackupSize;
    i64 TotalRead;
};

} // namespace NGzt


inline void SaveProtectedSize(TOutputStream* output, size_t size) {
    if (EXPECT_FALSE(static_cast<ui64>(size) > static_cast<ui64>(Max<ui32>())))
        ythrow yexception() << "Size " << size << " exceeds ui32 limits.";
    ::Save<ui32>(output, size);
    ::Save<ui32>(output, IntHash<ui32>(size));
}

inline bool LoadProtectedSize(TInputStream* input, size_t& res) {
    ui32 size = 0, hash = 0;
    ::Load<ui32>(input, size);
    ::Load<ui32>(input, hash);
    if (hash == IntHash<ui32>(size)) {
        res = size;
        return true;
    }
    return false;
}

inline size_t LoadProtectedSize(TInputStream* input) {
    size_t size;
    if (!LoadProtectedSize(input, size)) {
        Stroka msg = "Failed to load data: incompatible format or corrupted binary.";
        Cerr << msg << Endl;
        ythrow yexception() << msg;
    }
    return size;
}

// containers protection: to prevent allocating random (if corrupted) big amount of memory
template <typename T>
inline void SaveProtected(TOutputStream* output, const T& data) {
    ::SaveProtectedSize(output, data.size());       // just prepend extra ui64 (instead of redefining ::Save/::Load for all containers)
    ::Save(output, data);
}

template <typename T>
inline void LoadProtected(TInputStream* input, T& data) {
    ::LoadProtectedSize(input);    // fail if a size seems to be corrupted
    ::Load(input, data);
}


namespace NGzt {

class TMessageSerializer: public NProtoBuf::io::TProtoSerializer {
    typedef NProtoBuf::io::TProtoSerializer TBase;
    typedef NProtoBuf::Message TMessage;
public:
    using TBase::Save;
    using TBase::Load;

    // a little bit faster then with TInputStream
    static inline void Load(TMemoryInput* input, TMessage& msg) {
        NGzt::TMemoryInputStreamAdaptor adaptor(input);
        NProtoBuf::io::CodedInputStream decoder(&adaptor);
        if (!TBase::Load(&decoder, msg))
            ythrow yexception() << "Cannot read protobuf Message from input stream";
    }

    // override TBase::Load
    static inline void Load(TInputStream* input, TMessage& msg) {
        if (dynamic_cast<TMemoryInput*>(input) != NULL)
            Load(static_cast<TMemoryInput*>(input), msg);
        else
            TBase::Load(input, msg);
    }

    // skip message without parsing.
    static inline bool Skip(NProtoBuf::io::CodedInputStream* input) {
        ui32 size;
        return input->ReadVarint32(&size) && input->Skip(size);
    }
};






// Collection of binary large objects which are serialized separately from other gazetteer data.
//
// When gazetteer is serialized it transforms its inner structures to special protobuf objects first
// (described in binary.proto) and then uses protobuf serialization to save them in binary format.
// But not all gazetteer data serialized in this way. There are several big objects (BLOBs)
// which are already serialized and which are later on de-serialization just mapped into memory, without copying.
// These, for example, are TArticlePool::ArticleData (all articles as protobuf binary)
// and all inner TCompactTrie of TGztTrie as well.
// All such blobs are stored separately in TBlobCollection and referred from other data
// by corresponding blob key.
class TBlobCollection
{
public:
    class TError: public yexception {};

    inline TBlobCollection() {
    }

    inline void Save(TOutputStream* output) const {
        ::SaveProtectedSize(output, Blobs.size());
        for (ymap<Stroka, TBlob>::const_iterator it = Blobs.begin(); it != Blobs.end(); ++it) {
            ::Save(output, it->first);
            SaveBlob(output, it->second);
        }
    }

    inline void Load(TMemoryInput* input) {
        size_t count = ::LoadProtectedSize(input);
        for (size_t i = 0; i < count; ++i) {
            Stroka key;
            ::Load(input, key);
            Add(key, LoadBlob(input));
        }
    }

    const TBlob& operator[] (const Stroka& blobkey) const {
        ymap<Stroka, TBlob>::const_iterator res = Blobs.find(blobkey);
        if (res == Blobs.end())
            ythrow TError() << "No blob with key \"" << blobkey << "\" found in collection.";
        else
            return res->second;
    }

    inline bool HasBlob(const Stroka& blobkey) const {
        return Blobs.find(blobkey) != Blobs.end();
    }

    void SaveRef(const Stroka& key, const TBlob& blob, Stroka* blobkey) {
        Add(key, blob);
        blobkey->assign(key);
    }

    inline void SaveNoCopy(const Stroka& key, TBuffer& buffer, Stroka* blobkey) {
        // @buffer is cleared after operation
        SaveRef(key, TBlob::FromBuffer(buffer), blobkey);
    }

    inline void SaveCopy(const Stroka& key, const TBuffer& buffer, Stroka* blobkey) {
        SaveRef(key, TBlob::Copy(buffer.Data(), buffer.Size()), blobkey);
    }

    template <typename T>
    inline void SaveCompactTrie(const Stroka& key, const T& trie, Stroka* blobkey) {
        TBufferOutput tmpbuf;
        trie.Save(tmpbuf);
        SaveNoCopy(key, tmpbuf.Buffer(), blobkey);
    }

    template <typename T>
    inline void SaveObject(const Stroka& key, const T& object, Stroka* blobkey) {
        // any arcadia-serializeable object (with ::Save defined)
        TBufferOutput tmpbuf;
        ::Save(&tmpbuf, object);
        SaveNoCopy(key, tmpbuf.Buffer(), blobkey);
    }

    template <typename T>
    inline void LoadObject(T& object, const Stroka& blobkey) const {
        // any arcadia-serializeable object (with ::Load defined)
        const TBlob& blob = (*this)[blobkey];
        TMemoryInput input(blob.Data(), blob.Length());
        ::Load(&input, object);
    }


private:
    static inline void SaveBlob(TOutputStream* output, const TBlob& blob) {
        YASSERT(blob.Size() <= Max<ui32>());
        ::SaveProtectedSize(output, blob.Size());
        ::SaveArray(output, blob.AsCharPtr(), blob.Size());
    }

    static inline TBlob LoadBlob(TMemoryInput* input) {
        size_t blob_len = 0;

        if (::LoadProtectedSize(input, blob_len)) {
            const char* data;
            size_t data_len;
            if (!input->Next(&data, &data_len) || data_len < blob_len)
                ythrow yexception() << "Unexpected end of memory stream.";
            input->Reset(data + blob_len, data_len - blob_len);
            return TBlob::NoCopy(data, blob_len);
        } else
            return TBlob();
    }

    inline void Add(const Stroka& key, const TBlob& blob) {
        if (Blobs.find(key) != Blobs.end())
            ythrow TError() << "Two blobs with same key \"" << key << "\". Keys must be unique.";
        Blobs[key] = blob;
    }

    ymap<Stroka, TBlob> Blobs;

    DECLARE_NOCOPY(TBlobCollection);
};


// TSentinel is used for validation of serialized data.
// A sentinel (special fixed 8-byte sequence) is inserted between blocks of data on serialization
// and verified on de-serialization

class TSentinel
{
private:
    static inline ui64 SentinelValue()
    {
        return 0x0DF0EFBEADDECEFAULL; //1004566319143505658ULL;  //0xFACEDEADBEEFF00D reversed;
    }

public:
    static inline void Set(TOutputStream* output)
    {
        ::Save(output, SentinelValue());
    }

    static inline bool Check(TInputStream* input)
    {
        ui64 n = 0;
        ::Load(input, n);
        return n == SentinelValue();
    }

    class TMissingSentinelError: public yexception
    {
    };

    static inline void Verify(TInputStream* input)
    {
        if (!Check(input))
        {
            Cerr << "Failed to de-serialize data: incompatible format or corrupted binary." << Endl;
            ythrow TMissingSentinelError();
        }
    }
};

template <typename TValue>
struct TRepeatedFieldTypeSelector {
    typedef NProtoBuf::RepeatedField<TValue> TField;

    static inline void AddTo(TField* field, TValue value) {
        field->Add(value);
    }
};

template <>
struct TRepeatedFieldTypeSelector<Stroka> {
    typedef NProtoBuf::RepeatedPtrField<Stroka> TField;

    static inline void AddTo(TField* field, Stroka value) {
        *field->Add() = value;
    }
};


template <typename TItemIterator>
inline void SaveToField(typename TRepeatedFieldTypeSelector<typename TItemIterator::TValue>::TField* field,
                        const TItemIterator& itemIter, size_t itemSize)
{
    field->Reserve(field->size() + itemSize);
    for (; itemIter.Ok(); ++itemIter)
        TRepeatedFieldTypeSelector<typename TItemIterator::TValue>::AddTo(field, *itemIter);
}

template <typename TItemCollection>
inline void SaveToField(typename TRepeatedFieldTypeSelector<typename TItemCollection::value_type>::TField* field,
                        const TItemCollection& items)
{
    field->Reserve(field->size() + items.size());
    for (typename TItemCollection::const_iterator it = items.begin(); it != items.end(); ++it)
        TRepeatedFieldTypeSelector<typename TItemCollection::value_type>::AddTo(field, *it);
}

template <typename TRepeatedField, typename TVector>
inline void LoadVectorFromField(const TRepeatedField& field, TVector& items)
{
    items.reserve(field.size());
    for (int i = 0; i < field.size(); ++i)
        items.push_back(field.Get(i));
}

template <typename TRepeatedField, typename TSet>
inline void LoadSetFromField(const TRepeatedField& field, TSet& items)
{
    for (int i = 0; i < field.size(); ++i)
        items.insert(field.Get(i));
}


template <typename TProtoObject, typename TObject>
inline void SaveAsProto(TOutputStream* output, const TObject& object)
{
    TProtoObject proto;
    TBlobCollection blobs;
    object.Save(proto, blobs);

    TMessageSerializer::Save(output, proto);
    TSentinel::Set(output);

    // blobs
    blobs.Save(output);
    TSentinel::Set(output);
}

template <typename TProtoObject, typename TObject>
inline void LoadAsProto(TMemoryInput* input, TObject& object)
{
    TProtoObject proto;
    TBlobCollection blobs;
    LoadProtoBlobs(input, proto, blobs);
    object.Load(proto, blobs);
}

template <typename TProtoObject>
inline void LoadProtoOnly(TMemoryInput* input, TProtoObject& proto)
{
    TMessageSerializer::Load(input, proto);
    TSentinel::Verify(input);
}

template <typename TProtoObject>
inline void LoadProtoBlobs(TMemoryInput* input, TProtoObject& proto, TBlobCollection& blobs)
{
    LoadProtoOnly(input, proto);
    blobs.Load(input);
    TSentinel::Verify(input);
}



class TTempFileOutput: public TOutputStream {
public:
    TTempFileOutput(const Stroka& fileName)
        : FileName(fileName)
        , TmpFile(fileName + "." + ToString(RandomNumber<ui64>()))
        , Slave(TmpFile.Name())
    {
    }

    void Commit() {
        Finish();
        if (NFs::Rename(~TmpFile.Name(), ~FileName) != 0)
            ythrow yexception() << "cannot rename temp file";
    }

protected:
    virtual void DoWrite(const void* buf, size_t len) {
        Slave.Write(buf, len);
    }

    virtual void DoFlush() {
        Slave.Flush();
    }

    virtual void DoFinish() {
        Slave.Finish();
    }

private:
    Stroka FileName;
    TTempFile TmpFile;
    TOFStream Slave;
};


} // namespace NGzt
