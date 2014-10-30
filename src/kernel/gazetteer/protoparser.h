#pragma once

// Implements parsing of extended Protocol Buffers syntax - with message inheritance.

#include "builtin.h"

#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/stream/input.h>

#include <contrib/libs/protobuf/io/tokenizer.h>
#include <contrib/libs/protobuf/io/zero_copy_stream.h>
#include <contrib/libs/protobuf/compiler/importer.h>

#include "common/protohelpers.h"

namespace NGzt
{

class TGztSourceTree;

class TSimpleHierarchy;
class TDependencyCollectorSourceTree;

class TProtoParser {
public:
    typedef NProtoBuf::Descriptor TDescriptor;
    typedef NProtoBuf::FileDescriptor TFileDescriptor;
    typedef ymap<const TDescriptor*, const TDescriptor*> TDescriptorMap;

    // Set @builtinFiles as builtin protos for current compilation
    TProtoParser(TGztSourceTree* source_tree,
                 TErrorCollector* error_collector,
                 const NBuiltin::TFileCollection& builtinFiles);

    ~TProtoParser();

    // Parses specified filename (it must be virtual filename) and builds RawFileDescriptors.
    // into its internally supported descriptors database.
    // "Raw" means "not containing inherited fields, only fields defined in source proto files".
    // Uses NProtoBuf::compiler::Importer + TDependencyCollectorSourceTree.
    bool LoadRawProto(const Stroka& filename);

    const yvector<const TFileDescriptor*>& RawFileDescriptors() const {
        return RawFileDescriptors_;
    }

    // Returns pointer to Descriptor of message declared as base for specified @message_descr.
    // Returns NULL if @message_descr has no base type.
    const TDescriptor* GetBaseDescriptor(const TDescriptor* message_descr) const;

private:
    // Builds Descriptor* -> Descriptor* mapping from messages of @mRawFileDescriptors
    // and inheritance graph collected in SourceTree.
    // Asserts that no cycle dependency among messages exists, i.e. there is no message derived from itself.
    // Returns false on error.
    bool BuildMessageInheritanceGraph();
    bool BuildMessageInheritanceGraph(const TDescriptor* msg_descr, const TSimpleHierarchy& graph);

    bool CheckIfSelfDerived(const TDescriptor* message_descr);

    // Returns descriptor of @derived_message's base message or NULL if not found.
    // In last case adds an error to ErrorCollector.
    const TDescriptor* ResolveBaseMessageByName(const Stroka& base_message_name,
                                                const TDescriptor* derived_message);

private:
    // =================================================================
    TErrorCollector* Errors;

    // this is special SourceTree memorizing all read via it files + collecting message inheritance map.
    THolder<TDependencyCollectorSourceTree> SourceTree;

    // Current compilation builtins
    const NBuiltin::TFileCollection& BuiltinFiles;

    // After parsing contains all built file descriptors (both builtin and parsed from files on disk)
    // "Raw" here means "not containing inherited fields, only fields defined in source proto files".
    TMixedDescriptorPool RawPool;

    //File descriptor pointers from RawPool (to iterate over parsed files, which is impossible with RawPool)
    yvector<const TFileDescriptor*> RawFileDescriptors_;

    // Descriptor* -> Descriptor* mapping, reflects message inheritance.
    TDescriptorMap RawMessageGraph;

    DECLARE_NOCOPY(TProtoParser);
};


} // namespace NGzt
