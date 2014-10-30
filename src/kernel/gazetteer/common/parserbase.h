#pragma once

#include "tokenizer.h"

#include <util/stream/input.h>
#include <util/charset/codepage.h>

#include <contrib/libs/protobuf/compiler/importer.h>
#include <contrib/libs/protobuf/stubs/substitute.h>


namespace NGzt {

using NProtoBuf::strings::Substitute;


// This code is mostly copied from Protocol Buffers parser
// (located at <contrib/libs/protobuf/compiler/parser.h>)
// and tuned a bit to satisfy our needs.

class TParserBase: public NProtoBuf::io::ErrorCollector {
public:
    TParserBase();
    virtual ~TParserBase() {
    }

    typedef NProtoBuf::io::Tokenizer Tokenizer;
    typedef Tokenizer::Token TToken;
    typedef Tokenizer::TokenType TTokenType;

    // -----------------------------------------------------------------

    template <typename TInputTokenizer>
    void Start(TInputTokenizer* input);

    inline const Stroka& CurrentFile() const {
        return CurrentFile_;
    }

    inline ECharset CurrentEncoding() const {
        return Encoding;
    }

    inline const TToken& CurrentToken() const {
        return Input->Current();
    }

    virtual void NextToken() {
        Input->Next();
    }

    virtual bool Finish();

    virtual void SetEncoding(ECharset encoding);

    virtual void SetCurrentFile(const Stroka& fname) {
        CurrentFile_ = fname;
    }

    // -----------------------------------------------------------------
    // Error logging helpers

    // implements NProtoBuf::io::ErrorCollector
    virtual void AddError(int line, int column, const TProtoStringType& message);
    virtual void AddWarning(int line, int column, const TProtoStringType& message);

    // Invokes AddError() with the line and column number of the current token.
    void AddError(const Stroka& error) {
        if (Input == NULL)
            AddError(-1, -1, error);
        else
            AddError(CurrentToken().line, CurrentToken().column, error);
    }

    //void RecordLocation(NTomita::TFileLocation* location) const;

    // Requests that errors be recorded to the given ErrorCollector while
    // parsing.  Set to NULL (the default) to discard error messages.
    void RecordErrorsTo(NProtoBuf::compiler::MultiFileErrorCollector* error_collector) {
        ErrorCollector = error_collector;
    }


    // Single-token consuming helpers
    // These make parsing code more readable.

    // True if the current token is TYPE_END.
    inline bool AtEnd() const {
        return LookingAtType(Tokenizer::TYPE_END);
    }

    // True if the next token matches the given text or symbol.
    inline bool LookingAt(const char* text) const {
        return CurrentToken().text == text;
    }

    inline bool LookingAt(const TStringBuf& text) const {
        return CurrentToken().text == text;
    }

    inline bool LookingAt(char symbol) const {
        return CurrentToken().text.size() == 1 && CurrentToken().text[0] == symbol;
    }

    // True if the next token is of the given type.
    inline bool LookingAtType(TTokenType token_type) const {
        return CurrentToken().type == token_type;
    }

    inline bool LookingAtNonAscii() const {
        return CurrentToken().type == Tokenizer::TYPE_SYMBOL &&
        static_cast<ui8>(CurrentToken().text[0]) > 0x7F;
    }

    inline bool LookingAtNonAsciiAlpha() const {
        return LookingAtNonAscii() && CodePage_ != NULL &&
        CodePage_->IsAlpha(CurrentToken().text[0]);
    }

    // True if next token is located on same line as current (no new lines between)
    inline bool LookingAtSameLine() const {
        return Input->AtSameLine();
    }

    inline bool LookingAtNewLine() const {
        return !LookingAtSameLine();
    }

    inline bool AdjacentToPrevious() const {
        return LookingAtSameLine() && Input->SpaceBefore() == 0;
    }

    inline bool LookingAtMacro() const {
        return LookingAtNewLine() && LookingAt('#');
    }


    // Consume the rest tokens on current line
    inline void SkipLine() {
        while (!AtEnd() && LookingAtSameLine())
            NextToken();
    }


    // Recodes string to utf-8, using current encoding and internal recoding buffer
    // Not thread-safe
    bool RecodeToUtf8(Stroka& text);

    // If the next token exactly matches the text given (or symbol), consume it and return
    // true.  Otherwise, return false without logging an error.
    inline bool TryConsume(const char* text);
    inline bool TryConsume(char symbol);


    // Try to skip Byte Order Mark (usually at the start of text stream).
    // Return true, if the BOM was actually read, otherwise return false.
    // If there was an incomplete BOM, print an error message.
    bool TryConsumeBOM();

    // These attempt to read some kind of token from the input.  If successful,
    // they return true.  Otherwise they return false and add the given error
    // to the error list.

    // Consume a token with the exact text given.
    bool Consume(const char* text, const char* error);
    // Same as above, but automatically generates the error "Expected \"text\".",
    // where "text" is the expected token text.
    bool Consume(const char* text);
    // Consume a token of type IDENTIFIER and store its text in "output".
    bool ConsumeIdentifier(Stroka* output, const char* error);
    // Consume a token of type IDENTIFIER and append its text to "output".
    bool ConsumeIdentifierAppend(Stroka* output, const char* error);
    // Consume an integer and store its value in "output".
    bool ConsumeInteger(int* output, const char* error);
    // Consume a 64-bit integer and store its value in "output".  If the value
    // is greater than max_value, an error will be reported.
    bool ConsumeInteger64(ui64 max_value, ui64* output, const char* error);
    // Consume a number and store its value in "output".  This will accept
    // tokens of either INTEGER or FLOAT type.
    bool ConsumeNumber(double* output, const char* error);
    // Consume a string literal and store its (unescaped) value in "output". No re-encoding, as is.
    bool ConsumeString(Stroka* output, const char* error);
    // consume string and recode it to UTF8 using current encoding
    bool ConsumeUtf8String(Stroka* output, const char* error);

    bool ConsumeDoubleSymbol(const char* symbols, const char* error);

    // Read sequence of @delimiter separated identifiers as single identifier.
    // The sequence should start with identifier and ends at first non-identifier
    // or non-@delimiter token (also at first whitespace!).
    bool ConsumeDelimitedIdentifier(Stroka* output, const char delimiter, const char* error);


    bool ConsumeExtendedIdentifier(Stroka* output, const char* error);

    // Consume a token of type IDENTIFIER or STRING literal, or unquoted non-ASCII identifiers (e.g. cyrillic alphabet characters)
    bool ConsumeExtendedIdentifierOrString(Stroka* output, const char* error);

protected:
    // special constructor for creating inner parser
    TParserBase(const TParserBase* parser);

private:
    TTokenizer* Input;  // without ownership

    NProtoBuf::compiler::MultiFileErrorCollector* ErrorCollector;
    bool HadErrors;

    Stroka CurrentFile_;

    // Encoding for extended (non-ASCII) identifiers
    ECharset Encoding;
    const CodePage* CodePage_;
    Wtroka RecodeBuffer;
};


// inlined implementations


template <typename TInputTokenizer>
void TParserBase::Start(TInputTokenizer* input) {
    Input = input;
    HadErrors = false;

    if (LookingAtType(Tokenizer::TYPE_START)) {
        // Advance to first token.
        Input->FirstNext();
/*
        // If it is a BOM, consume it and reset default encoding to UTF8
        if (TryConsumeBOM())
            SetEncoding(CODES_UTF8);
*/
    }
}

inline bool TParserBase::TryConsume(const char* text) {
    if (LookingAt(text)) {
        NextToken();
        return true;
    } else
        return false;
}


inline bool TParserBase::TryConsume(char symbol) {
    if (LookingAt(symbol)) {
        NextToken();
        return true;
    } else
        return false;
}


}   // namespace NGzt
