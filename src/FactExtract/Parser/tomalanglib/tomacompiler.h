#pragma once

#include <util/generic/stroka.h>
#include <util/generic/ptr.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

#include <kernel/gazetteer/common/binaryguard.h>

#include "abstract.h"

// Defined in this file:
class TTomitaCompiler;

// Forward declarations:
class TErrorCollector;
namespace NTomita {
    class TFileProto;
    class TRuleProto;
    class TFileLocation;
}
struct CWorkGrammar;

class TTomitaCompiler
{
public:
    TTomitaCompiler(const IArticleNames* articles = NULL,
                    const IFactTypeDictionary* facttypes = NULL,
                    const ITerminalDictionary* terminals = NULL,
                    TOutputStream& log = Cerr);

    ~TTomitaCompiler(); // for sake of TErrorCollector

    bool Compile(TInputStream* input, CWorkGrammar* targetGrammar, const Stroka& root = "");        // used in dict/tomaparser

    bool Compile(const Stroka& srcFile, CWorkGrammar* targetGrammar);
    bool Compile(const Stroka& srcFile, const Stroka& targetFile, bool force = false);

    // Returns false if grammar binary file corresponds to grammar sources (if available)
    // This will mean the @srcFile grammar compilation can be safely skipped.
    static bool IsGoodBinary(const Stroka& srcFile, const Stroka& binFile);

    static bool CompilationRequired(const Stroka& srcFile, const Stroka& binFile, TBinaryGuard::ECompileMode mode);

    // for debugging
    static Stroka PrintRule(const NTomita::TRuleProto& rule, size_t detailLevel);
    static Stroka PrintFile(const NTomita::TFileProto& file);

private:
    void AddError(const Stroka& file, const Stroka& error);
    void AddError(const NTomita::TFileLocation& location, const Stroka& error);
    void AddRuleError(const NTomita::TRuleProto& rule, const Stroka& error);

    bool BuildFile(NTomita::TFileProto& file);
    bool BuildGrammar(const NTomita::TFileProto& file, CWorkGrammar* target);

    // Removes rules never referenced within a right part of any rule.
    bool RemoveUnrefRules(NTomita::TFileProto& file);

    bool CheckRules(NTomita::TFileProto& file);
    bool CheckUndefinedTerms(NTomita::TFileProto& file);
    bool CheckUniqRules(NTomita::TFileProto& file);

    bool CheckMainWords(NTomita::TRuleProto* rule);
    bool CheckEpsilonErrors(const NTomita::TRuleProto& rule);
    bool CheckBinaryAgreements(const NTomita::TRuleProto& rule);
    bool CheckGztFields(NTomita::TRuleProto* rule);

    bool FixTerminalInterps(NTomita::TFileProto& file);
    bool DowngradePostfixes(NTomita::TFileProto& file);
    bool KleeneTransform(NTomita::TFileProto& file);

    void CollectDependencies(const TStringBuf& baseDir, const NTomita::TFileProto& file, CWorkGrammar* target);

private: // data //

    bool HadErrors;
    THolder<TErrorCollector> ErrorCollector;
    TOutputStream& Log;

    // external dictionaries for name validation
    const IArticleNames* Articles;
    const IFactTypeDictionary* FactTypes;
    const ITerminalDictionary* Terminals;

    DECLARE_NOCOPY(TTomitaCompiler);
};
