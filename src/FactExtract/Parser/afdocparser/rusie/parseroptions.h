#pragma once

#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/libxmlutil.h>
#include <FactExtract/Parser/common/gramstr.h>


struct SKWDumpOptions {

    bool m_bShowText;
    bool m_bPlainText;
    Stroka m_strTextFileName;
    Stroka m_strXmlFileName;

    typedef yset<TUnresolvedArtPointer> TUnresolvedArticles;
    TUnresolvedArticles UnresolvedMultiWords;
    TUnresolvedArticles UnresolvedSit;

    // same articles resolved in gazetteer
    yset<SArtPointer> MultiWordsToFind;
    yset<SArtPointer> SitToFind;


    SKWDumpOptions()
        : m_bShowText(true)
        , m_bPlainText(false)
    {
    }

    bool HasArticleTitleOrKW(const Wtroka& strArt, TKeyWordType t) const;

private:
    bool HasArticle(const SArtPointer& art) const;
    bool HasArticle(const TUnresolvedArtPointer& art) const;
};

struct SFragmOptions {
    yset<TUnresolvedArtPointer> m_MultiWordsToFind;
    SFragmOptions();
};

class TTextMinerConfig;

class CParserOptions {

public:
    struct SFactOutputParams {
        SFactOutputParams()
            : m_bPrintEquality(true)
        {
        }

        bool m_bPrintEquality;
    };

    class COutputOptions {
        friend class CParserOptions;
        public:
            COutputOptions()
            {
                m_bShowFactsWithEmptyField = false;
            }

            bool HasFactToShow(const Stroka& s) const;
            bool HasToPrintFactEquality(const Stroka& s) const;
            bool ShowFactsWithEmptyField() const;
        protected:
            //показывать ли факты с обязательным полем, заполненным "empty"
            bool    m_bShowFactsWithEmptyField;
            ymap<Stroka, SFactOutputParams> m_FactsToShow;
    };

public:
    CParserOptions();

    // reads specified xml file, initializes m_strFdoDic and returns partially parsed xml for further processing
    TXmlDocPtr ReadXMLAndInitFdoDir(const Stroka& name);
    void ParseXML(const TXmlDocPtr& xml);
    void ParseXMLFile(const Stroka& name);  // parse file without kwtypes references
    void InitFromProtobuf(const Stroka& name);
    void InitFromConfigObject(const TTextMinerConfig& config);

    bool HasFactToShow(const Stroka& s) const;
    void ResolveOptionArticles();

protected:
    bool ResolveArticleByTitle(const Wtroka& title, yset<SArtPointer>& resolved) const;
    void ResolveArticle(const TUnresolvedArtPointer& art, yset<SArtPointer>& resolved) const;
    void ParseParserOptions(TXmlNodePtrBase piNode);
    void ParseFragmOptions(TXmlNodePtrBase piNode);
    void ParseListOfMW(TXmlNodePtrBase piNode, yset<TUnresolvedArtPointer>& artPointers);
    void ParseListOfFacts(TXmlNodePtrBase piNode);
    bool Resolved;
    bool m_IgnoreUpperCase;

public:
    void AddFactToShow(const Stroka& name, bool bPrintEquality);
    void AddArticle(const Wtroka& name);
    void AddSituation(const Wtroka& name);
    void SetIgnoreUpperCase(const bool ignoreUpperCase);
    bool GetIgnoreUpperCase() const;

public: // data

    SKWDumpOptions m_KWDumpOptions;
    THolder<SFragmOptions> m_FragmOptions;
    COutputOptions m_ParserOutputOptions;
    size_t m_maxNamesCount;
};
