#include "parseroptions.h"
#include <FactExtract/Parser/common/pathhelper.h>
#include <FactExtract/Parser/common/libxmlutil.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/common/gztarticle.h>
#include <kernel/gazetteer/protopool.h>
#include <kernel/gazetteer/config/protoconf.h>

#include "dictsholder.h"
#include "FactExtract/Parser/common/textminerconfig.pb.h"

//================================================================
//-------------- SKWDumpOptions ----------------------------------
//================================================================

static inline bool MatchUnresolved(const TUnresolvedArtPointer& stored, const TUnresolvedArtPointer& searched) {

    // @stored could be a mask (with * or /)

    if (!searched.ArticleTitle.empty() && !stored.ArticleTitle.empty() && CDictsHolder::IsTitleMatch(searched.ArticleTitle, stored.ArticleTitle))
        return true;

    if (!searched.KWTypeTitle.empty() && !stored.KWTypeTitle.empty() && stored.KWTypeTitle == searched.KWTypeTitle)
        return true;

    return false;
}

bool SKWDumpOptions::HasArticle(const TUnresolvedArtPointer& art) const {
    for (TUnresolvedArticles::const_iterator it = UnresolvedMultiWords.begin(); it != UnresolvedMultiWords.end(); ++it) {
        if (MatchUnresolved(*it, art))
            return true;
    }
    return false;
}

bool SKWDumpOptions::HasArticle(const SArtPointer& art) const {
    TUnresolvedArtPointer tmp;
    tmp.ArticleTitle = art.GetStrType();
    if (art.HasKWType())
        tmp.KWTypeTitle = art.GetKWType()->name();
    return HasArticle(tmp);
}

bool SKWDumpOptions::HasArticleTitleOrKW(const Wtroka& strArt, TKeyWordType t) const {
    return  HasArticle(SArtPointer(t, strArt)) ||
            HasArticle(SArtPointer(t)) ||
            HasArticle(SArtPointer(strArt));

}

// SFragmOptions ======================================================

SFragmOptions::SFragmOptions() {
    const TFixedKWTypeNames& kwtypes = GetFixedKWTypeNames();

    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.Number));
    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.Geo));
    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.Date));
    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.CompanyAbbrev));
    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.CompanyInQuotes));
    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.Fio));
    m_MultiWordsToFind.insert(TUnresolvedArtPointer::AsKWType(kwtypes.FioChain));
}

//================================================================
//-------------- CParserOptions ----------------------------------
//================================================================

CParserOptions::CParserOptions() {
    Resolved = false;
    m_IgnoreUpperCase = false;
    m_maxNamesCount = 1000;
}


void CParserOptions::ParseXML(const TXmlDocPtr& xml) {
    TXmlNodePtrBase piList = xml->children->children;
    for (; piList.Get() != NULL; piList = piList->next) {
        if (piList.HasName("parser_params"))
            ParseParserOptions(piList);
        else if (piList.HasName("fragm_params"))
            ParseFragmOptions(piList);
        else if (piList.HasName("logs_params"))
            ythrow yexception() << "Deprecated option";
    }
}

void CParserOptions::ParseXMLFile(const Stroka& name) {
    TXmlDocPtr xml = TXmlDocPtr::ParseFromFile(name);
    if (xml.Get() == NULL)
        ythrow yexception() << "Cannot load parser params from " << name << ".";
    ParseXML(xml);
}

void CParserOptions::InitFromProtobuf(const Stroka& name) {
    TTextMinerConfig c;
    if (!NProtoConf::LoadFromFile(name, c))
       ythrow yexception() << "Cannot read the config from \"" << name << "\".";

    InitFromConfigObject(c);
}

void CParserOptions::InitFromConfigObject(const TTextMinerConfig& config) {
    for (int i = 0; i < config.facts_size(); ++i) {
        TTextMinerConfig::TFactTypeRef ref = config.get_idx_facts(i);
        AddFactToShow(ref.name(), ref.nonequality());
    }

    for (int i = 0; i < config.articles_size(); ++i) {
        TTextMinerConfig::TArticleRef ref = config.get_idx_articles(i);
        AddArticle(UTF8ToWide(ref.name()));
    }

    for (int i = 0; i < config.situations_size(); ++i) {
        TTextMinerConfig::TArticleRef ref = config.get_idx_situations(i);
        AddSituation(UTF8ToWide(ref.name()));
    }

    SetIgnoreUpperCase(config.GetIgnoreUpperCase());

    m_maxNamesCount = config.GetMaxNamesCount();
}

bool CParserOptions::ResolveArticleByTitle(const Wtroka& title, yset<SArtPointer>& resolved) const {
    typedef NGzt::TArticlePool::TTitleMap TTitleMap;
    const wchar16 STAR = '*';

    size_t maxSize = resolved.size() + 100;
    bool found = false;

    TWtringBuf suffix = title;
    TWtringBuf preffix = suffix.NextTok(STAR);
    TTitleMap subtrie = Singleton<CDictsHolder>()->GetGztArticleIndex()->FindTails(~preffix, +preffix);

    for (TTitleMap::TConstIterator it = subtrie.Begin(); it != subtrie.End(); ++it) {
        if (!it.IsEmpty()) {
            Wtroka key = it.GetKey();
            key.prepend(preffix);
            if (CDictsHolder::IsTitleMatch(key, title)) {
                if (title != key)
                    Cerr << "XML parameters: " << NStr::DebugEncode(title) << " -> " << NStr::DebugEncode(key) << Endl;
                resolved.insert(SArtPointer(key));
                found = true;
            }
        }
        if (resolved.size() >= maxSize) {
            Cerr << "XML parameters: too many gzt-articles correspond to \"" << NStr::DebugEncode(title)
                 << "\". Please use more specific name." << Endl;
            ythrow yexception() << "Too many gzt-articles resolved.";
        }
    }

    // it also could be a situation article name from aux_dic_kw.cxx
    if (Singleton<CDictsHolder>()->GetDict(KW_DICT).has_article(title)) {
        resolved.insert(SArtPointer(title));
        found = true;
    }

    return found;
}

void CParserOptions::ResolveArticle(const TUnresolvedArtPointer& art, yset<SArtPointer>& resolved) const {
    bool found = false;
    if (!art.ArticleTitle.empty() && ResolveArticleByTitle(art.ArticleTitle, resolved))
        found = true;

    if (!art.KWTypeTitle.empty() && GlobalDictsHolder->Gazetteer()->ProtoPool().FindMessageTypeByName(art.KWTypeTitle)) {
        resolved.insert(SArtPointer(art, Singleton<CDictsHolder>()->Gazetteer()->ProtoPool()));
        found = true;
    }

    if (!found)
        Cerr << "Unknown article specified (\"" << NStr::DebugEncode(art.ArticleTitle) << "\")." << Endl;

}

void CParserOptions::ResolveOptionArticles() {

    if(Resolved)
        return;
    YASSERT(Singleton<CDictsHolder>()->Gazetteer() != NULL);

    typedef SKWDumpOptions::TUnresolvedArticles TUnresolvedArts;
    {
        const TUnresolvedArts& arts = m_KWDumpOptions.UnresolvedMultiWords;
        for (TUnresolvedArts::const_iterator it = arts.begin(); it != arts.end(); ++it)
            ResolveArticle(*it, m_KWDumpOptions.MultiWordsToFind);
    }

    {
        const TUnresolvedArts& arts = m_KWDumpOptions.UnresolvedSit;
        for (TUnresolvedArts::const_iterator it = arts.begin(); it != arts.end(); ++it)
            ResolveArticle(*it, m_KWDumpOptions.SitToFind);
    }
    Resolved = true;
}


void CParserOptions::ParseFragmOptions(TXmlNodePtrBase piNode) {
    m_FragmOptions.Reset(new SFragmOptions());
    yset<TUnresolvedArtPointer> parsed_mw;
    ParseListOfMW(piNode, parsed_mw);
    if (!parsed_mw.empty())
        m_FragmOptions->m_MultiWordsToFind.swap(parsed_mw);
}

void CParserOptions::ParseListOfMW(TXmlNodePtrBase piNode, yset<TUnresolvedArtPointer>& artPointers) {
    TXmlNodePtrBase piMW = piNode->children;

    for (; piMW.Get() != NULL; piMW = piMW->next) {
        if (!piMW.HasName("mw"))
            continue;
        Stroka str = piMW.GetAttr("kw");

        TUnresolvedArtPointer artPointer;
        if (!str.empty())
            artPointer.KWTypeTitle = str;
        else
            artPointer.ArticleTitle = piMW.GetWAttr("art");

        artPointer.ForLink = piMW.HasAttr("link");
        artPointer.ForLinkFindOnly = piMW.HasAttr("link_find_only");
        artPointer.ForMainPage = piMW.HasAttr("main_page");
        artPointer.ForUrl = piMW.HasAttr("url");

        artPointers.insert(artPointer);

    }
}

void CParserOptions::ParseParserOptions(TXmlNodePtrBase piNode) {
    m_KWDumpOptions.m_bPlainText = piNode.HasAttr("plain_text");
    m_KWDumpOptions.m_bShowText = piNode.HasAttr("show_text");
    m_KWDumpOptions.m_strTextFileName = piNode.GetAttr("plain_text");
    m_KWDumpOptions.m_strXmlFileName = piNode.GetAttr("xml_result");

    if (piNode.HasAttr("log_fios"))
        ythrow yexception() << "log_fios option is not supported any more";

    CGramInfo::s_bRunSituationSearcher = piNode.HasAttr("sit");

    ParseListOfMW(piNode, m_KWDumpOptions.UnresolvedMultiWords);

    TXmlNodePtrBase piList = piNode->children;
    for (; piList.Get() != NULL; piList = piList->next) {
        if (piList.HasName("situations")) {
            ParseListOfMW(piList, m_KWDumpOptions.UnresolvedSit);
        } else if (piList.HasName("facts"))
            ParseListOfFacts(piList);
    }
}

bool CParserOptions::COutputOptions::HasFactToShow(const Stroka& s) const {
    return m_FactsToShow.find(s) != m_FactsToShow.end();
}

bool CParserOptions::COutputOptions::HasToPrintFactEquality(const Stroka& s) const {
    ymap<Stroka, SFactOutputParams>::const_iterator it = m_FactsToShow.find(s);
    if (it == m_FactsToShow.end())
        return false;
    return it->second.m_bPrintEquality;
}

bool CParserOptions::COutputOptions::ShowFactsWithEmptyField() const {
    return m_bShowFactsWithEmptyField;
}

void CParserOptions::ParseListOfFacts(TXmlNodePtrBase piNode) {
    TXmlNodePtrBase piFacts = piNode->children;
    m_ParserOutputOptions.m_bShowFactsWithEmptyField = piNode.HasAttr("addEmpty");
    for (; piFacts.Get() != NULL; piFacts = piFacts->next) {
        if (!piFacts.HasName("fact"))
            continue;

        Stroka str = piFacts.GetAttr("name");
        if (!str.empty())
            AddFactToShow(str, !piFacts.HasAttr("noequality"));
    }
}

void CParserOptions::AddFactToShow(const Stroka& name, bool bPrintEquality) {
    SFactOutputParams factOutputParams;
    factOutputParams.m_bPrintEquality = bPrintEquality;
    m_ParserOutputOptions.m_FactsToShow[name] = factOutputParams;
}

void CParserOptions::AddArticle(const Wtroka& name) {
    m_KWDumpOptions.UnresolvedMultiWords.insert(TUnresolvedArtPointer::Any(name));
}

void CParserOptions::AddSituation(const Wtroka& name) {
    m_KWDumpOptions.UnresolvedSit.insert(TUnresolvedArtPointer::Any(name));
}

void CParserOptions::SetIgnoreUpperCase(const bool ignoreUpperCase) {
    m_IgnoreUpperCase = ignoreUpperCase;
}

bool CParserOptions::GetIgnoreUpperCase() const {
    return m_IgnoreUpperCase;
}
