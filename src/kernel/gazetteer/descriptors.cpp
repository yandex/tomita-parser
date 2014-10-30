#include "descriptors.h"

#include <kernel/gazetteer/base.pb.h>
#include <kernel/gazetteer/binary.pb.h>

#include <contrib/libs/protobuf/dynamic_message.h>

namespace NGzt {

class TFileDescriptorCollection::TPrototyper {
public:
    TPrototyper() {
        Factory.SetDelegateToGeneratedFactory(true);
    }

    const TMessage* SelectPrototype(const TDescriptor* generated, const TDescriptor* runtime) const {
        return Factory.GetPrototype(generated ? generated : runtime);
/*
        // if descriptor is generated message-type, try returning generated prototype for perfomance.
        if (generated)
            return NProtoBuf::MessageFactory::generated_factory()->GetPrototype(generated);
        else
            // DynamicMessageFactory::GetPrototype() is thread-safe, so no extra locking
            return Factory.GetPrototype(runtime);
*/
    }

private:
    mutable NProtoBuf::DynamicMessageFactory Factory;
};


TFileDescriptorCollection::TFileDescriptorCollection(TErrorCollector* errors)
    : Errors(errors)
    , Database()
    , Pool_(&Database, Errors)
    , Prototyper(new TPrototyper)
{
}

TFileDescriptorCollection::~TFileDescriptorCollection() {
}

const TMessage* TFileDescriptorCollection::SelectPrototype(const TDescriptor* generated, const TDescriptor* runtime) const {
    return Prototyper->SelectPrototype(generated, runtime);
}

bool TFileDescriptorCollection::Compile() {
    for (size_t i = 0; i < Files.size(); ++i) {
        // this will transform added file protos into ready file descriptors
        const TFileDescriptor* file = Pool_.FindFileByName(Files[i]->name());
        if (file == NULL) {
            if (Errors)
                Errors->AddError(-1, 0, "Cannot find protobuf file descriptor: " + Files[i]->name());
            return false;
        }
    }
    return true;
}

void TFileDescriptorCollection::Save(NBinaryFormat::TProtoPool& proto) const {
    // save patched file-protos as vector of TFileDescriptorProto objects - preserving order of files
    for (size_t i = 0; i < Files.size(); ++i)
        *proto.AddFile() = Files[i]->SerializeAsString();
}

void TFileDescriptorCollection::Load(const NBinaryFormat::TProtoPool& proto) {
    Files.clear();
    FileIndex.clear();

    for (size_t i = 0; i < proto.FileSize(); ++i) {
        THolder<TFileDescriptorProto> fileProto(new TFileDescriptorProto());
        if (!fileProto->ParseFromString(proto.GetFile(i)))
            ythrow yexception() << "Cannot parse serialized descriptors.";
        if (!fileProto->IsInitialized())
            ythrow yexception() << "Deserialized descriptor is not complete.";
        if (!AddFile(fileProto.Release()))
            ythrow yexception() << "Deserialized descriptor is non-consistent with previously read descriptors.";
    }
    if (!Compile())
        ythrow yexception() << "Error on gazetteer protopool deserialization.";
}

void THierarchy::Save(NBinaryFormat::TProtoPool& proto) const {
    SaveToField(proto.MutableBaseIndex(), BaseIndex);
}

void THierarchy::Load(const NBinaryFormat::TProtoPool& proto) {
    LoadVectorFromField(proto.GetBaseIndex(), BaseIndex);
}

size_t THierarchy::GetParentCount(ui32 index) const {
    size_t count = 0;
    while (BaseIndex[index] != index) {
        count += 1;
        index = BaseIndex[index];
    }
    return count;
}

ui32 THierarchy::GetNthParent(ui32 index, size_t n) const {
    for (; n > 0; --n)
        index = BaseIndex[index];
    return index;
}

ERelation THierarchy::GetRelation(size_t i1, size_t i2) const {

    if (i1 == i2)
        return SAME;

    size_t depth1 = GetParentCount(i1);
    size_t depth2 = GetParentCount(i2);

    if (depth1 > depth2) {
        i1 = GetNthParent(i1, depth1 - depth2);
        if (i1 == i2)
            return CHILD_PARENT;
        depth1 = depth2;

    } else if (depth2 > depth1) {
        i2 = GetNthParent(i2, depth2 - depth1);
        if (i1 == i2)
            return PARENT_CHILD;
    }

    for (; depth1 > 0; --depth1) {
        i1 = BaseIndex[i1];
        i2 = BaseIndex[i2];
        if (i1 == i2)
            return COMMON_PARENT;
    }

    return UNRELATED;
}


/*
static bool IsExactlySameDescriptors(const TDescriptor* d1, const TDescriptor* d2,
                                     TDescriptorProto& tmpProto1, TDescriptorProto& tmpProto2,
                                     Stroka& tmpBuffer1, Stroka& tmpBuffer2) {
    if (d1 == d2)
        return true;

    // TODO: use more robust method of equality check

    tmpProto1.Clear();
    tmpProto2.Clear();
    d1->CopyTo(&tmpProto1);
    d2->CopyTo(&tmpProto2);

    int size = tmpProto1.ByteSize();
    if (size != tmpProto2.ByteSize())
        return false;

    tmpBuffer1.ReserveAndResize(size);
    tmpBuffer2.ReserveAndResize(size);
    tmpProto1.SerializeWithCachedSizesToArray(reinterpret_cast<ui8*>(tmpBuffer1.begin()));
    tmpProto2.SerializeWithCachedSizesToArray(reinterpret_cast<ui8*>(tmpBuffer2.begin()));
    return tmpBuffer1 == tmpBuffer2;
}

static bool IsExactlySameDescriptors(const TDescriptor* d1, const TDescriptor* d2) {
    TDescriptorProto proto1, proto2;
    Stroka buffer1, buffer2;
    return IsExactlySameDescriptors(d1, d2, proto1, proto2, buffer1, buffer2);
}

static void RaiseInconsistentDescriptorsError(const TDescriptor* descr) {
    ythrow yexception() << "Type \"" << descr->full_name() << "\" loaded from gazetteer binary differs from"
        << " the corresponding built-in type. This means that file " << descr->file()->name() << " has been modified,"
        << " but either your gazetteer binary (.gzt.bin) or the program itself has not been updated since and"
        << " thus one contains an obsolete version of proto definitions from this file."
        << " You should either fully re-compile your .gzt.bin or re-build the program in order to have them contain "
        << " same version of " << descr->full_name() << ".";
}
*/

static const TDescriptor* FindEquivalentGeneratedDescriptor(const TDescriptor* descriptor) {
    if (FromGeneratedPool(descriptor))
        return descriptor;

    // Look in generated pool by full name and check if generated descriptor is exactly the same type.
    const TDescriptor* generated = GeneratedPool()->FindMessageTypeByName(descriptor->full_name());

    /*
    if (generated == NULL || IsExactlySameDescriptors(descriptor, generated))
        return generated;

    // There could be a problem here: if ProtoPool and generated pool contain distinct versions of same descriptor
    // For now, complain only for types from base.proto

    // 2011-09-20, mowgli: do not complain at all and see if this works nice
    if (TProtoParser::IsBuiltinFile(descriptor->file()->name()))
        RaiseInconsistentDescriptorsError(descriptor);
    */

    // for other protos just return generated descriptor
    // and hope it is compatible with the one loaded from gzt.bin.
    // TODO: make compatibility check here (?)

    return generated;
}


static const TFieldDescriptor* FindFirstSearchKey(const TDescriptor* descriptor) {
    for (int f = 0; f < descriptor->field_count(); ++f) {
        const TFieldDescriptor* field = descriptor->field(f);
        if (field->cpp_type() == TFieldDescriptor::CPPTYPE_MESSAGE &&
            FindEquivalentGeneratedDescriptor(field->message_type()) == TSearchKey::descriptor())

            return field; // use only first field
    }
    return NULL;
}

TDescriptorIndex::TInfo::TInfo(const TDescriptor* runtime)
    : Runtime(runtime)
    , Generated(FindEquivalentGeneratedDescriptor(Runtime))
    , Best(Generated != NULL ? Generated : Runtime)
    , SearchKey(FindFirstSearchKey(Best))
    , Name(Wtroka::FromAscii(runtime->name()))
    , FullName(Wtroka::FromAscii(runtime->full_name()))
{
}

void TDescriptorIndex::Add(const TDescriptor* descr) {
    YASSERT(descr != NULL);
    if (Index.find(descr) == Index.end()) {
        TInfo info(descr);
        Index[info.Runtime] = Descriptors.size();
        if (info.Generated != NULL)
            Index[info.Generated] = Descriptors.size();
        Descriptors.push_back(info);
    }
}

bool TDescriptorIndex::Reset(const NProtoBuf::DescriptorPool& pool, const yvector<Stroka>& fullNames,
                             TErrorCollector* errors) {
    Clear();
    Pool = &pool;
    for (size_t i = 0; i < fullNames.size(); ++i) {
        const TDescriptor* descr = pool.FindMessageTypeByName(fullNames[i]);
        if (descr != NULL)
            Add(descr);

        else {
            const Stroka errorText = "Cannot find protobuf descriptor: " + fullNames[i];
            if (errors != NULL)
                errors->AddError(-1, 0, errorText);
            else
                ythrow yexception() << errorText;
            return false;
        }
    }
    return true;
}

const TFieldDescriptor* TDescriptorIndex::GetSearchKeyField(const TDescriptor* descriptor) const {
    ui32 index = 0;
    if (GetIndex(descriptor, index))
        return Descriptors[index].SearchKey;
    else
        return FindFirstSearchKey(descriptor);
}



bool TDescriptorCollectionBuilder::AddMessage(TDescriptorProto* msgProto, const Stroka& fullName) {
    size_t idx = Messages.size();
    MessageIndex[fullName] = idx;
    Messages.push_back(msgProto);
    FullNames.push_back(fullName);

    ui32 baseIdx = idx;             // default: no base class
    if (msgProto->options().HasExtension(GztProtoDerivedFrom)) {
        const Stroka& base = msgProto->options().GetExtension(GztProtoDerivedFrom);
        yhash<Stroka, ui32>::const_iterator it = MessageIndex.find(base);
        if (it == MessageIndex.end()) {
            AddError(fullName + ": cannot find base message descriptor " + base);
            return false;
        } else
            baseIdx = it->second;
    }
    Hierarchy.SetBase(idx, baseIdx);
    return true;
}

static void CollectMessageProtos(const TDescriptor* msg, TDescriptorProto* msgProto, yhash<Stroka, TDescriptorProto*>& ret) {
    VERIFY(msg->name() == msgProto->name(), "Distinct names of message and messageProto: %s vs %s", ~msg->name(), ~msgProto->name());
    ret[msg->full_name()] = msgProto;

    YASSERT(msg->nested_type_count() == msgProto->nested_type_size());
    for (int i = 0; i < msg->nested_type_count(); ++i)
        CollectMessageProtos(msg->nested_type(i), msgProto->mutable_nested_type(i), ret);
}

bool TDescriptorCollectionBuilder::ImportPool(const TDescriptorCollection& pool) {
    // mapping: full_name of descriptor -> imported descriptor proto
    yhash<Stroka, TDescriptorProto*> newDescriptors;

    for (size_t i = 0; i < pool.Files.Size(); ++i) {
        const Stroka& fileName = pool.Files[i]->name();
        if (!HasFile(fileName)) {
            const TFileDescriptor* file = pool.Files.Pool().FindFileByName(fileName);
            if (!file) {
                AddError("Cannot find file descriptor in imported pool for \"" + fileName + "\"");
                return false;
            }

            THolder<TFileDescriptorProto> fileProto(new TFileDescriptorProto(*pool.Files[i]));
            for (int j = 0; j < fileProto->message_type_size(); ++j)
                CollectMessageProtos(file->message_type(j), fileProto->mutable_message_type(j), newDescriptors);

            if (!AddFile(fileProto.Release()))
                return false;
        }
    }

    for (size_t i = 0; i < pool.Size(); ++i) {
        const TDescriptor* desc = pool[i];
        if (!FindMessageProto(desc->full_name())) {
            yhash<Stroka, TDescriptorProto*>::const_iterator it = newDescriptors.find(desc->full_name());
            if (it == newDescriptors.end()) {
                AddError("Cannot import type \"" +  desc->full_name() + "\" from supplied proto-pool");
                return false;
            }

            if (!AddMessage(it->second, desc->full_name()))
                return false;
        }
    }

    return true;
}

bool TDescriptorCollectionBuilder::Compile() {
    if (!Files.Compile())
        return false;
    return TDescriptorIndex::Reset(Files.Pool(), FullNames, Errors);
}

void TDescriptorCollectionBuilder::Save(NBinaryFormat::TProtoPool& proto) const {
    Files.Save(proto);
    Hierarchy.Save(proto);
    // message-types full names in order of parsing
    SaveToField(proto.MutableDescriptorName(), FullNames);
}


void TDescriptorCollection::Load(const NBinaryFormat::TProtoPool& proto) {
    Files.Load(proto);
    Hierarchy.Load(proto);

    yvector<Stroka> descriptorNames;
    LoadVectorFromField(proto.GetDescriptorName(), descriptorNames);
    Hierarchy.GrowUpto(descriptorNames.size());
    TDescriptorIndex::Reset(Files.Pool(), descriptorNames, NULL);
}

}   // namespace NGzt
