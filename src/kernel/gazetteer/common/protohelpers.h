#pragma once

#include "serialize.h"

#include <library/protobuf/json/proto2json.h>

#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/messagext.h>
#include <contrib/libs/protobuf/text_format.h>
#include <contrib/libs/protobuf/compiler/importer.h>
#include <contrib/libs/protobuf/compiler/parser.h>
#include <contrib/libs/protobuf/io/tokenizer.h>

#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <util/generic/stroka.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

// Defined in this file:
class TErrorCollector;
class TDiskSourceTree;
class TSimpleJsonConverter;



//simple implementation of ErrorCollector and MultiFileErrorCollector - just prints all errors to stderr
class TErrorCollector: public NProtoBuf::io::ErrorCollector,
                       public NProtoBuf::compiler::MultiFileErrorCollector,
                       public NProtoBuf::DescriptorPool::ErrorCollector
{
public:
    // implements NProtoBuf::io::ErrorCollector
    virtual void AddError(int line, int column, const TProtoStringType& message);

    // implements NProtoBuf::compiler::MultiFileErrorCollector
    virtual void AddError(const TProtoStringType& filename, int line, int column, const TProtoStringType& message);

    // implements NProtoBuf::DescriptorPool::ErrorCollector
    virtual void AddError(const TProtoStringType& filename, const TProtoStringType& element_name,
                          const NProtoBuf::Message* descriptor,
                          NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation location,
                          const TProtoStringType& message);


    void AddErrorAtCurrentFile(int line, int column, const TProtoStringType& message);
    void SetCurrentFile(const Stroka& filename) {
        CurrentFile = filename;
    }

    // print error with @tree current file
    void AddError(const TDiskSourceTree* tree, int line, int column, const TProtoStringType& message);

    // just error without specific location
    void AddError(const TProtoStringType& message) {
        AddError(-1, 0, message);
    }

private:
    Stroka CurrentFile;
};


// Adds some helpers to NProtoBuf::compiler::DiskSourceTree
class TDiskSourceTree: public NProtoBuf::compiler::SourceTree
{
    typedef NProtoBuf::compiler::DiskSourceTree TProtobufDiskSourceTree;
    typedef TProtobufDiskSourceTree::DiskFileToVirtualFileResult EResolveRes;
    class TPathStack;

public:
    TDiskSourceTree();
    TDiskSourceTree(const yvector<Stroka>& importPaths);
    virtual ~TDiskSourceTree();

    class TFileBase {
    public:
        // does Stack.Push() in derived class
        TFileBase(const Stroka& vfile, TPathStack& stack)
            : Stack(stack)
            , VirtualName(vfile)
        {
        }

        // does Stack.Pop()
        ~TFileBase();

        const Stroka& Virtual() const {
            return VirtualName;
        }

    private:
        TPathStack& Stack;
        const Stroka VirtualName;
    };

    // Using RAII idiom for push/pop in path stack
    class TRootFile: public TFileBase {
    public:
        TRootFile(const Stroka& file, TDiskSourceTree& tree)
            : TFileBase(tree.PushRootFile(file), *tree.RootPath)
        {
        }
    };

    class TCurrentFile: public TFileBase {
    public:
        TCurrentFile(const Stroka& file, TDiskSourceTree& tree)
            : TFileBase(tree.PushCurrentFile(file), *tree.CurPath)
        {
        }
    };


public:
    // Split @path by ":" and add given paths as roots.
    // @path could be a value of command-line argument  "-I" or "--proto_path", for example, as in protoc.
    void AddImportPaths(const Stroka& path);

    void AddImportPaths(const yvector<Stroka>& paths) {
        for (size_t i = 0; i != paths.size(); ++i)
            AddImportPaths(paths[i]);
    }

    Stroka RootDiskFile() const;

    Stroka CurrentDiskFile() const;
    Stroka CurrentDir() const {
        return GetDirName(CurrentDiskFile());
    }


    // Find or throw exception
    // Try returning virtual path from root or import paths in the first place, than go searching from current dir
    Stroka FindVirtualFile(const Stroka& diskFile) const;

    // Non-ambiguous @virtualFile from corresponding to @diskFile, or false
    bool DiskFileToVirtualFile(const Stroka& diskFile, Stroka& virtualFile) const;

    // Resolve @diskFile from @virtualFile
    bool VirtualFileToDiskFile(const Stroka& virtualFile, Stroka& diskFile) const;

    // Simple helper for previous
    Stroka GetDiskFileName(const Stroka& virtual_file) const {
        Stroka diskFile;
        return VirtualFileToDiskFile(virtual_file, diskFile) ? diskFile : Stroka();
    }

    // normalization: re-map to disk and back
    Stroka CanonicName(const Stroka& virtualFile) const;

    // Open file for reading via memory mapping, supports .gz (gzipped)
    TAutoPtr<TInputStream> OpenDiskFile(const Stroka& diskFile) const;
    TAutoPtr<TInputStream> OpenVirtualFile(const Stroka& virtualFile) const;

    // implements SourceTree -------------------------------------------
    NProtoBuf::io::ZeroCopyInputStream* Open(const Stroka& virtualFile);

private:
    static void AddPathAsRoot(const Stroka& path, TProtobufDiskSourceTree& tree) {
        if (!path.Empty())
            tree.MapPath("", path);
    }

    static void RaiseMappingError(EResolveRes res, const Stroka& file, const Stroka& shadowingDiskFile);

    EResolveRes DiskFileToVirtualFile(const Stroka& diskFile, Stroka* virtualFile, Stroka* shadowingDiskFile) const;

    Stroka PushCurrentFile(const Stroka& file);
    Stroka PushRootFile(const Stroka& file);


    const TPathStack* CurrentPathStack() const;

private:
    THolder<TProtobufDiskSourceTree> ImportTree;
    THolder<TPathStack> RootPath;
    THolder<TPathStack> CurPath;
    TMutex Mutex;       // for const access to non-const methods of DiskSourceTree
};



// to make yset<const NProtoBuf::Message*>
// NOT THREAD-SAFE!
class MessageLess
{
public:
    MessageLess();
    bool operator()(const NProtoBuf::Message* a, const NProtoBuf::Message* b);
private:
    Stroka BufferA, BufferB;
};



class TJsonPrinter
{
public:
    TJsonPrinter();
    Stroka ToString(const NProtoBuf::Message& message);
    void ToString(const NProtoBuf::Message& message, Stroka& out);
    void ToStream(const NProtoBuf::Message& message, TOutputStream& out);

    void SetSingleLineMode(bool mode) {
        Config.FormatOutput = !mode;
    }

private:
    NProtobufJson::TProto2JsonConfig Config;
};



// If @from has exactly the same message-type as TGeneratedMessage then @from is simply casted to TGeneratedMessage and returned.
// Otherwise @from is copied into @buffer via Reflection or re-serialization.
template <class TGeneratedMessage>
inline const TGeneratedMessage* CastProto(const NProtoBuf::Message* from, TGeneratedMessage* buffer) {
    const TGeneratedMessage* direct = dynamic_cast<const TGeneratedMessage*>(from);
    if (direct)
        return direct;

    if (from->GetDescriptor() == buffer->GetDescriptor())
        buffer->CopyFrom(*from);
    else if (!ReserializeProto(from, buffer))
        ythrow yexception() << "Incompatible @to and @from messages";

    return buffer;
}

// Convert @from data to @to via binary format,
// @from and @to should be binary compatible.
// Return true on successful conversion.
bool ReserializeProto(const NProtoBuf::Message* from, NProtoBuf::Message* to);
