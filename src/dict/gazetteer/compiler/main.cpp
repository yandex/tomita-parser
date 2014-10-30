#include "kernel/gazetteer/common/serialize.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/gazetteer/protopool.h>

#include <library/getopt/last_getopt.h>

#include <util/folder/dirut.h>
#include <util/generic/stroka.h>
#include <util/stream/output.h>


int Compile(const Stroka& inputFile, const Stroka& outputFile,
            const yvector<Stroka>& protoPaths, bool force, TOutputStream& logStream)
{
    TGazetteerBuilder builder(protoPaths);
    return builder.CompileVerbose(inputFile, outputFile, logStream, force) ? 0 : 1;
}

int Verify(const Stroka& gztSrc, const Stroka& gztBin,
           const yvector<Stroka>& importPaths, TOutputStream& logStream)
{
    logStream << "Verifying \"" << gztSrc << "\" vs \"" << gztBin << "\" ..." << Endl;
    NGzt::TGztSourceTree gztTree(importPaths);
    try {
        gztTree.VerifyBinary(gztSrc, gztBin, true);
    } catch (...) {
        Cerr << "\nFailed:" <<  CurrentExceptionMessage() << Endl;
        return 1;
    }
    logStream << "Ok" << Endl;
    return 0;
}

int ConvertProto(const Stroka& inputFile, const Stroka& outputFile, const yvector<Stroka>& protoPaths)
{
    NGzt::TGztSourceTree sourceTree(protoPaths, false); // no default builtins
    NGzt::TProtoPoolBuilder builder(&sourceTree);
    builder.SetArcadiaBuildMode();      // .gztproto -> .proto, read all dependencies from disk (even builtins)
    return builder.GenerateCompatibleProto(inputFile, outputFile) ? 0 : 1;
}

int PrintUsage()
{
    Cout << "Name:\n\tgztcompiler - compiles human readable gazetteer dictionary to binary format.\n\n";
    Cout << "Usage:\n\tgztcompiler [OPTIONS] [-I=proto_paths] INPUTFILE [OUTPUTFILE]\n\n";
    Cout << "Options:\n";

    Cout << "\t-I, --import-path\n\t\tSame as protoc \"--proto_path\": specifies directories where compiler\n";
    Cout << "\t\tshould search imported proto and gzt files. Several directories could be specified \n";
    Cout << "\t\teither as separate -I options or as colon (\':\') delimited single option.\n\n";

    Cout << "\t-f, --force\n\t\tForce compilation.\n";
    Cout << "\t\tWithout this option gztcompiler is allowed to skip compilation if the binary\n";
    Cout << "\t\talready exists and is consistent with all source files.\n\n";

    Cout << "\t-v, --verify\n\t\tVerify if the binary is consistent with the source.\n\n";

    Cout << "\t-p, --convert-proto\n\t\tSpecial mode: only convert input proto to standard syntax (compatible with default protoc).\n";
    Cout << "\t\tWhen this option specified, an input should be a proto file and output proto should be provided\n";
    Cout << "\t\texplicitly.\n\n";

    Cout << "\tINPUTFILE\n\t\tPath to your dictionary in gazetteer text format (*.gzt) or - if '-p' specified - proto file\n";
    Cout << "\t\twith articles types.\n\n";
    Cout << "\tOUTPUTFILE\n\t\tPath where result should be placed. It could be omitted when compiling *.gzt file,\n";
    Cout << "\t\tin this case, default path is INPUTFILE + \".bin\".\n\n";
    return 1;
}

int main(int argc, char* argv[])
{
    using namespace NLastGetopt;
    TOpts opts;
    // --unicode is now deprecated: gazetteer is always wide-char.
    TOpt& optUnicode = opts.AddLongOption('u', "unicode",
        "Make unicode binary for direct unicode (Wtroka) strings searching, without any encodings. "
        "Without this option compiler builds article keys search-trie using encoding specified in dictionary.");
    optUnicode.Optional().NoArgument();

    TOpt& optConvertProto = opts.AddLongOption('p', "convert-proto",
        "Only convert input proto to standard syntax (compatible with default protoc). "
        "Used for including C++ code generated from gzt-proto files into your program. "
        "When this option specified, both input and output proto filenames should be provided.");
    optConvertProto.Optional().NoArgument();

    TOpt& optProtoPath = opts.AddLongOption('I', "import-path",
        "Same as protoc \"--proto_path\" (\"-I\") option, specifies directories where compiler should search imported proto and gzt files.");
    optProtoPath.Optional().RequiredArgument().DefaultValue("");

    TOpt& optArcPath = opts.AddLongOption('A', "arcadia",
        "Specifies the arcadia source top");
    optArcPath.Optional().RequiredArgument().DefaultValue("");

    TOpt& optDataPath = opts.AddLongOption('D', "arcdata",
        "Specifies the arcadia_tests_data source top");
    optDataPath.Optional().RequiredArgument().DefaultValue("");

    TOpt& optTopPath = opts.AddLongOption('T', "common-top",
        "Specifies the common top contaning arcadia and arcadia_tests_data (overrides both -A and -D)");
    optTopPath.Optional().RequiredArgument().DefaultValue("");

    TOpt& optForce = opts.AddLongOption('f', "force",
        "Force compilation. Without this option gztcompiler is allowed to skip compilation if the binary already exists and is consistent with source files.");
    optForce.Optional().NoArgument();

    TOpt& optVerify = opts.AddLongOption('v', "verify",
        "Only check if the source is consistent with a binary.");
    optVerify.Optional().NoArgument();

    TOpt& optQuite = opts.AddLongOption('q', "quite",
        "Suppress log output.");
    optQuite.Optional().NoArgument();

    //opts.AddHelpOption();

    TOptsParseResult r(&opts, argc, argv);
    yvector<Stroka> free = r.GetFreeArgs();

    TOutputStream& logStream = !r.Has(&optQuite) ? Cerr : *Singleton<TNullOutput>();

    if (free.size() >= 1 && free.size() <= 2) {
        Stroka inputFile = free[0];
        bool outputSpecified = free.size() > 1;

        const TOptParseResult* optRes;

        optRes = r.FindOptParseResult(&optProtoPath, true);
        yvector<Stroka> protoPaths;
        if (NULL != optRes && !optRes->Empty()) {
            const TOptParseResult::TValues &vals = optRes->Values();
            protoPaths.reserve(vals.size());
            for (size_t i = 0; i != vals.size(); ++i)
                protoPaths.push_back(vals[i]);
        }

        const Stroka topPath = r.Get(&optTopPath);
        if (!topPath.Empty())
            protoPaths.push_back(topPath);
        const Stroka arcPath = topPath.Empty()
            ? r.Get(&optArcPath) : topPath + "/arcadia";
        if (!arcPath.Empty())
            protoPaths.push_back(arcPath);

        if (r.Has(&optConvertProto)) {
            if (outputSpecified)
                return ConvertProto(inputFile, free[1], protoPaths);

        } else {
            const Stroka dataPath = topPath.Empty()
                ? r.Get(&optDataPath) : topPath + "/arcadia_tests_data";
            if (!dataPath.Empty())
                protoPaths.push_back(dataPath);
            Stroka outputFile = outputSpecified ? free[1] : (inputFile + ".bin");
            bool force = r.Has(&optForce);
            if (r.Has(&optUnicode))
                Cerr << "-u (--unicode) option is deprecated and will be ignored." << Endl;

            if (r.Has(&optVerify))
                return Verify(inputFile, outputFile, protoPaths, logStream);
            else
                return Compile(inputFile, outputFile, protoPaths, force, logStream);
        }
    }
    // something wrong with options
    return PrintUsage();
}
