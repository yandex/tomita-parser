#include <util/string/cast.h>

#include "factsxmlwriter.h"
#include "textprocessor.h"

Wtroka CFactsWriterBase::GetWords(const CTextWS& wordsPair) const
{
    Wtroka res = wordsPair.GetLemma();
    SubstGlobal(res, ASCIIToWide("\""), ASCIIToWide(" "));
    TMorph::ToUpper(res);

    TWtringBuf tmp = StripString(TWtringBuf(res));
    while (tmp.size() > 0 && tmp[0] == '\'')
        tmp.Skip(1);
    while (tmp.size() > 0 && tmp.back() == '\'')
        tmp.Chop(1);
    return ToWtring(StripString(tmp));
}

bool CFactsWriterBase::CheckFieldValue(const CWordsPair* wp, const fact_field_descr_t& fieldDescr, const Stroka& strFactName) const
{
    if (wp != NULL)
        return true;
    else if (!fieldDescr.m_bNecessary)
        return false;
    else
        ythrow yexception() << "Can't insert NULL value of the filed \"" << fieldDescr.m_strFieldName
                            << "\" of the fact " << strFactName;
}


static void InsertGeoArticle(const Wtroka& strDescr, const Wtroka& pArtTitle, TKeyWordType kwType, TKeyWordType kwMainType,
                                      bool bAnyGeoType, yset<Wtroka>& arts)
{
    const TBuiltinKWTypes& builtins = GlobalDictsHolder->BuiltinKWTypes();
    if (builtins.IsGeo(kwType)) {
        if (bAnyGeoType) {
            if (kwType != builtins.GeoVill)
                arts.insert(pArtTitle);
        } else {
            if (strDescr.empty()) {
                if (kwType == kwMainType || (builtins.IsGeoCity(kwType) && builtins.IsGeoCity(kwMainType)))
                    arts.insert(pArtTitle);
            } else if (strDescr.empty()) {
                if (kwType == kwMainType || (builtins.IsGeoVill(kwType) && builtins.IsGeoVill(kwMainType)))
                    arts.insert(pArtTitle);
            } else if ((kwType == kwMainType) || (builtins.IsGeoCity(kwType) && builtins.IsGeoCity(kwMainType)))
                arts.insert(pArtTitle);
        }
    }
}


static void GetGeoArticlesFromWord(const CWord& w, Wtroka strDescr, bool bAnyGeoType, TKeyWordType kwMainType, yset<Wtroka>& arts)
{
    CWord::SHomIt it = w.IterHomonyms();
    for (; it.Ok(); ++it) {
        const Wtroka& title = GlobalDictsHolder->GetArticleTitle(*it, KW_DICT);
        TKeyWordType kwType = GlobalDictsHolder->GetKWType(*it, KW_DICT);
        if (kwType == NULL && title.empty())
            continue;

        InsertGeoArticle(strDescr, title, kwType, kwMainType, bAnyGeoType, arts);

        CHomonym::TKWType2Articles::const_iterator it_aux_art = it->GetKWType2Articles().begin();
        for (; it_aux_art != it->GetKWType2Articles().end(); ++it_aux_art)
            for (size_t i = 0; i < it_aux_art->second.size(); ++i) {
                const article_t* pArt = GlobalDictsHolder->GetAuxArticle(it_aux_art->second[i]);
                if (pArt != NULL)
                    InsertGeoArticle(strDescr, pArt->get_title(), pArt->get_kw_type(), kwMainType, bAnyGeoType, arts);
            }

        CHomonym::TExtraGztArticles::const_iterator it_gzt_art = it->GetExtraGztArticles().begin();
        for (; it_gzt_art != it->GetExtraGztArticles().end(); ++it_gzt_art) {
            InsertGeoArticle(strDescr, it_gzt_art->GetTitle(), it_gzt_art->GetType(), kwMainType, bAnyGeoType, arts);
        }
    }
}

bool CFactsWriterBase::GetGeoArts(const CTextProcessor& pText, const CTextWS* pFieldWS,
                                  const CSentenceRusProcessor* pSent, const Stroka& strFactName,
                                  Wtroka& sRes, const SFactAddress& factAddress) const
{
    sRes.clear();

    if (strFactName != "ComplexGeo")
        return false;

    const CFactFields& fact = pText.GetFact(factAddress);

    const CTextWS* pDesctField = fact.GetTextValue(CFactFields::SettlementDescr);
    Wtroka strDescr;
    if (pDesctField != NULL)
        strDescr = pDesctField->GetLemma();

    const CWordVector& rRusWords = pSent->GetRusWords();
    const SWordHomonymNum& rHomNum = pFieldWS->GetMainWord();
    const CWord& rMainWrd = rRusWords.GetWord(rHomNum);
    const CHomonym& mainHomonym = rRusWords[rHomNum];

    TKeyWordType kwMainType = GlobalDictsHolder->GetKWType(mainHomonym, KW_DICT);
    const CWord& wSource = rRusWords.GetWord(factAddress);
    //если весь факт состоит только из одного гео-объекта без всяких дескрипторов
    bool bAnyGeoType = rMainWrd.GetSourcePair() == wSource.GetSourcePair();
    yset<Wtroka> arts;

    GetGeoArticlesFromWord(rMainWrd, strDescr, bAnyGeoType, kwMainType, arts);
    if((arts.size() == 0) && !(rMainWrd.GetSourcePair() == *pFieldWS) )
    {
        //грязный хак для областей, так как синтаксически главным является слово область,
        //а статья приписана только прилагательному.
        int iFirstWord = pFieldWS->FirstWord();
        kwMainType = GlobalDictsHolder->BuiltinKWTypes().GeoAdm;
        GetGeoArticlesFromWord(rRusWords.GetOriginalWord(iFirstWord), strDescr, false, kwMainType, arts);
    }

    for (yset<Wtroka>::iterator it_arts = arts.begin(); it_arts != arts.end(); ++it_arts) {
        if (!sRes.empty())
            sRes += ' ';
        sRes += *it_arts;
    }

    return !sRes.empty();
}

CFactsXMLWriter::CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, Stroka path, ECharset encoding)
    : CSimpleXMLWriter(path, "fdo_objects", encoding)
    , CFactsWriterBase(parserOutputOptions)
    , m_pErrorStream(&Cerr)
{
}

CFactsXMLWriter::CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, TOutputStream* outStream, ECharset encoding)
    : CSimpleXMLWriter(outStream, "fdo_objects", encoding, Stroka("<fdo_objects>"), false)
    , CFactsWriterBase(parserOutputOptions)
    , m_pErrorStream(&Cerr)
{
}

void CFactsXMLWriter::PutWriteLeads(bool b) {
    IsWriteLeads = b;
}

void CFactsXMLWriter::PutWriteSimilarFacts(bool b) {
    IsWriteSimilarFacts = b;
}

CFactsXMLWriter::CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, Stroka path, ECharset encoding, bool bReload, bool bAppend /*= false*/)
    : CSimpleXMLWriter(path, "fdo_objects", encoding,
                       bReload ? "<fdo_objects reload=\"\">" : "<fdo_objects>",
                       bAppend)
    , CFactsWriterBase(parserOutputOptions)
    , m_pErrorStream(&Cerr)
{
}

CFactsXMLWriter::CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, TOutputStream* outStream, ECharset encoding, bool bReload, bool bAppend /*= false*/)
    : CSimpleXMLWriter(outStream, "fdo_objects", encoding,
                       bReload ? "<fdo_objects reload=\"\">" : "<fdo_objects>",
                       bAppend)
    , CFactsWriterBase(parserOutputOptions)
    , m_pErrorStream(&Cerr)
{
}


TXmlNodePtr CFactsXMLWriter::WriteFoundLinks(const CTextProcessor& pText, const SKWDumpOptions& task)
{
    const yset<SArtPointer>& mwToFind = task.MultiWordsToFind;
    yvector<SMWAddressWithLink> mwFounds;
    pText.GetFoundMWInLinks(mwFounds, mwToFind);

    TXmlNodePtr piLinks("links");
    for (size_t j = 0; j < mwFounds.size(); j++) {
        SMWAddressWithLink& mw = mwFounds[j];
        const CSentenceRusProcessor& pSent = *(pText.GetSentenceProcessor(mw.m_iSentNum));
        const CHomonym& h = pSent.GetRusWords()[mw];
        piLinks.AddNewChild("link").AddAttr("art", GlobalDictsHolder->GetArticleTitle(h, KW_DICT));
    }
    return piLinks;
}

void CFactsXMLWriter::AddFacts(const CTextProcessor& pText, const Stroka& url, const Stroka& siteUrl,
                               const Stroka& strArtInLink, const SKWDumpOptions* pTask)
{
    AddFactsToStream(*m_ActiveOutput, pText, url, siteUrl, strArtInLink, pTask);
}

void CFactsXMLWriter::AddFactsToStream(TOutputStream& out_stream, const CTextProcessor& pText, const Stroka& url,
                                       const Stroka& siteUrl, const Stroka& strArtInLink,
                                       const SKWDumpOptions* pTask)
{
    TGuard<TMutex> singleLock(m_SaveFactsCS);

    const CFactsCollection& factsCollection = pText.GetFactsCollection();
    if (factsCollection.GetFactsCount() == 0)
        return;

    TXmlNodePtr document("document");
    document.AddAttr("url", url);
    if (!siteUrl.empty())
        document.AddAttr("host", siteUrl);
    if (pTask)
        document.AddAttr("UrlType", strArtInLink);
    document.AddAttr("di", pText.GetAttributes().m_iDocID);
    document.AddAttr("bi", pText.GetAttributes().m_iSourceID);

    if (pTask)
        document.AddChild(WriteFoundLinks(pText, *pTask));

    Stroka strDate;

    if (!((pText.GetDateFromDateField().GetYear() == 1970) &&
        (pText.GetDateFromDateField().GetMonth() == 1) &&
        (pText.GetDateFromDateField().GetDay() == 1)))
        strDate = pText.GetDateFromDateField().Format("%Y-%m-%d");
    document.AddAttr("date", strDate);

    CLeadGenerator leadGenerator(pText, pText.GetAttributes().m_iDocID, url, Encoding);

    FactIdMap.clear();      // facts ids are not shared between distinct documents

    document.AddChild(WriteFacts(pText, leadGenerator));

    if (IsWriteSimilarFacts)
        document.AddChild(WriteEqualFacts(pText));

    if (IsWriteLeads)
        document.AddChild(WriteLeads(pText, leadGenerator));

    GetXMLRoot().AddChild(document.Release());
    SaveSubTreeToStream(out_stream);
}

void CFactsXMLWriter::PrintError(const Stroka& str)
{
    (*m_pErrorStream) << str << Endl;
}

void CFactsXMLWriter::SetErrorStream(TOutputStream* errorStream)
{
    m_pErrorStream = errorStream;
}

TXmlNodePtr CFactsXMLWriter::WriteLeads(const CTextProcessor& pText, CLeadGenerator& leadGenerator)
{
    TXmlNodePtr piLeads("Leads");
    for (size_t i = 0; i < pText.GetSentenceCount(); i++) {
        const yvector<CLeadGenerator::SSentenceLead>& leads = leadGenerator.GetLeadsBySent(i);
        const CWordVector& rRusWords = pText.GetSentenceProcessor(i)->GetRusWords();
        const CWord& w1 = rRusWords.GetAnyWord(0);
        const CWord& w2 = rRusWords.GetAnyWord(rRusWords.OriginalWordsCount()-1);
        for (size_t j = 0; j < leads.size(); j++) {
            piLeads.AddNewChild("Lead")
                .AddAttr("text", leads[j].m_strLead)
                .AddAttr("id", leads[j].m_iLeadID)
                .AddAttr("pos", w1.m_pos)
                .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
        }
    }
    return piLeads;
}

TXmlNodePtr CFactsXMLWriter::WriteEqualFacts(const CTextProcessor& pText)
{
    TXmlNodePtr piEqualFacts("EqualFacts");
    const CFactsCollection& facts = pText.GetFactsCollection();

    CFactsCollection::SetOfEqualFactsConstIt equalFactsIt = facts.GetEqualFactsItBegin();
    for (; equalFactsIt != facts.GetEqualFactsItEnd(); ++equalFactsIt) {
        const SEqualFacts& equalFacts = *equalFactsIt;
        SEqualFacts::SPairsIterator equalPairIt = equalFacts.GetPairsIterator(pText);
        ResetSimilarFacts();    // limit number of output pairs for each cluster
        for (; !equalPairIt.IsEnd(); ++equalPairIt) {
            EFactEntryEqualityType t;
            SFactAddress fact_addr1, fact_addr2;
            equalPairIt.GetFactsPair(fact_addr1, fact_addr2, t);
            WriteEqualFactsPair(piEqualFacts, pText, fact_addr1, fact_addr2, t,
                                equalFacts.m_strFieldName, equalFacts.m_strFieldName);
        }
    }

    ResetSimilarFacts();
    CFactsCollection::SetOfEqualFactPairsConstIt pairIt = facts.GetEqualFactPairsItBegin();
    for (; pairIt != facts.GetEqualFactPairsItEnd(); ++pairIt) {
        const SFactAddressAndFieldName& fact1 = pairIt->GetFact1();
        const SFactAddressAndFieldName& fact2 = pairIt->GetFact2();
        WriteEqualFactsPair(piEqualFacts, pText, fact1.m_FactAddr, fact2.m_FactAddr,
                            pairIt->m_EntryType, fact1.m_strFieldName, fact2.m_strFieldName);
    }
    return piEqualFacts;
}

TXmlNodePtr CFactsXMLWriter::WriteFacts(const CTextProcessor& pText, CLeadGenerator& leadGenerator)
{
    const CFactsCollection& factsCollection = pText.GetFactsCollection();
    CFactsCollection::SetOfFactsConstIt factsIt = factsCollection.GetFactsItBegin();
    int iFactID = 0;
    TXmlNodePtr piFacts("facts");
    for (; factsIt != factsCollection.GetFactsItEnd(); factsIt++) {
        const SFactAddress& factAddress = *factsIt;

        //может вернуть NULL, ( если это был, например, временный факт, или год у даты не влезает в sql)
        TXmlNodePtr piFact = WriteFact(factAddress, iFactID, pText, leadGenerator);
        if (piFact.Get() != NULL) {
            FactIdMap[factAddress] = iFactID++;
            piFacts.AddChild(piFact);
        }
    }

    return piFacts;
}

bool GetFirstLastWord(int& iFirstWord, int& iLastWord, const CWordSequence* ws)
{
       iFirstWord = -1;
       iLastWord = -1;
    if (!ws)
        return false;
    if (ws->IsArtificialPair())
        return false;
    if (!ws->IsValid())
        return false;

    iFirstWord = ws->FirstWord();
    iLastWord = ws->LastWord();
    return (iFirstWord  != -1) && (iLastWord != -1);
}


void CFactsXMLWriter::AddFioAttributes(const CTextProcessor& pText, const CFioWS* pFioWS, TXmlNodePtrBase piFact,
                                             const fact_field_descr_t& fieldDescr, const Stroka& strFactName, const SFactAddress& factAddress)
{
    if (!CheckFieldValue(pFioWS, fieldDescr, strFactName))
        return;

    Stroka strFioPrefix = fieldDescr.m_strFieldName;    //префикс, который добавляется перед именем тега, чтобы получались не просто
                                                        //Surname или FirstName, а еще и имя поля учитывалось
    strFioPrefix += "_";

    Wtroka s = pFioWS->m_Fio.m_strSurname;
    TMorph::ToUpper(s);
    TXmlNodePtrBase piSurname = piFact.AddNewChild(strFioPrefix + "Surname").AddAttr(g_strValAttr, s);

    int iFirstWord  = -1, iLastWord = -1;
    if (GetFirstLastWord(iFirstWord, iLastWord, pFioWS)) {
           const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
        const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
        const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
        piSurname.AddAttr("pos", w1.m_pos)
               .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
    }


    if (!pFioWS->m_Fio.m_strName.empty()) {
        s = pFioWS->m_Fio.m_strName;
        TMorph::ToUpper(s);
        piFact.AddNewChild(strFioPrefix + "FirstName").AddAttr(g_strValAttr, s);
    }

    if (!pFioWS->m_Fio.m_strPatronomyc.empty()) {
        s = pFioWS->m_Fio.m_strPatronomyc;
        TMorph::ToUpper(s);
        piFact.AddNewChild(strFioPrefix + "Patronymic").AddAttr(g_strValAttr, s);
    }

    if (pFioWS->m_Fio.m_bFoundSurname)
        piFact.AddNewChild(strFioPrefix + "SurnameIsDictionary").AddAttr(g_strValAttr, "1");

    if (pFioWS->m_Fio.m_bSurnameLemma)
        piFact.AddNewChild(strFioPrefix + "SurnameIsLemma").AddAttr(g_strValAttr, "1");
}


void CFactsXMLWriter::AddTextAttribute(const CTextProcessor& pText, const CTextWS* pFieldWS, const Wtroka& strVal,
                                       TXmlNodePtrBase piFact, const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr,
                                       const Stroka& strFactName, const SFactAddress& factAddress)
{
    if (CheckFieldValue(pFieldWS, fieldDescr, strFactName)) {
        TXmlNodePtrBase piChild = piFact.AddNewChild(fieldDescr.m_strFieldName);
        piChild.AddAttr(g_strValAttr, strVal);
        if (fieldDescr.m_bSaveTextInfo) //"info", написанное в '[' ']', в fact_types.cxx
            piChild.AddAttr(g_strInfoAttr, pFieldWS->GetMainWordsDump());
        yset<Stroka>::const_iterator it = fieldDescr.m_options.begin();
        for (; it != fieldDescr.m_options.end(); ++it)
            piChild.AddAttr(it->c_str(), "");
        Wtroka sGeoArts;
        if (GetGeoArts(pText, pFieldWS, pSent, strFactName, sGeoArts, factAddress))
            piChild.AddAttr(g_strGeoArtAttr, sGeoArts);

        int iFirstWord  = -1, iLastWord = -1;
        if (GetFirstLastWord(iFirstWord, iLastWord, pFieldWS)) {
               const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
            const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
            const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
            piChild.AddAttr("pos", w1.m_pos)
                   .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
        }

    }
}

void CFactsXMLWriter::AddBoolAttribute(bool val, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr)
{
    piFact.AddNewChild(fieldDescr.m_strFieldName).AddAttr(g_strValAttr, val ? "1" : "0");
}

void CFactsXMLWriter::AddDateAttribute(const CTextProcessor& pText, const CDateWS* pDateWS, TXmlNodePtrBase piFact,
                                       const fact_field_descr_t& fieldDescr, const SFactAddress& factAddress)
{
    Stroka strDateBegin;
    Stroka strDateEnd;
    if (pDateWS->m_SpanInterps.size() == 1)
        strDateBegin = strDateEnd = pDateWS->m_SpanInterps.begin()->GetSQLFormat();
    else if (pDateWS->m_SpanInterps.size() > 1) {
        strDateBegin = pDateWS->m_SpanInterps.begin()->GetSQLFormat();
        strDateEnd = pDateWS->m_SpanInterps.rbegin()->GetSQLFormat();
    }

    if (!strDateBegin.empty() && !strDateEnd.empty()) {
        //так как дата у нас может быть неточной ("октябрь 2000г."), то
        //представляется она всегда в виде интервала,
        //для этого прибавляем к имени поля из fact_type.cxx "Begin" и "End"
        Stroka strDateBeginNodeName = fieldDescr.m_strFieldName + "Begin";
        Stroka strDateEndNodeName = fieldDescr.m_strFieldName + "End";

        TXmlNodePtrBase piDateBegin = piFact.AddNewChild(strDateBeginNodeName).AddAttr(g_strValAttr, strDateBegin);
        TXmlNodePtrBase piDateEnd = piFact.AddNewChild(strDateEndNodeName).AddAttr(g_strValAttr, strDateEnd);

        int iFirstWord  = -1, iLastWord = -1;
        if (GetFirstLastWord(iFirstWord, iLastWord, pDateWS)) {
               const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
            const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
            const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
            piDateBegin.AddAttr("pos", w1.m_pos)
                   .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
            piDateEnd.AddAttr("pos", w1.m_pos)
                   .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
        }

    }
}

TXmlNodePtr CFactsXMLWriter::WriteFact(const SFactAddress& factAddress, int iFactId, const CTextProcessor& pText, CLeadGenerator& leadGenerator)
{
    const CFactFields& fact  = pText.GetFact(factAddress);
    const CDictsHolder* pDictsHolder = GlobalDictsHolder;
    const fact_type_t* pFactType = &pDictsHolder->RequireFactType(fact.GetFactName());

    int iFirstWord = -1;
    int iLastWord = -1;

    if (pFactType->m_bTemporary)
        return TXmlNodePtr();

    if (!ParserOutputOptions.HasFactToShow(fact.GetFactName()))
        return TXmlNodePtr();

    const CSentenceRusProcessor* pSent = pText.GetSentenceProcessor(factAddress.m_iSentNum);
    TXmlNodePtr piFact(pFactType->m_strFactTypeName);
    for (size_t i= 0; i < pFactType->m_Fields.size(); i++) {
        const fact_field_descr_t& fieldDescr = pFactType->m_Fields[i];
        switch (fieldDescr.m_Field_type) {
            case FioField:
                            {
                                const CFioWS* fioWS = fact.GetFioValue(fieldDescr.m_strFieldName);
                                if (fioWS != NULL)
                                    AddFioAttributes(pText, fioWS , piFact, fieldDescr, pFactType->m_strFactTypeName, factAddress);
                                UpdateFirstLastWord(iFirstWord, iLastWord, fioWS);
                                break;
                            }
            case TextField:
                            {
                                const CTextWS* textWS = fact.GetTextValue(fieldDescr.m_strFieldName);
                                UpdateFirstLastWord(iFirstWord, iLastWord, textWS);
                                Wtroka strVal;
                                if (textWS != NULL) {
                                    strVal = GetWords(*textWS);
                                    //если это обязательное поле со значением empty - не печатаем этот факт
                                    if (fieldDescr.m_bNecessary && NStr::EqualCiRus(strVal, "empty")
                                        && !ParserOutputOptions.ShowFactsWithEmptyField())
                                        return TXmlNodePtr();
                                }
                                AddTextAttribute(pText, textWS, strVal, piFact, pSent, fieldDescr,
                                                 pFactType->m_strFactTypeName, factAddress);
                                break;
                            }
            case DateField:
                            {
                                const CDateWS* dateWS = fact.GetDateValue(fieldDescr.m_strFieldName);
                                UpdateFirstLastWord(iFirstWord, iLastWord, dateWS);
                                if (dateWS != NULL) {
                                    //проверим, что дата попадает в интервал, который влезает в интервал для datetime у sql-я

                                    // 2011-06-24, mowgli: do not check SQL-compatibility and print out all dates, see jira://FACTEX-90

                                    //if (dateWS->HasStorebaleYear())
                                        AddDateAttribute(pText, dateWS, piFact, fieldDescr, factAddress);
                                    //else if (fieldDescr.m_bNecessary)
                                    //        return NULL;
                                }
                                break;
                            }
            case BoolField:
                AddBoolAttribute(fact.GetBoolValue(fieldDescr.m_strFieldName), piFact, fieldDescr);
                break;

            default:
                break;
        }
    }

    piFact.AddAttr("FactID", iFactId);

    if (!pFactType->m_bNoLead) {
        //получаем id лида и строчку с аттрибутами, кторые надо подсвечивать именно для этого факта
        yvector<CLeadGenerator::CAttrWP> attrs;
        int iLeadID = leadGenerator.AddFact(factAddress, attrs);
        piFact.AddAttr("LeadID", iLeadID)
              .AddAttr("FieldsInfo", CLeadGenerator::JoinAttrs(attrs));
    }

    if (pText.GetAttributes().m_TitleSents.first <= factAddress.m_iSentNum &&
        pText.GetAttributes().m_TitleSents.second >= factAddress.m_iSentNum)
        piFact.AddAttr("tl", factAddress.m_iSentNum);

    const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
    const CWordsPair& rWP = rRusWords.GetWord((SWordHomonymNum)(factAddress)).GetSourcePair();

    if ((iFirstWord != -1) && (iLastWord != -1)) {
        const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
        const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
        piFact.AddAttr("pos", w1.m_pos)
              .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
    }

    piFact.AddAttr("sn", factAddress.m_iSentNum)
          .AddAttr("fw", rWP.FirstWord())
          .AddAttr("lw", rWP.LastWord());

    return piFact;
}

void CFactsXMLWriter::WriteEqualFactsPair(TXmlNodePtrBase& node, const CTextProcessor& text,
                                          const SFactAddress& factAddr1, const SFactAddress& factAddr2,
                                          EFactEntryEqualityType equalityType, Stroka fieldName1, Stroka fieldName2) {

    const CFactFields& fact1  = text.GetFact(factAddr1);
    const CFactFields& fact2  = text.GetFact(factAddr2);

    if (ParserOutputOptions.HasToPrintFactEquality(fact1.GetFactName()) &&
        ParserOutputOptions.HasToPrintFactEquality(fact2.GetFactName())) {

        int id1 = 0, id2 = 0;
        // это вполне штатная ситуация, когда временный факт, типа PostCompany
        // не записывается в выходной XML, а, следовательно, и в FactIdMap
        if (FindFactId(factAddr1, id1) && FindFactId(factAddr2, id2))
            if (CheckSimilarFactsLimit())     // "топор" по количеству пар
                node.AddChild(WriteEqualFactsPair(id1, id2, equalityType, fieldName1, fieldName2).Release());
    }
}

TXmlNodePtr CFactsXMLWriter::WriteEqualFactsPair(int iFact1, int iFact2,
                                                 EFactEntryEqualityType FactEntryEqualityType,
                                                 Stroka strFieldName1, Stroka strFieldName2)
{
        TXmlNodePtr ret("SimilarFacts");
        ret.AddAttr("FactID1", iFact1)
            .AddAttr("FactID2", iFact2)
            .AddAttr("SimilarAttributeSetName1", strFieldName1)
            .AddAttr("SimilarAttributeSetName2", strFieldName2)
            .AddAttr("SimilarityTypeName", FactEntryEqualityTypeStr[FactEntryEqualityType]);
        return ret;
}

