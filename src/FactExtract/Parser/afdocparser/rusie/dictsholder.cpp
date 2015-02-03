#include <util/system/mutex.h>
#include <util/stream/file.h>
#include <util/memory/blob.h>
#include <util/system/oldfile.h>

#include <FactExtract/Parser/common/pathhelper.h>

#include <FactExtract/Parser/afdocparser/common/gztarticle.h>

#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>

#include <FactExtract/Parser/afdocparser/aux_dic_frag/aux_dic_frag.h>

#include "dictsholder.h"
#include "normalization.h"

#include <FactExtract/Parser/tomadictscompiler/tdcompiler.h>

#ifndef TOMITA_EXTERNAL
#include <FactExtract/Parser/tag_lib/tomita/tagger.h>
#endif

#include <FactExtract/Parser/afdocparser/builtins/articles_base.pb.h>

#include <FactExtract/Parser/surname_predictor/surname_predictor.h>


const char g_FdoDicDir[] = "fdo"; //will be deleted after @lazyfrog's changes

//---------------------class CDictsHolder------------------------------------

CDictsHolder::CDictsHolder()
{
    m_bFramDictsLoaded = false;
    RootFile = DEFAULT_GRAMROOT_FILE;
    PathCompatibilityMode = true;
}

CDictsHolder::~CDictsHolder()
{
    for (int i = 0; i < DICTYPE_COUNT; ++i)
        m_Dicts[i].Clear();

    for (TGrammarIndex::iterator it = RegisteredGrammars.begin(); it != RegisteredGrammars.end(); ++it)
        delete it->second;

#ifndef TOMITA_EXTERNAL
    for (TTaggerIndex::iterator it = RegisteredTaggers.begin(); it != RegisteredTaggers.end(); ++it)
        delete it->second;
#endif
}

void CDictsHolder::InitRoot(const Stroka& strDicPath, const Stroka& strBinPath)
{
    TStringBuf strDir, strFile;
    PathHelper::Split(strDicPath, strDir, strFile);
    if (PathHelper::IsDir(strDicPath)) {
        DicPath = strDicPath;
        DicFdoPath = PathHelper::Join(DicPath, g_FdoDicDir);
    } else {
        DicPath = DicFdoPath = strDir;
        RootFile = strFile;
        PathCompatibilityMode = false;
    }

    if (strBinPath.empty())
        BinPath = DicFdoPath;
    else
        BinPath = strBinPath;
}

void CDictsHolder::Init(const Stroka& strDicPath, const Stroka& strBinPath)
{
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    // if gazetteer is initialized - treat the whole dictsholder as already initialized.
    if (m_Gazetteer.Get() == NULL) {

        InitRoot(strDicPath, strBinPath);

        //// two basic scenarios:
        //// 1. there is xml-file with parameters
        //// 2. parameters are given with command line

        {
            // recompile gazetteer, aux-dic or tomita grammars if required
            CTDCompiler compiler(DicFdoPath, BinPath, RootFile, Clog);
            if (!compiler.Compile(PathHelper::Join(DicFdoPath, g_strKWDicSrc), s_bForceRecompile ? TBinaryGuard::Forced : TBinaryGuard::Optional, s_bForceRecompile)) {
                Clog << "Missing required data binaries and failed to re-compile." << Endl;
                ythrow yexception() << "Cannot live anymore.";
            }
        }

        InitGazetteer();
        if (s_bNeedAuxKwDict)
            InitAuxKwDict();
        InitFactTypes();

        InitGazetteerTitleIndexImpl();

        if (s_bLoadLemmaFreq)
            InitLemmaFreq();

    }
}

void CDictsHolder::InitNames(const Stroka& /*strNameFile*/, ECharset /*encoding*/) {
    InitNamesFromProtobuf();
}

void CDictsHolder::InitNamesFromProtobuf() {
    if (NULL != m_FirstNames.Get())
        return;

    m_FirstNames.Reset(new TNameIndex);

    const yset<ui32> *s = m_Gazetteer->ArticlePool().FindCustomArticleOffsetsByType(TNameThesaurusEntry::descriptor());

    if (NULL != s) {
        yset<ui32>::const_iterator it = s->begin();

        for (; s->end() != it; ++it) {
            NGzt::TArticlePtr art(*it, m_Gazetteer->ArticlePool());
            const TNameThesaurusEntry *entry = art.As<TNameThesaurusEntry>();
            if (NULL == entry)
                ythrow yexception() << "Failed while loading names thesaurus entry from protobuf.";

            Wtroka name = UTF8ToWide(entry->Getname());
            if (0 == name.length())
                continue;

            (*m_FirstNames)[name] = SName();

            for (size_t i = 0; i < entry->diminutiveSize(); ++i) {
                Wtroka dim = UTF8ToWide(entry->Getdiminutive(i));
                SName dimName(false);
                TNameIndex::iterator it = m_FirstNames->find(name);
                if (it != m_FirstNames->end())
                    it->second.m_NameVariants.push_back(dim);
                dimName.m_NameVariants.push_back(name);
                (*m_FirstNames)[dim] = dimName;
            }
        }
    } else {
        // TODO: print warning in case VERBOSE MODE is ON
    }
}

TSurnamePredictor* CDictsHolder::LoadSurnamePrediction() const {
    TSurnamePredictor *p = new TSurnamePredictor;

    const yset<ui32> *s = m_Gazetteer->ArticlePool().FindCustomArticleOffsetsByType(TSurnameLemmaModel::descriptor());

    if (NULL != s) {
        yset<ui32>::const_iterator it = s->begin();

        for (; s->end() != it; ++it) {
            NGzt::TArticlePtr art(*it, m_Gazetteer->ArticlePool());
            const TSurnameLemmaModel *model = art.As<TSurnameLemmaModel>();
            if (NULL == model)
                ythrow yexception() << "Failed while loading surname lemma model from protobuf.";

            p->AddFromProto<TSurnameLemmaModel>(*model);
        }
    } else {
        // TODO: print warning in case VERBOSE MODE is ON
    }

    return p;
}

yset<Wtroka> CDictsHolder::LoadRusExtension() const {
    yset<Wtroka> extList;

    const yset<ui32> *s = m_Gazetteer->ArticlePool().FindCustomArticleOffsetsByType(TExtensionList::descriptor());

    if (NULL != s) {
        yset<ui32>::const_iterator it = s->begin();

        for (; s->end() != it; ++it) {
            NGzt::TArticlePtr art(*it, m_Gazetteer->ArticlePool());
            const TExtensionList *list = art.As<TExtensionList>();
            if (NULL == list)
                ythrow yexception() << "Failed while loading extension list from protobuf.";

            for (size_t i = 0; i < list->itemSize(); ++i) {
                Wtroka wstr = UTF8ToWide(list->Getitem(i));
                extList.insert(wstr);
            }
        }
    } else {
        // TODO: print warning in case VERBOSE MODE is ON
    }

    return extList;
}

TMapAbbrev CDictsHolder::LoadRusAbbrev() const {
    TMapAbbrev abbrevDic;

    const yset<ui32> *s = m_Gazetteer->ArticlePool().FindCustomArticleOffsetsByType(TAbbreviation::descriptor());

    if (NULL != s) {
        yset<ui32>::const_iterator it = s->begin();

        for (; s->end() != it; ++it) {
            NGzt::TArticlePtr art(*it, m_Gazetteer->ArticlePool());
            const TAbbreviation *abbr = art.As<TAbbreviation>();
            if (NULL == abbr)
                ythrow yexception() << "Failed while loading TAbbreviation from protobuf.";

            Wtroka wstr = UTF8ToWide(abbr->Gettext());
            abbrevDic[wstr] = abbr->Gettype();
        }
    } else {
        // TODO: print warning in case VERBOSE MODE is ON
    }

    return abbrevDic;
}

void CDictsHolder::InitGazetteerTitleIndexImpl()
{
    // no locking, private
    if (GztArticleIndex.Get() == NULL) {
        THolder<NGzt::TArticlePool::TTitleMap> tmp(new NGzt::TArticlePool::TTitleMap);
        m_Gazetteer->ArticlePool().BuildTitleMap(*tmp);
        GztArticleIndex.Reset(tmp.Release());
    }
}

void CDictsHolder::InitGazetteerTitleIndex()
{
    if (GztArticleIndex.Get() == NULL) {
        static TAtomic lock;
        TGuard<TAtomic> guard(lock);
        InitGazetteerTitleIndexImpl();
    }
}

const NGzt::TArticlePool::TTitleMap* CDictsHolder::GetGztArticleIndex() const
{
    return GztArticleIndex.Get();
}

bool CDictsHolder::GetGztArticleByTitle(const Wtroka& title, TGztArticle& article) const
{
    ui32 offset;
    if (GztArticleIndex.Get() == NULL || !GztArticleIndex->Find(~title, +title, &offset))
        return false;
    article = TGztArticle(offset, m_Gazetteer->ArticlePool());
    return true;
}

bool CDictsHolder::GetGztArticleByTitlePrefix(const Wtroka& titlePrefix, TGztArticle& article) const
{
    NGzt::TArticlePool::TTitleMap::TConstIterator it = GztArticleIndex->UpperBound(titlePrefix);
    if (it == GztArticleIndex->End() || !it.GetKey().has_prefix(titlePrefix)) {
        return false;
    }
    ui32 offset;
    it.GetValue(offset);
    article = TGztArticle(offset, m_Gazetteer->ArticlePool());
    return true;
}

size_t CDictsHolder::GetLemmaCorpusFreq(Stroka Lemma) const
{
    Lemma.to_lower();
    ymap<Stroka, size_t>::const_iterator it = m_LemmaFreq.find(Lemma);
    if (it == m_LemmaFreq.end())
        return 1;
    else
        return it->second;
}

void CDictsHolder::InitLemmaFreq()
{
    // protected method - no mutex aquired

    m_LemmaFreq.clear();

    Stroka path = PathHelper::Join(DicFdoPath, "lemma_freqs.txt");

    char buffer[1000];
    FILE* fp = fopen(path.c_str(), "r");
    int lineno =0;
    while (fgets (buffer, 1000, fp)) {
        lineno++;
        char* lemma = strtok(buffer,"; \n\r\t");
        char* freq_s = strtok(NULL,"; \n\r\t");
        if (!lemma || !freq_s || !*lemma || !*freq_s || !isdigit((unsigned char)*freq_s)) {
            Cerr << Substitute("Cannot parse line $0 file $1", lineno, path) << Endl;
            fclose (fp);
            ythrow yexception();
        }
        Stroka sLemma = lemma;
        sLemma.to_upper();
        ymap<Stroka, size_t>::iterator it =  m_LemmaFreq.find(sLemma);
        if (it != m_LemmaFreq.end())
            it->second += atoi(freq_s);
        else
            m_LemmaFreq[sLemma] = atoi(freq_s);

    }
    fclose (fp);
}

//bool CDictsHolder::HasXmlLogFile() const
//{
//    return !GetXmlLogFile().empty();
//}
//
//Stroka CDictsHolder::GetXmlLogFile() const
//{
//    return m_ParserOptions.m_KWDumpOptions.m_strXmlFileName;
//}

bool CDictsHolder::HasArtPointer(const CHomonym& h, EDicType dicType, const yset<SArtPointer>& artPointers) const
{
    if (dicType == KW_DICT && h.HasGztArticle())
        return artPointers.find(SArtPointer(h.GetGztArticle().GetType())) != artPointers.end()
            || artPointers.find(SArtPointer(h.GetGztArticle().GetTitle())) != artPointers.end();

    else if (h.HasAuxArticle(dicType)) {
        const article_t* pArt = GetAuxArticle(h, dicType);
        TKeyWordType t = pArt->get_kw_type();
        if (t != NULL)
            return artPointers.find(SArtPointer(t)) != artPointers.end();
        else
            return artPointers.find(SArtPointer(pArt->get_title())) != artPointers.end();
    } else
        return false;
}

static inline bool SimpleMaskMatch(TWtringBuf text, TWtringBuf mask) {
    const wchar16 STAR = '*';

    while (!mask.empty() && mask[0] != STAR) {
        if (text.empty() || mask[0] != text[0])
            return false;
        mask.Skip(1);
        text.Skip(1);
    }

    while (!mask.empty() && mask.back() != STAR) {
        if (text.empty() || mask.back() != text.back())
            return false;
        mask.Chop(1);
        text.Chop(1);
    }

    if (mask.empty())
        return text.empty();    // no '*' in mask

    YASSERT(mask[0] == STAR && mask.back() == STAR);
    mask.Skip(1).Chop(1);
    while (!mask.empty()) {
        TWtringBuf prefix = mask.NextTok(STAR);
        const wchar16* p = TCharTraits<TChar>::Find(~text, +text, ~prefix, +prefix);
        if (p == NULL)
            return false;
        text.Skip(p - text.data() + prefix.size());
    }

    return true;
}

bool CDictsHolder::IsTitleMatch(const TWtringBuf& artTitle, const TWtringBuf& searchTitle, bool exactMatch) {
    if (exactMatch)
        return artTitle == searchTitle;

    if (SimpleMaskMatch(artTitle, searchTitle))
        return true;
    else if (NGzt::TArticlePool::IsArticleFolder(searchTitle, artTitle))
        return true;
    else
        return false;
}

bool CDictsHolder::IsArticleMatch(const TGztArticle& art, const SArtPointer& searchArt, bool exactMatch) {
    if (searchArt.HasKWType() && art.GetType() != searchArt.GetKWType())
        return false;

    if (searchArt.HasStrType() && !IsTitleMatch(art.GetTitle(), searchArt.GetStrType(), exactMatch))
        return false;

    // empty @searchArt will match to any @art
    return true;
}

bool CDictsHolder::IsArticleMatch(const article_t& art, const SArtPointer& searchArt, bool exactMatch) {
    if (searchArt.HasKWType() && art.get_kw_type() != searchArt.GetKWType())
        return false;

    if (searchArt.HasStrType() && !IsTitleMatch(art.get_title(), searchArt.GetStrType(), exactMatch))
        return false;

    // empty @searchArt will match to any @art
    return true;
}

bool CDictsHolder::MatchArticlePtr(const CHomonym& h, EDicType dic, const SArtPointer& art, bool exactMatch) const
{

    // gzt article
    TGztArticle tmp;
    if (GetMatchingArticle(h, art, tmp, exactMatch))
        return true;

    // aux-dic article
    if (h.HasAuxArticle(dic) && IsArticleMatch(*GetAuxArticle(h, dic), art, exactMatch))
        return true;

    return false;
}

bool CDictsHolder::GetMatchingArticle(const CHomonym& h, const SArtPointer& art, TGztArticle& result, bool exactMatch) const
{
    if (IsArticleMatch(h.GetGztArticle(), art, exactMatch)) {
        result = h.GetGztArticle();
        return true;
    }

    // check all extra gzt-articles too
    const CHomonym::TExtraGztArticles& extra = h.GetExtraGztArticles();
    for (CHomonym::TExtraGztArticles::const_iterator it = extra.begin(); it != extra.end(); ++it)
        if (IsArticleMatch(*it, art, exactMatch)) {
            result = *it;
            return true;
        }

    return false;
}

const article_t* CDictsHolder::GetAuxArticle(const Wtroka& strTitle, EDicType dicType) const
{
    return GetDict(dicType).get_article(strTitle);
}

int CDictsHolder::GetAuxArticleIndex(const Wtroka& strTitle, EDicType dicType) const
{
    return GetDict(dicType).get_article_index(strTitle);
}

const article_t* CDictsHolder::GetAuxArticle(const CHomonym& h, EDicType dicType) const
{
    //YASSERT(dicType != GZT_DICT);       // to retrieve gazetteer article use h.GetGztArticle()

    if (h.m_DicIndexes[dicType] == -1) {
        //так как по смыслу оба словаря описывают вершину клаузы, то каждый раз вызывать эту ф-цию 2 раза для
        //словаря предикатов и шаблонов геморойно, тогда сделаю так, чтобы по запросу на PredicDic, если нет
        //статьи из него то смотрелся TempleDic
        if (dicType == PREDIC_DICT)
            return GetAuxArticle(h, TEMPLATE_DICT);
        else
            return GetEmptyAuxArticle();
    }

    switch (dicType) {
    case CONJ_DICT:
    case PREP_DICT:
    case TEMPLATE_DICT:
    case KW_DICT:
    case PREDIC_DICT:
        return GetDict(dicType).GetArticle(h.m_DicIndexes[dicType]);
    default:
        return GetEmptyAuxArticle();
    }
}

Stroka CDictsHolder::GetStrKWType(const CHomonym& h, ECharset encoding) const
{
    TKeyWordType t;
    Stroka title;

    if (h.HasGztArticle()) {
        const TGztArticle& gzt_art = h.GetGztArticle();
        t = gzt_art.GetType();
        title = NStr::Encode(gzt_art.GetTitle(), encoding);
    } else if (h.HasAuxArticle(KW_DICT)) {
        const article_t* art = GetAuxArticle(h, KW_DICT);
        t = art->get_kw_type();
        title = NStr::Encode(art->get_title(), encoding);
    }

    Stroka res;
    if (t != NULL)
        res = Substitute("$0 [$1]", t->full_name(), title);
    else if (!title.empty())
        res = Substitute("[$0]", title);
    return res;
}

or_valencie_list_t CDictsHolder::GetSubjValVars(const CHomonym& h) const
{
    or_valencie_list_t or_valencie_list;
    if (!h.HasAuxArticle(PREDIC_DICT))
        return or_valencie_list;
    const article_t* piArt = GetAuxArticle(h, PREDIC_DICT);
    const valencie_list_t& piValencies = piArt->get_valencies();
    for (size_t i = 0; i < piValencies.size(); ++i) {
        const or_valencie_list_t& piValVars = piValencies[i];
        for (size_t j = 0; j < piValVars.size(); ++j) {
            const valencie_t& piVal = piValVars[j];
            ESynRelation synRel = piVal.m_syn_rel;
            if (synRel == Subj || synRel == SubjDat || synRel == SubjGen)
                return piValVars;
        }
    }
    return or_valencie_list;
}

TKeyWordType CDictsHolder::GetKWType(const CHomonym& h, EDicType dic) const
{
    if (dic == KW_DICT && h.HasGztArticle())
        return h.GetGztArticle().GetType();
    else if (h.HasAuxArticle(dic))
        return GetAuxArticle(h, dic)->get_kw_type();
    else
        return NULL;
}

Wtroka CDictsHolder::GetArticleTitle(const CHomonym& h, EDicType dic) const
{
    if (dic == KW_DICT && h.HasGztArticle())
        return h.GetGztArticle().GetTitle();
    else if (h.HasAuxArticle(dic))
        return GetAuxArticle(h, dic)->get_title();
    else
        return Wtroka();
}

bool CDictsHolder::HasArticle(const CHomonym& h, const SArtPointer& artP, EDicType dic /*= KWDict*/) const
{
    return artP.IsValid() && MatchArticlePtr(h, dic, artP);
}

bool CDictsHolder::IsSimConjWithCorr(const CHomonym& h) const
{
    return h.HasGrammem(gSimConj) &&
           GetAuxArticle(h, CONJ_DICT)->get_corrs_count() > 0;
}

Wtroka CDictsHolder::GetUnquotedDictionaryLemma(const CHomonym& h, EDicType dic) const
{
    if (dic == KW_DICT && h.HasGztArticle()) {
        const NGzt::TMessage* lemma_info = h.GetGztArticle().GetLemmaInfo();
        if (lemma_info != NULL)
            return h.GetGztArticle().GetLemmaText(*lemma_info);
    }
    const article_t* piArt = GetAuxArticle(h, dic);
    if (piArt != NULL)
        return piArt->get_lemma();
    else
        return Wtroka();
}

bool CDictsHolder::HasArticle(const CWord& w, yset<TKeyWordType>& kw_types) const
{
    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it)
        if (kw_types.find(GetKWType(*it, KW_DICT)) != kw_types.end())
            return true;
    return false;
}

bool CDictsHolder::HasArticle(const CWord& w, const SArtPointer& artP) const
{
    return HasArticle_i(w, artP) != -1;
}

int CDictsHolder::HasArticle_i(const CWord& w, const SArtPointer& artP) const
{
    for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it)
        if (HasArticle(*it, artP, KW_DICT))
            return it.GetID();
    return -1;

}

bool CDictsHolder::HasSimConjNotWithCommaObligatory(const CWord& w) const
{
    int iH = w.HasSimConjHomonym_i();
    if (iH == -1)
        return false;
    const CHomonym& hom = w.GetRusHomonym(iH);
    if (GetAuxArticle(hom, CONJ_DICT)->get_has_comma_before())
        return false;
    return  true;
}

TKeyWordType CDictsHolder::GetAuxKWType(const SDictIndex& di) const
{
    return GetAuxArticle(di)->get_kw_type();
}

Wtroka CDictsHolder::GetAuxArticleTitle(const SDictIndex& di) const
{
    return GetAuxArticle(di)->get_title();
}

const article_t* CDictsHolder::GetAuxArticle(const SDictIndex& DictIndex) const
{
    YASSERT(DictIndex.m_DicType < DICTYPE_COUNT &&
            DictIndex.m_iArt >= 0 &&
            (size_t)DictIndex.m_iArt < GetDict(DictIndex.m_DicType).size());

    YASSERT(m_Dicts[DictIndex.m_DicType][DictIndex.m_iArt] != NULL);
    return m_Dicts[DictIndex.m_DicType][DictIndex.m_iArt];
}

void CDictsHolder::InitAuxDic(const Stroka& strDicName, const Stroka &text, SDefaultFieldValues& DefaultFieldValues, EDicType dic,
                              bool bInitGrammemsTemplate /*= false*/)
{
    // protected method - no locking

    YASSERT(dic < DICTYPE_COUNT);
    if (m_Dicts[dic].size() > 0)
        return;

    Stroka path = PathHelper::Join(DicPath, strDicName);
    bool bRes = ReadDict(m_Dicts[dic], DefaultFieldValues, path, text);
    if (!bRes)
        ythrow yexception() << "Failed to create " << path;

    if (!bInitGrammemsTemplate)
        bRes = m_Dicts[dic].Init();
    else
        bRes = m_Dicts[dic].InitGrammemsTemplates();

    if (!bRes)
        ythrow yexception() << "Failed to initialize " << path;
}

void CDictsHolder::InitFragmDics()
{
    static TAtomic lock;
    TGuard<TAtomic> guard(lock);

    if (!m_bFramDictsLoaded) {

        SDefaultFieldValues default1;
        default1.m_ActantTypeVal = Fragment;
        InitAuxDic(g_strConjDic, GetAuxDicByName(g_strConjDic), default1, CONJ_DICT);

        SDefaultFieldValues default2;
        InitAuxDic(g_strPredicDic, GetAuxDicByName(g_strPredicDic), default2, PREDIC_DICT);

        SDefaultFieldValues default3;
        InitAuxDic(g_strPrepDic, GetAuxDicByName(g_strPrepDic), default3, PREP_DICT);

        SDefaultFieldValues default4;
        InitAuxDic(g_strTemplateDic, GetAuxDicByName(g_strTemplateDic), default4, TEMPLATE_DICT, true);

        m_bFramDictsLoaded = true;
    }
}

bool CDictsHolder::ReadDict(dictionary_t& dict, SDefaultFieldValues& DefaultFieldValues, const Stroka& path, const Stroka &text)
{
    //protected method - no locking!

    YASSERT(m_Gazetteer.Get() != NULL); // should not be called before gazetteer initialization

    articles_parser_t articles_parser(dict, DefaultFieldValues);
    articles_parser.put_dic_folder(DicFdoPath);
    return articles_parser.process(path, text) && !articles_parser.m_error;
}


template <typename T>
class TResourceLoaderBase {
public:
    typedef ymap<Stroka, T*> TIndex;

    TResourceLoaderBase(TIndex& index, const TRWMutex* indexLock)
        : Index(index)
        , Lock(indexLock)
    {
    }

    virtual ~TResourceLoaderBase() {
    }

    // NULL can be returned if the @path is not found, or cannot load resource for some reason
    const T* GetResourceOrRegister(const Stroka& path, const Stroka& name) {
        TStringBuf normName = NormalizeName(name);
        T* res = NULL;
        return FindRegistered(normName, res) ? res : Register(path, ::ToString(normName));
    }

private:
    static TStringBuf NormalizeName(const Stroka& path);

    bool FindRegistered(const TStringBuf& normName, T*& res) const {
        TReadGuard guard(Lock);
        return FindRegisteredNoLock(normName, res);
    }

    bool FindRegisteredNoLock(const TStringBuf& normPath, T*& res) const {
        typename TIndex::const_iterator it = Index.find(normPath);
        if (it != Index.end()) {
            res = it->second;
            return true;
        }
        else
            return false;
    }

    T* Register(const Stroka& path, const TStringBuf& normName) {
        TWriteGuard guard(Lock);
        T* res = NULL;
        // double check
        if (!FindRegisteredNoLock(normName, res)) {
            Stroka normNameStr(normName);
            res = RegisterNoLock(normNameStr, PathHelper::Join(path, normNameStr  + ".bin"));
        }
        return res;
    }

    T* RegisterNoLock(const Stroka& normName, const Stroka& filePath) {
        YASSERT(Index.find(normName) == Index.end());
        T* res = NULL;
        if (PathHelper::Exists(filePath))
            res = LoadFromFile(filePath).Release();
        Index[normName] = res;       // could be NULL
        return res;
    }

    virtual TAutoPtr<T> LoadFromFile(const Stroka& path) const = 0;


private:
    TIndex& Index;
    const TRWMutex* Lock;
};


template <typename T>
TStringBuf TResourceLoaderBase<T>::NormalizeName(const Stroka& path) {
    TStringBuf res = ::StripString(TStringBuf(path));
    while (PathHelper::IsDirSeparator(res[0]))
        res = res.Skip(1);

    // truncate an extension if any
    size_t ii = res.rfind('.');
    if (ii != Stroka::npos)
        res.Trunc(ii);

    return res;
}




class TWorkGrammarLoader: public TResourceLoaderBase<CWorkGrammar> {
    typedef TResourceLoaderBase<CWorkGrammar> TBase;
public:
    TWorkGrammarLoader(TBase::TIndex& index, const TRWMutex* indexLock)
        : TBase(index, indexLock)
    {
    }

private:
    virtual TAutoPtr<CWorkGrammar> LoadFromFile(const Stroka& path) const {
        TAutoPtr<CWorkGrammar> pGrammar(new CWorkGrammar);
        pGrammar->LoadFromFile(path, Default<TFactexTerminalDictionary>());
        return pGrammar;
    }
};


#ifndef TOMITA_EXTERNAL
class TTaggerLoader: public TResourceLoaderBase<TTagger> {
    typedef TResourceLoaderBase<TTagger> TBase;
public:
    TTaggerLoader(TBase::TIndex& index, const TRWMutex* indexLock, const TGztArticle& taggerArticle)
        : TBase(index, indexLock)
        , TaggerArticle(taggerArticle)
    {
    }

private:
    virtual TAutoPtr<TTagger> LoadFromFile(const Stroka& path) const {
        return new TTagger(TaggerArticle, path);
    }
private:
    TGztArticle TaggerArticle;
};
#endif


const CWorkGrammar* CDictsHolder::GetGrammarOrRegisterInt(const Stroka& path) const {
    TWorkGrammarLoader loader(RegisteredGrammars, &GrammarLock);
    return loader.GetResourceOrRegister(BinPath, path);
}

const CWorkGrammar& CDictsHolder::GetGrammarOrRegister(const Stroka& path) const {
    const CWorkGrammar* ret = GetGrammarOrRegisterInt(path);
    if (ret == NULL)
        ythrow yexception() << "Can't load grammar " << path << ".";
    return *ret;
}

#ifndef TOMITA_EXTERNAL
const TTagger& CDictsHolder::GetTaggerOrRegister(const Stroka& path, const TGztArticle& article) const {
    YASSERT(article.IsInstance<TTaggerArticle>());
    TTaggerLoader loader(RegisteredTaggers, &TaggerLock, article);
    const TTagger* ret = loader.GetResourceOrRegister(DicFdoPath, path);
    if (ret == NULL)
        ythrow yexception() << "Can't load tagger " << path << ".";
    return *ret;
}
#endif

bool CDictsHolder::InitSynAnGrammar(const Stroka& dicsPath)
{
    // protected, no locking
    YASSERT(m_Gazetteer.Get() != NULL);
    return m_SynAnGrammar.LoadFromFile(PathHelper::Join(dicsPath, g_FdoDicDir, "syn-parser.bin"));
}

const CIndexedDictionary& CDictsHolder::GetDict(EDicType DicType) const
{
    YASSERT(DicType < DICTYPE_COUNT);
    return m_Dicts[DicType];
}

const article_t* CDictsHolder::GetEmptyAuxArticle() const
{
    return m_Dicts[0].get_empty_article();
}

long CDictsHolder::FindTemplateArticle(const CHomonym& homonym) const
{
    //trying to find related article by POS-grammems
    CIndexedDictionary::TGrammemTemplates::const_iterator it =
        m_Dicts[TEMPLATE_DICT].m_GrammemsTemplates.find(homonym.GetMinimalPOS());

    if (it == m_Dicts[TEMPLATE_DICT].m_GrammemsTemplates.end())
        return -1;

    const yvector<SGrammemsTemplate>& templates = it->second;
    if (templates.size() == 0)
        return -1;

    for (size_t i = 0; i < templates.size(); ++i) {
        const SGrammemsTemplate& GrammemsTemplate = templates[i];

        if (GrammemsTemplate.m_Grammems.empty())
            return GrammemsTemplate.m_iArtID;

        for (TGrammarBunch::const_iterator it = GrammemsTemplate.m_Grammems.begin(); it != GrammemsTemplate.m_Grammems.end(); ++it)
            if (homonym.Grammems.HasAll(*it))
                return GrammemsTemplate.m_iArtID;
    }
    return -1;
}

const CFilterStore& CDictsHolder::GetFilter(TKeyWordType type) const
{
    Stroka strFileName;
    if (type != NULL)
        strFileName += type->name();
    strFileName += "_filter.bin";

    //если файла с фильтрами нет (что нормально), то будем возвращать просто пустую грамматику
    const CWorkGrammar* gram = GetGrammarOrRegisterInt(strFileName);
    return gram == NULL ? EmptyGrammar.m_FilterStore : gram->m_FilterStore;
}

void CDictsHolder::InitAuxKwDict()
{
    // no locking - this method is protected

    Stroka path = PathHelper::Join(BinPath, "", g_strKWDicBin);

    if (!PathHelper::Exists(path))
        ythrow yexception() << "Can't find aux_dic: " << path << ".";
    if (!m_Dicts[KW_DICT].LoadFromFile(path))
        ythrow yexception() << "Failed to read aux_dic";
}

void CDictsHolder::InitGazetteer()
{
    // no locking - this method is protected

    Stroka gztfile = PathHelper::Join(BinPath, PathCompatibilityMode ? g_strArticlesGztBin : RootFile + ".bin");

    if (!PathHelper::Exists(gztfile))
        ythrow yexception() << "Cannot find gazetteer: \"" << gztfile << "\".";
    m_Gazetteer.Reset(new TGazetteer(TBlob::FromFile(gztfile)));
    GetMutableKWTypePool().RegisterPool(m_Gazetteer->ProtoPool());
    m_BuiltinKWTypes.Reset(new TBuiltinKWTypes(m_Gazetteer->ProtoPool()));
}

void CDictsHolder::InitFactTypes() {
    // no locking - this method is protected
    m_FactTypes.Reset(new TFactTypeHolder);

    // combine fact-types from aux-dic and gazetteer
    m_FactTypes->Add(m_Dicts[KW_DICT].GetFactTypes());
    m_FactTypes->AddFromGazetteer(m_Gazetteer->ProtoPool());
}


