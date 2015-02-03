#include "tdcompiler.h"

#include <util/stream/str.h>

#include <FactExtract/Parser/tomalanglib/tomacompiler.h>
#include <FactExtract/Parser/afglrparserlib/glr.h>
#include <FactExtract/Parser/common/pathhelper.h>

#include <FactExtract/Parser/afdocparser/common/facttypeholder.h>
#include <FactExtract/Parser/afdocparser/builtins/builtins.h>

#include <kernel/gazetteer/gazetteer.h>

static Stroka GetGramFilenameWithoutExt(Stroka strFile)
{
    strFile = StripString(strFile);
    size_t ii = strFile.rfind('.');
    if (ii == Stroka::npos)
        return strFile;
    else
        return strFile.substr(0, ii);
}

CTDCompiler::CTDCompiler(const Stroka& dictFolder, const Stroka& binFolder, const Stroka& strGramRoot, TOutputStream& log)
    : GramRootFile(strGramRoot)
    , Log(log)
{
    DictFolder = PathHelper::AppendSlash(dictFolder);
    BinFolder = PathHelper::AppendSlash(binFolder);

    ArticlesGzt = PathHelper::Join(dictFolder, GramRootFile);   // default path to articles.gzt
    ArticlesGztBin = PathHelper::Join(BinFolder, GramRootFile + ".bin");

    m_dictIndex.SetExternalArticles(this);
    m_dictIndex.SetExternalFactTypes(this);

    SDefaultFieldValues defaultFieldValues;
    defaultFieldValues.m_comp_by_lemma = true;
    m_pParser.Reset(new articles_parser_t(m_dictIndex, defaultFieldValues));
    m_pParser->put_dic_folder(DictFolder);

    TomitaCompiler.Reset(new TTomitaCompiler(this, this, Singleton<TFactexTerminalDictionary>(), Log));
}

CTDCompiler::~CTDCompiler() {
    // defined here for sake of TomitaCompiler
}

const fact_type_t* CTDCompiler::GetFactType(const Stroka& fact) const {
    return FactTypes.Get() != NULL ? FactTypes->Find(fact) : NULL;
}

bool CTDCompiler::CompileGztArticles(const Stroka& auxdic_path, ECompileMode mode)
{
    ArticlesGzt = PathHelper::Join(PathHelper::DirName(auxdic_path), GramRootFile);
    {
        TGazetteerBuilder gzt_builder;
        gzt_builder.IntegrateFiles(TomitaBuiltinProtos());

        // setting a root for source-tree for BinaryGuard to search from
        TDiskSourceTree::TRootFile root(ArticlesGzt, gzt_builder.GetSourceTree());
        if (gzt_builder.BinaryGuard().RebuildRequired(ArticlesGzt, ArticlesGztBin, mode)) {

            if (!PathHelper::Exists(ArticlesGzt)) {
                Log << ArticlesGzt << "not found, it should be placed in the same directory with aux_dic_kw.cxx" << Endl;
                return false;
            }

            (Log << "  Compiling \"" << ArticlesGzt << "\" ... ").Flush();

            if (gzt_builder.BuildFile(ArticlesGzt)) {
                PathHelper::MakePath(PathHelper::DirName(ArticlesGztBin));
                TOFStream gzt_binary(ArticlesGztBin);
                gzt_builder.Save(&gzt_binary);
                Log << "OK" << Endl;
            } else {
                Log << "FAILED!" << Endl;
                return false;
            }
        }
    }

    if (PathHelper::Exists(ArticlesGztBin)) {
        Gazetteer.Reset(new TGazetteer(TBlob::FromFile(ArticlesGztBin)));
        GetMutableKWTypePool().RegisterPool(Gazetteer->ProtoPool());
        GztArticles.Reset(new TLazyGztReferrer(*Gazetteer));
        return true;
    } else
        return false;
}

bool CTDCompiler::CompileGrammar(const Stroka& grammarFileName, ECompileMode mode)
{
    Stroka grammarName = GetGramFilenameWithoutExt(grammarFileName);
    Stroka grammarSrc = PathHelper::Join(DictFolder, grammarName + ".cxx");
    Stroka grammarBin = PathHelper::Join(BinFolder,  grammarName + ".bin");

    if (!TomitaCompiler->CompilationRequired(grammarSrc, grammarBin, mode))
        return true;

    if (!isexist(~grammarSrc)) {
        Log << "Cannot find grammar file or corresponding binary: " <<  grammarFileName << "." << Endl;
        return false;
    }

    (Log << "  Compiling " <<  grammarFileName << " ... ").Flush();

    if (TomitaCompiler->Compile(grammarSrc, grammarBin, true)) {
        Log << "OK" << Endl;
        return true;
    } else
        return false;
}

bool CTDCompiler::CompileCustomGztArticles(const Stroka& articlesGztBin, ECompileMode mode)
{
    const TStringBuf TOMITA_PREFIX = AsStringBuf("tomita:");

    if (!PathHelper::Exists(ArticlesGztBin))
        return false;

    bool result = true;

    yset<Stroka> customGztKeys;
    TGazetteerBuilder::LoadCustomKeys(articlesGztBin, customGztKeys);
    for (yset<Stroka>::const_iterator it = customGztKeys.begin(); it != customGztKeys.end(); ++it) {
        Stroka key = *it;
        if (!key.has_prefix(TOMITA_PREFIX))
            continue;

        key = key.substr(TOMITA_PREFIX.size());
        if (!CompileGrammar(key, mode))
            result = false;
    }

    return result;
}

static inline Stroka GetBinFileName(const Stroka& strFile, const Stroka& binFolder) {
    Stroka strFileWithoutExt = GetGramFilenameWithoutExt(strFile);
    return PathHelper::Join(binFolder, PathHelper::BaseName(strFileWithoutExt)) + ".bin";
}

bool CTDCompiler::CompileAuxDic(const Stroka& strFile, ECompileMode mode)
{
    Stroka binFileName = GetBinFileName(strFile, BinFolder);
    if (m_dictIndex.CompilationRequired(strFile, binFileName, mode)) {

        if (!isexist(~strFile)) {
            // still return true - we can continue with empty auxdic
            return true;
        }

        (Log << "  Compiling \"" << strFile << "\" ... ").Flush();
        if (!m_pParser->process(strFile) || m_pParser->m_error)
            return false;

        PathHelper::MakePath(PathHelper::DirName(binFileName));
        m_dictIndex.Init();
        m_dictIndex.InitKWIndex();
        m_dictIndex.SaveToFile(binFileName);
        Log << "OK" << Endl;
    }
    return true;
}

bool CTDCompiler::Compile(const Stroka& auxDicFile, ECompileMode mode, bool forceTomita)
{
    bool bError = !CompileGztArticles(auxDicFile, mode);
    if (bError)
        return false;   // do not continue on gazetteer compilation errors

    FactTypes.Reset(new TFactTypeHolder);
    if (Gazetteer.Get() != NULL)
        FactTypes->AddFromGazetteer(Gazetteer->ProtoPool());

    ECompileMode modeTomita = forceTomita ? TBinaryGuard::Forced : mode;

    Stroka binFileName = GetBinFileName(auxDicFile, BinFolder);
    if (CompileAuxDic(auxDicFile, modeTomita)) {
        if (isexist(~binFileName))
            m_dictIndex.LoadFromFile(binFileName);
        // otherwise continue with empty aux-dic
    } else
        bError = true;

    FactTypes->Add(m_dictIndex.GetFactTypes());

    //compile grammars, referred from aux-dic index
    for (size_t i = 0; i < m_dictIndex.size(); ++i) {
        const article_t* pArt = m_dictIndex.GetArticle(i);
        if (pArt->has_gram_file_name() && !CompileGrammar(pArt->get_gram_file_name(), modeTomita))
            bError = true;
    }

    // compile grammars, referred from article.gzt
    if (!CompileCustomGztArticles(ArticlesGztBin, modeTomita))
        bError = true;

    return !bError;
}

struct TLazyGztReferrer::TTitleIndex {
    NGzt::TArticlePool::TTitleMap Impl;
};

TLazyGztReferrer::TLazyGztReferrer(const TGazetteer& gzt)
    : Gazetteer(&gzt)
{
}

TLazyGztReferrer::~TLazyGztReferrer() {
}

void TLazyGztReferrer::Init() const {
    TGuard<TMutex> guard(Mutex);
    if (TitleIndex.Get() == NULL) {
        TitleIndex.Reset(new TTitleIndex);
        Gazetteer->ArticlePool().BuildTitleMap(TitleIndex->Impl);
    }
}

bool TLazyGztReferrer::HasArticle(const TWtringBuf& title) const {
    if (TitleIndex.Get() == NULL)
        Init();

    const wchar16 sep = NGzt::ARTICLE_FOLDER_SEPARATOR;
    NGzt::TArticlePool::TTitleMap tails = TitleIndex->Impl.FindTails(~title, +title);
    return tails.Find(&sep, 0) || !tails.FindTails(&sep, 1).IsEmpty();
}

bool TLazyGztReferrer::HasArticleField(ui32 articleId, const Stroka& field) const {
    const NGzt::TDescriptor* descr = Gazetteer->ArticlePool().FindDescriptorByOffset(articleId);
    return descr != NULL && descr->FindFieldByName(field);
}

bool TLazyGztReferrer::HasArticleField(const TWtringBuf& title, const Stroka& field) const {
    if (TitleIndex.Get() == NULL)
        Init();

    const wchar16 sep = NGzt::ARTICLE_FOLDER_SEPARATOR;
    NGzt::TArticlePool::TTitleMap tails = TitleIndex->Impl.FindTails(~title, +title);

    ui32 id = 0;
    if (tails.Find(&sep, 0, &id) && HasArticleField(id, field))
        return true; // exact title match

    // title/sub-articles
    tails = tails.FindTails(&sep, 1);
    for (NGzt::TArticlePool::TTitleMap::TConstIterator it = tails.Begin(); it != tails.End(); ++it)
        if (!it.IsEmpty() && HasArticleField(it.GetValue(), field))
            return true;

    return false;
}



