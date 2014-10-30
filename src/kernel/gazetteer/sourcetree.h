#pragma once

#include "builtin.h"

#include "common/protohelpers.h"
#include "common/binaryguard.h"

#include <util/generic/stroka.h>
#include <util/generic/vector.h>

namespace NGzt {

class TGztSourceTree: public TDiskSourceTree, public TBinaryGuard {
public:
    // if defaultBuiltins then NBuiltin::DefaultProtoFiles() are integrated
    TGztSourceTree(const Stroka& importPath = Stroka(), bool defaultBuiltins = true);
    TGztSourceTree(const yvector<Stroka>& importPath, bool defaultBuiltins = true);

    const NBuiltin::TFileCollection& BuiltinFiles() const {
        return BuiltinFiles_;
    }

    // Append @files to list of builtin files.
    void IntegrateFiles(const NBuiltin::TFileCollection& files);

    // Append TProto's descriptor file to builtins.
    template <typename TProto>
    void IntegrateBuiltinFile() {
        BuiltinFiles_.AddFileSafe<TProto>(TProto::descriptor()->file()->name());
    }

    // Treat as much files as possible as builtins.
    void MaximizeBuiltins() {
        BuiltinFiles_.Maximize();
    }

    Stroka CanonicName(const Stroka& name) const;
    Stroka Checksum(const Stroka& name) const;

    TBinaryGuard::TDependency AsDependency(const Stroka& name) const;

    using TDiskSourceTree::GetDiskFileName;

    void VerifyBinary(const Stroka& gztSrc, const Stroka& gztBin, bool missingOrEmpty = false);

private:
    // implements TBinaryGuard --------------------------------

    // Only loads list of dependencies files from binary file (*.gzt.bin) with their checksums
    virtual bool LoadDependencies(const Stroka& binFile, yvector<TBinaryGuard::TDependency>& res) const;
    virtual Stroka GetDiskFileName(const TBinaryGuard::TDependency&) const;
    virtual Stroka CalculateBuiltinChecksum(const TBinaryGuard::TDependency&) const;

private:
    class TGztBuiltinFiles: public NBuiltin::TFileCollection {
    public:
        TGztBuiltinFiles(bool defaultBuiltins);
    };

    TGztBuiltinFiles BuiltinFiles_;
};


}   // namespace NGzt
