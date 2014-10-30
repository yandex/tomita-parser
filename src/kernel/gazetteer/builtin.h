#pragma once

#include <contrib/libs/protobuf/descriptor.h>
#include <contrib/libs/protobuf/descriptor_database.h>
#include <contrib/libs/protobuf/compiler/importer.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/hash.h>

namespace NGzt {

typedef NProtoBuf::FileDescriptor TFileDescriptor;

namespace NBuiltin {

    const TFileDescriptor* FindInGeneratedPool(const Stroka& name);

    class TFile: public TRefCounted<TFile> {
    public:
        explicit TFile(const Stroka& name);

        TFile& AddAlias(const Stroka& name);            // automatically adds .gztproto alias to .proto and vice verse
        void MergeAliases(const TFile& file);

        const Stroka& Name() const {
            return Aliases_[0];
        }

        const TFileDescriptor* FindInGeneratedPool() const {
            return GeneratedDescriptor;
        }

        const Stroka& GeneratedPoolName() const {
            // could be different with Name()
            return GeneratedDescriptor ? GeneratedDescriptor->name() : Name();
        }

        const yvector<Stroka>& Aliases() const {
            return Aliases_;
        }

    private:
        bool Is(const TStringBuf& name) const;
        void AddAliasImpl(const Stroka& name);

    private:
        yvector<Stroka> Aliases_;
        const TFileDescriptor* GeneratedDescriptor; // could be NULL!
    };


    typedef TIntrusivePtr<TFile> TRef;
    typedef TIntrusiveConstPtr<TFile> TConstRef;


    class TFileCollection {
    public:
        TFileCollection()
            : Maximized(false)
        {
        }

        // adding of ready files
        const TFile& Merge(const TFile& file);
        void MergeFiles(const TFileCollection& files);

        // construct and merge
        const TFile& AddFile(const Stroka& name, const Stroka& alias1 = Stroka(), const Stroka& alias2 = Stroka());

        // This method is guaranteed to find specified descriptor parent file descriptor in generated pool.
        // If not present it will run protobuf static descriptors registration
        template <typename TProto>
        const TFile& AddFileSafe(const Stroka& alias1 = Stroka(), const Stroka& alias2 = Stroka()) {
            // this will register TGeneratedProto in generated_pool if this did not run yet
            if (EXPECT_FALSE(TProto::descriptor() == NULL))
                ythrow yexception() << "TFileCollection::AddFileSafe() failed: TGeneratedProto descriptor is not not linked into executable";
            return AddFile(TProto::descriptor()->file(), alias1, alias2);
        }

        // Add whole @file descriptor (probably from generated_pool) to file collection.
        // The name of this file will be canonical name for added file.
        // Also all dependencies (imports) are added recursively.
        const TFile& AddFile(const TFileDescriptor* file, const Stroka& alias1 = Stroka(), const Stroka& alias2 = Stroka()) {
            for (int i = 0; i < file->dependency_count(); ++i)
                AddFile(file->dependency(i));   // without aliases
            return AddFile(file->name(), alias1, alias2);
        }

        size_t Size() const {
            return Files.size();
        }

        const TFile& operator[] (size_t index) const {
            return *Files[index];
        }

        // treat all generated_pool as builtin files
        void Maximize() {
            Maximized = true;
        }

        TConstRef Find(const TStringBuf& name) const;
        const TFileDescriptor* FindFileDescriptor(const TStringBuf& name) const;

        // either find explicit or add implicit and return it.
        const TFile* FindOrAdd(const Stroka& name);

        bool Has(const TStringBuf& name) const {
            return Find(name).Get() != NULL;
        }

        Stroka DebugString() const;

    private:
        TFile* FindByAlias(const TFile& file);
        TRef FindExplicit(const TStringBuf& name) const;
        TRef FindImplicit(const Stroka& name) const;
        void AddToIndex(TFile& file);

    private:
        yvector<TRef> Files;
        ymap<Stroka, TRef> Index;

        bool Maximized;

        DECLARE_NOCOPY(TFileCollection);
    };



    // Contains:
    // 1. "contrib/libs/protobuf/descriptor.proto" for protobuf options
    // 2. "base.proto" with base gazetteer definitions
    const TFileCollection& DefaultProtoFiles();

    const Stroka& BaseProtoName();      // Canonical name of base.proto
    const TFile& BaseProtoFile();       // the builtin file itself

}   // namespace NBuiltin



// DescriptorDatabase with NBuiltin::TFileCollection inside.
// Makes copies of all requested files descriptors which are marked as builtins.
// Otherwise reads files from disk (doing dependency path canonization)
class TMixedDescriptorDatabase: public NProtoBuf::DescriptorDatabase {
public:
    TMixedDescriptorDatabase(const NBuiltin::TFileCollection& builtinFiles,
                             NProtoBuf::compiler::SourceTree* srcTree,
                             NProtoBuf::compiler::MultiFileErrorCollector* errors);

    NProtoBuf::DescriptorPool::ErrorCollector* GetValidationErrorCollector() {
        return DiskDb.GetValidationErrorCollector();
    }

    // implements NProtoBuf::DescriptorDataBase
    virtual bool FindFileByName(const Stroka& filename, NProtoBuf::FileDescriptorProto* output);
    virtual bool FindFileContainingSymbol(const Stroka& symbol_name, NProtoBuf::FileDescriptorProto* output);
    virtual bool FindFileContainingExtension(const Stroka& containing_type, int field_number, NProtoBuf::FileDescriptorProto* output);

    const yset<Stroka>& RequestedFiles() const {
        return Files;
    }

    Stroka SearchableName(const Stroka& filename);

private:
    struct TInfo {
        NProtoBuf::FileDescriptorProto* File;       // NULL if unsuccessful;
        const NBuiltin::TFile* Builtin;

        TInfo(NProtoBuf::FileDescriptorProto* file);
    };

    bool DoFindFileByName(const Stroka& filename, TInfo& output);
    bool DoFindFileContainingSymbol(const Stroka& symbol_name, TInfo& output);
    bool DoFindFileContainingExtension(const Stroka& containing_type, int field_number, TInfo& output);

    bool CheckBuiltinOrClear(TInfo& info);
    bool Preprocess(TInfo& info);
    void PatchImports(TInfo& info);

private:
    NBuiltin::TFileCollection Builtins;
    NProtoBuf::DescriptorPoolDatabase BuiltinDb;
    NProtoBuf::compiler::SourceTreeDescriptorDatabase DiskDb;

    // All requested files with all their dependencies, sorted alphabetically
    yset<Stroka> Files;
};


// Mixed descriptor pool: a set of specified builtin protos + protos read from disk
class TMixedDescriptorPool {
public:
    TMixedDescriptorPool(const NBuiltin::TFileCollection& builtins,
                         NProtoBuf::compiler::SourceTree* srcTree,
                         NProtoBuf::compiler::MultiFileErrorCollector* errors)
        : Database(builtins, srcTree, errors)
        , Pool(&Database, Database.GetValidationErrorCollector())
    {
    }

    // imitate DescriptorPool, search descriptor using mixed Database:
    // either in memory (if it is built-in) or on disk
    const TFileDescriptor* FindFileByName(const Stroka& name);

    const TFileDescriptor* ImportBuiltinProto(const NBuiltin::TFile& proto) {
        return FindFileByName(proto.Name());
    }

    // Collect all requested file descriptors with all their dependencies
    // Files are sorted by their canonical name.
    void CollectFiles(yvector<const TFileDescriptor*>& files);

    Stroka DebugString() {
        Stroka s("TMixedDescriptorPool::RequestedFiles():");
        yvector<const TFileDescriptor*> files;
        CollectFiles(files);
        for (size_t i = 0; i < files.size(); ++i)
            s.append("\n\t").append(files[i]->name());
        return s;
    }

private:
    TMixedDescriptorDatabase Database;
    NProtoBuf::DescriptorPool Pool;
};

}   // namespace NGzt
