#include <contrib/libs/protobuf/stubs/substitute.h>

#include "protopool.h"
#include "sourcetree.h"

#include <kernel/gazetteer/base.pb.h>
#include <kernel/gazetteer/binary.pb.h>

namespace NGzt
{

using NProtoBuf::strings::Substitute;

// this nasty windows.h macro... interfers with NProtoBuf::Reflection::GetMessage()
#if defined(GetMessage)
#undef GetMessage
#endif


// TProtoPool ===========================================

TProtoPool::TProtoPool(TErrorCollector* error_collector)
    : TDescriptorCollection(error_collector)
{
}

void TProtoPool::Load(TMemoryInput* input)
{
    NBinaryFormat::TProtoPool proto;
    TMessageSerializer::Load(input, proto);
    TDescriptorCollection::Load(proto);
}

const TDescriptor* TProtoPool::RequireMessageType(const Stroka& name) const {
    const TDescriptor* res = FindMessageTypeByName(name);
    if (res == NULL)
        ythrow yexception() << "Message type \"" << name << "\" not found in pool.";
    return res;
}

Stroka TProtoPool::DebugString() const
{
    TStringStream res;
    for (size_t i = 0; i < Size(); ++i)
        res << GetDescriptor(i)->DebugString() << "\n\n";
    return res;
}


// TProtoPoolBuilder ===========================

TProtoPoolBuilder::TProtoPoolBuilder(TGztSourceTree* source_tree)
    : Errors()
    , Impl(&Errors)
    , SourceTree(source_tree)
    , ArcadiaBuildMode(false)
{
}

TProtoPoolBuilder::~TProtoPoolBuilder() {
}

inline void TProtoPoolBuilder::AddError(const TDescriptor* descriptor, const Stroka& error) {
    Errors.AddError(descriptor->file()->name(), -1, 0, error);
}

Stroka TProtoPoolBuilder::CanonicFileName(const Stroka& virtualFileName) const {
    const TFileDescriptor* builtin = SourceTree->BuiltinFiles().FindFileDescriptor(virtualFileName);
    Stroka name(builtin != NULL ? builtin->name() : SourceTree->CanonicName(virtualFileName));

    // replace all .gztproto imports with .proto for arcadia build mode:
    if (ArcadiaBuildMode && name.has_suffix(STRINGBUF(".gztproto")))
        name.remove(name.size() - 8, 3);

    return name;
}

// Replace file->name() and all imports names with corresponding canonical from SourceTree
void TProtoPoolBuilder::PatchFileNames(TFileDescriptorProto* file) const {
    file->set_name(CanonicFileName(file->name()));
    for (int i = 0; i < file->dependency_size(); ++i)
        file->set_dependency(i, CanonicFileName(file->dependency(i)));
}

static inline bool HasDerivedFromOption(const TDescriptorProto& d) {
    if (d.options().HasExtension(GztProtoDerivedFrom))
        return true;
    // also check all nested sub-types
    for (int i = 0; i < d.nested_type_size(); ++i)
        if (HasDerivedFromOption(d.nested_type(i)))
            return true;

    return false;
}

static inline bool HasDerivedFromOption(const TFileDescriptorProto* f) {
    for (int i = 0; i < f->message_type_size(); ++i)
        if (HasDerivedFromOption(f->message_type(i)))
            return true;
    return false;
}

bool TProtoPoolBuilder::ImportBaseProtoIfNecessary(TFileDescriptorProto* file) {
    if (!HasDerivedFromOption(file))
        return false;

    if (file->name() == NBuiltin::BaseProtoName())
        return false;     // avoid cycles
    for (int i = 0; i < file->dependency_size(); ++i)
        if (file->dependency(i) == NBuiltin::BaseProtoName())
            return false;

    file->add_dependency(NBuiltin::BaseProtoName());

    // make sure this implicily imported dependency will be built
    return BuildFile(NBuiltin::BaseProtoFile().FindInGeneratedPool());
}

bool TProtoPoolBuilder::LoadVirtualFile(const Stroka& filename)
{
    if (HasFile(filename))
        return true;    // file has been already fully processed

    Parser.Reset(new TProtoParser(SourceTree, &Errors, SourceTree->BuiltinFiles()));

    // initially build raw Descriptors - i.e. without inherited fields, exactly as defined in proto-files.
    if (!Parser->LoadRawProto(filename))
        return false;

    for (size_t i = 0; i < Parser->RawFileDescriptors().size(); ++i)
        if (!BuildFile(Parser->RawFileDescriptors()[i]))
            return false;

    return Compile();
}

bool TProtoPoolBuilder::GenerateCompatibleProto(const Stroka& src_proto_file, const Stroka& dst_proto_file)
{
    const TFileDescriptor* file_descr = CompileToFileDescriptor(src_proto_file);
    if (file_descr == NULL)
        return false;

    Stroka patched_proto = file_descr->DebugString();
    TOFStream f(dst_proto_file);
    f.Write(~patched_proto, +patched_proto);
    return true;
}

const TFileDescriptor* TProtoPoolBuilder::CompileToFileDescriptor(const Stroka& srcProtoFile) {
/*
    Stroka diskFile;
    if (!SourceTree->VirtualFileToDiskFile(virtualFile, diskFile)) {
        Errors.AddError(virtualFile, -1, 0, "Cannot find file in source tree");
        return NULL;
    }
*/
    // we do not set @srcProtoFile as a root (only as a current file) in order to make all imports
    // in generated proto relative to previously set source tree root (usually it should be an arcadia root)

    TDiskSourceTree::TCurrentFile src(srcProtoFile, *SourceTree);
    if (!LoadVirtualFile(src.Virtual()))
        return NULL;

    return Impl.Pool().FindFileByName(CanonicFileName(src.Virtual()));
}

template <typename TDescrProto, typename TDescr>
static inline TAutoPtr<TDescrProto> MakeDescriptorProto(const TDescr* descr) {
    TAutoPtr<TDescrProto> ret(new TDescrProto);
    descr->CopyTo(ret.Get());
    return ret;
}

bool TProtoPoolBuilder::BuildFile(const TFileDescriptor* file_descr)
{
    if (HasFile(file_descr->name()))
        return true;    //file_descr has been already fully processed

    // first build all dependencies
    for (int i = 0; i < file_descr->dependency_count(); ++i) {
        if (!BuildFile(file_descr->dependency(i)))
            return false;
    }

    THolder<TFileDescriptorProto> new_file_proto(MakeDescriptorProto<TFileDescriptorProto>(file_descr));
    PatchFileNames(new_file_proto.Get());


    // check all messages of this file and insert inherited fields into them.
    for (int i = 0; i < new_file_proto->message_type_size(); ++i) {
        // copied message proto which is raw, i.e. without inherited fields.
        TDescriptorProto* orig_msg_proto = new_file_proto->mutable_message_type(i);
        const TDescriptor* msg_descr = file_descr->FindMessageTypeByName(orig_msg_proto->name());
        YASSERT(msg_descr != NULL);

        // build "patched" message-proto - with inherited fields
        THolder<TDescriptorProto> final_msg_proto(BuildPatchedMessageProto(msg_descr));
        if (!final_msg_proto)
            return false;

        // replace @org_msg_proto at new_file_proto->message_type() with @final_msg_proto;
        // this is somewhat low-level operation for protobuf, so we should be very careful here.
        new_file_proto->mutable_message_type()->mutable_data()[i] = final_msg_proto.Release();
        delete orig_msg_proto;
    }

    ImportBaseProtoIfNecessary(new_file_proto.Get());

    //now as new_file_proto is complete we can safely put it in FinalPool.Database and DescriptorCollection
    return Impl.AddFile(new_file_proto.Release());
}

bool TProtoPoolBuilder::AddPool(const TProtoPool& pool)
{
    if (!Impl.ImportPool(pool))
        return false;

    return Compile();
}

TDescriptorProto* TProtoPoolBuilder::BuildPatchedMessageProto(const TDescriptor* message_descr)
{
    TDescriptorProto* ret = Impl.FindMessageProto(message_descr->full_name());
    if (ret != NULL)
        return ret;

    // This message is still not built, do it now.
    // During this process an error could occur, in this case a newly @message_proto will be destroyed
    // and not placed into MessageProtos, NULL returned.
    THolder<TDescriptorProto> message_proto(MakeDescriptorProto<TDescriptorProto>(message_descr));
    if (BuildPatchedMessageProtoImpl(message_descr, message_proto.Get()))
        return message_proto.Release();

    return NULL;
}

bool TProtoPoolBuilder::BuildPatchedMessageProtoImpl(const TDescriptor* message_descr, TDescriptorProto* message_proto)
{
    // Check out base descriptor. There are two possible cases:
    // 1) @message_descr was taken from builtin (generated_pool). In that case it's base type name is stored in its
    //    GztProtoDerivedFrom option and it should already have all inherited content within it.
    // 2) @message_descr was read from disk proto (by TProtoParser) and its base are collect in parser inheritance graph.
    //    In that case it does not have inherited fields yet and they should be explicitly copied.

    const TDescriptor* base_message_descr = Parser->GetBaseDescriptor(message_descr);
    if (base_message_descr != NULL) {
        // if the base message if from other file, this file should be already built.

        const TDescriptorProto* base_message_proto = BuildPatchedMessageProto(base_message_descr);
        if (base_message_proto == NULL)
            return false;

        // copy fields from @base_message_proto to @message_proto, only for explicit base (read from file)
        if (!message_descr->options().HasExtension(GztProtoDerivedFrom))
            if (!CopyInheritedContent(message_proto, base_message_proto,
                                      message_descr, base_message_descr))
                return false;
    }

    for (int i = 0; i < message_proto->nested_type_size(); ++i)
        if (!BuildPatchedMessageProtoImpl(
            message_descr->nested_type(i), message_proto->mutable_nested_type(i)))
            return false;

    // now as message-proto is built we can safely put it into built-descriptors collections.
    return Impl.AddMessage(message_proto, message_descr->full_name());
}

bool TProtoPoolBuilder::CopyInheritedContent(TDescriptorProto* derived_message_proto,
                                             const TDescriptorProto* base_message_proto,
                                             const TDescriptor* derived_message_descr,
                                             const TDescriptor* base_message_descr)
{
    // we should assert that all derived fields numbers are allowed by base message-type (i.e. belongs to its extensions range)
    for (int i = 0; i < derived_message_descr->field_count(); ++i)
    {
        const int curidx = derived_message_descr->field(i)->number();
        if (!base_message_descr->IsExtensionNumber(curidx)) {
            AddError(derived_message_descr,
                Substitute("Field number $0 of message \"$1\" is outside of its base message \"$2\" extensions range.",
                           curidx, derived_message_descr->full_name(), base_message_descr->full_name()));
            return false;
        }
    }

    for (int i = 0; i < base_message_proto->field_size(); ++i)
    {
        const NProtoBuf::FieldDescriptorProto& base_field = base_message_proto->field(i);

        //we should also assert that all resulted message-type field numbers are unique, not clashing with each other.
        const TFieldDescriptor* clashed_field = derived_message_descr->FindFieldByNumber(base_field.number());
        if (clashed_field != NULL) {
            AddError(derived_message_descr,
                Substitute("Field number $0 has already been used in \"$1\" by derived field \"$2\".",
                           clashed_field->number(), derived_message_descr->full_name(), base_field.name()));
            return false;
        }
    }

    // now finally insert inherited fields from base at the beginning of derived
    // an order is important, as it defines positional field-values assignment.


    int orig_field_size = derived_message_proto->field_size();
    int new_field_size = base_message_proto->field_size();

    // first, append required number of new fields to the end of @derived_message_proto
    for (int i = 0; i < new_field_size; ++i)
        derived_message_proto->add_field();

    // then move existing fields to the end of field list
    for (int i = orig_field_size - 1; i >= 0; --i)
        derived_message_proto->mutable_field()->SwapElements(i, i + new_field_size);

    // and copy new fields - now they are in the beginning of the field list
    for (int i = 0; i < new_field_size; ++i)
        derived_message_proto->mutable_field(i)->CopyFrom(base_message_proto->field(i));


    // set MessageOptions (in order to identify base type later)
    derived_message_proto->mutable_options()->SetExtension(GztProtoDerivedFrom, base_message_descr->full_name());

    return true;
}

void TProtoPoolBuilder::Save(NBinaryFormat::TProtoPool& proto, TBlobCollection& /*blobs*/) const {
    Impl.Save(proto);
}

void TProtoPoolBuilder::Save(TOutputStream* output) const {
    SaveAsProto<NBinaryFormat::TProtoPool>(output, *this);
}

void TProtoPoolBuilder::AddLoadedFiles(yset<Stroka>& files) const {
    Impl.ExportFiles(files);
}

bool TProtoPoolBuilder::Compile() {
    return Impl.Compile();
}

const TFieldDescriptor* TProtoPoolBuilder::FindRuntimeTRefId() const {
    // Retrieve TRef::id field-descriptor for runtime-built TRef (sic! not generated TRef)

    const TDescriptor* runtimeTRef = Impl.Pool().FindMessageTypeByName(TRef::descriptor()->full_name());
    if (runtimeTRef == NULL || !Impl.IsSameAsGeneratedType(runtimeTRef, TRef::descriptor()))
        return NULL;

    return runtimeTRef->FindFieldByNumber(TRef::kIdFieldNumber);
}

TSearchKeyIterator::TSearchKeyIterator(const TMessage& article, const TProtoPool* pool,
                                       TSearchKey& tmpKey, const TSearchKey* defaults)
    : Article(&article)
    , CurrentPtr(NULL)
    , TmpKey(tmpKey)
    , DefaultKey(defaults)
    , KeyCount(0)
{
    KeyField = pool->GetSearchKeyField(Article->GetDescriptor());
    operator++();
}

TSearchKeyIterator::TSearchKeyIterator(const NProtoBuf::Message& article, const TFieldDescriptor* keyField,
                                       TSearchKey& tmpKey, const TSearchKey* defaults)
    : Article(&article)
    , KeyField(keyField)
    , CurrentPtr(NULL)
    , TmpKey(tmpKey)
    , DefaultKey(defaults)
    , KeyCount(0)
{
    operator++();
}

TSearchKeyIterator::~TSearchKeyIterator() {
    // destructor here in order to avoid "deletion of pointer to incomplete type 'NGzt::TSearchKey'" problem (with TmpKey)
}

#define MERGE_KEY_REPEATED_FIELD(field) \
    if (!dst.field##_size() && src.field##_size()) \
        dst.mutable_##field()->MergeFrom(src.field());

#define MERGE_KEY_OPTIONAL_FIELD(field) \
    if (!dst.has_##field() && src.has_##field()) \
        dst.mutable_##field()->MergeFrom(src.field());

static inline void MergeSearchKeys(const TSearchKey& src, TSearchKey& dst) {
    MERGE_KEY_REPEATED_FIELD(morph);
    MERGE_KEY_REPEATED_FIELD(gram);
    MERGE_KEY_REPEATED_FIELD(case_);
    MERGE_KEY_REPEATED_FIELD(agr);
    MERGE_KEY_REPEATED_FIELD(lang);
    MERGE_KEY_OPTIONAL_FIELD(tokenize);
}

#undef MERGE_KEY_OPTIONAL_FIELD
#undef MERGE_KEY_REPEATED_FIELD

void TSearchKeyIterator::operator++() {
    CurrentPtr = NULL;
    if (KeyField != NULL) {
        const TMessage* value = ExtractKey();
        if (value) {
            ++KeyCount;
            CurrentPtr = CastProto(value, &TmpKey);
            if (DefaultKey != NULL) {
                // cannot merge DefaultKey into const @value, copy explicitly into TmpKey
                if (CurrentPtr != &TmpKey) {
                    TmpKey.CopyFrom(*value);
                    CurrentPtr = &TmpKey;
                }
                MergeSearchKeys(*DefaultKey, TmpKey);
            }
        }
    }
}

inline const NProtoBuf::Message* TSearchKeyIterator::ExtractKey() const {
    const NProtoBuf::Reflection* ref = Article->GetReflection();
    if (KeyField->is_repeated()) {
        if (KeyCount < ref->FieldSize(*Article, KeyField))
            return &(ref->GetRepeatedMessage(*Article, KeyField, KeyCount));
    } else if (KeyCount == 0)
        return &(ref->GetMessage(*Article, KeyField));

    return NULL;
}

} // namespace NGzt
