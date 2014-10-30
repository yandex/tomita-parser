#include "sourcetree.h"
#include "builtin.h"
#include "version.h"

#include <kernel/gazetteer/binary.pb.h>

#include <contrib/libs/protobuf/descriptor.h>

#include <util/system/fstat.h>

namespace NGzt {

TGztSourceTree::TGztBuiltinFiles::TGztBuiltinFiles(bool defaultBuiltins) {
    if (defaultBuiltins) {
        // built-in base.proto / descriptors.proto
        MergeFiles(NBuiltin::DefaultProtoFiles());
    }
}

// TGztSourceTree =========================================================

TGztSourceTree::TGztSourceTree(const Stroka& importPath, bool defaultBuiltins)
    : BuiltinFiles_(defaultBuiltins)
{
    if (!importPath.empty())
        AddImportPaths(importPath);
}

TGztSourceTree::TGztSourceTree(const yvector<Stroka>& importPath, bool defaultBuiltins)
    : BuiltinFiles_(defaultBuiltins)
{
    AddImportPaths(importPath);
}

void TGztSourceTree::IntegrateFiles(const NBuiltin::TFileCollection& files) {
    BuiltinFiles_.MergeFiles(files);
}

Stroka TGztSourceTree::CanonicName(const Stroka& name) const {
    NBuiltin::TConstRef builtin = BuiltinFiles().Find(name);
    if (builtin.Get())
        return builtin->Name();
    else
        return TDiskSourceTree::CanonicName(name);
}

static inline Stroka ProtoFileChecksum(const NProtoBuf::FileDescriptor& file) {
    NProtoBuf::FileDescriptorProto proto;
    file.CopyTo(&proto);
    return NUtil::CalculateDataChecksum(proto.SerializeAsString());
}

Stroka TGztSourceTree::Checksum(const Stroka& name) const {
    const NProtoBuf::FileDescriptor* file = BuiltinFiles().FindFileDescriptor(name);
    return file != NULL ? ProtoFileChecksum(*file)
                        : NUtil::CalculateFileChecksum(GetDiskFileName(name));
}

TBinaryGuard::TDependency TGztSourceTree::AsDependency(const Stroka& name) const {
    TBinaryGuard::TDependency ret(name, Stroka(), false);

    NBuiltin::TConstRef builtin = BuiltinFiles().Find(name);
    if (builtin.Get()) {
        ret.IsBuiltin = true;
        ret.Name = builtin->Name();
        const NProtoBuf::FileDescriptor* file = builtin->FindInGeneratedPool();
        if (file != NULL)
            ret.Checksum = ProtoFileChecksum(*file);
    } else  {
        const Stroka diskFileName = GetDiskFileName(name);
        ret.Checksum = NUtil::CalculateFileChecksum(diskFileName);
        ret.Name = TDiskSourceTree::CanonicName(name);
        ret.IsRoot = !ret.Name.empty() && ret.Name == FindVirtualFile(RootDiskFile());
    }

    return ret;
}

bool TGztSourceTree::LoadDependencies(const Stroka& binFile, yvector<TBinaryGuard::TDependency>& res) const {
    if (TFileStat(~binFile).Size == 0)
        return false;

    // Here we do not need the whole binary, just a small header, so no precharge or pread please
    TBlob data = TBlob::FromFile(binFile);
    TMemoryInput input(data.Data(), data.Length());

    if (!TGztBinaryVersion::Load(&input))
        return false;

    NBinaryFormat::TGazetteer proto;
    LoadProtoOnly(&input, proto);
    for (size_t i = 0; i < proto.GetHeader().ImportedFileSize(); ++i) {
        const NBinaryFormat::TSourceFile& dependency = proto.GetHeader().GetImportedFile(i);
        const Stroka& name = dependency.GetPath();
        bool builtin = dependency.HasBuiltin() ? dependency.GetBuiltin() : BuiltinFiles_.Has(name);
        res.push_back(TBinaryGuard::TDependency(name, dependency.GetHash(), builtin, dependency.GetRoot()));
    }
    return true;
}


Stroka TGztSourceTree::GetDiskFileName(const TBinaryGuard::TDependency& dep) const {
    Stroka diskFile;
    return VirtualFileToDiskFile(dep.Name, diskFile) ? diskFile : Stroka();
}

Stroka TGztSourceTree::CalculateBuiltinChecksum(const TBinaryGuard::TDependency& dep) const {
    YASSERT(dep.IsBuiltin);
    const TFileDescriptor* file = NBuiltin::FindInGeneratedPool(dep.Name);
    return (file != NULL) ? ProtoFileChecksum(*file) : Stroka();
}


void TGztSourceTree::VerifyBinary(const Stroka& gztSrc, const Stroka& gztBin, bool missingOrEmpty)
{
    TRootFile root(gztSrc, *this);
    yvector<Stroka> badFiles;
    if (CollectBadSources(gztSrc, gztBin, badFiles, missingOrEmpty)) {
        Stroka msg = "Gazetteer binary does not match to existing source files. "
              "Please update out-dated files from svn or re-compile the binary using gztcompiler.\n"
              "Binary: " + gztBin + "\nSource: " + gztSrc + "\nConflicts (mismatching files):";
        for (size_t i = 0; i < badFiles.size(); ++i)
            msg += "\n\t" + badFiles[i];
        ythrow yexception() << msg << Endl;
    }
}


}   // namespace NGzt
