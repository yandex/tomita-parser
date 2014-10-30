#include <util/generic/ylimits.h>
#include <util/digest/md5.h>
#include <contrib/libs/protobuf/stubs/substitute.h>

#include "common/recode.h"
#include "common/binaryguard.h"

#include "articlepool.h"
#include "gztparser.h"
#include "protopool.h"
#include "gztarticle.h"
#include "builtin.h"
#include "options.h"

#include <kernel/gazetteer/base.pb.h>
#include <kernel/gazetteer/syntax.pb.h>
#include <kernel/gazetteer/binary.pb.h>



namespace NGzt
{

using NProtoBuf::strings::Substitute;


// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false



struct TArticlePoolBuilder::TUnresolvedRefs {
    size_t Count;
    typedef yhash<Stroka, yvector<TMessage*> > TRefs;
    TRefs Items;

    struct TUnresolvedArticleInfo {
        TSharedPtr<TMessage> Article;
        Stroka Name;
        ui32 Offset;

        TUnresolvedArticleInfo(TMessage* article, const Stroka& name, ui32 offset)
            : Article(article), Name(name), Offset(offset)
        {
        }
    };

    yvector<TUnresolvedArticleInfo> Articles;     // article with unresolved refs + offsets

    inline TUnresolvedRefs()
        : Count(0)
    {
    }

    void AddReference(const Stroka& referred_name, TMessage* reference);
    void AddArticle(TMessage* article, const Stroka& name, ui32 offset);

    // Resolve all previuosly unresolved references, collected here
    // Returns true if all are successfully resolved, false + error otherwise.
    bool Resolve(TArticlePoolBuilder& pool);

    void UpdateResolved(TArticlePoolBuilder& pool, size_t index);
    void Clear();
};


// ==========================================================
// TArticlePoolBuilder

TArticlePoolBuilder::TArticlePoolBuilder(TGztSourceTree* source_tree, TProtoPoolBuilder* proto_builder,
                                         IKeyCollector* key_indexer)
    : HadErrors(false)
    , SourceTree(source_tree)
    , ProtoBuilder(proto_builder)
    , CurrentTopLevelArticle(NULL)
    , UnresolvedRefs(new TUnresolvedRefs)
    , RuntimeTRefId(NULL)
    , CurrentOptions(new TOptions)
    , Keys(key_indexer)
#ifdef GZT_DEBUG
    , ArticleCounter("articles total", 100000)
#endif
{
}

TArticlePoolBuilder::~TArticlePoolBuilder()
{
    // clear prototypes
    for (TPrototypes::iterator it = Prototypes.begin(); it != Prototypes.end(); ++it)
        delete it->second;
}

void TArticlePoolBuilder::AddError(const TMessage* descriptor, const Stroka& error)
{
    // TODO: make referensing to error position more precise - not by top-level article, but by every value.
    int line = -1, column = 0;
    SourceLocationTable.Find(descriptor, NProtoBuf::DescriptorPool::ErrorCollector::OTHER, &line, &column);
    Errors.AddError(SourceTree, line, column, error);
    HadErrors = true;
}

void TArticlePoolBuilder::AddError(const Stroka& error)
{
    AddError(CurrentTopLevelArticle, error);
}

bool TArticlePoolBuilder::AddImportedFile(const Stroka& virtual_file)
{
    TBinaryGuard::TDependency dep = SourceTree->AsDependency(virtual_file);
    TPair<ymap<Stroka, TBinaryGuard::TDependency>::iterator, bool> ins = LoadedFiles.insert(MakePair(dep.Name, dep));
    if (!ins.second && !dep.Checksum.empty() && dep.Checksum != ins.first->second.Checksum)
        ins.first->second = dep;

    return ins.second;
}

void TArticlePoolBuilder::SaveHeader(NBinaryFormat::TGztInfo& header) const
{
    // imported files
    for (ymap<Stroka, TBinaryGuard::TDependency>::const_iterator it = LoadedFiles.begin(); it != LoadedFiles.end(); ++it) {
        NBinaryFormat::TSourceFile* f = header.AddImportedFile();
        const TBinaryGuard::TDependency& dep = it->second;
        f->SetPath(dep.Name);
        f->SetHash(dep.Checksum);
        if (dep.IsBuiltin)          // do not modify default value
            f->SetBuiltin(true);
        if (dep.IsRoot)
            f->SetRoot(true);
    }

    // and custom keys
    SaveToField(header.MutableCustomKey(), CustomKeys);
}

ui32 TArticlePoolBuilder::StoreArticle(const TStringBuf& name, const TMessage* article)
{
    ui32 offset = ArticleData.Buffer().Size();
    SaveArticle(&ArticleData, name, article);

    // to make sure we are not running out of ui32 space for offset values.
    if (EXPECT_FALSE(!TArticlePool::IsValidOffset(ArticleData.Buffer().Size())))
        ythrow yexception() << "Article memory limit (" << TArticlePool::MaxValidOffset()/(1024*1024) << " MB) is reached.";

    // remember offsets only for named articles
    if (!name.empty())
        Articles.InsertUnique(name, offset);
    return offset;
}

ui32 TArticlePoolBuilder::GetTotalSize() const
{
    return static_cast<ui32>(ArticleData.Buffer().Size());
}


bool TArticlePoolBuilder::FindOffsetByName(const TStringBuf& name, ui32& offset) const
{
    TArticleHash::const_iterator res = Articles.Find(name);
    if (res == Articles.End())
        return false;

    offset = res->second;
    return true;
}

bool TArticlePoolBuilder::FromVirtualFile(const Stroka& filename)
{
    // do caching here to avoid import cycles and repeated builds of same file.
    // Returned false means we had already such name in dependencies.
    if (!AddImportedFile(filename))
        return true;

    THolder<NProtoBuf::io::ZeroCopyInputStream> input(SourceTree->Open(filename));
    if (input.Get() == NULL) {
        AddError(NULL, Substitute("File \"$0\" not found in source tree mapping.", filename));
        return false;
    }

    return BuildFromStream(input.Get());
}

bool TArticlePoolBuilder::FromDiskFile(const Stroka& diskFile) {
    // set @diskFile as a root for current compilation
    TDiskSourceTree::TRootFile root(diskFile, *SourceTree);
    return FromVirtualFile(root.Virtual());
}

bool TArticlePoolBuilder::FromText(const Stroka& text) {
    NProtoBuf::io::ArrayInputStream input(~text, +text);
    return BuildFromStream(&input);
}

bool TArticlePoolBuilder::FromStream(TInputStream* input) {
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(input);
    return BuildFromStream(&adaptor);
}

bool TArticlePoolBuilder::BuildFromStream(NProtoBuf::io::ZeroCopyInputStream* input) {
    HadErrors = false;

    // backup current options before processing
    TOptions::TBackup backup(CurrentOptions);

    TGztParser::Tokenizer tokenizer(input, &Errors);

    TGztParser parser;
    parser.RecordErrorsTo(&Errors);
    parser.RecordSourceLocationsTo(&SourceLocationTable);
    TGztFileDescriptorProto proto;
    while (tokenizer.current().type != NProtoBuf::io::Tokenizer::TYPE_END) {
        DO(parser.ParseNextStatement(&tokenizer, &proto));
        DO(BuildPartialFile(proto));
        proto.Clear();
    }

    return !HadErrors && UnresolvedRefs->Resolve(*this);
}

bool TArticlePoolBuilder::FromProtobuf(const TGztFileDescriptorProto& parsedFile) {
    HadErrors = false;

    // backup current options before processing
    TOptions::TBackup backup(CurrentOptions);

    DO(BuildPartialFile(parsedFile));
    return !HadErrors && UnresolvedRefs->Resolve(*this);
}

bool TArticlePoolBuilder::LoadToGztFileDescriptor(const Stroka& filename, TGztFileDescriptorProto& file)
{
    THolder<NProtoBuf::io::ZeroCopyInputStream> input(SourceTree->Open(filename));
    if (input.Get() == NULL) {
        AddError(NULL, Substitute("File \"$0\" not found in source tree mapping.", filename));
        return false;
    }

    TGztParser::Tokenizer tokenizer(input.Get(), &Errors);

    TGztParser parser;
    parser.RecordErrorsTo(&Errors);
    parser.RecordSourceLocationsTo(&SourceLocationTable);
    return parser.Parse(&tokenizer, &file);
}

bool TArticlePoolBuilder::BuildPartialFile(const TGztFileDescriptorProto& file)
{
    if (file.has_encoding()) {
        CurrentOptions->Encoding = CharsetByName(~file.encoding());
        YASSERT(CurrentOptions->Encoding != CODES_UNKNOWN);
    }

    // TODO: deprecated, remove
    if (file.strip_names())
        CurrentOptions->GztOptions.SetStoreArticleTitles(false);

    // import all dependencies first
    for (int i = 0; i < file.dependency_size(); ++i)
        if (!LoadDependency(file.dependency(i))) {
            AddError("Cannot load dependency: " + file.dependency(i));
            return false;
        }

    for (int i = 0; i < file.article_size(); ++i)
        DO(BuildArticle(file.article(i)));

    return true;
}

static void NormalizeArticleName(TStringBuf& name) {
    if (name.size() > 0 && name[0] == ARTICLE_MARKER)
        name = name.SubStr(1);
}

static inline void SaveArticleName(NProtoBuf::io::CodedOutputStream* encoder, const TStringBuf& name)
{
    encoder->WriteVarint32(name.size());    // + article name size
    encoder->WriteRaw(~name, +name);        // + article name
}

void TArticlePoolBuilder::SaveArticle(TOutputStream* output, const TStringBuf& name, const TMessage* article) const
{
    NProtoBuf::io::TCopyingOutputStreamAdaptor adaptor(output);
    NProtoBuf::io::CodedOutputStream encoder(&adaptor);

    // copy @article to tmp without keys and serialize this tmp
    // (this trick significantly decreases gzt.bin size: ~60 Mb out of 155 Mb for main.gzt.bin, for example)
    // TODO: make it more efficiently (now this copying and keys removing is quite slow)
    THolder<TMessage> tmp(article->New());
    tmp->CopyFrom(*article);
    ClearArticleKeys(tmp.Get());

    encoder.WriteVarint32(ProtoBuilder->Index().GetIndex(article->GetDescriptor()));    // descriptor index
    TMessageSerializer::Save(&encoder, *tmp);                                    // article itself
    SaveArticleName(&encoder, CurrentOptions->GztOptions.GetStoreArticleTitles() ? name : TStringBuf());    // article title
}

void TArticlePoolBuilder::Save(NBinaryFormat::TArticlePool& proto, TBlobCollection& blobs) const
{
    SaveToField(proto.MutableCustomArticle(), CustomArticles);
    blobs.SaveCopy("ArticleData", ArticleData.Buffer(), proto.MutableArticleData()->MutableBlobKey());

    const TBlob artData = blobs["ArticleData"];

    TBuffer buffer;
    BuildTitleMap(artData, buffer);
    blobs.SaveNoCopy("TitleIndex", buffer, proto.MutableTitleIndex()->MutableBlobKey());
}

void TArticlePoolBuilder::Save(TOutputStream* output) const
{
    SaveAsProto<NBinaryFormat::TArticlePool>(output, *this);
}

static inline bool IsProtoFileName(const TStringBuf& filename) {
    return filename.has_suffix(STRINGBUF(".proto"))
        || filename.has_suffix(STRINGBUF(".gztproto"));     // gazetteer proto extension
}

bool TArticlePoolBuilder::LoadDependency(const Stroka& filename)
{
    bool isProto = IsProtoFileName(filename);

    Stroka diskFile;
    bool foundDiskFile = SourceTree->VirtualFileToDiskFile(filename, diskFile);

    if (isProto) {
        if (foundDiskFile) {
            TDiskSourceTree::TCurrentFile curFile(diskFile, *SourceTree);
            return LoadProto(curFile.Virtual());
        } else
            return LoadProto(filename);        // still try parsing original @filename (it could be a builtin)
    }

    //otherwise treat file as gazetteer source (*.gzt)
    if (!foundDiskFile) {
        AddError(NULL, "Cannot find imported file \"" + filename + "\"");
        return false;
    }

    TDiskSourceTree::TCurrentFile curFile(diskFile, *SourceTree);
    return FromVirtualFile(curFile.Virtual());
}

bool TArticlePoolBuilder::LoadProto(const Stroka& filename) {
    DO(ProtoBuilder->LoadVirtualFile(filename));
    // collect all loaded protos too.
    yset<Stroka> importedProto;
    ProtoBuilder->AddLoadedFiles(importedProto);
    for (yset<Stroka>::const_iterator it = importedProto.begin(); it != importedProto.end(); ++it)
        AddImportedFile(*it);
    return true;
}

void TArticlePoolBuilder::ClearArticleKeys(TMessage* article) const
{
    const TFieldDescriptor* keyField = ProtoBuilder->Index().GetSearchKeyField(article->GetDescriptor());

    if (keyField != NULL && keyField->is_repeated()) {
        const TFieldDescriptor* typeField =  keyField->message_type()->FindFieldByNumber(TSearchKey::kTypeFieldNumber);

        const NProtoBuf::Reflection* ref = article->GetReflection();
        size_t keyCount = ref->FieldSize(*article, keyField);
        // do not clear keys if there is a custom key
        for (size_t i = 0; i < keyCount; ++i) {
            const TMessage& key = ref->GetRepeatedMessage(*article, keyField, i);
            if (key.GetReflection()->GetEnum(key, typeField)->number() == TSearchKey::CUSTOM)
                return;
        }
        ref->ClearField(article, keyField);
    }
}

bool TArticlePoolBuilder::VerifyUniqueName(const TStringBuf& name)
{
    ui32 existing_offset;
    if (FindOffsetByName(name, existing_offset)) {
        AddError(Substitute("Article with name \"$0\" has been already defined (every article should have unique name).", ::ToString(name)));
        return false;
    }
    return true;
}

bool TArticlePoolBuilder::CollectKeys(const TMessage& article, ui32 offset, const TStringBuf& article_name)
{
    try {
        Keys->Collect(article, offset, *CurrentOptions);
        return true;
    } catch (const yexception& e) {
        AddError(Substitute("Error on collecting article \"$0\" keys: $1", ::ToString(article_name), e.what()));
        return false;
    }
}

bool TArticlePoolBuilder::BuildArticle(const TArticleDescriptorProto& article_proto)
{
    //not thread-safe obviously.

    CurrentTopLevelArticle = &article_proto;

    // convert name to utf8
    TStaticCharCodec<> codec;
    TStringBuf article_name = codec.CharToUtf8(article_proto.name(), CurrentOptions->Encoding);
    NormalizeArticleName(article_name);
    DO(VerifyUniqueName(article_name));

    const NProtoBuf::Descriptor* descriptor = ProtoBuilder->Index().FindMessageTypeByName(article_proto.type());
    if (descriptor == NULL) {
        AddError(Substitute("Article message-type \"$0\" definition is not found.", article_proto.type()));
        return false;
    }

    TMessage* NULL_MESSAGE = NULL;
    TPair<TPrototypes::iterator, bool> article_prototype = Prototypes.insert(MakePair(descriptor, NULL_MESSAGE));
    if (article_prototype.second)
        article_prototype.first->second = ProtoBuilder->GetPrototype(descriptor)->New();

    TMessage* article = article_prototype.first->second;
    size_t unresolved_refs_count = UnresolvedRefs->Count;

    //fill article with data from proto
    DO(FillArticle(article, article_proto.field()));

    // TGztOptions is processed separately (not stored into pool)
    if (article->GetDescriptor() == TGztOptions::descriptor()) {
        CurrentOptions->GztOptions.CopyFrom(*article);
        article->Clear();
        return true;
    }

    // save it into ArticleData
    ui32 offset = StoreArticle(article_name, article);

    // collect its keys
    DO(CollectKeys(*article, offset, article_name));

    // if this article contains unresolved refs - put it aside (to resolve later)
    if (unresolved_refs_count < UnresolvedRefs->Count) {
        UnresolvedRefs->AddArticle(article, ::ToString(article_name), offset);
        // @article passed to UnresolvedRefs ownership, we should create new one for Prototypes
        article_prototype.first->second = article->New();
    } else
        article->Clear();

#ifdef GZT_DEBUG
    ++ArticleCounter;
#endif

    return true;
}

bool TArticlePoolBuilder::FillArticle(TMessage* article, const NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto>& fields_proto)
{
    //first assign positional fields - should be at the beginning of field-list only.
    int i = 0;
    for (; i < fields_proto.size(); ++i) {
        // look field-descriptor by position only, stop at first keyword field.
        if (fields_proto.Get(i).has_name())
            break;
        DO(AssignPositionalValue(article, i, fields_proto.Get(i).value()));
    }

    for (; i < fields_proto.size(); ++i) {
        YASSERT(fields_proto.Get(i).has_name());     //this should be ensured by TProtoParser
        // look field-descriptor by name
        DO(AssignKeywordValue(article, fields_proto.Get(i).name(), fields_proto.Get(i).value()));
    }

    return CheckRequiredFields(article);
}

bool TArticlePoolBuilder::AssignPositionalValue(TMessage* article, size_t field_index, const TFieldValueDescriptorProto& value)
{
    const NProtoBuf::Descriptor* descriptor = article->GetDescriptor();
    // look field-descriptor by its position
    if (field_index >= (size_t)descriptor->field_count()) {
        AddError(Substitute("Cannot assign $0 positional values to message of type \"$1\" (has only $2 fields).",
                            field_index + 1, descriptor->full_name(), descriptor->field_count()));
        return false;
    } else
        return AssignFieldValue(article, descriptor->field(field_index), value);
}

bool TArticlePoolBuilder::AssignKeywordValue(TMessage* article, const Stroka& field_name, const TFieldValueDescriptorProto& value)
{
    const NProtoBuf::Descriptor* descriptor = article->GetDescriptor();

    // look field-descriptor by name
    const NProtoBuf::FieldDescriptor* field = descriptor->FindFieldByName(field_name);
    if (field == NULL) {
        AddError(Substitute("Field \"$0\" is not defined for message-type \"$1\".", field_name, descriptor->full_name()));
        return false;
    } else
        return AssignFieldValue(article, field, value);
}

// Checks if @value could represent article name
static bool IsPossibleArticleName(const TFieldValueDescriptorProto& value)
{
    // Note that returning true does not necessarily means that this is an article name
    // It should be resolved later anyway.
    if (value.type() == TFieldValueDescriptorProto::TYPE_IDENTIFIER)
        return true;
    else if (value.type() == TFieldValueDescriptorProto::TYPE_STRING)
        return (value.string_or_identifier().length() > 0 && value.string_or_identifier()[0] != ARTICLE_MARKER) ||
               value.string_or_identifier().length() > 1;
    else
        return false;
}

// Returns true only if @value could be assigned to field of type @field_type. Does overflow check for numeric types.
static bool IsCompatible(NProtoBuf::FieldDescriptor::CppType field_type, const TFieldValueDescriptorProto& value)
{
    using NProtoBuf::FieldDescriptor;
    switch (field_type) {
        case FieldDescriptor::CPPTYPE_INT32:
            return value.type() == TFieldValueDescriptorProto::TYPE_INTEGER &&
                   value.int_number() <= NProtoBuf::kint32max && value.int_number() >= NProtoBuf::kint32min;
        case FieldDescriptor::CPPTYPE_INT64:
            return value.type() == TFieldValueDescriptorProto::TYPE_INTEGER &&
                   value.int_number() <= NProtoBuf::kint64max && value.int_number() >= NProtoBuf::kint64min;
        case FieldDescriptor::CPPTYPE_UINT32:
            return value.type() == TFieldValueDescriptorProto::TYPE_INTEGER &&
                   value.int_number() <= NProtoBuf::kuint32max && value.int_number() >= 0;
        case FieldDescriptor::CPPTYPE_UINT64:
            // TODO: currently uint64 fields cannot hold values more than int64::max due to TFieldValueDescriptorProto format, fix it.
            return value.type() == TFieldValueDescriptorProto::TYPE_INTEGER &&
                   value.int_number() <= NProtoBuf::kint64max && value.int_number() >= 0;

        case FieldDescriptor::CPPTYPE_DOUBLE:
        case FieldDescriptor::CPPTYPE_FLOAT:
            return value.type() == TFieldValueDescriptorProto::TYPE_INTEGER ||
                   value.type() == TFieldValueDescriptorProto::TYPE_FLOAT;

        case FieldDescriptor::CPPTYPE_BOOL:
            // only [0, 1, true, false] allowed as values.
            if (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER)
                return value.int_number() == 0 || value.int_number() == 1;
            else if (value.type() == TFieldValueDescriptorProto::TYPE_IDENTIFIER)
                return value.string_or_identifier() == STRINGBUF("true") || value.string_or_identifier() == STRINGBUF("false");
            else
                return false;

        case FieldDescriptor::CPPTYPE_ENUM:
            // the check if value is really from required ENUM will be done later
            return value.type() == TFieldValueDescriptorProto::TYPE_IDENTIFIER ||
                   value.type() == TFieldValueDescriptorProto::TYPE_INTEGER;

        case FieldDescriptor::CPPTYPE_STRING:
            return value.type() == TFieldValueDescriptorProto::TYPE_STRING;

        case FieldDescriptor::CPPTYPE_MESSAGE:
            // the check will be done later when assigning values to embedded message.
            return true;
    }
    return false;
}


bool TArticlePoolBuilder::AssignFieldValue(TMessage* article, const NProtoBuf::FieldDescriptor* field, const TFieldValueDescriptorProto& value)
{
    // process lists recursively
    if (value.type() == TFieldValueDescriptorProto::TYPE_LIST) {
        if (field->is_repeated()) {
            for (int i = 0; i < value.list().value_size(); ++i)
                DO(AssignFieldValue(article, field, value.list().value(i)));
            return true;
        } else if (field->cpp_type() != NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            AddError(Substitute("Cannot assign list of values to non-repeated field \"$0\".", field->full_name()));
            return false;
        }
    }

    //check if assigned value is compatible with field type
    if (!IsCompatible(field->cpp_type(), value)) {
        AddError(Substitute("Incompatible value assigned to field \"$0\".", field->full_name()));
        return false;
    }

    using NProtoBuf::FieldDescriptor;
    const NProtoBuf::Reflection* reflection = article->GetReflection();

    switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
            if (field->is_repeated())
                reflection->AddInt32(article, field, (i32)value.int_number());
            else
                reflection->SetInt32(article, field, (i32)value.int_number());
            return true;

        case FieldDescriptor::CPPTYPE_INT64:
            if (field->is_repeated())
                reflection->AddInt64(article, field, value.int_number());
            else
                reflection->SetInt64(article, field, value.int_number());
            return true;

        case FieldDescriptor::CPPTYPE_UINT32:
            if (field->is_repeated())
                reflection->AddUInt32(article, field, (ui32)value.int_number());
            else
                reflection->SetUInt32(article, field, (ui32)value.int_number());
            return true;

        case FieldDescriptor::CPPTYPE_UINT64:
            if (field->is_repeated())
                reflection->AddUInt64(article, field, value.int_number());
            else
                reflection->SetUInt64(article, field, value.int_number());
            return true;

        case FieldDescriptor::CPPTYPE_DOUBLE:
            if (field->is_repeated())
                reflection->AddDouble(article, field, (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER) ? value.int_number() : value.float_number());
            else
                reflection->SetDouble(article, field, (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER) ? value.int_number() : value.float_number());
            return true;

        case FieldDescriptor::CPPTYPE_FLOAT:
            if (field->is_repeated())
                reflection->AddFloat(article, field, (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER) ? value.int_number() : (float)value.float_number());
            else
                reflection->SetFloat(article, field, (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER) ? value.int_number() : (float)value.float_number());
            return true;

        case FieldDescriptor::CPPTYPE_BOOL: {
            bool bool_value = (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER) ? value.int_number() == 1 : value.string_or_identifier() == STRINGBUF("true");
            if (field->is_repeated())
                reflection->AddBool(article, field, bool_value);
            else
                reflection->SetBool(article, field, bool_value);
            return true;
        }

        case FieldDescriptor::CPPTYPE_ENUM: {
            // check if the value is really from required ENUM
            const NProtoBuf::EnumValueDescriptor* enum_value = NULL;
            if (value.type() == TFieldValueDescriptorProto::TYPE_IDENTIFIER) {
                enum_value = field->enum_type()->FindValueByName(value.string_or_identifier());
                if (!enum_value)
                   AddError(Substitute("Enum \"$0\" does not have value \"$1\" assigned to field \"$2\".", field->enum_type()->full_name(), value.string_or_identifier(), field->full_name()));

            } else if (value.type() == TFieldValueDescriptorProto::TYPE_INTEGER) {
                enum_value = field->enum_type()->FindValueByNumber(value.int_number());
                if (!enum_value)
                    AddError(Substitute("Enum \"$0\" does not have value $1 assigned to field \"$2\".", field->enum_type()->full_name(), value.int_number(), field->full_name()));
            }

            if (!enum_value)
                return false;

            if (field->is_repeated())
                reflection->AddEnum(article, field, enum_value);
            else
                reflection->SetEnum(article, field, enum_value);
            return true;
        }

        case FieldDescriptor::CPPTYPE_STRING: {
            // recode all string fields to utf-8
            TStaticCharCodec<> codec;
            TStringBuf utf8Str = codec.CharToUtf8(value.string_or_identifier(), CurrentOptions->Encoding);

            //TSecondHandString str(StrPool); // re-utilize previously allocated string data
            Stroka str = ::ToString(utf8Str);

            if (field->is_repeated())
                reflection->AddString(article, field, str);
            else
                reflection->SetString(article, field, str);
            return true;
        }

        case FieldDescriptor::CPPTYPE_MESSAGE:
            TMessage* sub_article;
            if (field->is_repeated())
                sub_article = reflection->AddMessage(article, field);
            else
                sub_article = reflection->MutableMessage(article, field);

            if (value.type() == TFieldValueDescriptorProto::TYPE_BLOCK)
                return FillArticle(sub_article, value.sub_field());
            else if (ProtoBuilder->Index().IsSameAsGeneratedTypeField(field, TRef::descriptor())) {
                // treat references specifically
                if (!IsPossibleArticleName(value)) {
                    AddError(Substitute("An article identifier should be provided for TRef field \"$0\".", field->full_name()));
                    return false;
                }
                return ResolveReference(sub_article, value);
            } else {
                YASSERT(field->message_type()->full_name() != "TRef");
                return AssignPositionalValue(sub_article, 0, value) &&
                       CheckRequiredFields(sub_article);
            }
    }
    return false;

}

bool TArticlePoolBuilder::CheckRequiredFields(TMessage* article)
{
    if (article->IsInitialized())
        return true;
    else {
        AddError(Substitute("Article of type \"$0\" is missing required fields: $1",
                            article->GetDescriptor()->full_name(), article->InitializationErrorString()));
        return false;
    }
}

void TArticlePoolBuilder::TUnresolvedRefs::AddReference(const Stroka& referred_name, TMessage* reference)
{
    Items[referred_name].push_back(reference);
    Count += 1;
}

void TArticlePoolBuilder::TUnresolvedRefs::AddArticle(TMessage* article, const Stroka& name, ui32 offset)
{
    Articles.push_back(TUnresolvedArticleInfo(article, name, offset));
}

bool TArticlePoolBuilder::TUnresolvedRefs::Resolve(TArticlePoolBuilder& pool)
{
    if (Count == 0)
        return true;

    yvector<Stroka> failed;
    for (TRefs::iterator it = Items.begin(); it != Items.end(); ++it)
    {
        const Stroka& identifier = it->first;
        yvector<TMessage*>& refs = it->second;

        ui32 offset = 0;
        if (pool.FindOffsetByName(identifier, offset))
            for (yvector<TMessage*>::const_iterator it_ref = refs.begin(); it_ref != refs.end(); ++it_ref)
                pool.SetRefId(*it_ref, offset);
        else
            failed.push_back(identifier);
    }

    if (failed.empty())
    {
        //NTools::TPerformanceCounter update_counter("articles resolved", 50000);
        for (size_t i = 0; i < Articles.size(); ++i)
        {
            UpdateResolved(pool, i);
            //++update_counter;
        }
        Clear();

        return true;
    }
    else
    {
        const size_t MAX_UNRESOLVED_REFS_TO_PRINT = 10;
        Stroka error = "Could not find following referred articles: " + failed[0];
        for (size_t i = 1; i < failed.size() && i < MAX_UNRESOLVED_REFS_TO_PRINT; ++i)
            error += ", \"" + failed[i] + "\"";
        if (failed.size() > MAX_UNRESOLVED_REFS_TO_PRINT)
            error += Substitute("... ($0 more)", failed.size() - MAX_UNRESOLVED_REFS_TO_PRINT);
        pool.AddError(NULL, error);
        return false;
    }

}

void TArticlePoolBuilder::TUnresolvedRefs::UpdateResolved(TArticlePoolBuilder& pool, size_t index)
{
    const TUnresolvedArticleInfo& info = Articles[index];
    TMemoryOutput tmp(~pool.ArticleData.Buffer() + info.Offset, pool.ArticleData.Buffer().Size() - info.Offset);
    pool.SaveArticle(&tmp, info.Name, info.Article.Get());
}

void TArticlePoolBuilder::TUnresolvedRefs::Clear()
{
    Items.clear();
    Articles.clear();
    Count = 0;
}

/*
ui32 TArticlePoolBuilder::GetRefId(const TMessage* reference) const
{
    if (reference->GetDescriptor() == TRef::descriptor())
        return static_cast<const TRef*>(reference)->id();
    else
        return reference->GetReflection()->GetUInt32(*reference, RuntimeTRefId);
}
*/

void TArticlePoolBuilder::SetRefId(TMessage* reference, ui32 value) const
{
    if (reference->GetDescriptor() == TRef::descriptor())
        static_cast<TRef*>(reference)->set_id(value);
    else {
        YASSERT(RuntimeTRefId);
        reference->GetReflection()->SetUInt32(reference, RuntimeTRefId, value);
    }
}

bool TArticlePoolBuilder::ResolveReference(TMessage* reference, const TFieldValueDescriptorProto& value)
{
    if (EXPECT_FALSE(!RuntimeTRefId)) {
        RuntimeTRefId = ProtoBuilder->FindRuntimeTRefId();
        if (!RuntimeTRefId) {
            AddError("Cannot find TRef runtime descriptor, try importing base.proto explicitly");
            return false;
        }
    }

    YASSERT(IsPossibleArticleName(value));

    TStaticCharCodec<> codec;
    TStringBuf name = codec.CharToUtf8(value.string_or_identifier(), CurrentOptions->Encoding);
    NormalizeArticleName(name);

    ui32 offset = 0;
    if (FindOffsetByName(name, offset))
        SetRefId(reference, offset);
    else
    {
        // for now just remember such reference to resolve it later when all names are parsed and return true.
        SetRefId(reference, ui32(-1));
        UnresolvedRefs->AddReference(::ToString(name), reference);
    }
    return true;
}

bool TArticlePoolBuilder::ParseSingleArticle(const Stroka gztFile, const TDescriptor& descriptor, TMessage& message)    // static
{
    TGztSourceTree sourceTree;
    TArticlePoolBuilder builder(&sourceTree, NULL, NULL);
    TGztFileDescriptorProto fileProto;

    const TDiskSourceTree::TRootFile root(gztFile, sourceTree);
    DO(builder.LoadToGztFileDescriptor(root.Virtual(), fileProto));

    if (fileProto.has_encoding())
        builder.CurrentOptions->Encoding = CharsetByName(~fileProto.encoding());

    if (fileProto.article_size() != 1) {
        builder.AddError("Expected single top-level article, found several ones.");
        return false;
    }

    const TArticleDescriptorProto& artProto = fileProto.article(0);
    if (artProto.type() != descriptor.full_name()) {
        builder.AddError(Substitute("Expected article with type \"$0\", found \"$1\"", descriptor.full_name(), artProto.type()));
        return false;
    }

    return builder.FillArticle(&message, artProto.field());
}

// TArticleReader ================================================
// Helper classes with for reading article from binary input.

typedef NProtoBuf::io::CodedInputStream TCodedInputStream;

class TArticleReaderBase {
public:
    inline TArticleReaderBase(TCodedInputStream* input)
        : Input(input)
    {
    }

    // Reading article entry various parts (Input should be positioned at the beginning of the corresponding part).
    inline ui32 ReadDescriptorIndex(size_t descriptorCount) {
        ui32 res;
        if (Input->ReadVarint32(&res) && res < descriptorCount)
            return res;
        else
            ythrow yexception() << "Cannot read descriptor from blob.";
    }

    inline void ReadBody(TMessage& result) {
        if (!TMessageSerializer::Load(Input, result))
            ythrow yexception() << "Cannot read article from blob.";
    }

    inline void ReadTitle(Wtroka& result) {
        TStringBuf utf8Name = GetDirectTitleBuffer();
        ::UTF8ToWide(utf8Name, result);
    }

    // Skip descriptor and article body without parsing (so the reader is positioned at the title)
    inline bool SkipToTitle() {
        ui32 descriptor_index;
        return Input->ReadVarint32(&descriptor_index) && TMessageSerializer::Skip(Input);
    }

    // Skip title (should be already positioned on it)
    inline bool SkipTitle() {
        ui32 titleSize;
        return Input->ReadVarint32(&titleSize) && Input->Skip(titleSize);
    }

    // Skip the whole entry and position the reader at the start of next article entry
    inline bool SkipEntry() {
        return SkipToTitle() && SkipTitle();
    }

    TBlob ReadArticleBinary();
    TStringBuf GetDirectTitleBuffer();     // utf-8

    inline bool GetPointer(const char*& ptr) const {
        intptr_t len;
        return Input->GetDirectBufferPointer(reinterpret_cast<const void**>(&ptr), &len);
    }

    void SetDecoder(TCodedInputStream* input) {
        Input = input;
    }

private:
    TCodedInputStream* Input;
};

TBlob TArticleReaderBase::ReadArticleBinary() {
    ui32 descriptor_index, article_size;
    const char* article_start;

    if (!Input->ReadVarint32(&descriptor_index) || !Input->ReadVarint32(&article_size)
        || !GetPointer(article_start))
        ythrow yexception() << "Cannot read article parameters from blob";

    return TBlob::NoCopy(article_start, article_size);
}

TStringBuf TArticleReaderBase::GetDirectTitleBuffer()
{
    const char* error = "Cannot read article name from blob";
    ui32 article_name_size;
    if (!Input->ReadVarint32(&article_name_size))
        ythrow yexception() << error;

    const char* data = NULL;
    if (article_name_size > 0)
        if (!GetPointer(data) || !Input->Skip(article_name_size))
            ythrow yexception() << error;

    return TStringBuf(data, article_name_size);
}


class TArticleReader: public TArticleReaderBase {
public:
    inline TArticleReader(const TBlob& data, ui32 offset = 0, bool verify = true)
        : TArticleReaderBase(NULL)
        , Data(data)
        , Decoder(Data.AsUnsignedCharPtr() + offset, Data.Length() - offset)
    {
        if (EXPECT_FALSE(offset >= Data.Length())) {
            if (verify)
                ythrow yexception() << "Gzt-article requested at invalid offset (" << offset
                                    << "). Article pool has only " << Data.Length() << " bytes of data.";
        }
        SetDecoder(&Decoder);
    }

    inline const char* CurrentPtr() const {
        const char* ptr;
        return TArticleReaderBase::GetPointer(ptr) ? ptr : NULL;
    }

    inline ui32 CurrentOffset() const {
        const char* ptr = CurrentPtr();
        return ptr ? ptr - Data.AsCharPtr() : Data.Length();
    }

    // false if decoder reached an end of blob.
    bool Ok() const {
        return CurrentPtr() != NULL;
    }

    size_t AvailSize() const {
        return Data.Length() - CurrentOffset();
    }

private:
    const TBlob& Data;
    TCodedInputStream Decoder;
};


// TArticleDataDecoder ===========================================

// Special reader for all articles in the pool
class TArticleScanner: public TArticleReader {
public:
    TArticleScanner(const TBlob& data)
        : TArticleReader(data, 0, false)        // accept empty data too
    {
        RestartDecoder();
    }

    bool Ok() {
        if (!TArticleReader::Ok())
            return false;

        // Here we should limit number of bytes read from single instance of CodedInputStream (@Decoder)
        // as it has total_bytes_limit after which it refuses to read more.
        // We could of course increase this total_bytes_limit but
        // a cleaner way would be just to re-create decoder when it is close to the limit.
        // Read at most 16MB from single decoder
        const int MAX_BYTES_FROM_DECODER = 1024*1024*16;
        if (CurrentPtr() - DecoderStart >= MAX_BYTES_FROM_DECODER)
            RestartDecoder();

        return true;
    }

private:
    void RestartDecoder() {
        DecoderStart = CurrentPtr();
        if (DecoderStart) {
            IntDecoder.Reset(new TCodedInputStream((const unsigned char*)DecoderStart, AvailSize()));
            SetDecoder(IntDecoder.Get());
        }
    }

private:
    THolder<TCodedInputStream> IntDecoder;      // replaceable decorder
    const char* DecoderStart;
};



// TArticlePool::TMessageFactory

class TArticlePool::TMessageFactory: public TRecyclePool<TMessage, TMessage> {
public:
    typedef TRecyclePool<TMessage, TMessage> TBase;
    //typedef TBase::TItemPtr TMessagePtr;
    typedef TArticlePool::TMessagePtr TMessagePtr;

    TMessageFactory(const TMessage* prototype)
        : TBase(prototype)
    {
    }

    TMessagePtr New() {
        //return TBase::Pop();
        return TBase::New();
    }
};



// TArticlePool ===================================================


TArticlePool::TArticlePool(TProtoPool& descriptors)
    : Descriptors(&descriptors)
{
}

TArticlePool::~TArticlePool()
{
}

void TArticlePool::Reset(const TBlob& data)
{
    ArticleData = data;
    ResetFactory();
}

void TArticlePool::Load(const NBinaryFormat::TArticlePool& proto, const TBlobCollection& blobs)
{
    yset<ui32> custom_offsets;
    LoadSetFromField(proto.GetCustomArticle(), custom_offsets);
    Reset(blobs[proto.GetArticleData().GetBlobKey()]);

    // if there is a ready title index serialized in the binary, load it.
    const Stroka& titleBlobKey = proto.GetTitleIndex().GetBlobKey();
    if (blobs.HasBlob(titleBlobKey)) {
        TitleIndexData = blobs[titleBlobKey];
        TitleIndex.Reset(new TTitleMap);
        TitleIndex->Init(TitleIndexData);
    }

    // build descriptor index for custom articles
    for (yset<ui32>::const_iterator it = custom_offsets.begin(); it != custom_offsets.end(); ++it)
    {
        CustomArticles[FindArticleNameByOffset(*it)] = *it;
        CustomDescriptors[FindDescriptorByOffset(*it)].insert(*it);
    }
}

void TArticlePool::Load(TMemoryInput* input)
{
    LoadAsProto<NBinaryFormat::TArticlePool>(input, *this);
}

void TArticlePool::ResetFactory()
{
    // initialize all registered descriptor prototypes at once.
    Factory.resize(Descriptors->Size());
    for (size_t i = 0; i < Descriptors->Size(); ++i) {
        const TMessage* prototype = Descriptors->GetPrototype((*Descriptors)[i]);
        Factory[i] = new TMessageFactory(prototype);
    }
}

class TArticlePool::TIterator::TImpl: public TArticleScanner {
public:
    TImpl(const TArticlePool& p)
        : TArticleScanner(p.ArticleData)
    {
    }
};

TArticlePool::TIterator::TIterator(const TArticlePool& p)
    : Impl(new TImpl(p))
    , Pool(p)
    , Offset(Impl->CurrentOffset())
{
}

TArticlePool::TIterator::~TIterator() {
}

void TArticlePool::TIterator::operator++() {
    Impl->SkipEntry();
    Offset = Impl->CurrentOffset();
}

TArticlePool::TMessagePtr TArticlePool::LoadArticleAtOffset(ui32 offset) const {
    TArticleReader reader(ArticleData, offset);
    ui32 descrIndex = reader.ReadDescriptorIndex(Descriptors->Size());
    TMessagePtr ret = Factory[descrIndex]->New();
    reader.ReadBody(*ret);
    return ret;
}

TArticlePool::TMessagePtr TArticlePool::LoadArticleAtOffset(ui32 offset, Wtroka& name) const {
    TArticleReader reader(ArticleData, offset);
    ui32 descrIndex = reader.ReadDescriptorIndex(Descriptors->Size());
    TMessagePtr ret = Factory[descrIndex]->New();
    reader.ReadBody(*ret);
    reader.ReadTitle(name);
    return ret;
}

const TDescriptor* TArticlePool::FindDescriptorByOffset(ui32 offset) const {
    return (*Descriptors)[TArticleReader(ArticleData, offset).ReadDescriptorIndex(Descriptors->Size())];
}

Wtroka TArticlePool::FindArticleNameByOffset(ui32 offset) const {
    Wtroka name;
    TArticleReader reader(ArticleData, offset);
    reader.SkipToTitle();
    reader.ReadTitle(name);
    return name;
}

TBlob TArticlePool::GetArticleBinaryAtOffset(ui32 offset) const {
    return TArticleReader(ArticleData, offset).ReadArticleBinary();
}

bool TArticlePool::FindArticleByName(const TWtringBuf& title, TArticlePtr& article) const {
    if (TitleIndex.Get() == NULL)
        return false;

    ui32 offset;
    if (!TitleIndex->Find(~title, +title, &offset))
        return false;

    article = TArticlePtr(offset, *this);
    return true;
}

bool TArticlePool::FindCustomArticleByName(const Wtroka& name, TArticlePtr& article) const {
    yhash<Wtroka, ui32>::const_iterator res = CustomArticles.find(name);
    if (res != CustomArticles.end()) {
        article = TArticlePtr(res->second, *this);
        return true;
    } else
        return false;
}

const yset<ui32>* TArticlePool::FindCustomArticleOffsetsByType(const TDescriptor* type) const {
    yhash<const TDescriptor*, yset<ui32> >::const_iterator it = CustomDescriptors.find(type);
    return (it != CustomDescriptors.end()) ? &it->second : NULL;
}


void TArticlePoolBuilder::BuildTitleMap(const TBlob& articleData, TBuffer& result) // static
{
    TCompactTrieBuilder<wchar16, ui32> trieBuilder;
    Wtroka tmp;
    TArticleScanner reader(articleData);
    while (reader.Ok()) {
        ui32 offset = reader.CurrentOffset();
        reader.SkipToTitle();
        TStringBuf article_name = reader.GetDirectTitleBuffer();
        if (!article_name.empty()) {
            TWtringBuf wtitle = ::UTF8ToWide(article_name, tmp);
            trieBuilder.Add(~wtitle, +wtitle, offset);
        }
    }
    TBufferOutput output(result);
    trieBuilder.Save(output);
}

void TArticlePool::BuildTitleMap(TTitleMap& result) const
{
    if (TitleIndex.Get() != NULL) {
        result.Init(TitleIndexData);
    } else {
        TBuffer buffer;
        TArticlePoolBuilder::BuildTitleMap(ArticleData, buffer);
        result.Init(TBlob::FromBuffer(buffer));
    }
}

void TArticlePool::BuildInternalTitleMap()
{
    if (TitleIndex.Get() == NULL) {
        TBuffer buffer;
        TArticlePoolBuilder::BuildTitleMap(ArticleData, buffer);
        TitleIndexData = TBlob::FromBuffer(buffer);
        TitleIndex.Reset(new TTitleMap);
        TitleIndex->Init(TitleIndexData);
    }
}

Stroka TArticlePool::DebugString(const TMessage* article, const Stroka title) const
{
    NProtoBuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(false);
    printer.SetUseShortRepeatedPrimitives(true);
    printer.SetUseUtf8StringEscaping(true);
    printer.SetInitialIndentLevel(1);

    Stroka article_text;
    printer.PrintToString(*article, &article_text);

    TStringStream res;
    res << article->GetDescriptor()->full_name() << ' ' << title << " {\n" << article_text << "}\n";
    return res;
}

void TArticlePool::DebugString(TOutputStream& output) const
{
    // prepare placeholders for articles
    yvector<TMessagePtr> cache(Factory.size());
    for (size_t i = 0; i < Factory.size(); ++i)
        cache[i] = Factory[i]->New();

    Wtroka title;
    Stroka utf8Title;

    TArticleScanner reader(ArticleData);
    while (reader.Ok())
    {
        ui32 descriptor_index = reader.ReadDescriptorIndex(cache.size());
        reader.ReadBody(*cache[descriptor_index]);
        reader.ReadTitle(title);
        ::WideToUTF8(title, utf8Title);
        output << DebugString(cache[descriptor_index].Get(), utf8Title) << "\n";
        cache[descriptor_index]->Clear();
    }
}


#undef DO

} // namespace NGzt
