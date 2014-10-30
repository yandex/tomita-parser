#pragma once

#include <util/generic/stroka.h>
#include <util/stream/output.h>
#include <util/stream/null.h>

#include <FactExtract/Parser/auxdictlib/articles_parser.h>
#include <FactExtract/Parser/auxdictlib/dictindex.h>
#include <FactExtract/Parser/tomalanglib/abstract.h>

#define DEFAULT_GRAMROOT_FILE "articles.gzt"
#define DEFAULT_GRAMROOT_DIR "fdo"

// Defined  in this file:
class CTDCompiler;
class TLazyGztReferrer;

// forward declarations
class TTomitaCompiler;
class TFactTypeHolder;

namespace NGzt { class TGazetteer; }

class TLazyGztReferrer {
public:
    TLazyGztReferrer(const NGzt::TGazetteer& gzt);
    ~TLazyGztReferrer();

    bool HasArticle(const TWtringBuf& title) const;
    bool HasArticleField(const TWtringBuf& title, const Stroka& field) const;

private:
    void Init() const;
    bool HasArticleField(ui32 articleId, const Stroka& field) const;

    const NGzt::TGazetteer* Gazetteer;

    struct TTitleIndex;
    mutable THolder<TTitleIndex> TitleIndex;        // filled lazily, on first request
    TMutex Mutex;
};

class CTDCompiler: public IArticleNames, public IFactTypeDictionary,
                   public IExternalArticles // for aux-dic
{
public:
    CTDCompiler(const Stroka& dictFolder, const Stroka& binFolder, const Stroka& strGramRoot = DEFAULT_GRAMROOT_FILE, TOutputStream& log = Cnull);
    virtual ~CTDCompiler();

    typedef TBinaryGuard::ECompileMode ECompileMode;

    // Full compilation: gazetteer + aux-dic + grammars referred either from aux-dic, or from articles.gzt
    bool Compile(const Stroka& auxDicFile, ECompileMode mode = TBinaryGuard::Normal, bool forceTomita = false);
private:
    // Separate compilation
    bool CompileGztArticles(const Stroka& auxdic_path, ECompileMode mode);
    bool CompileAuxDic(const Stroka& strFile, ECompileMode mode);
    bool CompileGrammar(const Stroka& grammarFileName, ECompileMode mode);
    bool CompileCustomGztArticles(const Stroka& articlesGztBin, ECompileMode mode);
public:
    inline bool HasGztArticle(const Wtroka& title) const {
        return GztArticles.Get() != NULL &&
               GztArticles->HasArticle(title);
    }

    // implements IExternalArticles (for m_dictIndex) ------
    virtual bool HasArticle(const Wtroka& title) const {
        return HasGztArticle(title);
    }

    // implements IArticleNames ----------------------------
    virtual bool HasArticleOrType(const Wtroka& name) const {
        return
            // either kwtype name
            GetKWType(WideToChar(name)) != NULL ||
            // or gzt-article name
            HasGztArticle(name) ||
            // or aux-dic article name
            m_dictIndex.has_article(name);
    }

    virtual TKeyWordType GetKWType(const Stroka& name) const {
        return GetKWTypePool().FindMessageTypeByName(name);
    }

    virtual bool HasArticleField(const Wtroka& name, const Stroka& field) const {
        // either kwtype name
        TKeyWordType kwtype = GetKWType(WideToChar(name));
        if (kwtype != NULL)
            return kwtype->FindFieldByName(field);

        // or gzt-article name
        return GztArticles.Get() != NULL
            && GztArticles->HasArticleField(name, field);
    }

    // implements IFactTypeDictionary ----------------------
    virtual const fact_type_t* GetFactType(const Stroka& fact) const;

private:
    bool NeedToBeCompiled(Stroka strSrcFile, Stroka strBinFile);

    THolder<NGzt::TGazetteer> Gazetteer;
    THolder<TLazyGztReferrer> GztArticles;

    Stroka DictFolder, BinFolder;
    Stroka GramRootFile; // file name (articles.gzt)
    Stroka ArticlesGzt, ArticlesGztBin;     // usually dic/fdo/articles.gzt and dic/fdo/articles.gzt.bin
    CIndexedDictionary m_dictIndex;
    THolder<articles_parser_t> m_pParser;
    THolder<TTomitaCompiler> TomitaCompiler;

    THolder<TFactTypeHolder> FactTypes;


    TOutputStream& Log;
};

