#include "parserbase.h"

#include <util/charset/wide.h>
#include <util/charset/utf.h>


// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false


namespace NGzt {


TParserBase::TParserBase()
    : Input(NULL)
    , ErrorCollector(NULL)
    , HadErrors(false)
    , Encoding(CODES_UNKNOWN)
    , CodePage_(NULL)
{
}

// special constructor for creating inner parser
TParserBase::TParserBase(const TParserBase* parser)
    : Input(NULL)
    , ErrorCollector(parser->ErrorCollector)
    , HadErrors(false)
    , Encoding(parser->Encoding)         // use parent encoding as default encoding
    , CodePage_(parser->CodePage_)
{
}

void TParserBase::SetEncoding(ECharset encoding) {
    // treat next non-ASCII symbols in this encoding
    if (encoding != Encoding) {
        Encoding = encoding;
        CodePage_ = ::SingleByteCodepage(encoding) ? CodePageByCharset(encoding) : NULL;
    }
}

bool TParserBase::Finish() {
    Input = NULL;
    return !HadErrors;
}

void TParserBase::AddError(int line, int column, const TProtoStringType& message)
{
    if (ErrorCollector != NULL)
        ErrorCollector->AddError(CurrentFile_, line, column, HadErrors ? message : "\n" + message);
    HadErrors = true;
}

void TParserBase::AddWarning(int line, int column, const TProtoStringType& message) {
    // same as in AddError, but HadErrors flag is not set
    if (ErrorCollector != NULL)
        ErrorCollector->AddError(CurrentFile_, line, column, HadErrors ? message : "\n" + message);
}


bool TParserBase::RecodeToUtf8(Stroka& text)
{
    if (Encoding == CODES_UNKNOWN) {
        if (!IsStringASCII(~text, ~text + +text)) {
            AddError(Substitute("Unknown encoding: \"$0\".", text));
            return false;
        }

    } else if (Encoding == CODES_UTF8) {
        if (!IsUtf(text)) {
            AddError(Substitute("Invalid utf8: \"$0\".", text));
            return false;
        }

    } else {
        try {
            CharToWide(text, RecodeBuffer, Encoding);
            WideToUTF8(RecodeBuffer, text);
        } catch (yexception&) {
            AddError(Substitute("Cannot recode from $0 to utf8: \"$1\".", NameByCharset(Encoding), text));
            return false;
        }
    }

    return true;
}


bool TParserBase::TryConsumeBOM()
{
    // EF BB BF
    if (TryConsume('\xEF')) {
        if (TryConsume('\xBB'))
            if (TryConsume('\xBF'))
                return true;
        AddError("Found incomplete Byte Order Mark.");
    }
    return false;
}

bool TParserBase::Consume(const char* text, const char* error)
{
    if (TryConsume(text))
        return true;
    else {
        AddError(error);
        return false;
    }
}

bool TParserBase::Consume(const char* text)
{
    if (TryConsume(text))
        return true;
    else {
        AddError(Substitute("Expected \"$0\".", text));
        return false;
    }
}

bool TParserBase::ConsumeIdentifier(Stroka* output, const char* error)
{
    output->clear();
    return ConsumeIdentifierAppend(output, error);
}

bool TParserBase::ConsumeIdentifierAppend(Stroka* output, const char* error)
{
    if (LookingAtType(Tokenizer::TYPE_IDENTIFIER)) {
        // To avoid allocating new memory blocks for CurrentToken().text in subsequent Input->Next()
        // do explicit copying (without aliasing) into @output

        const Stroka& current_text = CurrentToken().text;
        output->AppendNoAlias(~current_text, +current_text);

        Input->Next();
        return true;
    } else {
        AddError(error);
        return false;
    }
}


bool TParserBase::ConsumeInteger(int* output, const char* error)
{
    if (LookingAtType(Tokenizer::TYPE_INTEGER)) {
        ui64 value = 0;
        if (!Tokenizer::ParseInteger(CurrentToken().text, NProtoBuf::kint32max, &value)) {
            AddError("Integer out of range.");
            // We still return true because we did, in fact, parse an integer.
        }
        *output = (int)value;
        NextToken();
        return true;
    } else {
        AddError(error);
        return false;
    }
}

bool TParserBase::ConsumeInteger64(ui64 max_value, ui64* output, const char* error)
{
    if (LookingAtType(Tokenizer::TYPE_INTEGER)) {
        if (!Tokenizer::ParseInteger(CurrentToken().text, max_value, output)) {
            AddError("Integer out of range.");
            // We still return true because we did, in fact, parse an integer.
            *output = 0;
        }
        NextToken();
        return true;
    } else {
        AddError(error);
        return false;
    }
}

bool TParserBase::ConsumeNumber(double* output, const char* error)
{
    if (LookingAtType(Tokenizer::TYPE_FLOAT)) {
        *output = Tokenizer::ParseFloat(CurrentToken().text);
        NextToken();
        return true;
    } else if (LookingAtType(Tokenizer::TYPE_INTEGER)) {
        // Also accept integers.
        ui64 value = 0;
        if (!Tokenizer::ParseInteger(CurrentToken().text, NProtoBuf::kuint64max, &value)) {
            AddError("Integer out of range.");
            // We still return true because we did, in fact, parse a number.
        }
        *output = (double)value;
        NextToken();
        return true;
    } else {
        AddError(error);
        return false;
    }
}

bool TParserBase::ConsumeString(Stroka* output, const char* error)
{
    if (LookingAtType(Tokenizer::TYPE_STRING)) {
        Tokenizer::ParseString(CurrentToken().text, output);
        NextToken();
        return true;
    } else {
        AddError(error);
        return false;
    }
}

bool TParserBase::ConsumeUtf8String(Stroka* output, const char* error)
{
    DO(ConsumeString(output, error));
    return RecodeToUtf8(*output);
}

bool TParserBase::ConsumeDoubleSymbol(const char* symbols, const char* error)
{
    YASSERT(strlen(symbols) == 2);

    // symbols should follow one another without whitespace between,
    // so we are checking positions.
    if (!TryConsume(symbols[0]) || !AdjacentToPrevious() || !TryConsume(symbols[1])) {
        AddError(error);
        return false;
    }
    return true;
}

bool TParserBase::ConsumeDelimitedIdentifier(Stroka* output, const char delimiter, const char* error)
{
/*
    // First piece should be an identifier, not @delimiter
    if (!LookingAtType(Tokenizer::TYPE_IDENTIFIER)) {
        AddError(error);
        return false;
    }

    Stroka piece;

    do {
        if (LookingAtType(Tokenizer::TYPE_IDENTIFIER)) {
            DO(ConsumeIdentifier(&piece, error));
            *output += piece;
        } else if (TryConsume(delimiter)) {
            *output += delimiter;
        } else
            break;
    } while (AdjacentToPrevious());
*/

    output->clear();
    do {
        if (!ConsumeIdentifierAppend(output, error))
            return false;
        if (!AdjacentToPrevious() || !TryConsume(delimiter))
            return true;
        output->append(delimiter);
    }
    while (AdjacentToPrevious());

    return true;
}

bool TParserBase::ConsumeExtendedIdentifier(Stroka* output, const char* error)
{
    // Always return UTF8

    // First piece should be an identifier, non-ASCII alpha character or underscore
    if (!LookingAtType(Tokenizer::TYPE_IDENTIFIER) && !LookingAt('_') &&
        !LookingAtNonAsciiAlpha()) {
        AddError(error);
        return false;
    }

    output->clear();
    do {
        if (LookingAtNonAscii()) {
            if (CodePage_ == NULL) {
                AddError(Substitute("Unquoted non-ASCII characters allowed only for single-byte encodings (e.g. cp1251).\n"
                                    "Current encoding: $0.", NameByCharset(Encoding)));
                return false;
            } else if (!CodePage_->IsAlpha(CurrentToken().text[0]))
                break;
        } else if (LookingAtType(Tokenizer::TYPE_FLOAT)) {
            // We don't accept syntax like "blah.123".
            AddError("Need space between unquoted identifier and decimal point.");
            return false;
        } else if (!LookingAtType(Tokenizer::TYPE_IDENTIFIER) &&
                   !(LookingAtType(Tokenizer::TYPE_INTEGER) && ::IsAlnum(CurrentToken().text[0])) &&
                   !LookingAt('_'))
            break;

        *output += CurrentToken().text;
        NextToken();

    } while (AdjacentToPrevious());

    YASSERT(!output->empty());
    return RecodeToUtf8(*output);
}

bool TParserBase::ConsumeExtendedIdentifierOrString(Stroka* output, const char* error)
{
    // always returns UTF8
    if (LookingAtType(Tokenizer::TYPE_STRING)) {
        Tokenizer::ParseString(CurrentToken().text, output);
        NextToken();
        return RecodeToUtf8(*output);
    } else
        return ConsumeExtendedIdentifier(output, error);
}


}   // namespace NGzt

#undef DO

