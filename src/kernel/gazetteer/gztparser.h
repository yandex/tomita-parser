#pragma once

#include "common/parserbase.h"
#include <kernel/gazetteer/syntax.pb.h>


// Implements parsing of .gzt file to TGztFileDescriptorProto.


namespace NGzt {


// Defined in this file.
class TGztParser;


class TGztParser: private NGzt::TParserBase {
    typedef NGzt::TParserBase TBase;
public:
    TGztParser()
        : SourceLocationTable(NULL)
    {
    }


    typedef NProtoBuf::io::Tokenizer Tokenizer;
    typedef Tokenizer::Token TToken;
    typedef Tokenizer::TokenType TTokenType;

    // Parse the entire input and construct a TGztFileDescriptorProto representing
    // it. Returns true if no errors occurred, false otherwise.
    bool Parse(Tokenizer* input, TGztFileDescriptorProto* file);

    // Parse the first top-level statement from @input and append built data to TGztFileDescriptorProto corresponding field.
    // Returns true if no errors occurred, false otherwise (in this case failed statement is skipped).
    bool ParseNextStatement(Tokenizer* input, TGztFileDescriptorProto* file);

    // Parse all "import" statements from the input and construct a TGztFileDescriptorProto with corresponding dependencies.
    // Returns true if no errors occurred, false otherwise.
    bool ParseImports(Tokenizer* input, TGztFileDescriptorProto* file);


    // Optional features:

    // Requests that locations of certain definitions be recorded to the given
    // SourceLocationTable while parsing.  This can be used to look up exact line
    // and column numbers for errors reported by DescriptorPool during validation.
    // Set to NULL (the default) to discard source location information.
    void RecordSourceLocationsTo(NProtoBuf::compiler::SourceLocationTable* location_table) {
        SourceLocationTable = location_table;
    }

    using TBase::RecordErrorsTo;

private:
    // =================================================================
    // Error recovery helpers

    // Consume the rest of the current high-level statement.  This consumes tokens
    // until it sees one of:
    //   ';'  Consumes the token and returns. Only outside of a block.
    //   '{'  Consumes the brace then calls SkipRestOfBlock().
    //   '}'  Returns without consuming.
    //   EOF  Returns (can't consume).
    // The Parser often calls SkipStatement() after encountering a syntax
    // error.  This allows it to go on parsing the following lines, allowing
    // it to report more than just one error in the file.
    void SkipStatement();

    // Consume the rest of the current block, including nested blocks,
    // ending after the closing '}' is encountered and consumed, or at EOF.
    void SkipRestOfBlock();

    void SkipBlockEnd();


    // True if the next token is one of list delimiters (currently "|" or "+").
    inline bool LookingAtListDelimiter() {
        return LookingAt('|') || LookingAt('+');
    }


    // Consume a sequence of '.'-separated IDENTIFIERs and store its text in "output".
    inline bool ConsumeType(Stroka* output, const char* error) {
        return ConsumeDelimitedIdentifier(output, '.', error);
    }


    // -----------------------------------------------------------------
    // Error logging helpers

    // Record the given line and column and associate it with this descriptor
    // in the SourceLocationTable.
    void RecordLocation(const NProtoBuf::Message* descriptor,
                        NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation location,
                        int line, int column);

    // Record the current line and column and associate it with this descriptor
    // in the SourceLocationTable.
    void RecordLocation(const NProtoBuf::Message* descriptor,
                        NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation location);

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

    // Parse a top-level message, enum, service, etc.
    bool ParseTopLevelStatement(TGztFileDescriptorProto* file);
    void ParseTopLevelStatementOrSkip(TGztFileDescriptorProto* file);

    // Parse top-level article.
    bool ParseArticleDefinition(TArticleDescriptorProto* article);

    // Parse the content of article (or sub-article). Consume the entire block including
    // the beginning and ending brace.
    bool ParseArticleBlock(NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto>* article_fields);

    // Parse a single field of article (i.e. "[name =] value").
    // Consume any field delimiter if any (";" or ",")
    bool ParseArticleField(NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto>* article_fields);

    // Parse a compound value of article's field - it could be a list of values, joined with delimiter "|" or "+"
    // All found delimiters should be of the same type at same level, not mixed.
    bool ParseChainedFieldValues(TFieldValueDescriptorProto* value);

    // Parse a single non-compound value of article's field - i.e. all types of values except chained list.
    bool ParseSingleFieldValue(TFieldValueDescriptorProto* value);

    // Parse a bracketed list of values - this consumes entire list, including brackets.
    bool ParseBracketedValuesList(TFieldValueDescriptorProto* value);

    bool ParseImport(Stroka* import_filename);
    bool ParseEncoding(Stroka* encoding_name);
    bool ParseGztFileOption(TGztFileDescriptorProto* file);

private:
    NProtoBuf::compiler::SourceLocationTable* SourceLocationTable;

    DECLARE_NOCOPY(TGztParser);
};


}  // namespace NGzt
