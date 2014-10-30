#pragma once

#include "common/binaryguard.h"

#include "sourcetree.h"
#include "protopool.h"
#include "articlepool.h"
#include "gzttrie.h"
#include "articleiter.h"
#include "gztarticle.h"


#include <util/generic/ptr.h>
#include <util/generic/hash.h>


namespace NGzt
{

// Defined in this file:
class TGazetteer;
class TGazetteerBuilder;

// re-define basic protobuf types for easy use from outside
typedef NProtoBuf::Message TMessage;
typedef NProtoBuf::Descriptor TDescriptor;
typedef NProtoBuf::FieldDescriptor TFieldDescriptor;
typedef NProtoBuf::Reflection TReflection;

// forward declaration from binary.proto
namespace NBinaryFormat {
    class TGazetteer;
}

class TOptions;


class TGazetteer
{
public:
    // Construct gazetteer based on @data serialized previously with TGazetteerBuilder
    // Note that the gazetteer takes ownership on given blob (increases its ref-counter)
    explicit TGazetteer(const TBlob& data);

    // Construst gazetteer from *.gzt.bin file (produced by gztcompiler, for instance)
    explicit TGazetteer(const Stroka& gztBinFile, bool precharge = false);

    // Empty gazetteer
    TGazetteer();


    // Reset specified @iter to iterate over all article entries in @text from the beginning to the ending.
    // @iter is positionned to the first found article (if any), otherwise @iter will be set empty (iter.Ok() == false).
    template <class TInput>
    inline void IterArticles(const TInput& text, TArticleIter<TInput>* iter) const {
        Trie.IterArticles(text, iter);
    }

    // Returns smart-pointer to protobuf article associated with current article-entry in @article_iterator.
    template <class TInput>
    inline TArticlePtr GetArticle(const TArticleIter<TInput>& iter) const {
        return TArticlePtr(*iter, ArticlePool());
    }

    // reads only descriptor of article (without deserializing article itself - which is much faster)
    template <class TInput>
    inline const TDescriptor* GetDescriptor(const TArticleIter<TInput>& iter) const {
        return ArticlePool().FindDescriptorByOffset(*iter);
    }

    // All stored articles
    inline const TArticlePool& ArticlePool() const {
        return *ArticlePool_;
    }

    // All protobuf descriptors (article types and other, defined in proto files)
    inline const TProtoPool& ProtoPool() const {
        return *ProtoPool_;
    }

    inline const TGztTrie& GztTrie() const {
        return Trie;
    }

    // Build and fill internal article title index,
    // to enable later title -> article search (LoadArticle)
    inline void BuildTitleIndex() {
         ArticlePool_->BuildInternalTitleMap();
    }

    // Find and load article from the pool by its title.
    // If no such article found, an null TArticlePtr is returned.
    TArticlePtr LoadArticle(const TWtringBuf& title) const;

    // Prints all content of gazetteer (article descriptors and article themselves) into @output
    // in human-readable text format (for debugging).
    void DebugString(TOutputStream& output) const;

private:
    void Init();

private:
    TBlob Data;

    TIntrusivePtr<TProtoPool> ProtoPool_;
    TIntrusivePtr<TArticlePool> ArticlePool_;
    TGztTrie Trie;
};


// A gazetteer with collection of gzt-articles, used to avoid loading (deserializing) same article twice.
// This will save some memory and processor time on long texts,
// where the same article is found several times on different words.
class TCachingGazetteer {
public:
    TCachingGazetteer(const TGazetteer& gazetteer)
        : Gazetteer_(gazetteer)
    {
    }

    const TGazetteer& Gazetteer() const {
        return Gazetteer_;
    }

    void Reset() {
        Collection.clear();
    }

    bool Has(ui32 offset) const {
        return Collection.find(offset) != Collection.end();
    }

    TArticlePtr GetArticle(ui32 offset) {
        TPair<TOffsetMap::iterator, bool> ins = Collection.insert(MakePair(offset, TArticlePtr()));
        if (ins.second)
            ins.first->second = TArticlePtr(offset, Gazetteer_.ArticlePool());
        return ins.first->second;
    }

    template <class TInput>
    inline TArticlePtr GetArticle(const TArticleIter<TInput>& iter) {
        return GetArticle(*iter);
    }

private:
    const TGazetteer& Gazetteer_;

    typedef yhash<ui32, TArticlePtr> TOffsetMap;
    TOffsetMap Collection;
};


class TGazetteerBuilder: private IKeyCollector
{
public:
    // @proto_path is analogous of protoc command-line parameter "proto_path".
    // It specifies paths where search for imported proto-files.
    // Could hold several paths delimited by semicolon ";"
    // If empty (default) then only current proto-path is used.
    TGazetteerBuilder(const Stroka& proto_path = "", bool defaultBuiltins = true);
    TGazetteerBuilder(const yvector<Stroka>& proto_paths, bool defaultBuiltins = true);
    virtual ~TGazetteerBuilder();

    // When gzt-compiler should treat some imported filenames as "builtins",
    // i.e. it should not search them on disk and should use corresponding protos from generated_pool instead.
    // These files will also be serialized into gzt.bin with a special flag in binary-guard header.
    // Files from NBuiltin::DefaultProtoFiles() (base.proto and protobuf/descriptor.proto) are built-in by default
    // unless @defaultBuiltins was false on construction.
    void IntegrateFiles(const NBuiltin::TFileCollection& files) {
        SourceTree.IntegrateFiles(files);
    }

    // Instead of parsing imported proto-files from disk, use ready compiled descriptors
    // from supplied proto pool of some other gazetteer. If @descriptors do not contain
    // a proto imported by currently compiled gazetteer, it will be still parsed from disk.
    void UseDescriptors(const TProtoPool& descriptors);
    void UseDescriptors(const TGazetteer& gazetteer) {
        UseDescriptors(gazetteer.ProtoPool());
    }

    // If you have a ready protobuf-generated class TProto it is possible to use
    // its builtin descriptor definition (generated_pool) directly
    template <typename TProto>
    void UseBuiltinDescriptor() {
        SourceTree.IntegrateBuiltinFile<TProto>();
    }

    void UseAllBuiltinDescriptors() {
        SourceTree.MaximizeBuiltins();
    }


    // All-in-one quick helpers -----------------------------------------------

    // Compiles specified @gztfile and saves its binary to @binfile.
    // If @binfile exists and up-to-date, do compilation only if @force is true
    bool Compile(const Stroka& gztfile, const Stroka& binfile, bool force = false);
    // Same with some extra information about compilation printed to @log
    bool CompileVerbose(const Stroka& gztfile, const Stroka& binfile, TOutputStream& log, bool force = false);

    // Compiles @gztfile and transform it to gazetteer on-the-fly
    TAutoPtr<TGazetteer> Compile(const Stroka& gztFile) {
        return BuildFromFile(gztFile) ? MakeGazetteer() : NULL;
    }


    // Basic steps of compilation: parse, build and serialize ------------------

    // Parse and load all data from specified .gzt file (or its content) and imported .gzt and .proto files
    // Save() or MakeGazetteer() should be called after.
    bool BuildFromFile(const Stroka& diskFile);
    bool BuildFromText(const Stroka& text);
    bool BuildFromStream(TInputStream* input);
    bool BuildFromProtobuf(const TGztFileDescriptorProto& parsedFile);      // see syntax.proto

    // deprecated synonym for BuildFromFile, to be removed
    bool BuildFile(const Stroka& diskFile) {
        return BuildFromFile(diskFile);
    }

    void Save(TOutputStream* output) const;
    void Save(NBinaryFormat::TGazetteer& proto, TBlobCollection& blobs) const;

    // Transform built data into gazetteer directly (without intermediate .gzt.bin on disk)
    TAutoPtr<TGazetteer> MakeGazetteer() const;


    // Misc helpers -------------------------------------------------------------

    // Loads custom keys from binary file (*.gzt.bin usually) into @keys
    static bool LoadCustomKeys(const Stroka& bin_file, yset<Stroka>& keys);


    inline const TBinaryGuard& BinaryGuard() {
        return SourceTree;
    }

    // Returns true when specified binary is consistent with specified source file -
    // and therefore need not be re-compiled.
    inline bool IsGoodBinary(const Stroka& gzt_file, const Stroka& bin_file) {
        TDiskSourceTree::TRootFile root(gzt_file, SourceTree);
        return BinaryGuard().IsGoodBinary(gzt_file, bin_file);
    }

    // This is required for de-virtualizing custom sources filenames
    inline TGztSourceTree& GetSourceTree() {
        return SourceTree;
    }

private:
    // implements IKeyCollector
    virtual void Collect(const NProtoBuf::Message& article, ui32 offset, const TOptions& options);

private:
    TGztSourceTree SourceTree;

    TProtoPoolBuilder ProtoPoolBuilder;
    TArticlePoolBuilder ArticlePoolBuilder;
    TGztTrieBuilder TrieBuilder;

    THolder<TSearchKey> TmpKey;
};


inline void VerifyGazetteerBinary(const Stroka& gztSrc, const Stroka& gztBin, const Stroka& importPath) {
    TGztSourceTree gztTree(importPath);
    gztTree.VerifyBinary(gztSrc, gztBin);
}

} // namespace NGzt

// make these names global
using NGzt::TGazetteer;
using NGzt::TGazetteerBuilder;
using NGzt::TArticleIter;
using NGzt::TArticlePtr;
