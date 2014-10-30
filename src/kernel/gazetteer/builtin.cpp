#include "builtin.h"

#include <util/generic/yexception.h>
#include <util/generic/singleton.h>
#include <util/generic/algorithm.h>

#include "base.pb.h"

namespace NGzt {

typedef NProtoBuf::FileDescriptorProto TFileDescriptorProto;

namespace NBuiltin {

    const TFileDescriptor* FindInGeneratedPool(const Stroka& name) {
        return NProtoBuf::DescriptorPool::generated_pool()->FindFileByName(name);
    }

    TFile::TFile(const Stroka& name)
        : GeneratedDescriptor(NULL)
    {
        if (!name)
            ythrow yexception() << "NBuiltin::TFile's base name cannot be empty";
        AddAlias(name);
    }

    inline bool TFile::Is(const TStringBuf& name) const {
        return ::Find(Aliases_.begin(), Aliases_.end(), name) != Aliases_.end();
    }

    inline void TFile::AddAliasImpl(const Stroka& name) {
        if (!name.empty() && !Is(name)) {
            Aliases_.push_back(name);
            if (!GeneratedDescriptor)
                GeneratedDescriptor = NBuiltin::FindInGeneratedPool(name);
        }
    }

    TFile& TFile::AddAlias(const Stroka& name) {
        AddAliasImpl(name);

        const TStringBuf proto = STRINGBUF(".proto");
        const TStringBuf gztproto = STRINGBUF(".gztproto");

        // also add complementing .gztproto or .proto
        if (name.has_suffix(proto))
            AddAliasImpl(Stroka(TStringBuf(name).Chop(proto.size()), gztproto));
        else if (name.has_suffix(gztproto))
            AddAliasImpl(Stroka(TStringBuf(name).Chop(gztproto.size()), proto));

        return *this;
    }

    void TFile::MergeAliases(const TFile& file) {
        for (size_t i = 0; i < file.Aliases().size(); ++i)
            AddAliasImpl(file.Aliases()[i]);
    }



    const TFile& TFileCollection::AddFile(const Stroka& name, const Stroka& alias1, const Stroka& alias2) {
        return Merge(TFile(name).AddAlias(alias1).AddAlias(alias2));
    }

    TFile* TFileCollection::FindByAlias(const TFile& file) {
        TFile* ret = NULL;
        for (size_t i = 0; i < file.Aliases().size(); ++i) {
            ymap<Stroka, TRef>::iterator it = Index.find(file.Aliases()[i]);
            if (it != Index.end()) {
                if (!ret)
                    ret = it->second.Get();
                else if (it->second.Get() != ret)
                    ythrow yexception() << "Cannot identify builtin file unambiguously";
            }
        }
        return ret;
    }

    void TFileCollection::AddToIndex(TFile& file) {
        for (size_t i = 0; i < file.Aliases().size(); ++i)
            Index[file.Aliases()[i]] = &file;
    }

    const TFile& TFileCollection::Merge(const TFile& file) {
//        if (!file.FindInGeneratedPool())
//            return;

        TRef myfile = FindByAlias(file);
        if (!myfile) {
            myfile = new TFile(file);
            Files.push_back(myfile);
        } else
            myfile->MergeAliases(file);

        AddToIndex(*myfile);
        return *myfile;
    }

    void TFileCollection::MergeFiles(const TFileCollection& files) {
        for (size_t i = 0; i < files.Size(); ++i)
            Merge(files[i]);

        if (files.Maximized)
            Maximized = true;
    }

    TRef TFileCollection::FindExplicit(const TStringBuf& name) const {
        ymap<Stroka, TRef>::const_iterator it = Index.find(name);
        return it != Index.end() ? it->second : NULL;
    }

    TRef TFileCollection::FindImplicit(const Stroka& name) const {
        // search in generated_pool and construct TFile on the fly
        TRef file(new TFile(name));
        return file->FindInGeneratedPool() ? file : NULL;
    }

    TConstRef TFileCollection::Find(const TStringBuf& name) const {
        TRef ret = FindExplicit(name);
        if (!ret && Maximized)
            ret = FindImplicit(Stroka(name));
        return ret.Get();
    }

    const TFileDescriptor* TFileCollection::FindFileDescriptor(const TStringBuf& name) const {
        TConstRef ret = Find(name);
        return ret.Get() ? ret->FindInGeneratedPool() : NULL;
    }

    const TFile* TFileCollection::FindOrAdd(const Stroka& name) {
        TRef ret = FindExplicit(name);
        if (!ret && Maximized) {
            TRef impl = FindImplicit(name);
            if (impl.Get()) {
                Merge(*impl);
                ret = FindExplicit(name);
            }
        }
        return ret.Get();
    }

    Stroka TFileCollection::DebugString() const {
        Stroka ret;
        for (size_t i = 0; i < Files.size(); ++i) {
            ret.append(Files[i]->Name()).append(" ->");
            for (size_t j = 0; j < Files[i]->Aliases().size(); ++j)
                ret.append(' ').append(Files[i]->Aliases()[j]);
            ret.append('\n');
        }
        return ret;
    }


    static const Stroka BASE_PROTO = "kernel/gazetteer/base.proto";

    class TDefaultProtoFiles: public TFileCollection {
    public:
        const TFile& DescriptorProto;
        const TFile& BaseProto;

        TDefaultProtoFiles()
            : DescriptorProto(
                AddFileSafe<NProtoBuf::DescriptorProto>("contrib/libs/protobuf/descriptor.proto", "descriptor.proto"))
            , BaseProto(
                AddFileSafe<TArticle>(BASE_PROTO, "base.proto"))
        {
        }
    };

    const TFileCollection& DefaultProtoFiles() {
        return Default<TDefaultProtoFiles>();
    }

    const Stroka& BaseProtoName() {
        return BASE_PROTO;
    }

    const TFile& BaseProtoFile() {
        return Default<TDefaultProtoFiles>().BaseProto;
    }



}   // namespace NBuiltin



TMixedDescriptorDatabase::TMixedDescriptorDatabase(const NBuiltin::TFileCollection& builtins,
                                                   NProtoBuf::compiler::SourceTree* srcTree,
                                                   NProtoBuf::compiler::MultiFileErrorCollector* errors)
    : BuiltinDb(*NProtoBuf::DescriptorPool::generated_pool())
    , DiskDb(srcTree)
{
    Builtins.MergeFiles(builtins);
    DiskDb.RecordErrorsTo(errors);
}

TMixedDescriptorDatabase::TInfo::TInfo(NProtoBuf::FileDescriptorProto* file)
    : File(file)
    , Builtin(NULL)
{
}

bool TMixedDescriptorDatabase::CheckBuiltinOrClear(TInfo& info) {
    info.Builtin = Builtins.FindOrAdd(info.File->name());
    if (info.Builtin)
        return true;

    info.File->Clear();
    return false;
}

bool TMixedDescriptorDatabase::DoFindFileByName(const Stroka& filename, TInfo& info) {
    info.Builtin = Builtins.FindOrAdd(filename);
    if (info.Builtin) {
        return BuiltinDb.FindFileByName(info.Builtin->GeneratedPoolName(), info.File);
    } else
        return DiskDb.FindFileByName(filename, info.File);
}

bool TMixedDescriptorDatabase::DoFindFileContainingSymbol(const Stroka& symbol_name, TInfo& info) {
    if (BuiltinDb.FindFileContainingSymbol(symbol_name, info.File))
        if (CheckBuiltinOrClear(info))
            return true;

    return DiskDb.FindFileContainingSymbol(symbol_name, info.File);
}

bool TMixedDescriptorDatabase::DoFindFileContainingExtension(const Stroka& containing_type, int field_number, TInfo& info) {
    if (BuiltinDb.FindFileContainingExtension(containing_type, field_number, info.File))
        if (CheckBuiltinOrClear(info))
            return true;

    return DiskDb.FindFileContainingExtension(containing_type, field_number, info.File);
}

bool TMixedDescriptorDatabase::FindFileByName(const Stroka& filename, NProtoBuf::FileDescriptorProto* output) {
    TInfo info(output);
    return output != NULL
        && DoFindFileByName(filename, info)
        && Preprocess(info);
}

bool TMixedDescriptorDatabase::FindFileContainingSymbol(const Stroka& symbol_name, NProtoBuf::FileDescriptorProto* output) {
    TInfo info(output);
    return output != NULL
        && DoFindFileContainingSymbol(symbol_name, info)
        && Preprocess(info);
}

bool TMixedDescriptorDatabase::FindFileContainingExtension(const Stroka& containing_type, int field_number,
                                                           NProtoBuf::FileDescriptorProto* output) {
    TInfo info(output);
    return output != NULL
        && DoFindFileContainingExtension(containing_type, field_number, info)
        && Preprocess(info);
}

bool TMixedDescriptorDatabase::Preprocess(TInfo& info) {
    if (!info.Builtin)
        PatchImports(info);
    Files.insert(info.File->name());
    return true;
}

void TMixedDescriptorDatabase::PatchImports(TInfo& info) {
    // Canonicalize builtins' imports to make sure parent DescriptorPool
    // finds these imported files later using its FindFileByName() method.
    // See comment to TMixedDescriptorPool::FindFileByName() below.
    for (int i = 0; i < info.File->dependency_size(); ++i) {
        const NBuiltin::TFile* file = Builtins.FindOrAdd(info.File->dependency(i));
        if (file)
            info.File->set_dependency(i, file->GeneratedPoolName());
    }
}

Stroka TMixedDescriptorDatabase::SearchableName(const Stroka& filename) {
    const NBuiltin::TFile* b = Builtins.FindOrAdd(filename);
    return b ? b->GeneratedPoolName() : filename;
}

const TFileDescriptor* TMixedDescriptorPool::FindFileByName(const Stroka& name) {
    // Here it is important to search builtin files by their names from generated_pool.
    // For example, "test.gztproto" should be searched as "test.proto",
    // otherwise Database finds it as "test.proto", Pool puts it in its tables as "test.proto"
    // and then try find it there as "test.gztproto" and fails.

    return Pool.FindFileByName(Database.SearchableName(name));
}

void TMixedDescriptorPool::CollectFiles(yvector<const TFileDescriptor*>& files) {
    files.clear();
    const yset<Stroka>& fileNames = Database.RequestedFiles();
    for (yset<Stroka>::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        const TFileDescriptor* fd = FindFileByName(*it);
        if (fd)
            files.push_back(fd);
    }
}

}   // namespace NGzt
