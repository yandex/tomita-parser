#include <util/ysaveload.h>
#include <util/system/fstat.h>
#include <util/folder/path.h>

#include "gazetteer.h"
#include "builtin.h"
#include "version.h"
#include "options.h"

#include <kernel/gazetteer/binary.pb.h>
#include <kernel/gazetteer/base.pb.h>



namespace NGzt {

// TGazetteer =========================================================

TGazetteer::TGazetteer(const TBlob& data)
    : Data(data)
    , ProtoPool_(new TProtoPool(NULL))
    , ArticlePool_(new TArticlePool(*ProtoPool_))
    , Trie()
{
    Init();
}

TGazetteer::TGazetteer(const Stroka& gztBinFile, bool precharge)
    : Data()
    , ProtoPool_(new TProtoPool(NULL))
    , ArticlePool_(new TArticlePool(*ProtoPool_))
    , Trie()
{
    if (precharge)
        Data = TBlob::FromFileContent(gztBinFile);
    else
        Data = TBlob::FromFile(gztBinFile);
    Init();
}

TGazetteer::TGazetteer()
    : Data()
    , ProtoPool_(new TProtoPool(NULL))
    , ArticlePool_(new TArticlePool(*ProtoPool_))
    , Trie()
{
}

void TGazetteer::Init()
{
    TMemoryInput input(Data.Data(), Data.Length());

    TGztBinaryVersion::Verify(&input);

    NBinaryFormat::TGazetteer proto;
    TBlobCollection blobs;
    LoadProtoBlobs(&input, proto, blobs);

    ProtoPool_->Load(proto.GetDescriptors());
    ArticlePool_->Load(proto.GetArticles(), blobs);
    Trie.Load(proto.GetTrie(), blobs);
}

TArticlePtr TGazetteer::LoadArticle(const TWtringBuf& title) const
{
    TArticlePtr art;
    ArticlePool().FindArticleByName(title, art);
    return art;
}

void TGazetteer::DebugString(TOutputStream& output) const
{
    output << "// DESCRIPTORS (PROTO)\n";
    output << ProtoPool().DebugString() << "\n\n";
    output << "// ARTICLES (GZT)\n\n";
    ArticlePool().DebugString(output);
}


// TGazetteerBuilder ===================================================

TGazetteerBuilder::TGazetteerBuilder(const Stroka& proto_path, bool defaultBuiltins)
    : SourceTree(proto_path, defaultBuiltins)
    , ProtoPoolBuilder(&SourceTree)
    , ArticlePoolBuilder(&SourceTree, &ProtoPoolBuilder, this)
    , TrieBuilder(&ArticlePoolBuilder)
    , TmpKey(new TSearchKey)
{
}

TGazetteerBuilder::TGazetteerBuilder(const yvector<Stroka>& proto_paths, bool defaultBuiltins)
    : SourceTree(proto_paths, defaultBuiltins)
    , ProtoPoolBuilder(&SourceTree)
    , ArticlePoolBuilder(&SourceTree, &ProtoPoolBuilder, this)
    , TrieBuilder(&ArticlePoolBuilder)
    , TmpKey(new TSearchKey)
{
}

TGazetteerBuilder::~TGazetteerBuilder() {
    // for THolder<TSearchKey>
}

void TGazetteerBuilder::UseDescriptors(const TProtoPool& descriptors) {
    ProtoPoolBuilder.AddPool(descriptors);
}

bool TGazetteerBuilder::BuildFromFile(const Stroka& filename)
{
    return ArticlePoolBuilder.FromDiskFile(filename);
}

bool TGazetteerBuilder::BuildFromText(const Stroka& text)
{
    return ArticlePoolBuilder.FromText(text);
}

bool TGazetteerBuilder::BuildFromStream(TInputStream* input)
{
    return ArticlePoolBuilder.FromStream(input);
}

bool TGazetteerBuilder::BuildFromProtobuf(const TGztFileDescriptorProto& parsedFile) {
    return ArticlePoolBuilder.FromProtobuf(parsedFile);
}

void TGazetteerBuilder::Save(NBinaryFormat::TGazetteer& proto, TBlobCollection& blobs) const
{
    // header info
    ArticlePoolBuilder.SaveHeader(*proto.MutableHeader());

    // all components
    ProtoPoolBuilder.Save(*proto.MutableDescriptors(), blobs);
    ArticlePoolBuilder.Save(*proto.MutableArticles(), blobs);
    TrieBuilder.Save(*proto.MutableTrie(), blobs);
}

void TGazetteerBuilder::Save(TOutputStream* output) const
{
    TGztBinaryVersion::Save(output);
    SaveAsProto<NBinaryFormat::TGazetteer>(output, *this);
}

TAutoPtr<TGazetteer> TGazetteerBuilder::MakeGazetteer() const {
    TBufferOutput out;
    this->Save(&out);
    return new TGazetteer(TBlob::FromBuffer(out.Buffer()));
}


bool TGazetteerBuilder::LoadCustomKeys(const Stroka& binFile, yset<Stroka>& names)
{
    if (TFileStat(~binFile).Size == 0)
        return false;

    // Here we do not need the whole binary, just a small header, so no precharge or pread please
    TBlob data = TBlob::FromFile(binFile);
    TMemoryInput input(data.Data(), data.Length());

    if (!TGztBinaryVersion::Load(&input))
        return false;

    NBinaryFormat::TGazetteer proto;
    LoadProtoOnly(&input, proto);
    LoadSetFromField(proto.GetHeader().GetCustomKey(), names);
    return true;
}

bool TGazetteerBuilder::CompileVerbose(const Stroka& gztFile, const Stroka& binFile, TOutputStream& log, bool force) {
    TFsPath srcPath(gztFile);
    if (!srcPath.Exists()) {
        log << "Cannot find " << gztFile << "\n" << Endl;
        return false;
    }

    log << "Compiling \"" << gztFile << "\" ..." << Endl;

    if (!force && IsGoodBinary(gztFile, binFile)) {
        log << "Skipped: the binary already exists and consistent with sources.\n"
            << "Use --force option to re-compile the binary explicitly.\n" << Endl;
        return true;
    }

    if (!BuildFromFile(gztFile))
        return false;

    NGzt::TTempFileOutput bin(binFile);
    try {
        Save(&bin);
        bin.Commit();
    } catch (...) {
        log << "Cannot write gzt binary: " << CurrentExceptionMessage() << "\n" << Endl;
        return false;
    }

    log << "Done.\n" << Endl;
    return true;
}

bool TGazetteerBuilder::Compile(const Stroka& gztFile, const Stroka& binFile, bool force) {
    return CompileVerbose(gztFile, binFile, Cnull, force);
}


void TGazetteerBuilder::Collect(const NProtoBuf::Message& article, ui32 offset, const TOptions& options)
{
    const TFieldDescriptor* keyField = ProtoPoolBuilder.Index().GetSearchKeyField(article.GetDescriptor());

    const TSearchKey* defaultKey = NULL;
    if (options.GztOptions.HasDefaultKey())
        defaultKey = &options.GztOptions.GetDefaultKey();

    for (TSearchKeyIterator key(article, keyField, *TmpKey, defaultKey); key.Ok(); ++key)
        TrieBuilder.Add(*key, offset, options);
}


} // namespace NGzt
