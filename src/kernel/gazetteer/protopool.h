#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/stream/input.h>

#include <contrib/libs/protobuf/message.h>
#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/descriptor_database.h>
#include <contrib/libs/protobuf/compiler/importer.h>

#include "common/serialize.h"
#include "protoparser.h"

#include "descriptors.h"

// forward declarations from base.proto
class TSearchKey;


namespace NGzt {

// Defined in this file
class TProtoPool;
class TProtoPoolBuilder;
class TSearchKeyIterator;


// forward
class TGztSourceTree;


class TProtoPool: public TRefCounted<TProtoPool, TAtomicCounter>
                , public TDescriptorCollection {
    // Ref-counting is required for ProtoPool to outlive all TArticlePtr created with its descriptors
    // Specifically, if the MessageFactory is destroyed before dynamic message (which is in most of TArticlePtr),
    // then this message's destructor will fail.
public:
    TProtoPool(TErrorCollector* error_collector = NULL);

    void Load(TMemoryInput* input);
    using TDescriptorCollection::Load;


    // Indexing of descriptors ------------------------------

    using TDescriptorIndex::Size;               // Number of message-type descriptors
    using TDescriptorIndex::operator[];         // Descriptor by its index
    using TDescriptorIndex::GetIndex;           // Reverse: an index by descriptor pointer, use for serialization mainly

    // Descriptor names -------------------------------------

    using TDescriptorIndex::FindMessageTypeByName;

    // same as FindMessageTypeByName() but fails if @name is not in the pool
    const TDescriptor* RequireMessageType(const Stroka& name) const;

    // Decoded name/full_name, for mixing with article titles
    using TDescriptorIndex::GetDescriptorName;
    using TDescriptorIndex::GetDescriptorFullName;


    // Descriptors relationship --------------------------------

    using TDescriptorCollection::GetBase;            // Pointer to base-type descriptor if any, or NULL
    using TDescriptorCollection::GetBaseIndex;       // Index of base type, if no base type, @index returned
    using TDescriptorCollection::GetRelation;   // Relationship between two type descriptors.

    // Returns true if @type is sub-type of @parent_type or if these are same types (i.e. <= )
    inline bool IsSubType(const TDescriptor* type, const TDescriptor* parent_type) const {
        ERelation rel = GetRelation(type, parent_type);
        return rel == CHILD_PARENT || rel == SAME;
    }

    // Test for descriptor equality. This also works for comparing generated (builtin) and runtime (loaded from binary) descriptors.
    // Alternatively, see IsSameAsBuiltinType() method
    inline bool IsSameType(const TDescriptor* type1, const TDescriptor* type2) const {
        return GetRelation(type1, type2) == SAME;
    }


    // Generated (i.e. known at compile-time) types stuff  ---------------------

    using TDescriptorIndex::IsSameAsGeneratedType;
    using TDescriptorIndex::IsSameAsGeneratedTypeField;


    // Custom generated/runtime message stuff -------------------------------------

    using TDescriptorCollection::GetPrototype;  // Same as MessageFactory::GetPrototype(), but tries to use generated factory if possible.
    using TDescriptorIndex::GetSearchKeyField;  // Return first field-descriptor of type TSearchKey for specified @articleType

    // Prints all descriptors in human-readable form (used for dumping, for example).
    Stroka DebugString() const;
};


// TProtoPoolBuilder (with TProtoParser) implements extended Protocol Buffers syntax: with possibility to derive a message
// from some other message.
// The new syntax is following:
//     message TBase {
//         optional string A = 1;
//         extensions 10 to max;
//     }
//
//     message TDerived: TBase {      /* here ": TBase" corresponds to extended syntax, non-compliant with public proto2-syntax */
//         optional int32 B = 10;
//     }
//
// Derived message-type will have all base message-type fields literally included into its body
//   as if they were excplicitly specified in derived message definition by proto-file author.
// Considering given example, TDerived will be equivalent to following proto:
//     message TDerived: TBase {
//         optional string A = 1;   /* inherited field - simply inserted here */
//         optional int32 B = 10;
//     }
//
// Derived message could define its own fields, but their numbers should belong to extension range specified by parent message-type.
// Extension range syntax is used only as a ready tool of safe message extension (to make any message fields numbers unique).
// So the changes to public syntax and overall ideology are minimal.
//
// The binary formats is not modified as well as C++ code generation.
// Binary representations of two relative messages are mostly compliant. Base-type messages could be always deserialized from
// derived-type messages binaries (with derived field values becoming UnknownFields). Reverse is also possible except one case:
// - when derived message defines its own "required" fields - it won't probably find these fields
// trying to deserialize from base-message binary.
//
// TProtoPoolBuilder is able to construct DecriptorPool from extended-syntax *.proto file with descriptors containing
// "patched" versions of derived message (with inherited fields), as well as remember Descriptors inheritance graph.
class TProtoPoolBuilder {
public:
    TProtoPoolBuilder(TGztSourceTree* source_tree);
    ~TProtoPoolBuilder();

    // Turn on arcadia compatibility mode (see comments on ArcadiaBuildMode below for details)
    void SetArcadiaBuildMode() {
        ArcadiaBuildMode = true;
    }

    // Parses and builds specified .proto file with user-defined article types.
    // @filename must be a virtual name relative to SourceTree.
    // Returns false on error.
    bool LoadVirtualFile(const Stroka& filename);

    // Inserts new FileProto built from @file_descr into BuiltFileProtos
    // and FinalPool with all inherited fields.
    // Do nothing if corresponding FileProto is already built.
    // Recursively builds all files, containing base message-types.
    // Returns false on error and records it to Errors.
    bool BuildFile(const TFileDescriptor* file_descr);


    // Add all descriptors from ready pool into @this FinalPool collection (in same order).
    bool AddPool(const TProtoPool& pool);

    const TDescriptorIndex& Index() const {
        return Impl;
    }

    // Loads @src_proto_file and saves its patched version to @dst_proto_file.
    // The resulting proto is compatible with standard protobuf compiler - protoc.
    // Used in gztcompiler when option "--proto" is specified.
    bool GenerateCompatibleProto(const Stroka& src_proto_file, const Stroka& dst_proto_file);

    // Compile and returns corresponding fully built file descriptor.
    // @srcProtoFile should be full disk file name, returned descriptor file name
    // will be relative to source tree root. NULL on error.
    const TFileDescriptor* CompileToFileDescriptor(const Stroka& srcProtoFile);

    // Serialize data required later to restore TProtoPool.
    // Corresponding de-serialization method is TProtoPool::Load().
    void Save(TOutputStream* output) const;
    void Save(NBinaryFormat::TProtoPool& proto, TBlobCollection& blobs) const;

    // Append all loaded files to @files
    void AddLoadedFiles(yset<Stroka>& files) const;

    const TFieldDescriptor* FindRuntimeTRefId() const;

    const TMessage* GetPrototype(const TDescriptor* descriptor) const {
        return Impl.GetPrototype(descriptor);
    }

private:
    Stroka CanonicFileName(const Stroka& virtualFileName) const;

    // Replace file->name() and all imports names with corresponding canonical from SourceTree
    void PatchFileNames(TFileDescriptorProto* file) const;

    // Check if explicit "base.proto" import is required and add this import.
    bool ImportBaseProtoIfNecessary(TFileDescriptorProto* file);

    bool HasFile(const Stroka& virtualFile) const {
        return Impl.HasFile(CanonicFileName(virtualFile));
    }

    // Inserts new MessageProto built from @message_descr into BuiltMessageProtos
    //  with all inherited fields and corresponding FileProto from Pool.Database.
    // Do nothing if corresponding MessageProto is already built.
    // Recursively builds all messages from which is derived if any.
    // Returns this newly built MessageProto or NULL if error occurs (records errors to Errors).
    TDescriptorProto* BuildPatchedMessageProto(const TDescriptor* message_descr);
    bool BuildPatchedMessageProtoImpl(const TDescriptor* message_descr, TDescriptorProto *message_proto);

    // For message @derived_message_proto derived from @base_message_proto
    // copies all fields and other inherited content.
    bool CopyInheritedContent(TDescriptorProto* derived_message_proto,  const TDescriptorProto* base_message_proto,
                              const TDescriptor* derived_message_descr, const TDescriptor* base_message_descr);


    bool Compile();

    // ================================================================

    inline void AddError(const TDescriptor* descriptor, const Stroka& error);

private:
    TErrorCollector Errors;                // has its own ErrorCollector!
    TDescriptorCollectionBuilder Impl;

    TGztSourceTree* SourceTree;
    THolder<TProtoParser> Parser;

    // If true then gztcompiler is being used for generating cpp-code from .gztproto in Arcadia.
    // In this mode it generates protoc compatible .proto in binary tree from corresponding .gztproto in source tree.
    // All imported .gztproto are renamed to .proto in generated proto file,
    // so for example [import "b.gztproto";] in a.gztproto will produce a.proto with corresponding [import "b.proto";]
    bool ArcadiaBuildMode;

    DECLARE_NOCOPY(TProtoPoolBuilder);
};



class TSearchKeyIterator
{
public:
    TSearchKeyIterator(const NProtoBuf::Message& article, const TProtoPool* pool,
                       TSearchKey& tmpKey, const TSearchKey* defaults = NULL);

    TSearchKeyIterator(const NProtoBuf::Message& article, const TFieldDescriptor* keyField,
                       TSearchKey& tmpKey, const TSearchKey* defaults = NULL);

    ~TSearchKeyIterator();

    bool Ok() const {
        return CurrentPtr != NULL;
    }

    void operator++();

    const TSearchKey* operator->() {
        return CurrentPtr;
    }

    const TSearchKey& operator*() {
        return *CurrentPtr;
    }
private:
    inline const NProtoBuf::Message* ExtractKey() const;

private:
    const NProtoBuf::Message* Article;
    const TFieldDescriptor* KeyField;
    const TSearchKey* CurrentPtr;
    TSearchKey& TmpKey;
    const TSearchKey* DefaultKey;
    int KeyCount;
};

} // namespace NGzt
