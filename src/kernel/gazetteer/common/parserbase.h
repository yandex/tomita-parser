// The following code includes modified parts of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Recursive descent FTW.

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
