#include <util/string/cast.h>

#include "factsrdfwriter.h"
#include "textprocessor.h"

#define RDF_NS "xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\""

// Collect-mode constructor
CFactsRDFWriter::CFactsRDFWriter(const CParserOptions::COutputOptions& parserOutputOptions, Stroka path, ECharset encoding)
    : CSimpleXMLWriter(path, "rdf:RDF", encoding, "<rdf:RDF>", false)
    , CFactsWriterBase(parserOutputOptions)
    , m_pErrorStream(&Cerr) {

}

// Stream-mode constructor
//CFactsRDFWriter::CFactsRDFWriter(const CParserOptions::COutputOptions& parserOutputOptions, TOutputStream* outStream, ECharset encoding) {
//
//}

void CFactsRDFWriter::AddFacts(const CTextProcessor& pText, const Stroka& url, const Stroka& siteUrl, const Stroka& strArtInLink, const SKWDumpOptions* pTask) {
    if (pText.GetFactsCollection().GetFactsCount() == 0)
        return;

    AddFactsToStream(*m_ActiveOutput, pText, url, siteUrl, strArtInLink, pTask);
}

void CFactsRDFWriter::AddFactsToStream(TOutputStream& out_stream, const CTextProcessor& pText, const Stroka& url, const Stroka& siteUrl, const Stroka& strArtInLink, const SKWDumpOptions* pTask) {
    TGuard<TMutex> singleLock(m_SaveFactsCS);

    const CFactsCollection& factsCollection = pText.GetFactsCollection();
    if (factsCollection.GetFactsCount() == 0)
        return;

    TXmlNodePtr annotationRoot("Aux:TextAnnotations");

    TXmlNodePtr docText("Aux:document_text");
    docText.AddAttr("xml:lang", IsoNameByLanguage(pText.GetLang()));
    docText.AddNewTextNodeChild(pText.GetRawText());
    annotationRoot.AddChild(docText.Release());

    FactIdMap.clear(); // facts ids are not shared between distinct documents
    int iFactID = 0;

    for (CFactsCollection::SetOfFactsConstIt factsIt = factsCollection.GetFactsItBegin(); factsIt != factsCollection.GetFactsItEnd(); factsIt++) {
        const SFactAddress& factAddress = *factsIt;

        int iStart = -1, iEnd = -1;
        //может вернуть NULL, если это был, например, временный факт, или год у даты не влезает в sql
        TXmlNodePtr piFact = WriteFact(factAddress, iFactID, pText, iStart, iEnd/*, leadGenerator*/);
        if (piFact.Get() != NULL) {
            FactIdMap[factAddress] = iFactID++;
            GetXMLRoot().AddChild(piFact.Release());

            // Instance annotation
            TXmlNodePtr ann("Aux:instance_annotation");
            TXmlNodePtr annInner("Aux:InstanceAnnotation");
            TXmlNodePtr annStart("Aux:annotation_start");
            TXmlNodePtr annEnd("Aux:annotation_end");
            TXmlNodePtr annRef("Aux:instance");

            annStart.AddAttr("rdf:datatype", "http://www.w3.org/2001/XMLSchema#int");
            char buffer[256];
            size_t len = ::ToString(iStart, buffer, 255);
            buffer[len] = 0;
            annStart.AddNewTextNodeChild(CharToWide(buffer));
            annEnd.AddAttr("rdf:datatype", "http://www.w3.org/2001/XMLSchema#int");
            len = ::ToString(iEnd, buffer, 255);
            buffer[len] = 0;
            annEnd.AddNewTextNodeChild(CharToWide(buffer));

            annRef.AddAttr("rdf:nodeID", "bnode-...");

            annInner.AddChild(annStart.Release());
            annInner.AddChild(annEnd.Release());
            annInner.AddChild(annRef.Release());

            ann.AddChild(annInner.Release());
            annotationRoot.AddChild(ann.Release());
        }
    }

//    TXmlNodePtr document("document");
//    document.AddAttr("url", url);
//    document.AddAttr("di", pText.GetAttributes().m_iDocID);
//    document.AddAttr("bi", pText.GetAttributes().m_iSourceID);

//    Stroka strDate;
//    if (!((pText.GetDateFromDateField().GetYear() == 1970) &&
//        (pText.GetDateFromDateField().GetMonth() == 1) &&
//        (pText.GetDateFromDateField().GetDay() == 1)))
//        strDate = pText.GetDateFromDateField().Format("%Y-%m-%d");
//    document.AddAttr("date", strDate);

    //CLeadGenerator leadGenerator(pText, pText.GetAttributes().m_iDocID, url, Encoding);

    //document.AddChild(WriteFacts(pText, leadGenerator));

//    if (IsWriteSimilarFacts)
//        document.AddChild(WriteEqualFacts(pText));

//    if (IsWriteLeads)
//        document.AddChild(WriteLeads(pText, leadGenerator));

    GetXMLRoot().AddChild(annotationRoot.Release());
    SaveSubTreeToStream(out_stream);
}

TXmlNodePtr CFactsRDFWriter::WriteFact(const SFactAddress& factAddress, int iFactId, const CTextProcessor& pText, int& iStart, int& iEnd/*, CLeadGenerator& leadGenerator*/) {
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
//                                const CFioWS* fioWS = fact.GetFioValue(fieldDescr.m_strFieldName);
//                                if (fioWS != NULL)
//                                    AddFioAttributes(pText, fioWS , piFact, fieldDescr, pFactType->m_strFactTypeName, factAddress);
//                                UpdateFirstLastWord(iFirstWord, iLastWord, fioWS);
//                                break;
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
//                                const CDateWS* dateWS = fact.GetDateValue(fieldDescr.m_strFieldName);
//                                UpdateFirstLastWord(iFirstWord, iLastWord, dateWS);
//                                if (dateWS != NULL)
//                                    AddDateAttribute(pText, dateWS, piFact, fieldDescr, factAddress);
//                                break;
                            }
            case BoolField:
                AddBoolAttribute(fact.GetBoolValue(fieldDescr.m_strFieldName), piFact, fieldDescr);
                break;

            default:
                break;
        }
    }

    piFact.AddAttr("FactID", iFactId);

//    if (!pFactType->m_bNoLead) {
//        //получаем id лида и строчку с аттрибутами, кторые надо подсвечивать именно для этого факта
//        yvector<CLeadGenerator::CAttrWP> attrs;
//        int iLeadID = leadGenerator.AddFact(factAddress, attrs);
//        piFact.AddAttr("LeadID", iLeadID)
//              .AddAttr("FieldsInfo", CLeadGenerator::JoinAttrs(attrs));
//    }

//    if (pText.GetAttributes().m_TitleSents.first <= factAddress.m_iSentNum &&
//        pText.GetAttributes().m_TitleSents.second >= factAddress.m_iSentNum)
//        piFact.AddAttr("tl", factAddress.m_iSentNum);

    const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
    const CWordsPair& rWP = rRusWords.GetWord((SWordHomonymNum)(factAddress)).GetSourcePair();

    if ((iFirstWord != -1) && (iLastWord != -1)) {
        const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
        const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
        iStart = w1.m_pos;
        iEnd = w2.m_pos + w2.m_len;
    }

//    piFact.AddAttr("sn", factAddress.m_iSentNum)
//          .AddAttr("fw", rWP.FirstWord())
//          .AddAttr("lw", rWP.LastWord());

    return piFact;
}

void CFactsRDFWriter::AddTextAttribute(const CTextProcessor& pText, const CTextWS* pFieldWS, const Wtroka& strVal, TXmlNodePtrBase piFact, const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr, const Stroka& strFactName, const SFactAddress& factAddress) {
    if (CheckFieldValue(pFieldWS, fieldDescr, strFactName)) {
        TXmlNodePtrBase piChild = piFact.AddNewChild(fieldDescr.m_strFieldName);

        piChild.AddNewTextNodeChild(strVal);
        piChild.AddAttr("xml:lang", IsoNameByLanguage(pText.GetLang()));

//        if (fieldDescr.m_bSaveTextInfo) //"info", написанное в '[' ']', в fact_types.cxx
//            piChild.AddAttr(g_strInfoAttr, pFieldWS->GetMainWordsDump());

//        yset<Stroka>::const_iterator it = fieldDescr.m_options.begin();
//        for (; it != fieldDescr.m_options.end(); ++it)
//            piChild.AddAttr(it->c_str(), "");

//        Wtroka sGeoArts;
//        if (GetGeoArts(pText, pFieldWS, pSent, strFactName, sGeoArts, factAddress))
//            piChild.AddAttr(g_strGeoArtAttr, sGeoArts);

//        int iFirstWord  = -1, iLastWord = -1;
//        if (GetFirstLastWord(iFirstWord, iLastWord, pFieldWS)) {
//            const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
//            const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
//            const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
//            piChild.AddAttr("pos", w1.m_pos)
//                   .AddAttr("len", w2.m_pos + w2.m_len - w1.m_pos);
//        }
    }
}

void CFactsRDFWriter::AddBoolAttribute(bool val, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr) {
    TXmlNodePtrBase piChild = piFact.AddNewChild(fieldDescr.m_strFieldName);
    piChild.AddNewTextNodeChild(val ? CharToWide("true") : CharToWide("false"));
    piChild.AddAttr("rdf:datatype", "http://www.w3.org/2001/XMLSchema#boolean");
}

void CFactsRDFWriter::SetErrorStream(TOutputStream* errorStream) {
    m_pErrorStream = errorStream;
}

void CFactsRDFWriter::PrintError(const Stroka& str) {
    (*m_pErrorStream) << str << Endl;
}
