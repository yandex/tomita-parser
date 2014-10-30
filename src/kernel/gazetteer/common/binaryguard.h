#pragma once

#include <util/generic/stroka.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>


namespace NUtil {
    // utility helpers
    i64 LastModificationTime(const Stroka& file);
    Stroka CalculateFileChecksum(const Stroka& file);
    Stroka CalculateDataChecksum(const Stroka& data);
}


class TBinaryGuard {
public:
    enum ECheckState {
        BadBinary,      // the binary is definitely compiled from sources other than currently available ones
        GoodBinary,     // the binary is fully consistent with all sources (and all sources are available at their remembered locations too)
        Undefined       // some of the sources (or the binary itself) cannot be found, it that case the state of the binary is undefined
    };

    // Returns flag displaying if @bin_file was built from @srcFile in its current exact state.
    // This check is similiar to IsBinaryUpToDate but more reliable in cases when
    // file modification time could be changed without real file modification.
    // This will read all source files and check their md5-hashes against checksum stored in @binFile.
    ECheckState CheckBinary(const Stroka& srcFile, const Stroka& binFile) const;



    // Same as CheckBinary, checks all sources and collects the list of files inconsistent with given binary.
    bool CollectBadSources(const Stroka& srcFile, const Stroka& binFile, yvector<Stroka>& results,
                           bool missingOrEmpty = false) const;


    // Returns true when specified binary is consistent with specified source file -
    // and therefore need not be re-compiled.
    inline bool IsGoodBinary(const Stroka& srcFile, const Stroka& binFile) const {
        return CheckBinary(srcFile, binFile) == GoodBinary;
    }

    // Returns true when specified binary file is inconsistent with specified source file.
    inline bool IsBadBinary(const Stroka& srcFile, const Stroka& binFile) const {
        return CheckBinary(srcFile, binFile) == BadBinary;
    }

    enum ECompileMode {
        Optional,    // compile only if the binary is missing or inconsistent with the sources.
        Normal,      // skip compilation only if the binary is consistent with all sources.
        Forced       // compile anyway
    };

    // to build or not to build - that's the question
    bool RebuildRequired(const Stroka& srcFile, const Stroka& binFile, ECompileMode mode) const;



    // Returns true if @srcFile is not newer than @binFile (as well as all imported gzt or proto files)
    // This would mean there is no need to re-build @srcFile in order to get its fresh up-to-date binary representation.
    bool IsBinaryUpToDate(const Stroka& srcFile, const Stroka& binFile) const;



    virtual ~TBinaryGuard() {
    }

    struct TDependency {
        Stroka Name;            // either builtin identity or virtual file name
        Stroka Checksum;
        bool IsBuiltin;         // a dependency could be a real file on disk or some builtin (virtual) object
        bool IsRoot;            // a root of compilation (main source gazetteer)

        inline TDependency(const Stroka& name, const Stroka& checksum, bool isbuiltin, bool isroot = false)
            : Name(name), Checksum(checksum), IsBuiltin(isbuiltin), IsRoot(isroot)
        {
        }
    };

private:
    enum EDependencyState {
        FileMissing,
        NoChecksum,
        BadChecksum,
        GoodChecksum
    };

    EDependencyState GetDependencyState(const TDependency& dep) const;
    inline bool IsSrcFile(const TDependency& dep, const Stroka& srcFile) const;

private:
    // TBinaryGuard interface

    virtual bool LoadDependencies(const Stroka& fileName, yvector<TDependency>& res) const = 0;

    virtual Stroka CalculateBuiltinChecksum(const TDependency&) const {
        return Stroka();
    }

    // For disk file return its path in file system (real path)
    virtual Stroka GetDiskFileName(const TDependency& dep) const {
        return dep.Name;
    }
};

