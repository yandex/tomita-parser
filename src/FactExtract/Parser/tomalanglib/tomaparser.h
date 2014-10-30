#pragma once

// Implements parsing of .tom (.cxx) file to NTomita::TFileProto.

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/stream/input.h>
#include <util/charset/codepage.h>

#include <contrib/libs/protobuf/compiler/parser.h>
#include <contrib/libs/protobuf/compiler/importer.h>
#include <contrib/libs/protobuf/io/tokenizer.h>

#include "abstract.h"
#include <FactExtract/Parser/tomalanglib/tomasyntax.pb.h>

#include <kernel/gazetteer/common/parserbase.h>


// Defined in this file.
class TTomitaParser;

class TTomitaParser: private NGzt::TParserBase {
    typedef NGzt::TParserBase TBase;
public:
    TTomitaParser();
    virtual ~TTomitaParser();

    // Parse the entire input and construct a NTomita::TFileProto representing
    // it. Returns true if no errors occurred, false otherwise.
    // Skip BOM (Byte Order Mark) at the beginning of @input if present (setting encoding to utf8)
    bool Parse(TInputStream* input, NTomita::TFileProto& file);
    bool Parse(const Stroka& inputFile, NTomita::TFileProto& file);

    // Optional features:

    // Requests that errors be recorded to the given ErrorCollector while
    // parsing.  Set to NULL (the default) to discard error messages.
    using TBase::RecordErrorsTo;

    // If some kwtype (kwset) is parsed, the extracted name will be checked
    // against @dictionary. If not found, an error will be printed.
    void CheckArticlesIn(const IArticleNames* articles) {
        Articles = articles;
    }

    // Same for fact-types
    void CheckFactTypesIn(const IFactTypeNames* factTypes) {
        FactTypes = factTypes;
    }

    // Same for terminals
    void CheckTerminalsIn(const ITerminalDictionary* terminals);

private:
    bool ParseFile(const Stroka& inputFile, NTomita::TFileProto& file);
    bool ParseTokens(::google::protobuf::io::Tokenizer* input, NTomita::TFileProto& file);

    virtual void NextToken();

    // =================================================================
    // Error recovery helpers

    // Consume the rest of the current high-level statement.  This consumes tokens
    // until it sees one of:
    //   ';'  Consumes the token and returns. Only outside of a block.
    //   EOF  Returns (can't consume).
    // The Parser often calls SkipStatement() after encountering a syntax
    // error.  This allows it to go on parsing the following lines, allowing
    // it to report more than just one error in the file.
    void SkipStatement();

    bool TrySkipBlock();
    void SkipRestOfBlock(char block_close);

    // Consumes constructs like '${var}'
    // if 'var' is a name of variable (defined with #def earlier) prepend current token stream with this variable's replacement tokens
    bool TryConsumeVariableAndReplace();


    // in order for Tokenizer to have access to ::google::protobuf::io::ErrorCollector interface
    friend class ::google::protobuf::io::Tokenizer;


    void RecordLocation(NTomita::TFileLocation* location) const;

    // =================================================================
    // Parsers for various language constructs

    // These methods parse various individual bits of code.  They return
    // false if they completely fail to parse the construct.  In this case,
    // it is probably necessary to skip the rest of the statement to recover.
    // However, if these methods return true, it does NOT mean that there
    // were no errors; only that there were no *syntax* errors.  For instance,
    // if a service method is defined using proper syntax but uses a primitive
    // type as its input or output, ParseMethodField() still returns true
    // and only reports the error by calling AddError().  In practice, this
    // makes logic much simpler for the caller.

    // Parse a single top-level statement: macro or rule definition.
    bool ParseTopLevelStatement(NTomita::TFileProto* file);

    bool ParseMacro(NTomita::TFileProto* file);
    bool ParseRule(NTomita::TFileProto* file);

    // conditional macros
    bool ParseIfdef(bool negative);
    bool ParseElse();
    bool ParseEndif();
    bool ParseDefine();
    bool ParseUndef();

    inline bool CheckNewLineAfterMacro();
    inline bool CheckNoEOFInsideMacro();
    inline bool CheckNoNewLineInsideMacro();
    inline bool CheckNoNewLineInsideMacro(int line);
    bool ConsumeMacroBeginning();

    inline bool ConsumeHyphenedIdentifier(Stroka* output, const char* error) {
        return ConsumeDelimitedIdentifier(output, '-', error);
    }

    // other macros
    bool ParseInclude(NTomita::TFileProto* file);
    bool ParseEncoding(NTomita::TFileProto* file);
    bool ParseGrammarKWSet(NTomita::TFileProto* file);
    bool ParseFilter(NTomita::TFilterProto* filter);
    bool ParseFilterItem(NTomita::TFilterProto* filter, int line);

    // rules
    bool ParseRuleBody(NTomita::TRuleProto* rule);
    bool ParseRuleItem(NTomita::TRuleItemProto* item);
    bool ParseRuleReplacement(NTomita::TRuleItemProto* item);
    bool ParseRuleInfo(NTomita::TRuleInfoProto* info);
    bool ParseRuleInfoOption(NTomita::TRuleInfoProto* info);

    // interp
    bool ParseInterp(NTomita::TRuleItemProto* item);
    bool ParseInterpItem(NTomita::TInterpProto* interp);
    bool ParseInterpAlg(NTomita::TInterpProto* interp);

    // right-part items ("terms") with postfix
    bool ParseTerm(NTomita::TTermProto* term);
    bool ParseTermPostfixItem(NTomita::TPostfixProto* postfix);
    bool ParseKeywords(NTomita::TKeyWordsProto* keywords);
    bool ParseGram(NTomita::TPostfixProto* postfix);
    bool ParseGrammemUnion(NTomita::TPostfixProto* postfix);
    bool ParseRegex(NTomita::TRegexProto* regex, NTomita::TRegexProto::EWord word);
    bool ParseComplexPostfixItem(NTomita::TPostfixProto* postfix);

    // misc
    bool ParseKWType(Stroka* kwtype);
    bool ParseAgrType(NTomita::EAgrType* agr);
    bool ParseGrammemsString(Stroka* posGrammems, Stroka* negGrammems = NULL);   // result @grammems are encoded as raw bytes (TGramBitSet::ToBytes())
    bool ParseGrammemsList(Stroka* grammems, Stroka* negGrammems = NULL);
    bool RecodeAndSplitGrammems(Stroka* grammems, Stroka* negGrammems = NULL);
    bool ParseRegexInSlashes(Stroka* regex);

    void CheckFactType(const Stroka& fact, const Stroka field);

    bool IsTerminalName(const Stroka& name) {
        return Terminals->IsTerminal(name);
    }

private:
    DECLARE_NOCOPY(TTomitaParser);
    TTomitaParser(const TTomitaParser* parser);
    void SwapDefinedVars(TTomitaParser& parser);

private:
    // =================================================================

    class TSubstTokenizer;
    TSubstTokenizer* SubstInput;        // TBase::TInput will point to the same object

    const IArticleNames* Articles;
    const IFactTypeNames* FactTypes;
    const ITerminalDictionary* Terminals;
    bool HadNoArticlesError, HadNoFactTypesError;

    Stroka BaseDir;     // directory part of the root grammar file path

    Wtroka TmpUtfRecodeBuffer;

    ymap<Stroka, yvector<TToken> > DefinedVars;     // Current set of variables, defined with macro #define
    yvector<bool> IfDefStack;                       // Current stack of opened (and not yet closed) #ifdef macros.
                                                    // Each value is same as corresponding #ifdef condition was evaluated.

};

// list of agreement abbreviations, accessed via Singleton
class TTomitaAgreementNames {
public:
    TTomitaAgreementNames();

    static bool GetAgr(const Stroka& name, NTomita::EAgrType* res);
    static const Stroka& GetName(NTomita::EAgrType agr);

private:
    ymap<Stroka, NTomita::EAgrType> NameToAgr;
    ymap<NTomita::EAgrType, Stroka> AgrToName;
};
