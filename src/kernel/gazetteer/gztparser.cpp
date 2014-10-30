#include "gztparser.h"

#include <util/generic/buffer.h>
#include <util/generic/ylimits.h>
#include <contrib/libs/protobuf/stubs/substitute.h>

#include "protoparser.h"

// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false


namespace NGzt {


void TGztParser::RecordLocation(const NProtoBuf::Message* descriptor,
                                NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation location,
                                int line, int column)
{
    if (SourceLocationTable != NULL)
        SourceLocationTable->Add(descriptor, location, line, column);
}

void TGztParser::RecordLocation(const NProtoBuf::Message* descriptor,
                                NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation location)
{
    RecordLocation(descriptor, location, CurrentToken().line, CurrentToken().column);
}

// -------------------------------------------------------------------

void TGztParser::SkipStatement()
{
    while (true) {
        if (AtEnd())
            return;
        else if (LookingAtType(Tokenizer::TYPE_SYMBOL)) {
            if (TryConsume(';'))
                return;
            else if (TryConsume('{')) {
                SkipRestOfBlock();
                return;
            } else if (LookingAt('}'))
                return;
        }
        NextToken();
    }
}

void TGztParser::SkipRestOfBlock()
{
    while (true) {
        if (AtEnd())
            return;
        else if (LookingAtType(Tokenizer::TYPE_SYMBOL)) {
            if (TryConsume('}'))
                return;
            else if (TryConsume('{'))
                SkipRestOfBlock();
        }
        NextToken();
    }
}

// ===================================================================


bool TGztParser::Parse(Tokenizer* input, TGztFileDescriptorProto* file)
{
    TTokenizer tokenizer(input);
    Start(&tokenizer);
    if (CurrentEncoding() != CODES_UNKNOWN)
        file->set_encoding(NameByCharset(CurrentEncoding()));

    // Repeatedly parse statements until we reach the end of the file.
    while (!AtEnd())
        ParseTopLevelStatementOrSkip(file);

    return Finish();
}

bool TGztParser::ParseNextStatement(Tokenizer* input, TGztFileDescriptorProto* file)
{
    TTokenizer tokenizer(input);
    Start(&tokenizer);
    if (CurrentEncoding() != CODES_UNKNOWN)
        file->set_encoding(NameByCharset(CurrentEncoding()));

    ParseTopLevelStatementOrSkip(file);

    return Finish();
}

bool TGztParser::ParseImports(Tokenizer* input, TGztFileDescriptorProto* file)
{
    TTokenizer tokenizer(input);
    Start(&tokenizer);
    if (CurrentEncoding() != CODES_UNKNOWN)
        file->set_encoding(NameByCharset(CurrentEncoding()));

    // Repeatedly parse statements until we reach the end of the file.
    while (!AtEnd())
    {
        if (TryConsume(';'))
            // empty statement; ignore
            continue;

        if (!LookingAt("import") || !ParseImport(file->add_dependency()))
        {
            // Parse imports or skip whole statement, but keep looping to parse
            // other statements.
            SkipStatement();
            SkipBlockEnd();
        }
    }

    return Finish();
}

void TGztParser::SkipBlockEnd() {
    if (LookingAt('}')) {
        AddError("Unmatched \"}\".");
        NextToken();
    }
}

void TGztParser::ParseTopLevelStatementOrSkip(TGztFileDescriptorProto* file)
{
    if (!ParseTopLevelStatement(file)) {
        // This statement failed to parse.  Skip it, but keep looping to parse
        // other statements.
        SkipStatement();
        SkipBlockEnd();
    }
}

bool TGztParser::ParseTopLevelStatement(TGztFileDescriptorProto* file)
{
    if (TryConsume(';'))
        // empty statement; ignore
        return true;

    else if (LookingAt("import"))
        return ParseImport(file->add_dependency());

    // syntax extension: encoding used for interpreting string literals and article identifiers.
    else if (LookingAt("encoding"))
        return ParseEncoding(file->mutable_encoding());

    // syntax extension: various gzt compilation options
    else if (LookingAt("option"))
        return ParseGztFileOption(file);

    else if (LookingAtType(Tokenizer::TYPE_IDENTIFIER))
        return ParseArticleDefinition(file->add_article());
    else {
        AddError("Expected top-level statement (e.g. article definition).");
        return false;
    }
}

// -------------------------------------------------------------------
// Articles

bool TGztParser::ParseArticleDefinition(TArticleDescriptorProto* article)
{
  RecordLocation(article, NProtoBuf::DescriptorPool::ErrorCollector::OTHER);
  DO(ConsumeType(article->mutable_type(), "Expected article type."));

  if (LookingAt('{'))
      article->mutable_name()->clear(); // allow articles without names
  else {
      if (LookingAtType(Tokenizer::TYPE_STRING)) {
          DO(ConsumeString(article->mutable_name(), "Expected article name."));
      }
      else
          DO(ConsumeIdentifier(article->mutable_name(), "Expected article name."));
  }

  DO(ParseArticleBlock(article->mutable_field()));
  return true;
}

bool TGztParser::ParseArticleBlock(NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto>* article_fields)
{
    DO(Consume("{"));

    bool had_keyword_fields = false;
    while (!TryConsume('}')) {
        if (AtEnd()) {
            AddError("Reached end of input in article definition (missing '}').");
            return false;
        }
        if (!ParseArticleField(article_fields))
            // This statement failed to parse.  Skip it, but keep looping to parse
            // other statements.
            SkipStatement();
        else {
            //check if there is forbidden keyword/positional fields sequence
            bool last_is_keyword_field = article_fields->Get(article_fields->size()-1).has_name();
            if (last_is_keyword_field)
                had_keyword_fields = true;
            else if (had_keyword_fields) {
                AddError("Positional field (i.e. only a value without explicit field name) could not be used after non-positional (named) fields.");
                SkipStatement();
            }
        }

    }
    return true;
}

bool TGztParser::ParseArticleField(NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto>* article_fields)
{
    if (TryConsume(';') || TryConsume(','))
        // empty field; ignore
        return true;
    else {
        //there could be two cases: NAME "=" LIST_OF_VALUES or just LIST_OF_VALUES
        // So first try reading a value, then check if it is just
        // single identifier and next token is "=".
        TArticleFieldDescriptorProto* field = article_fields->Add();
        TFieldValueDescriptorProto* value = field->mutable_value();
        DO(ParseChainedFieldValues(value));
        if (value->type() == TFieldValueDescriptorProto::TYPE_IDENTIFIER && TryConsume('=')) {
            //we have just actually read field name identifier.
            field->mutable_name()->swap(*value->mutable_string_or_identifier());
            // so read value again
            value->Clear();
            DO(ParseChainedFieldValues(value));
        }
        return true;
    }
}

bool TGztParser::ParseChainedFieldValues(TFieldValueDescriptorProto* value)
{
    DO(ParseSingleFieldValue(value));

    // try read several more values, interleaved with "+" or "|"
    if (!LookingAtListDelimiter())
        return true;

    // What was previously read into @value was actually a first item of chained list,
    // not a single value. So transform @value to a list and place its current content
    // as first sub-value of this list.


    // Re-use previuosly allocated items
    THolder<TFieldValueDescriptorProto> sub_value;
    if (value->mutable_list()->mutable_value()->ClearedCount() > 0) {
        sub_value.Reset(value->mutable_list()->mutable_value()->ReleaseCleared());
        sub_value->CopyFrom(*value);
        //sub_value->Swap(value);    -- Swap is unsafe here because it creates cycles for some reason!
    }
    else
        sub_value.Reset(new TFieldValueDescriptorProto(*value));

    value->Clear();

    value->set_type(TFieldValueDescriptorProto::TYPE_LIST);
    value->mutable_list()->mutable_value()->AddAllocated(sub_value.Release());

    // only single kind of separating token is allowed at single chained list.
    // so next we only accept a delimiters of same level which are equal to the first one.
    Stroka delimiter = CurrentToken().text;
    if (delimiter == "|")
        value->mutable_list()->set_type(TValuesListDescriptorProto::PIPE_DELIMITED);
    else if (delimiter == "+")
        value->mutable_list()->set_type(TValuesListDescriptorProto::PLUS_DELIMITED);
    else
        YASSERT(false);

    const char* delim_text = delimiter.c_str();
    while (TryConsume(delim_text))
        DO(ParseSingleFieldValue(value->mutable_list()->add_value()));

    // it is an error to meet any list delimiter here (as it will be mixed with previuos delimiters).
    if (!LookingAtListDelimiter())
        return true;
    else {
        AddError(Substitute("Distinct kinds of delimiters (\"$0\" and \"$1\") "
                            "should not be mixed in a single chained list at same level.",
                            delimiter, CurrentToken().text));
        return false;
    }
}

bool TGztParser::ParseSingleFieldValue(TFieldValueDescriptorProto* value)
{
    bool is_negative = TryConsume('-');

    switch (CurrentToken().type) {
    case Tokenizer::TYPE_START:
        AddError("Trying to read value before any tokens have been read.");
        return false;

    case Tokenizer::TYPE_END:
        AddError("Unexpected end of stream while parsing field value.");
        return false;

    case Tokenizer::TYPE_IDENTIFIER:
        if (is_negative) {
            AddError("Invalid '-' symbol before identifier.");
            return false;
        }
        value->set_type(TFieldValueDescriptorProto::TYPE_IDENTIFIER);
        DO(ConsumeIdentifier(value->mutable_string_or_identifier(), "Expected identifier."));
        return true;

    case Tokenizer::TYPE_INTEGER:
        ui64 number, max_value;
        max_value = is_negative ? static_cast<ui64>(NProtoBuf::kint64max) + 1 : NProtoBuf::kuint64max;
        DO(ConsumeInteger64(max_value, &number, "Expected integer."));
        value->set_type(TFieldValueDescriptorProto::TYPE_INTEGER);
        value->set_int_number(is_negative ? -(i64)number : number);
        return true;

    case Tokenizer::TYPE_FLOAT:
        value->set_type(TFieldValueDescriptorProto::TYPE_FLOAT);
        value->set_float_number(Tokenizer::ParseFloat(CurrentToken().text)  *  (is_negative ? -1 : 1));
        NextToken();
        return true;

    case Tokenizer::TYPE_STRING:
        if (is_negative) {
            AddError("Invalid '-' symbol before string.");
            return false;
        }
        value->set_type(TFieldValueDescriptorProto::TYPE_STRING);
        Tokenizer::ParseString(CurrentToken().text, value->mutable_string_or_identifier());
        NextToken();
        return true;

    case Tokenizer::TYPE_SYMBOL:
        if (LookingAt('{')) {
            if (is_negative) {
                AddError("Invalid '-' symbol before inner block.");
                return false;
            }
            //parse sub-article
            value->set_type(TFieldValueDescriptorProto::TYPE_BLOCK);
            return ParseArticleBlock(value->mutable_sub_field());
        } else if (LookingAt('[')) {
            if (is_negative) {
                AddError("Invalid '-' symbol before list.");
                return false;
            }
            return ParseBracketedValuesList(value);
        }
    }
    AddError("Expected field value.");
    return false;
}

bool TGztParser::ParseBracketedValuesList(TFieldValueDescriptorProto* value)
{
    DO(Consume("["));
    value->set_type(TFieldValueDescriptorProto::TYPE_LIST);
    value->mutable_list()->set_type(TValuesListDescriptorProto::BRACKETED);
    bool last_was_delimiter = false;
    while (last_was_delimiter || !TryConsume(']')) {
        if (AtEnd()) {
            AddError("Unexpected end of stream while parsing bracketed list.");
            return false;
        }
        if (LookingAt('}')) {
            AddError("Missing ']'.");
            return false;
        }
        // Note that a bracketed list could contain any type of sub-value, including sub-lists of any type.
        DO(ParseChainedFieldValues(value->mutable_list()->add_value()));

        // consume items delimiter (if any), but only one.
        if (TryConsume(','))
            last_was_delimiter = true;
        else
            last_was_delimiter = TryConsume(';');
    }
    return true;
}

bool TGztParser::ParseImport(Stroka* import_filename) {
  DO(Consume("import"));
  DO(ConsumeString(import_filename, "Expected a string naming the file to import."));
  DO(Consume(";"));
  return true;
}

bool TGztParser::ParseEncoding(Stroka* encoding_name) {
  DO(Consume("encoding"));
  DO(ConsumeString(encoding_name, "Expected a string naming the encoding to use."));
  ECharset enc = CharsetByName(encoding_name->c_str());
  if (enc == CODES_UNKNOWN) {
      AddError(Substitute("Unrecognized encoding \"$0\".", *encoding_name));
      return false;
  }
  DO(Consume(";"));

  SetEncoding(enc);

  return true;
}

bool TGztParser::ParseGztFileOption(TGztFileDescriptorProto* file) {
  DO(Consume("option"));
  Stroka option_name;
  DO(ConsumeString(&option_name, "Expected an option name."));
  if (option_name == "strip-names" || option_name == "strip_names" || option_name == "strip names")
      file->set_strip_names(true);
  //else if <other option here>
  else {
      AddError(Substitute("Unrecognized option \"$0\".", option_name));
      return false;
  }
  DO(Consume(";"));
  return true;
}


} // namespace NGzt

#undef DO
