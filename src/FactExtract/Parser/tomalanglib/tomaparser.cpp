#include "tomaparser.h"

#include <util/generic/buffer.h>
#include <util/generic/ylimits.h>
#include <util/generic/map.h>
#include <util/charset/wide.h>
#include <util/charset/utf.h>
#include <util/string/strip.h>
#include <util/stream/file.h>
#include <contrib/libs/protobuf/stubs/substitute.h>
#include <contrib/libs/protobuf/messagext.h>

#include <library/lemmer/dictlib/grammarhelper.h>

#include <kernel/gazetteer/common/protohelpers.h>
#include <kernel/gazetteer/common/tokenizer.h>
#include <kernel/gazetteer/common/pushinput.h>

#include <FactExtract/Parser/common/toma_constants.h>
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/afglrparserlib/agreement.h>


// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false

using namespace ::google;
using ::google::protobuf::io::Tokenizer;
using ::google::protobuf::strings::Substitute;


class TTomitaParser::TSubstTokenizer: public NGzt::TTokenizer {
    typedef NGzt::TTokenizer TBase;
public:
    TSubstTokenizer(Tokenizer* slave)
        : TBase(slave)
        , SubstIndex(0)
        , SubstOffset(0)
    {
    }

    void Next() {
        // hiding base method
        if (EXPECT_TRUE(SubstIndex >= SubstTokens.size()))
            TBase::Next();
        else
            SubstNext();
    }

    void SubstNext() {
        YASSERT(SubstIndex < SubstTokens.size());

        StoreCurrentPosition();
        // if this is the last token of current substitution, remember total substitution offset
        // in order to calculate correctly next token SpaceBefore
        if (SubstIndex + 1 == SubstTokens.size())
            TBase::NextColumnOffset = SubstOffset;     // will be applied to PrevTokenColumnStop on next Next()
        TBase::CurrentToken = &SubstTokens[SubstIndex++];
    }

    bool SubstFrom(const TToken& fromToken, size_t spaceBefore, const yvector<TToken>& subst) {
        // do not allow insertion to the middle (subst inside subst)
        if (SubstIndex < SubstTokens.size())
            return false;

        if (SubstIndex >= SubstTokens.size() && SubstTokens.size() > 0 && TBase::CurrentToken != &SubstTokens.back())
            SubstTokens.clear();
        SubstIndex = SubstTokens.size();
        for (size_t i = 0; i < subst.size(); ++i) {
            SubstTokens.push_back(subst[i]);
            SubstTokens.back().line = fromToken.line;
            SubstTokens.back().column = fromToken.column + subst[i].column - subst[0].column;
        }
        // calculate a difference in column due to substitution
        int origLen = Current().column + Current().text.size() - fromToken.column;
        int substLen = subst.empty() ? 0 : subst.back().column + subst.back().text.size() - subst[0].column;
        SubstOffset = origLen - substLen;

        // switch to the first token of substitution
        Next();

        // restore spaceBefore as it was before substitution
        TBase::PrevTokenColumnStop = Current().column;
        if (AtSameLine())
            TBase::PrevTokenColumnStop -= spaceBefore;

        return true;
    }

private:
    // Substitution as a source of tokens
    yvector<TToken> SubstTokens;
    size_t SubstIndex;
    int SubstOffset;
};


// ===================================================================

TTomitaParser::TTomitaParser()
    : SubstInput(NULL)
    , Articles(NULL)
    , FactTypes(NULL)
    , Terminals(Singleton<TDefaultTerminalDictionary>())
    , HadNoArticlesError(false), HadNoFactTypesError(false)
{
    SetEncoding(CODES_WIN);     // default encoding for Tomita
}

TTomitaParser::~TTomitaParser() {
}

void TTomitaParser::CheckTerminalsIn(const ITerminalDictionary* terminals) {
    // use TDefaultTerminalDictionary is no terminals specified
    Terminals = terminals != NULL ? terminals : Singleton<TDefaultTerminalDictionary>();
}

inline void TTomitaParser::NextToken()      // virtual
{
    SubstInput->Next();
    TryConsumeVariableAndReplace();
}

bool TTomitaParser::TryConsumeVariableAndReplace()
{
    if (EXPECT_FALSE(LookingAt('$'))) {
        yvector<TToken> skipped;
        skipped.push_back(CurrentToken());
        size_t spaceBefore = SubstInput->SpaceBefore();

        SubstInput->Next();
        skipped.push_back(CurrentToken());
        if (LookingAt('{') && AdjacentToPrevious()) {
            SubstInput->Next();
            skipped.push_back(CurrentToken());

            if (LookingAtType(Tokenizer::TYPE_IDENTIFIER)) {
                // From this point do not restore skipped tokens and produce a syntax error message if something is wrong
                if (!AdjacentToPrevious()) {
                    AddError("Expected variable name immediately after opening brace, e.g. \"${varname}\".");
                    return false;
                }
                Stroka varName;
                DO(ConsumeIdentifier(&varName, "Expected variable name defined with #def macro."));
                ymap<Stroka, yvector<TToken> >::const_iterator it = DefinedVars.find(varName);
                if (it != DefinedVars.end()) {
                    if (!LookingAt('}') || !AdjacentToPrevious()) {
                        AddError("Expected closing brace immediately after variable name, e.g. \"${varname}\".");
                        return false;
                    }
                    if (SubstInput->SubstFrom(skipped[0], spaceBefore, it->second))
                        return true;
                    else {
                        AddError("Nested substitutions are not allowed.");
                        return false;
                    }
                } else {
                    AddError(Substitute("Undefined variable $${$0}. Use #define macro to define substitution variables.", varName));
                    return false;
                }
            }
        }
        SubstInput->SubstFrom(skipped[0], spaceBefore, skipped);
        // still return true: that was something else, not a substitution var and these tokens will be parsed again.
    }
    return true;
}

void TTomitaParser::RecordLocation(NTomita::TFileLocation* location) const
{
    location->SetFile(CurrentFile());
    location->SetLine(CurrentToken().line);
    location->SetColumn(CurrentToken().column);
}

// -------------------------------------------------------------------

void TTomitaParser::SkipStatement()
{
    while (true) {
        if (AtEnd() || TryConsume(';'))
            return;
        else if (LookingAtType(Tokenizer::TYPE_SYMBOL) && TrySkipBlock())
            continue;
        NextToken();
    }
}

bool TTomitaParser::TrySkipBlock()
{
    if (TryConsume('('))
        SkipRestOfBlock(')');

    else if (TryConsume('['))
        SkipRestOfBlock(']');

    else if (TryConsume('<'))
        SkipRestOfBlock('>');

    else if (TryConsume('{'))
        SkipRestOfBlock('}');

    else
        return false;

    return true;
}

void TTomitaParser::SkipRestOfBlock(char block_close)
{
    while (true) {
        if (AtEnd())
            return;
        else if (LookingAtType(Tokenizer::TYPE_SYMBOL)) {
            if (TryConsume(block_close))
                return;
            else if (TrySkipBlock())
                continue;
        }
        NextToken();
    }
}

// ===================================================================

bool TTomitaParser::ParseTokens(Tokenizer* input, NTomita::TFileProto& file)
{
    TSubstTokenizer tokenizer(input);
    SubstInput = &tokenizer;
    Start(SubstInput);

    // Repeatedly parse statements until we reach the end of the file.
    while (!AtEnd()) {
        if (LookingAtMacro()) {
            if (!ParseMacro(&file))
                // This macro failed to parse. Skip it, but keep looping to parse
                // other statements.
                SkipLine();
        } else if (!ParseTopLevelStatement(&file)) {
            // This rule failed to parse. Skip it, but keep looping to parse
            // other statements.
            SkipStatement();
        }
    }

    SubstInput = NULL;
    return Finish();
}

bool TTomitaParser::Parse(TInputStream* input, NTomita::TFileProto& file)
{
    TSkipBOMInput skipBOM(input);
    if (skipBOM.HasBOM())
        SetEncoding(CODES_UTF8);

    protobuf::io::TCopyingInputStreamAdaptor inputAdaptor(&skipBOM);
    Tokenizer tokens(&inputAdaptor, this);
    return ParseTokens(&tokens, file);
}

bool TTomitaParser::Parse(const Stroka& inputFile, NTomita::TFileProto& file)
{
    // the root of grammar
    TStringBuf dir, base;
    PathHelper::Split(inputFile, dir, base);
    BaseDir = ::ToString(dir);
    file.SetName(::ToString(base));
    return ParseFile(file.GetName(), file);
}

bool TTomitaParser::ParseFile(const Stroka& inputFile, NTomita::TFileProto& file)
{
    Stroka path = PathHelper::Join(BaseDir, inputFile);
    if (!PathHelper::Exists(path)) {
        AddError("Cannot find path: " + path);
        return false;
    }

    SetEncoding(CODES_WIN);
    SetCurrentFile(inputFile);

    TIFStream input(path);
    return Parse(&input, file);
}

bool TTomitaParser::ParseTopLevelStatement(NTomita::TFileProto* file)
{
    if (TryConsume(';'))
        // empty statement; ignore
        return true;

    if (LookingAtType(Tokenizer::TYPE_IDENTIFIER))
        return ParseRule(file);
    else {
        AddError("Expected top-level statement: tomita rule definition.");
        return false;
    }
}

bool TTomitaParser::ParseMacro(NTomita::TFileProto* file)
{
    if (!ConsumeMacroBeginning()) {
        AddError("No macro name after '#'.");
        return false;
    }

    // conditional macros:
    if (LookingAt("ifdef"))
        return ParseIfdef(false);
    else if (LookingAt("ifndef"))
        return ParseIfdef(true);
    else if (LookingAt("else"))
        return ParseElse();
    else if (LookingAt("endif"))
        return ParseEndif();
    else if (LookingAt("define"))
        return ParseDefine();
    else if (LookingAt("undef"))
        return ParseUndef();

    // functional macros:
    else if (LookingAt("include"))
        return ParseInclude(file);
    else if (LookingAt("encoding"))
        return ParseEncoding(file);
    else if (LookingAt("GRAMMAR_KWSET"))
        return ParseGrammarKWSet(file);
    else if (LookingAt("filter"))
        return ParseFilter(file->AddFilter());

    else if (TryConsume("GRAMMAR_ROOT") || TryConsume("root")) {
        Stroka root;
        DO(CheckNoNewLineInsideMacro());
        DO(ConsumeIdentifier(&root, "Expected a name of root rule."));
        file->SetGrammarRoot(root);
    } else if (TryConsume("10_FIOS_LIMIT"))
        file->SetFioLimit(10);
    else if (TryConsume("FIO_LIMIT")) {
        int fio_limit = 0;
        DO(CheckNoNewLineInsideMacro());
        DO(ConsumeInteger(&fio_limit, "Expected an integer for FIO limit."));
        file->SetFioLimit(fio_limit);
    } else if (TryConsume("NO_INTERPRETATION"))
        file->SetNoInterp(true);
    else if (TryConsume("DEBUG"))
        file->SetDebug(true);

    else {
        // not a known macro
        if (LookingAtType(Tokenizer::TYPE_IDENTIFIER))
            AddError("Unknown macro: \"" + CurrentToken().text + "\".");
        else
            AddError("Expected macro name.");
        return false;
    }

    return CheckNewLineAfterMacro();
}

bool TTomitaParser::ConsumeMacroBeginning()
{
    DO(Consume("#"));
    return AdjacentToPrevious();
}

inline bool TTomitaParser::CheckNewLineAfterMacro()
{
    if (!AtEnd() && LookingAtSameLine()) {
        AddError("Expected new line after macro.");
        return false;
    }
    return true;
}

inline bool TTomitaParser::CheckNoEOFInsideMacro()
{
    if (AtEnd()) {
        AddError("Unexpected end of file inside multiword macro.");
        return false;
    }
    return true;
}

inline bool TTomitaParser::CheckNoNewLineInsideMacro()
{
    DO(CheckNoEOFInsideMacro());
    if (LookingAtNewLine()) {
        AddError("Unexpected new line inside multiword macro.");
        return false;
    }
    return true;
}

inline bool TTomitaParser::CheckNoNewLineInsideMacro(int line)
{
    DO(CheckNoEOFInsideMacro());
    if (CurrentToken().line != line) {
        AddError("Unexpected new line inside multiword macro.");
        return false;
    }
    return true;
}

bool TTomitaParser::ParseIfdef(bool negative)
{
    Stroka var;

    if (negative)
        DO(Consume("ifndef"));
    else
        DO(Consume("ifdef"));
    DO(CheckNoNewLineInsideMacro());
    DO(ConsumeIdentifier(&var, "Expected variable name after #ifdef/#ifndef macro."));
    DO(CheckNewLineAfterMacro());

    bool defined = DefinedVars.find(var) != DefinedVars.end();
    IfDefStack.push_back(defined != negative);
    if (IfDefStack.back())
        return true;    // following block is parsed normally until #endif or #else is found.

    // otherwise we skip all lines until next #else or #endif
    while (!AtEnd()) {
        if (LookingAtMacro() && ConsumeMacroBeginning()) {
            if (TryConsume("else"))
                return CheckNewLineAfterMacro();
            else if (LookingAt("endif"))
                return ParseEndif();
        } else
            NextToken();

        SkipLine();

    }
    AddError("Reached end of file within #ifdef/#ifndef block (no corresponding #endif).");
    return false;
}

bool TTomitaParser::ParseElse()
{
    DO(Consume("else"));
    DO(CheckNewLineAfterMacro());
    if (IfDefStack.empty()) {
        AddError("Unmatched #else found (no corresponding #ifdef or #ifndef before).");
        return false;
    }
    // skip all lines until #endif
    while (!AtEnd()) {
        if (LookingAtMacro() && ConsumeMacroBeginning()) {
            if (LookingAt("else")) {
                AddError("Unmatched #else found (inside other #else block).");
                return false;
            } else if (LookingAt("endif"))
                return ParseEndif();
        } else
            NextToken();

        SkipLine();
    }
    AddError("Reached end of file within #else block (no corresponding #endif).");
    return false;
}

bool TTomitaParser::ParseEndif()
{
    DO(Consume("endif"));
    DO(CheckNewLineAfterMacro());
    if (IfDefStack.empty()) {
        AddError("Unmatched #endif found (no corresponding #ifdef or #ifndef before).");
        return false;
    } else {
        IfDefStack.pop_back();
        return true;
    }
}

bool TTomitaParser::ParseDefine()
{
    Stroka varName;
    DO(Consume("define"));
    DO(CheckNoNewLineInsideMacro());
    DO(ConsumeIdentifier(&varName, "Expected variable name after #define macro."));

    // memorize all tokens from here upto new line.
    yvector<TToken>& varTokens = DefinedVars[varName];
    varTokens.clear();
    while (!AtEnd() && LookingAtSameLine()) {
        varTokens.push_back(CurrentToken());
        NextToken();
    }
    return true;
}

bool TTomitaParser::ParseUndef()
{
    Stroka var;
    DO(Consume("undef"));
    DO(CheckNoNewLineInsideMacro());
    DO(ConsumeIdentifier(&var, "Expected variable name after #undef macro."));
    DO(CheckNewLineAfterMacro());
    DefinedVars.erase(var);
    return true;
}


TTomitaParser::TTomitaParser(const TTomitaParser* parser)
    : TBase(parser)
    , SubstInput(NULL)
    , Articles(parser->Articles)
    , FactTypes(parser->FactTypes)
    , Terminals(parser->Terminals)
    , HadNoArticlesError(parser->HadNoArticlesError)
    , HadNoFactTypesError(parser->HadNoFactTypesError)
    , BaseDir(parser->BaseDir)
{
}

void TTomitaParser::SwapDefinedVars(TTomitaParser& parser) {
    ::DoSwap(DefinedVars, parser.DefinedVars);
    ::DoSwap(IfDefStack, parser.IfDefStack);
}

bool TTomitaParser::ParseInclude(NTomita::TFileProto* file)
{
    Stroka* include = file->AddInclude();
    DO(Consume("include"));
    DO(CheckNoNewLineInsideMacro());

    if (LookingAtType(Tokenizer::TYPE_STRING))
        DO(ConsumeString(include, "Expected included file name."));
    else if (TryConsume('<'))
        for (;; NextToken()) {
            DO(CheckNoNewLineInsideMacro());
            if (TryConsume('>'))
                break;
            else
                *include += CurrentToken().text;
        }

    StripString(*include);
    if (include->empty()) {
        AddError("Expected file name after #include macro, either in single/double quotes or angle brackets.");
        return false;
    }
    DO(CheckNewLineAfterMacro());

    // Parse included file right here, as if it content was directly included
    // in host file source text.

    TTomitaParser innerParser(this);
    // pass current defined variables and ifdef stack to included file parser
    SwapDefinedVars(innerParser);
    Stroka tmpGrammarRoot = file->GetGrammarRoot();
/*
    // First, remember current parameters to restore them after processing of included file
    THolder<TSubstTokenizer> tmpInput(SubstInput.Release());
    Stroka tmpCurrentFile = CurrentFile;
    ECharset tmpEncoding = Encoding;
    const CodePage* tmpCodePage = CodePage_;
*/
    if (!innerParser.ParseFile(*include, *file))
        AddError("Error in included file \"" + *include + "\".");

    // restore everything back
    file->SetGrammarRoot(tmpGrammarRoot);
    SwapDefinedVars(innerParser);
/*
    SubstInput.Reset(tmpInput.Release());
    CurrentFile = tmpCurrentFile;
    Encoding = tmpEncoding;
    CodePage_ = tmpCodePage;
*/

    // still return true as we did parsed #include
    return true;
}

bool TTomitaParser::ParseEncoding(NTomita::TFileProto* file)
{
    const char* expected_encoding_msg = "Expected encoding name.";

    DO(Consume("encoding"));
    DO(CheckNoNewLineInsideMacro());

    if (LookingAtType(Tokenizer::TYPE_STRING)) {
        DO(ConsumeString(file->MutableEncoding(), expected_encoding_msg));
        ECharset encoding = CharsetByName(file->GetEncoding().c_str());
        if (encoding == CODES_UNKNOWN) {
            AddError("Unknown encoding: " + file->GetEncoding());
            return false;
        }
        SetEncoding(encoding);

    } else {
        AddError(expected_encoding_msg);
        return false;
    }
    return CheckNewLineAfterMacro();
}

bool TTomitaParser::ParseGrammarKWSet(NTomita::TFileProto* file)
{
    DO(Consume("GRAMMAR_KWSET"));                                                   DO(CheckNoNewLineInsideMacro());
    DO(Consume("["));                                                               DO(CheckNoNewLineInsideMacro());
    do {
        DO(CheckNoNewLineInsideMacro());
        DO(ParseKWType(file->AddExtraKeyword()));
        DO(CheckNoNewLineInsideMacro());
    } while (TryConsume(','));
    DO(Consume("]"));
    return CheckNewLineAfterMacro();
}

bool TTomitaParser::ParseFilter(NTomita::TFilterProto* filter)
{
    int line = CurrentToken().line;

    DO(Consume("filter"));
    DO(CheckNoNewLineInsideMacro());

    // remember current position for parsed filter
    RecordLocation(filter->MutableLocation());

    while (line == CurrentToken().line)
        DO(ParseFilterItem(filter, line));

    return CheckNewLineAfterMacro();
}

bool TTomitaParser::ParseFilterItem(NTomita::TFilterProto* filter, int line)
{
    TryConsume('&');        // optional, from old syntax
    DO(CheckNoNewLineInsideMacro(line));

    NTomita::TFilterItemProto* item = filter->AddItem();
    DO(ParseTerm(item->MutableTerm()));

    if (LookingAtSameLine() && TryConsume('[')) {
        int dist = 0;
        DO(CheckNoNewLineInsideMacro(line));
        DO(ConsumeInteger(&dist, "Expected an integer (filter distance)."));
        DO(CheckNoNewLineInsideMacro(line));
        DO(Consume("]"));

        if (dist >= 0)
            item->SetDistance(dist);
        else
            AddError("Filter distance cannot be less than zero.");   // still return true
    }
    return true;
}

// -------------------------------------------------------------------
// Rules

bool TTomitaParser::ParseRule(NTomita::TFileProto* file)
{
    NTomita::TRuleProto* rule = file->AddRule();
    DO(ConsumeIdentifier(rule->MutableName(), "Expected non-terminal name (left part of the rule)."));
    if (IsTerminalName(rule->GetName())) {
        AddError(Substitute("Cannot use terminal \"$0\" as a name for non-terminal.", rule->GetName()));
        return false;
    }
    DO(ConsumeDoubleSymbol("->", "Expected rule arrow \"->\"."));

    RecordLocation(rule->MutableLocation());
    DO(ParseRuleBody(rule));

    // One rule can have several bodies, separated with '|'.
    // They will be treated as distinct rules with same left part.
    while (TryConsume('|')) {
        NTomita::TRuleProto* rule2 = file->AddRule();
        RecordLocation(rule2->MutableLocation());
        rule2->SetName(rule->GetName());
        DO(ParseRuleBody(rule2));
    }
    DO(Consume(";", "Expected ';' after rule definition."));
    return true;
}

bool TTomitaParser::ParseRuleBody(NTomita::TRuleProto* rule)
{
    DO(ParseRuleItem(rule->AddItem()));
    while (!LookingAt(';') && !LookingAt('|')) {
        if (LookingAt('{'))
            return ParseRuleInfo(rule->MutableInfo());
        DO(ParseRuleItem(rule->AddItem()));
    }
    return true;
}

bool TTomitaParser::ParseRuleItem(NTomita::TRuleItemProto* item)
{
    bool parenth = TryConsume('(');
    DO(ParseTerm(item->MutableTerm()));

    if (TryConsume('+'))
        item->SetOption(parenth ? NTomita::TRuleItemProto::STAR : NTomita::TRuleItemProto::PLUS);
    else if (TryConsume('*'))
        item->SetOption(NTomita::TRuleItemProto::STAR);
    else if (TryConsume('!')) {
        item->SetCoord(true);
        item->SetOption(parenth ? NTomita::TRuleItemProto::STAR : NTomita::TRuleItemProto::PLUS);
    } else
        item->SetOption(parenth ? NTomita::TRuleItemProto::PARENTH : NTomita::TRuleItemProto::SINGLE);

    NTomita::EAgrType agr = NTomita::Any;
    if (TryConsume('[')) {
        DO(ParseAgrType(&agr));
        DO(Consume("]"));
    }
    item->SetAgr(agr);

    if (LookingAt("interp")) {
/*
        if (item->GetTerm().GetType() != NTomita::TTermProto::NON_TERMINAL) {
            AddError("Interpretation is allowed for non-terminals only.");
            return false;
        }
*/
        DO(ParseInterp(item));
    }

    if (LookingAt(':'))
        DO(ParseRuleReplacement(item));

    if (parenth)
        DO(Consume(")"));
    return true;
}

bool TTomitaParser::ParseRuleReplacement(NTomita::TRuleItemProto* item)
{
    Stroka tmpRepl;

    DO(Consume(":"));
    if (LookingAtType(Tokenizer::TYPE_IDENTIFIER)) {
        DO(ConsumeIdentifier(&tmpRepl, "Expected terminal."));
        if (!IsTerminalName(tmpRepl)) {
            AddError("Expected terminal name.");
            return true;
        }
    } else
        DO(ConsumeString(&tmpRepl, "Expected terminal name or string with replacement info."));

    if (item->GetTerm().GetType() == NTomita::TTermProto::NON_TERMINAL) {
        AddWarning(-1, -1, Substitute("Replacement interpretation is assigned to non-terminal \"$0\" and will be ignored.",
                                      item->GetTerm().GetName()));
        return true;
    } else if (tmpRepl.empty()) {
        AddError("Empty replacement.");
        return true;
    } else if (item->GetTerm().GetName() == tmpRepl) {
        item->SetReplace(ToString(ITSELF_INTERP));
        return true;
    }

    int ipower = -1;
    TStringBuf sTokens = TStringBuf(tmpRepl).SubStr(0, tmpRepl.length() - 1);
    switch (tmpRepl.back()) {
        case 'm': ipower = 13; break;
        case 'h': ipower = 11; break;
        case 'D': ipower = 10; break;
        case 'd': ipower = 7; break;
        case 'w': ipower = 5; break;
        case 'M': ipower = 3; break;
        case 'y': ipower = 0; break;
        default: sTokens = tmpRepl;
    }

    sTokens = ::StripString(sTokens);
    while (!sTokens.empty()) {
        TStringBuf tok = sTokens.NextTok('/');
        sTokens = ::StripString(sTokens);

        i64 i = 0;
        try {
            i = ten_pow(::FromString<i64>(tok), ipower);
        } catch (...) {
            item->ClearReplace();
            AddError(Substitute("Invalid replacement \"$0\".", tmpRepl));
            return true;
        }
        if (ipower >= 0) {
            if (i < 0)
                i -= ITSELF_INTERP;
            else
                i += ITSELF_INTERP;
        }
        if (!item->GetReplace().empty())
            item->MutableReplace()->append('\\');
        item->MutableReplace()->append(::ToString(i));
    }
    return true;
}

bool TTomitaParser::ParseRuleInfo(NTomita::TRuleInfoProto* info)
{
    DO(Consume("{"));
    do {
        DO(ParseRuleInfoOption(info));
    } while (TryConsume(','));
    DO(Consume("}"));
    return true;
}

bool TTomitaParser::ParseRuleInfoOption(NTomita::TRuleInfoProto* info)
{
    if (TryConsume("outgram")) {
        DO(Consume("="));
        DO(ParseGrammemsString(info->MutableOutgram()));
    } else if (TryConsume("weight")) {
        double weight = 1.0;
        DO(Consume("="));
        DO(ConsumeNumber(&weight, "Expected a number (weight)."));
        info->SetWeight((float)weight);
    } else if (TryConsume("count")) {
        int count = 0;
        DO(Consume("="));
        DO(ConsumeInteger(&count, "Expected integer (count)."));
        info->SetCount(count);
    } else if (TryConsume("trim"))
        info->SetTrim(true);
    else if (TryConsume("left_dominant"))
        info->SetLeftDominant(true);
    else if (TryConsume("right_recessive"))
        info->SetRightRecessive(true);
    else if (TryConsume("not_hreg_fact"))
        info->SetNotHRegFact(true);

    if (info->GetLeftDominant() && info->GetRightRecessive()) {
        AddError("Cannot have \"left_dominant\" and \"right_recessive\" options at the same time.");
        return false;
    }
    return true;
}

bool TTomitaParser::ParseInterp(NTomita::TRuleItemProto* item)
{
    DO(Consume("interp"));
    DO(Consume("("));

    // From here in case of error, we should carefully skip () block
    // in order to avoid interpretting ';' between interp items as end of rule.
    do {
        if (!ParseInterpItem(item->AddInterp())) {
            SkipRestOfBlock(')');
            return false;
        }
    } while (TryConsume(';'));

    if (!Consume(")")) {
        SkipRestOfBlock(')');
        return false;
    }
    return true;
}

bool TTomitaParser::ParseInterpItem(NTomita::TInterpProto* interp)
{
    interp->SetPlus(TryConsume('+'));

    DO(ConsumeIdentifier(interp->MutableFact(), "Expected fact name."));
    DO(Consume("."));
    DO(ConsumeIdentifier(interp->MutableField(), "Expected field name."));
    CheckFactType(interp->GetFact(), interp->GetField());

    if (TryConsume("from")) {
        // could be: FieldName or FactName.FieldName
        DO(ConsumeIdentifier(interp->MutableFromFact(), "Expected field name after 'from'."));
        if (TryConsume('.')) {
            DO(ConsumeIdentifier(interp->MutableFromField(), "Expected field name."));
            CheckFactType(interp->GetFromFact(), interp->GetFromField());
        } else {
            // oops, that was a not a fact name, but just single field name
            interp->MutableFromField()->swap(*interp->MutableFromFact());
            //CheckFactType(interp->GetFact(), interp->GetFromField()); - no check, the field could be from any fact, built already for current symbols
        }

    } else if (LookingAt(':'))
        DO(ParseInterpAlg(interp));

    else if (TryConsume('=')) {
        if (TryConsume("true"))
            interp->SetAssignedValue("true");
        else if (TryConsume("false"))
            interp->SetAssignedValue("false");
        else
            DO(ConsumeUtf8String(interp->MutableAssignedValue(), "Expected string or bool value (true/false)."));
    }

    // TODO: check compatibility of assigned value.
    return true;
}

bool TTomitaParser::ParseInterpAlg(NTomita::TInterpProto* interp)
{
    DO(ConsumeDoubleSymbol("::", "Expected '::' before interpretation algorithm."));
    do {
        if (TryConsume("norm")) {
            DO(Consume("="));
            DO(ParseGrammemsString(interp->MutableNormGrammems()));
        } else {
            Stroka* alg = interp->AddOption();
            DO(ConsumeIdentifier(alg, "Expected field interpretation algorithm."));
            if (Str2FactAlgorithm(alg->c_str()) == eFactAlgCount) {
                AddError(Substitute("Unknown field interpretation algorithm: $0.", *alg));
                // We still return true because we did, in fact, parse an interp alg name.
            }
        }
    } while (TryConsume('&'));

    return true;
}

bool TTomitaParser::ParseTerm(NTomita::TTermProto* term)
{
    if (LookingAtType(Tokenizer::TYPE_STRING)) {
        DO(ConsumeUtf8String(term->MutableName(), "Expected lemma string."));
        if (term->GetName() == "interp") {
            AddError("Cannot use \"interp\" as non-terminal name.");
            return false;
        }
        term->SetType(NTomita::TTermProto::LEMMA);
    } else {
        DO(ConsumeIdentifier(term->MutableName(), "Expected non-terminal, terminal, or lemma string."));
        if (IsTerminalName(term->GetName()))
            term->SetType(NTomita::TTermProto::TERMINAL);
        else
            term->SetType(NTomita::TTermProto::NON_TERMINAL);
    }

    // call MutablePostfix() here in order to always set 'has' bit
    NTomita::TPostfixProto* postfix = term->MutablePostfix();
    if (TryConsume('<')) {
        do {
            DO(ParseTermPostfixItem(postfix));
        } while (TryConsume(','));
        DO(Consume(">"));
    }
    return true;
}

bool TTomitaParser::ParseTermPostfixItem(NTomita::TPostfixProto* postfix)
{
    // first, trying recognize named options: "name=value"
    // and simple one word options, e.g. "rt"

    // kwtypes and article names
    if (TryConsume("kwtype"))
        return ParseKeywords(postfix->MutableKWType());
    else if (TryConsume("label"))
        // TODO: forbid labels in filters
        return ParseKeywords(postfix->MutableLabel());
    else if (TryConsume("kwset"))
        return ParseKeywords(postfix->MutableKWSet());
    else if (TryConsume("kwsetf")) {
        postfix->MutableKWSet()->SetFirstWord(true);
        return ParseKeywords(postfix->MutableKWSet());
    }
    // grammems
    else if (TryConsume("gram"))
        return ParseGram(postfix);
    else if (TryConsume("GU"))
        // TODO: forbid grammem unions in terminals
        return ParseGrammemUnion(postfix);

    // regex
    else if (TryConsume("wff"))
        return ParseRegex(postfix->MutableRegex(), NTomita::TRegexProto::FIRST);
    else if (TryConsume("wfl"))
        return ParseRegex(postfix->MutableRegex(), NTomita::TRegexProto::LAST);
    else if (TryConsume("wfm"))
        return ParseRegex(postfix->MutableRegex(), NTomita::TRegexProto::MAIN);

    else if (TryConsume("rt"))
        postfix->SetRoot(true);
    else if (TryConsume("cut"))
        postfix->SetExcludeFromNorm(true);
    else if (TryConsume("no_hom"))
        postfix->SetNoHom(true);
    else if (TryConsume("hom")) {
        // TODO: simplify/unify syntax here
        DO(Consume("="));
        DO(Consume("~"));
        DO(Consume("prep"));
        postfix->SetNoPrep(true);
    }

    else if (TryConsume("gztweight")) {
        DO(Consume("="));
        DO(ConsumeString(postfix->MutableGztWeight()->MutableField(), "Expected a field name of gazetteer article."));
    }

    // finally try recognizing other more complex options, like agreements ("~gnc-agr[1]") or quotations ("~l-quoted")
    else
        return ParseComplexPostfixItem(postfix);

    return true;
}

static inline NTomita::TPostfixProto::EOption MakeOption(bool negative)
{
    return negative ? NTomita::TPostfixProto::DENY : NTomita::TPostfixProto::ALLOW;
}

bool TTomitaParser::ParseComplexPostfixItem(NTomita::TPostfixProto* postfix)
{
    bool negative = TryConsume('~');
    if (TryConsume("dict")) {
        if (!negative) {
            AddError("\"dict\" option currently cannot be used without negation: only as \"~dict\".");
            // We still return true because we did, in fact, parse an option.
        }
        postfix->SetBastard(true);
    } else if (TryConsume("lat"))
        postfix->SetLatin(MakeOption(negative));
    else if (TryConsume("name_dic"))
        postfix->SetIsName(MakeOption(negative));
    else if (TryConsume("geo_dic"))
        postfix->SetIsGeo(MakeOption(negative));
    else if (TryConsume("fw"))
        postfix->SetFirstWord(MakeOption(negative));
    else if (TryConsume("mw"))
        postfix->SetMultiWord(MakeOption(negative));

    else {
        Stroka option;
        DO(ConsumeHyphenedIdentifier(&option, "Expected postfix option."));
        NTomita::EAgrType agrType = NTomita::Any;
        if (TTomitaAgreementNames::GetAgr(option, &agrType)) {
            int agrId = 0;
            DO(Consume("[", "Expected agreement id in brackets, e.g. \"[1]\"."));
            DO(ConsumeInteger(&agrId, "Expected integer (agreement id)."));
            DO(Consume("]"));
            NTomita::TAgrProto* agr = postfix->AddAgr();
            agr->SetType(agrType);
            agr->SetNegative(negative);
            agr->SetId(agrId);

        } else if (option == "l-reg" || option == "h-reg" || option == "h-reg1") {
            if (negative) {
                AddError(Substitute("\"$0\" option does not support negation.", option));
                // We still return true because we did, in fact, parse an option.
            } else if (option == "l-reg")
                postfix->SetLReg(true);
            else if (option == "h-reg")
                postfix->SetHReg(true);
            else
                postfix->SetHReg1(true);
        } else if (option == "h-reg2")
            postfix->SetHReg2(MakeOption(negative));

        else if (option == "l-quoted")
            postfix->SetLeftQuoted(MakeOption(negative));
        else if (option == "r-quoted")
            postfix->SetRightQuoted(MakeOption(negative));
        else if (option == "quoted") {
            postfix->SetLeftQuoted(MakeOption(negative));
            postfix->SetRightQuoted(MakeOption(negative));

        } else {
            AddError(Substitute("Unknown postfix option \"$0\".", option));
            return false;
        }
    }
    return true;
}

bool TTomitaParser::ParseKeywords(NTomita::TKeyWordsProto* keywords)
{
    keywords->SetRelation(NTomita::TKeyWordsProto::SAME_TYPE);
    if (TryConsume('<')) {
        if (TryConsume('='))
            keywords->SetRelation(NTomita::TKeyWordsProto::SAME_OR_SUBTYPE);
        else
            keywords->SetRelation(NTomita::TKeyWordsProto::SUBTYPE);
    } else
        DO(Consume("="));

    keywords->SetNegative(TryConsume('~'));
    if (TryConsume('[')) {
        yset<Stroka> sorted;
        do {
            Stroka kw;
            DO(ParseKWType(&kw));
            sorted.insert(kw);
        } while (TryConsume(','));
        DO(Consume("]"));

        for (yset<Stroka>::const_iterator it = sorted.begin(); it != sorted.end(); ++it)
            keywords->AddKeyWord(*it);
    } else
        DO(ParseKWType(keywords->AddKeyWord()));

    if (keywords->GetRelation() != NTomita::TKeyWordsProto::SAME_TYPE) {
        if (keywords->GetNegative())
            AddError("Relation operators ('<' or '<=') cannot be used with negation.");
        else if (keywords->KeyWordSize() > 1)
            AddError("Relation operators ('<' or '<=') cannot be used with multiple kwtypes.");
    }

    return true;
}

bool TTomitaParser::ParseGram(NTomita::TPostfixProto* postfix)
{
    DO(Consume("="));
    NTomita::TGramProto* grammems = postfix->AddGram();
    grammems->SetNegative(TryConsume('~'));

    Stroka neg;     // for syntax like: gram='жен,~род' - split into two gram-bitsets
    DO(ParseGrammemsString(grammems->MutableGrammems(), &neg));
    if (!neg.empty()) {
        if (grammems->GetNegative())
            AddError("Double negation (internal + external) is not allowed.");       // still return true
        else {
            if (!grammems->GetGrammems().empty())
                grammems = postfix->AddGram();
            grammems->SetNegative(true);
            grammems->SetGrammems(neg);
        }
    }

    if (!(postfix->GramSize() == 2 && postfix->GetGram(0).GetNegative() != postfix->GetGram(1).GetNegative()) &&
        !(postfix->GramSize() == 1))
        AddError("Cannot have more than one \"gram=\" option.");

    return true;
}

bool TTomitaParser::ParseGrammemUnion(NTomita::TPostfixProto* postfix)
{
    DO(Consume("="));
    do {
        NTomita::TGramProto* grammems = postfix->AddGramUnion();
        if (TryConsume('~'))
            grammems->SetNegative(true);
        else if (TryConsume('&'))
            grammems->SetMixForms(true);       // mix homonymic forms when checking this GU

        Stroka neg;
        if (LookingAtType(Tokenizer::TYPE_STRING))
            DO(ParseGrammemsString(grammems->MutableGrammems(), &neg));
        else
            DO(ParseGrammemsList(grammems->MutableGrammems(), &neg));

        if (!neg.empty()) {
            if (grammems->GetNegative())
               AddError("Double negation (internal + external) is not allowed.");       // still return true
            else {
                if (!grammems->GetGrammems().empty()) {
                    NTomita::TGramProto* grammems2 = postfix->AddGramUnion();
                    grammems2->SetMixForms(grammems->GetMixForms());
                    grammems = grammems2;
                }
                grammems->SetNegative(true);
                grammems->SetGrammems(neg);
            }
        }
    } while (TryConsume('|'));
/*
    if (!(postfix->GramUnionSize() == 2 && postfix->GetGramUnion(0).GetNegative() != postfix->GetGramUnion(1).GetNegative()) &&
        !(postfix->GramUnionSize() == 1))
        AddError("Cannot have more than one \"GU=\" option.");
*/
    return true;
}

/*
static bool CanBeEncoded(const Wtroka& text, ECharset encoding) {
    Encoder encoder = EncoderByCharset(encoding);
    for (size_t i = 0; i < text.size(); ++i)
        if (encoder.Code(text[i]) == 0)
            return false;
    return true;
}
*/

bool TTomitaParser::ParseRegex(NTomita::TRegexProto* regex, NTomita::TRegexProto::EWord word)
{
    if (!regex->GetMask().empty())
        AddError("Cannot have more than one \"wf* =\" option.");

    DO(Consume("="));
    regex->SetWord(word);
    regex->SetNegative(TryConsume('~'));
    if (LookingAtType(Tokenizer::TYPE_STRING))
        DO(ConsumeUtf8String(regex->MutableMask(), "Expected string with regular expression mask."));
    else
        DO(ParseRegexInSlashes(regex->MutableMask()));

    if (regex->GetMask().empty()) {
        AddError("Empty regular expression.");
        // We still return true here
    } else {
        if (!regex->GetMask().has_prefix("^"))
            regex->MutableMask()->prepend("^");
        if (!regex->GetMask().has_suffix("$"))
            regex->MutableMask()->append("$");

        // just check if it is compatible with regex syntax
        ::UTF8ToWide(regex->GetMask(), TmpUtfRecodeBuffer);
        if (!TRegexMatcher::IsCompatible(TmpUtfRecodeBuffer)) {
            AddError("Syntax error in regular expression: \"" + NStr::DebugEncode(TmpUtfRecodeBuffer) + "\".");
            return false;
        }
    }
    return true;
}

bool TTomitaParser::ParseKWType(Stroka* kwtype)
{
    DO(ConsumeExtendedIdentifierOrString(kwtype, "Expected kw-type or article name."));
    if (Articles == NULL || !(IsNoneKWType(*kwtype) || Articles->HasArticleOrType(UTF8ToWide(*kwtype)))) {
        if (Articles == NULL && !HadNoArticlesError) {
            AddError("Cannot use kw-types within current tomita configuration: no dictionary specified.");
            HadNoArticlesError = true;
        }
        // kwtype name may contain a single * as wildcard
        // TODO: allow more complex masks (regex maybe?)
        size_t p = kwtype->find('*');
        if (p == Stroka::npos)
            AddError(Substitute("Unknown kw-type: $0.", *kwtype));
        // We still return true because we did, in fact, parse kw-type.
    }

    return true;
}

void TTomitaParser::CheckFactType(const Stroka& fact, const Stroka field)
{
    if (FactTypes == NULL || !FactTypes->HasFactField(fact, field)) {
        if (FactTypes == NULL && !HadNoFactTypesError) {
            AddError("Cannot use fact-types within current tomita configuration: no dictionary specified.");
            HadNoFactTypesError = true;
        }

        if (FactTypes == NULL || !FactTypes->HasFactType(fact))
            AddError(Substitute("Unknown fact-type: $0.", fact));
        else
            AddError(Substitute("Fact-type \"$0\" has not field \"$1\".", fact, field));
    }
}

bool TTomitaParser::ParseAgrType(NTomita::EAgrType* agr)
{
    Stroka name;
    DO(ConsumeHyphenedIdentifier(&name, "Expected agreement type name, e.g. \"gnc-agr\"."));
    if (!TTomitaAgreementNames::GetAgr(name, agr)) {
        AddError(Substitute("Unknown agreement type \"$0\".", name));
        // We still return true because we did, in fact, parse an agreement type.
    }
    return true;
}

TTomitaAgreementNames::TTomitaAgreementNames() {
    NameToAgr["any"] = NTomita::Any;
    NameToAgr["gnc-agr"] = NTomita::GenderNumberCase;
    NameToAgr["nc-agr"] = NTomita::NumberCase;
    NameToAgr["c-agr"] = NTomita::Case;
    NameToAgr["gn-agr"] = NTomita::GenderNumber;
    NameToAgr["gc-agr"] = NTomita::GenderCase;
    NameToAgr["sp-agr"] = NTomita::SubjectPredicate;
    NameToAgr["fio-agr"] = NTomita::Fio;
    NameToAgr["fem-c-agr"] = NTomita::FemCase;
    NameToAgr["izf-agr"] = NTomita::Izafet;
    NameToAgr["after-num-agr"] = NTomita::AfterNumber;
    NameToAgr["geo-agr"] = NTomita::Geo;

    for (ymap<Stroka, NTomita::EAgrType>::const_iterator it = NameToAgr.begin(); it != NameToAgr.end(); ++it)
        AgrToName[it->second] = it->first;
}

const Stroka& TTomitaAgreementNames::GetName(NTomita::EAgrType agr) {
    const TTomitaAgreementNames* dict = Singleton<TTomitaAgreementNames>();
    ymap<NTomita::EAgrType, Stroka>::const_iterator it = dict->AgrToName.find(agr);
    YASSERT(it != dict->AgrToName.end());
    return it->second;
}

bool TTomitaAgreementNames::GetAgr(const Stroka& name, NTomita::EAgrType* res) {
    const TTomitaAgreementNames* dict = Singleton<TTomitaAgreementNames>();
    ymap<Stroka, NTomita::EAgrType>::const_iterator it = dict->NameToAgr.find(name);
    if (it == dict->NameToAgr.end())
        return false;

    *res = it->second;
    return true;
}

bool TTomitaParser::RecodeAndSplitGrammems(Stroka* grammems, Stroka* negGrammems)
{
    UTF8ToWide(*grammems, TmpUtfRecodeBuffer);
    Stroka rusGrammems = WideToChar(TmpUtfRecodeBuffer, CODES_WIN);

    TGramBitSet pos, neg;
    TStringBuf buf = ::StripString(TStringBuf(rusGrammems));
    while (!buf.empty()) {
        TStringBuf gr = buf.NextTok(',');
        buf = ::StripString(buf);
        if (gr.empty()) {
            AddError(Substitute("Missing grammem: \"$0\".", *grammems));
            return false;
        }
        bool negative = (gr[0] == '~');
        if (negative)
            gr.Skip(1);

        TGrammar code = TGrammarIndex::GetCode(gr);
        if (code == gInvalid) {
            AddError(Substitute("Unknown grammem \"$0\".", ::ToString(gr)));
            grammems->clear();
            return false;
        }

        if (negative)
            neg.Set(code);
        else
            pos.Set(code);
    }

    if (negGrammems != NULL) {
        negGrammems->clear();
        neg.ToBytes(*negGrammems);
    } else if (neg.any()) {
        AddError("Individual grammem negation is not allowed here.");
        return false;
    }
    grammems->clear();
    pos.ToBytes(*grammems);
    return true;
}

bool TTomitaParser::ParseGrammemsString(Stroka* grammems, Stroka* negGrammems)
{
    DO(ConsumeUtf8String(grammems, "Expected string with grammems."));
    RecodeAndSplitGrammems(grammems, negGrammems);   // Ignore check result as we did, in fact, parse grammems;
    return true;
}

bool TTomitaParser::ParseGrammemsList(Stroka* grammems, Stroka* negGrammems)
{
    Stroka gram;
    grammems->clear();
    DO(Consume("[", "Expected string or bracketed list with grammems."));
    do {
        bool neg = TryConsume('~');
        DO(ConsumeExtendedIdentifierOrString(&gram, "Expected grammem."));
        if (!grammems->empty())
            *grammems += ',';
        if (neg)
            *grammems += '~';
        *grammems += gram;
    } while (TryConsume(','));
    DO(Consume("]"));
    RecodeAndSplitGrammems(grammems, negGrammems);   // Ignore check result as we did, in fact, parse grammems;
    return true;
}

bool TTomitaParser::ParseRegexInSlashes(Stroka* regex)
{
    regex->clear();
    DO(Consume("/", "Expected regular expression."));

    while (LookingAtSameLine()) {
        if (AtEnd()) {
            AddError("Unexpected end of file in regular expression");
            return false;
        }
        // append skipped whitespaces, if any.
        // TODO: take into account possible block comments and tab chars
        regex->append(SubstInput->SpaceBefore(), ' ');

        // end of regular expression.
        if (TryConsume('/'))
            return RecodeToUtf8(*regex);

        // we should skip escaped slash: \/
        if (TryConsume('\\')) {
            if (LookingAtNewLine())
                break;
            // do not allow whitespace after escaping back-slash
            if (SubstInput->SpaceBefore() == 0 && TryConsume('/'))
                *regex += '/';
            else
                *regex += '\\';

        } else {
            *regex += CurrentToken().text;
            NextToken();
        }
    }
    AddError("Unexpected new line in regular expression");
    return false;
}

#undef DO
