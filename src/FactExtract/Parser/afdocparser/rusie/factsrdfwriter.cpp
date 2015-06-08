#include <util/string/cast.h>
#include <util/random/random.h>

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

Stroka CreateUid();

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

            Stroka uuid = Stroka("bnode") + CreateUid();
            piFact.AddAttr("rdf:nodeID", uuid);
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

            annRef.AddAttr("rdf:nodeID", uuid);

            annInner.AddChild(annStart.Release());
            annInner.AddChild(annEnd.Release());
            annInner.AddChild(annRef.Release());

            ann.AddChild(annInner.Release());
            annotationRoot.AddChild(ann.Release());
        }
    }

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

    const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
    const CWordsPair& rWP = rRusWords.GetWord((SWordHomonymNum)(factAddress)).GetSourcePair();

    if ((iFirstWord != -1) && (iLastWord != -1)) {
        const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
        const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
        iStart = w1.m_pos;
        iEnd = w2.m_pos + w2.m_len;
    }

    return piFact;
}

void CFactsRDFWriter::AddTextAttribute(const CTextProcessor& pText, const CTextWS* pFieldWS, const Wtroka& strVal, TXmlNodePtrBase piFact, const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr, const Stroka& strFactName, const SFactAddress& factAddress) {
    if (CheckFieldValue(pFieldWS, fieldDescr, strFactName)) {
        TXmlNodePtrBase piChild = piFact.AddNewChild(fieldDescr.m_strFieldName);

        piChild.AddNewTextNodeChild(strVal);
        piChild.AddAttr("xml:lang", IsoNameByLanguage(pText.GetLang()));
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

Stroka CreateUid() {
    Stroka s;
    for (size_t i = 0; i < 16; i++) {
        ui8 x = RandomNumber<ui8>();
        if (3 == i || 5 == i || 7 == i || 9 == i)
            fcat(s, "%02hhX-", x);
        else
            fcat(s, "%02hhX", x);
    }

    return s;
}
