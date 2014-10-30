#include "protoparser.h"
#include "sourcetree.h"

#include <kernel/gazetteer/base.pb.h>

#include <contrib/libs/protobuf/stubs/substitute.h>

#include <util/string/escape.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>




namespace NGzt {

// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false


class TSimpleHierarchy: public TRefCounted<TSimpleHierarchy> {
public:
    bool SetBase(const Stroka& derived, const Stroka& base) {
        Stroka& p = Index[derived];
        if (p != base) {
            if (!p.empty())
                return false;
            p = base;
        }
        return true;
    }

    const Stroka* GetBase(const Stroka& derived) const {
        yhash<Stroka, Stroka>::const_iterator it = Index.find(derived);
        return it != Index.end() ? &it->second : NULL;
    }

private:
    yhash<Stroka, Stroka> Index;
};


namespace {

// An automation to search a special sequence of tokens in proto-file, corresponding to message inheritance syntax
// "message" Identifier ":" ["."]Identifier ["."] Identifier "{"
class TDerivedMessageTokensAutomation {
public:
    enum TState {
          stInitial                       // prior to any keyword
        , stIntermediate                  // not expecting a keyword
        //next states identifies a token just been read
        , stMessageKeyword                // keyword "message"
        , stPackageKeyword                // keyword "package"
        , stDerivedMessageIdentifier      // derived message identifier
        , stColon                         // symbol ":"
        , stBaseMessageIdentifier         // identifier corresponding to base message, or part of its qualified name
        , stPackageIdentifier             // identifier corresponding to the package, or part of its qualified name
        , stPeriod                        // symbol "." as part of base message qualified name (or package name)
        //final state corresponding to found inheritance sequence
        , stFinished

        , stErrorIdentifierExpected       // there is no identifier after stPeriod or
                                          // there is no identifier or period after stColon
        , stErrorPackageUnexpected        // "package" is not expected (either nested or have seen)
    };

    TDerivedMessageTokensAutomation()
        : State(stInitial)
        , PrevState(stInitial)
        , BlockLevel_(0)
    {
    }

    TState NextState(const NProtoBuf::io::Tokenizer::Token& token) {
        const TState state = State;
        NextStateImpl(token);
        PrevState = state;
        return State;
    }

    TState GetState() const {
        return State;
    }

    // if true then last read token should not be transferred further to next consumer in a chain
    // (it is extended syntax token, not understood by protoc)
    bool IsExtensionToken() const {
        return State == stColon || State == stBaseMessageIdentifier || (State == stPeriod && PrevState == stBaseMessageIdentifier);
    }

    bool Found() const {
        return State == stFinished || State == stErrorIdentifierExpected;
    }

    bool SaveDependency(TSimpleHierarchy& graph) const {
        YASSERT(!DerivedName.empty());
        YASSERT(!BaseName.empty());
        return graph.SetBase(DerivedName, BaseName);
    }

private:
    TState NextStateImpl(const NProtoBuf::io::Tokenizer::Token& token);

private:
    TState State;
    TState PrevState;
    Stroka DerivedName, BaseName;
    yvector<TPair<size_t, size_t> > Nested_;
    size_t BlockLevel_;
};


// An automation for searching an import statements in parsed proto file:
// "import" stringToken ";"
class TImportTokensAutomation {
public:
    enum TState {
          stReady                         // waiting for "import" keyword
        , stNotReady                      // not expecting "import" keyword
        //next states identifies a token just been read
        , stImportKeyword                 // found "import"
        , stImportedFileName
    };

    TImportTokensAutomation()
        : State(stReady)
        , PrevState(stReady)
        , BlockLevel(0)
    {
    }

    TState NextState(const NProtoBuf::io::Tokenizer::Token& token) {
        const TState state = State;
        NextStateImpl(token);
        PrevState = state;
        return State;
    }

    TState GetState() const {
        return State;
    }

    bool Found() const {
        return State == stImportedFileName;
    }

private:
    TState NextStateImpl(const NProtoBuf::io::Tokenizer::Token& token);

private:
    TState State;
    TState PrevState;
    size_t BlockLevel;
};



// Reads tokens of extended-proto format (with message inheritance), memorize inheritance graph into supplied TSimpleHierarchy,
// and outputs corresponding text in standard proto format (without inheritance tokens),
// which could be further parsed by NProtoBuf::compiler::Parser.
class TDependencyCollectorInputStream: public NProtoBuf::io::ZeroCopyInputStream
{
public:
    // takes ownership on @input only
    TDependencyCollectorInputStream(NProtoBuf::io::ZeroCopyInputStream* input,
                                    TGztSourceTree* source_tree,
                                    TErrorCollector* error_collector,
                                    TSimpleHierarchy* message_graph);

    // implements ZeroCopyInputStream ----------------------------------
    bool Next(const void** data, int* size);
    void BackUp(int count);
    bool Skip(int count);
    NProtoBuf::int64 ByteCount() const {
        return BytesRead;
    }

private:
    //Returns true if a token being read is related to message inheritance syntax ans should not be transferred further to user
    // In this next token should be read and (possibly) returned to user.
    bool TryConsumeExtensionToken(const NProtoBuf::io::Tokenizer::Token& token);

    bool TryConsumeImportFileName(const NProtoBuf::io::Tokenizer::Token& token);

    bool RestoreWhitespace();

private:
    TErrorCollector* ErrorCollector;
    THolder<NProtoBuf::io::ZeroCopyInputStream> InputHolder;
    NProtoBuf::io::Tokenizer Tokenizer_;

    TSimpleHierarchy* MessagesGraph;

    //automation for recognizing such a sequence of tokens: "message" Identifier1 ":" Identifier2
    TDerivedMessageTokensAutomation MessageProcessor;

    TGztSourceTree* SourceTree;

    //automation for recognizing such a sequence of tokens: "import" FileNameString
    TImportTokensAutomation ImportProcessor;

    ui64 BytesRead;
    bool LastIsToken;
    int BackUpSize;

    int Line, Column;
    Stroka Whitespace, CanonicImportedFile;

    DECLARE_NOCOPY(TDependencyCollectorInputStream);
};

typedef yhash<Stroka, TIntrusivePtr<TSimpleHierarchy> > TFileDependencies;

}   // anonymous namespace

// Reads requested proto-file from disk and returns TDependencyCollectorInputStream to caller (usually it is Importer)
// Remembers message inheritance structure for each requested file.
class TDependencyCollectorSourceTree: public NProtoBuf::compiler::SourceTree
{
public:
    TDependencyCollectorSourceTree(TGztSourceTree* base_source_tree,
                                   TErrorCollector* error_collector)
        : ErrorCollector(error_collector)
        , BaseSourceTree(base_source_tree)
    {
    }

    // implements NProtoBuf::compiler::SourceTree ------------------------
    virtual NProtoBuf::io::ZeroCopyInputStream* Open(const TProtoStringType& filename);

    const TFileDependencies& GetDependencies() const {
        return Dependencies;
    }

private:
    TErrorCollector* ErrorCollector;
    TGztSourceTree* BaseSourceTree;
    TFileDependencies Dependencies;   //filename -> messages graph;
};




// TDerivedMessageTokensAutomation ==============================

TDerivedMessageTokensAutomation::TState TDerivedMessageTokensAutomation::NextStateImpl(const NProtoBuf::io::Tokenizer::Token& token)
{
    static const Stroka MESSAGE_KEYWORD = "message";
    static const Stroka PACKAGE_KEYWORD = "package";
    static const Stroka COLON_SYMBOL = ":";
    static const Stroka PERIOD_SYMBOL = ".";

    using NProtoBuf::io::Tokenizer;

    TState nextState = stIntermediate;

    if (State != stIntermediate && token.type == Tokenizer::TYPE_IDENTIFIER) {
        if (State == stInitial || State == stFinished) {
            if (token.text == PACKAGE_KEYWORD) {
                if (!DerivedName.empty())
                    return State = stErrorPackageUnexpected;
                Nested_.push_back(MakePair(DerivedName.Size(), BlockLevel_));
                return State = stPackageKeyword;
            }
            if (token.text == MESSAGE_KEYWORD) {
                Nested_.push_back(MakePair(DerivedName.Size(), BlockLevel_));
                if (!DerivedName.Empty())
                    DerivedName += '.';
                return State = stMessageKeyword;
            }
        }
        if (State == stPackageKeyword) {
            DerivedName += token.text;
            return State = stPackageIdentifier;
        }
        // TODO: forbid protobuf keywords (like "import", "option", "required", etc) to be used here as message-type names
        if (State == stMessageKeyword) {
            DerivedName += token.text;
            return State = stDerivedMessageIdentifier;
        }
        if (State == stColon) {
            BaseName = token.text;
            return State = stBaseMessageIdentifier;
        }
        if (State == stPeriod) {
            if (PrevState == stBaseMessageIdentifier) {
                BaseName += token.text;
                return State = PrevState;
            }
            if (PrevState == stPackageIdentifier) {
                DerivedName += token.text;
                return State = PrevState;
            }
        }
    } else if (token.type == Tokenizer::TYPE_SYMBOL) {
        if (token.text == COLON_SYMBOL && State == stDerivedMessageIdentifier)
            return State = stColon;
        if (token.text == PERIOD_SYMBOL) {
            if (State == stColon) {
                BaseName = token.text;
                return State = stPeriod;
            }
            if (State == stBaseMessageIdentifier) {
                BaseName += token.text;
                return State = stPeriod;
            }
            if (State == stPackageIdentifier) {
                DerivedName += token.text;
                return State = stPeriod;
            }
        }
        switch (token.text[0])
        {
        case '{':
            ++BlockLevel_;
            nextState = stInitial;
            break;
        case '}':
            if (BlockLevel_ > 0)
                --BlockLevel_;
            if (!Nested_.empty() && BlockLevel_ == Nested_.back().second) {
                DerivedName.resize(Nested_.back().first);
                Nested_.pop_back();
            }
            nextState = stInitial;
            break;
        case ';':
            nextState = stInitial;
            break;
        }
    }
    else if (token.type == Tokenizer::TYPE_END) {
        Nested_.clear();
        DerivedName.clear();
    }

    if (State == stBaseMessageIdentifier)
        return State = stFinished;

    if (State == stPeriod || State == stColon) {
        if (State == stColon)
            BaseName = PERIOD_SYMBOL;
        return State = stErrorIdentifierExpected;
    }

    return State = nextState;
}

// TImportTokensAutomation ==============================

TImportTokensAutomation::TState TImportTokensAutomation::NextStateImpl(const NProtoBuf::io::Tokenizer::Token& token)
{
    using NProtoBuf::io::Tokenizer;

    if (token.type == Tokenizer::TYPE_SYMBOL) {
        switch (token.text[0]) {
        case '{':
            ++BlockLevel;
            return State = stNotReady;
        case '}':
            if (BlockLevel > 0)
                --BlockLevel;
            if (BlockLevel > 0)
                return State = stNotReady;
            else
                return State = stReady;
        }
    }

    static const Stroka IMPORT_KEYWORD = "import";
    if (State == stReady && token.type == Tokenizer::TYPE_IDENTIFIER && token.text == IMPORT_KEYWORD)
        return State = stImportKeyword;

    else if (State == stImportKeyword) {
        if (token.type == Tokenizer::TYPE_STRING)
            return State = stImportedFileName;
        else
            return State = stReady;
    }

    else if (State == stImportedFileName)
        return State = stReady;     // ready for next import

    else
        return State;
}


// TDependencyCollectorInputStream ==============================

TDependencyCollectorInputStream::TDependencyCollectorInputStream(NProtoBuf::io::ZeroCopyInputStream* input,
                                                                 TGztSourceTree* source_tree,
                                                                 TErrorCollector* error_collector,
                                                                 TSimpleHierarchy* message_graph)
    : ErrorCollector(error_collector)
      // takes ownership on @input
    , InputHolder(input)
    , Tokenizer_(input, ErrorCollector)
    , MessagesGraph(message_graph)

    , SourceTree(source_tree)

    , BytesRead(0), LastIsToken(true), BackUpSize(0)
    , Line(0), Column(0)
{
}

bool TDependencyCollectorInputStream::RestoreWhitespace()
{
    Whitespace.clear();
    for (; Line < Tokenizer_.current().line; ++Line)
        Whitespace += '\n';
    if (!Whitespace.empty())
        Column = 0;

    for (; Column < Tokenizer_.current().column; ++Column)
        Whitespace += ' ';

    return !Whitespace.empty();
}

bool TDependencyCollectorInputStream::Next(const void** data, int* size)
{
    //post-condition: BackUpSize == 0 or -1

    if (BackUpSize > 0) {
        const Stroka* lasttext = LastIsToken ? &Tokenizer_.current().text : &Whitespace;
        i64 offset = lasttext->size() - BackUpSize;
        YASSERT(offset >= 0);
        // do not pass backed up token to the automations,
        // it was already processed with them, just output it further
        *data = ~(*lasttext) + offset;
        *size = BackUpSize;
    } else if (LastIsToken && !Tokenizer_.Next()) {
        // read next token from tokenizer (only if we have already output last token!)
        BackUpSize = -1;   //disable backups
        return false;
    } else {
        // we are going to output current token
        if (RestoreWhitespace()) {
            LastIsToken = false;
            YASSERT(Whitespace.size() > 0);
            *data = ~Whitespace;
            *size = +Whitespace;
        } else {
            LastIsToken = true;     // proceed to next token on next call
            const NProtoBuf::io::Tokenizer::Token& curToken = Tokenizer_.current();
            Line = curToken.line;
            Column = curToken.column;
            if (TryConsumeExtensionToken(curToken))
                // skip extension tokens and return next one instead
                return Next(data, size);
            else if (TryConsumeImportFileName(curToken)) {
                *data = ~CanonicImportedFile;
                *size = +CanonicImportedFile;
                Column += curToken.text.size();
            } else {
                *data = ~curToken.text;
                *size = +curToken.text;
                Column += curToken.text.size();
            }
        }
    }

    BackUpSize = 0;
    BytesRead += *size;
    return true;
}

void TDependencyCollectorInputStream::BackUp(int count)
{
    //pre-conditions:
    YASSERT(BackUpSize == 0);      //negative means we are finished, positive is forbidden (second backup in a row)
    YASSERT(count >= 0);

    if (count > 0) {
        BackUpSize = count;
        BytesRead -= count;
    }
    //post-condition: BackUpSize == count
}

bool TDependencyCollectorInputStream::Skip(int count)
{
    YASSERT(count >= 0);    //pre-condition

    while (count > 0) {
        const void* tmp_data;       //non-initialized!
        int tmp_size;
        if (!Next(&tmp_data, &tmp_size))
            return false;
        count -= tmp_size;
    }
    if (count < 0)
        BackUp(-count);
    else
        BackUpSize = -1;   //disable backups temporarily

    return true;
}

bool TDependencyCollectorInputStream::TryConsumeExtensionToken(const NProtoBuf::io::Tokenizer::Token& token)
{
    //pre-condition: called immediately after next valid token read
    YASSERT(token.type != NProtoBuf::io::Tokenizer::TYPE_END);

    MessageProcessor.NextState(token);
    if (MessageProcessor.Found())
        if (!MessageProcessor.SaveDependency(*MessagesGraph))
            ErrorCollector->AddErrorAtCurrentFile(token.line, token.column, "Ambigous base type.");

    if (MessageProcessor.GetState() == TDerivedMessageTokensAutomation::stErrorIdentifierExpected)
        ErrorCollector->AddErrorAtCurrentFile(token.line, token.column, "Identifier expected.");
    if (MessageProcessor.GetState() == TDerivedMessageTokensAutomation::stErrorPackageUnexpected)
        ErrorCollector->AddErrorAtCurrentFile(token.line, token.column, "Package unexpected.");

    return MessageProcessor.IsExtensionToken();
}

bool TDependencyCollectorInputStream::TryConsumeImportFileName(const NProtoBuf::io::Tokenizer::Token& token)
{
    ImportProcessor.NextState(token);
    if (!ImportProcessor.Found())
        return false;

    YASSERT(token.type == NProtoBuf::io::Tokenizer::TYPE_STRING);

    // original imported file (unquoted)
    Stroka original;
    Tokenizer_.ParseString(token.text, &original);

    // canonic file
    const Stroka canonic = SourceTree->CanonicName(original);
    if (canonic.empty() || canonic == original) {
        // make a hint for this special case
        if (original == STRINGBUF("base.proto"))
            ErrorCollector->AddErrorAtCurrentFile(token.line, token.column,
                "Importing base.proto by shortcut name is not allowed from arcadia. You should use full path instead: import \"kernel/gazetteer/base.proto\";");
        return false;   // just proceed with original file name
    }

    // restore quotes and replace original with canonic in further pipeline.
    CanonicImportedFile.clear();
    CanonicImportedFile.append('"');
    EscapeC(canonic, CanonicImportedFile);
    CanonicImportedFile.append('"');
    return true;
}



// TDependencyCollectorSourceTree ===========================

NProtoBuf::io::ZeroCopyInputStream* TDependencyCollectorSourceTree::Open(const TProtoStringType& filename)
{
    //open file with means of ProtoBuffer
    THolder<NProtoBuf::io::ZeroCopyInputStream> protobuf_stream(BaseSourceTree->Open(filename));

    if (protobuf_stream.Get() == NULL)
        return NULL;

    TIntrusivePtr<TSimpleHierarchy>& graph = Dependencies[filename];
    if (!graph)
        graph = new TSimpleHierarchy();

    ErrorCollector->SetCurrentFile(filename);

    // do some preprocessing of stream (collecting inheritance tokens)
    // TDependencyCollectorInputStream will take ownership on protobuf_stream pointer
    // caller will take ownership on returned object
    return new TDependencyCollectorInputStream(protobuf_stream.Release(), BaseSourceTree, ErrorCollector, graph.Get());
}


// TProtoParser ===========================

TProtoParser::TProtoParser(TGztSourceTree* source_tree,
                           TErrorCollector* error_collector,
                           const NBuiltin::TFileCollection& builtinFiles)
    : Errors(error_collector)
    , SourceTree(new TDependencyCollectorSourceTree(source_tree, Errors))
    , BuiltinFiles(builtinFiles)
    , RawPool(BuiltinFiles, SourceTree.Get(), error_collector)
{
    // do implicit importing of specified builtin files
    for (size_t i = 0; i < BuiltinFiles.Size(); ++i)
        RawPool.ImportBuiltinProto(BuiltinFiles[i]);
}

TProtoParser::~TProtoParser() {
}

bool TProtoParser::BuildMessageInheritanceGraph()
{
    RawMessageGraph.clear();
    const TFileDependencies& graphs = SourceTree->GetDependencies();

    Stroka full_base_name, name_space;  //tmp string buffers
    for (size_t f = 0; f < RawFileDescriptors_.size(); ++f) {
        const NProtoBuf::FileDescriptor* file_descr = RawFileDescriptors_[f];
        TFileDependencies::const_iterator graph_it = graphs.find(file_descr->name());
        if (graph_it == graphs.end())
            continue;

        const TSimpleHierarchy& graph = *(graph_it->second);

        for (int i = 0; i < file_descr->message_type_count(); ++i)
            if (!BuildMessageInheritanceGraph(file_descr->message_type(i), graph))
                return false;
    }
    return true;
}

bool TProtoParser::BuildMessageInheritanceGraph(const TDescriptor* msg_descr, const TSimpleHierarchy& graph)
{
    for (int i = 0; i < msg_descr->nested_type_count(); ++i)
        if (!BuildMessageInheritanceGraph(msg_descr->nested_type(i), graph))
            return false;

    // Find a base descriptor. There are two possible cases:
    // 1) @msg_descr was taken from generated_pool (builtin) and it's base type name is stored in its GztProtoDerivedFrom option.
    // 2) @msg_descr was read from disk .proto (using TDependencyCollector) and its base is remembered in the inheritance graph.

    const Stroka* explicit_base_name = graph.GetBase(msg_descr->full_name());
    const Stroka implicit_base_name = msg_descr->options().GetExtension(GztProtoDerivedFrom);
    if (!explicit_base_name && !implicit_base_name)      // no base class
        return true;

    const NProtoBuf::Descriptor* explicit_base_descr = NULL;
    if (explicit_base_name) {
         explicit_base_descr = ResolveBaseMessageByName(*explicit_base_name, msg_descr);
         if (!explicit_base_descr)
            return false;
    }

    const NProtoBuf::Descriptor* implicit_base_descr = NULL;
    if (!implicit_base_name.empty()) {
         implicit_base_descr = ResolveBaseMessageByName(implicit_base_name, msg_descr);
         if (!implicit_base_descr)
            return false;
    }

    if (explicit_base_descr && implicit_base_descr && explicit_base_descr != implicit_base_descr) {
        Errors->AddError(msg_descr->file()->name(), -1, 0, NProtoBuf::strings::Substitute(
            "Ambiguous base type for $0: either $1 (disk) or $2 (builtin).", msg_descr->full_name(),
             explicit_base_descr->full_name(), implicit_base_descr->full_name()));
        return false;
    }

    RawMessageGraph[msg_descr] = explicit_base_descr ? explicit_base_descr : implicit_base_descr;
    // check if cycle dependency exists.
    if (!CheckIfSelfDerived(msg_descr))
        return false;

    return true;
}

bool TProtoParser::CheckIfSelfDerived(const TDescriptor* message_descr)
{
    TDescriptorMap::const_iterator tmp = RawMessageGraph.find(message_descr);
    for (; tmp != RawMessageGraph.end(); tmp = RawMessageGraph.find(tmp->second))
        if (tmp->second == message_descr) {
            //found a cycle, now repeat our path again to build error message.
            Stroka chain = message_descr->name();
            for (tmp = RawMessageGraph.find(tmp->second); tmp != RawMessageGraph.end(); tmp = RawMessageGraph.find(tmp->second)) {
                chain += " <- ";
                chain += tmp->second->name();
                if (tmp->second == message_descr) {
                    Errors->AddError(message_descr->file()->name(), -1, 0,
                        NProtoBuf::strings::Substitute("Message \"$0\" is derived from itself: $1", message_descr->full_name(), chain));
                    return false;
                }
            }
        }
    return true;
}

const NProtoBuf::Descriptor* TProtoParser::ResolveBaseMessageByName(const Stroka& base_message_name,
                                                                    const NProtoBuf::Descriptor* derived_message)
{
    YASSERT(!base_message_name.empty());
    const NProtoBuf::DescriptorPool* pool = derived_message->file()->pool();

    // resolved base message descriptor (returned)
    const NProtoBuf::Descriptor* base_descr = NULL;
    // first check if base-type should be looked in global namespace only
    if (base_message_name[0] == '.')
        base_descr = pool->FindMessageTypeByName(base_message_name.substr(1));
    else {
        // otherwise, define inner-most namespace, containing derived message
        // and then look in this and every parent namespace
        TStringBuf name_space = derived_message->full_name();
        while (base_descr == NULL && !name_space.empty()) {
            name_space.RNextTok('.');
            const Stroka full_base_name = name_space.empty() ? base_message_name : Stroka(name_space, ".", base_message_name);
            base_descr = pool->FindMessageTypeByName(full_base_name);
        }
    }
    if (base_descr == NULL) {
        Stroka err = NProtoBuf::strings::Substitute("Base class of \"$0\" is not defined", derived_message->full_name());
        err += (base_message_name == ".") ? "."  :  NProtoBuf::strings::Substitute(": \"$0\".", base_message_name);
        Errors->AddError(derived_message->file()->name(), -1, 0, err);
    }
    return base_descr;
}

bool TProtoParser::LoadRawProto(const Stroka& filename)
{
    if (!RawPool.FindFileByName(filename))
        return false;

    RawPool.CollectFiles(RawFileDescriptors_);
    return BuildMessageInheritanceGraph();
}

const NProtoBuf::Descriptor* TProtoParser::GetBaseDescriptor(const TDescriptor* message_descr) const
{
    TDescriptorMap::const_iterator graph_it = RawMessageGraph.find(message_descr);
    return (graph_it == RawMessageGraph.end()) ? NULL : graph_it->second;
}

#undef DO


} // namespace NGzt
