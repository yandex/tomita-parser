#include "prettywriter.h"

#include <library/xsltransform/xsltransform.h>
#include <library/archive/yarchive.h>
#include <util/generic/singleton.h>

#include "parseroptions.h"

CPrettyWriter::CPrettyWriter(Stroka strXSL, ECharset encoding, const CParserOptions& parserOptions)
    : Encoding(encoding)
    , XmlStyleSheet(Substitute("type=\"text/xsl\" href=\"$0\"", strXSL))
    , m_iPrevSentCount(0)
    , ParserOptions(parserOptions)
{
    Reset();
}

void CPrettyWriter::Reset() {
    m_piDoc.Reset("1.0");

    m_piDoc.AddNewChildPI("xml-stylesheet", XmlStyleSheet.c_str());
    m_piAfDoc = m_piDoc.AddNewChild("af_document", "");
    m_piText = m_piAfDoc.AddNewChild("text");
}

TXmlNodePtrBase CPrettyWriter::GetFactXMLSubTree(const CFactFields& fact)
{
    ymap<Stroka, TXmlNodePtrBase>::const_iterator it = m_FactNameToXML.find(fact.GetFactName());
    if (it == m_FactNameToXML.end()) {
        TXmlNodePtrBase piTable = m_piAfDoc.AddNewChild("FactTable");
        piTable.AddAttr("name", fact.GetFactName());
        m_FactNameToXML[fact.GetFactName()] = piTable;

        const CDictsHolder* pDict = GlobalDictsHolder;

        TXmlNodePtrBase piHeader = piTable.AddNewChild("FactTableHeader");
        const fact_type_t* pFactType = &pDict->RequireFactType(fact.GetFactName());
        for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
            const fact_field_descr_t& field_descr = pFactType->m_Fields[i];
            piHeader.AddNewChild("Cell").AddAttr("val", field_descr.m_strFieldName);
        }
        return piTable;
    } else
        return it->second;
}

static const Wtroka TRUE_VALUE = Wtroka::FromAscii("true");
static const Wtroka FALSE_VALUE = Wtroka::FromAscii("false");

static Wtroka ExtractFactFieldValue(const CFactFields& fact, const fact_field_descr_t& field) {
    const Stroka& fieldName = field.m_strFieldName;
    if (field.m_Field_type == TextField) {
        const CWordSequence* pWS = dynamic_cast<const CWordSequence*>(fact.GetValue(fieldName));
        if (pWS != NULL)
            return pWS->GetCapitalizedLemma();
        else if (!field.m_StringValue.empty())
            return field.m_StringValue;
    } else if (field.m_Field_type == FioField) {
        const CWordSequence* pWS = dynamic_cast<const CWordSequence*>(fact.GetValue(fieldName));
        if (pWS != NULL)
            return pWS->GetLemma();
    } else if (field.m_Field_type == BoolField) {
        if (fact.GetBoolValue(fieldName))
            return TRUE_VALUE;
        else
            return FALSE_VALUE;
    } else if (field.m_Field_type == DateField) {
        const CDateWS* pDateWS = fact.GetDateValue(fieldName);
        if (pDateWS != NULL)
            return pDateWS->ToString();
    }
    return Wtroka();
}

void CPrettyWriter::AddFactTable(const CFactFields& fact, int iSent, Stroka strUrl)
{
    const CDictsHolder* pDict = GlobalDictsHolder;

    if (!ParserOptions.m_ParserOutputOptions.HasFactToShow(fact.GetFactName()))
        return;

    TXmlNodePtrBase piTable = GetFactXMLSubTree(fact);
    Stroka strSitname;
    const fact_type_t* pFactType = &pDict->RequireFactType(fact.GetFactName());

    TXmlNodePtr piRaw("FactRaw");

    bool bHrefAdded = false;
    for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
        TXmlNodePtr piCell("Cell");
        Wtroka strVal = ExtractFactFieldValue(fact, pFactType->m_Fields[i]);
        if (!strVal.empty()) {
            Stroka strRef;
            if (!bHrefAdded)
                strRef = Substitute("#$0", iSent + m_iPrevSentCount);
            AddCellValue(piCell, strVal, strRef);
            bHrefAdded = true;
        }
        piRaw.AddChild(piCell);
    }

    if (!strUrl.empty()) {
        TXmlNodePtr piCell("Cell");
        AddCellValue(piCell, CharToWide(strUrl), strUrl);
        piRaw.AddChild(piCell);
    }

    piTable.AddChild(piRaw.Release());
}

void CPrettyWriter::AddCellValue(TXmlNodePtrBase piCell, Wtroka strVal, Stroka strRef)
{
    SubstGlobal(strVal, ASCIIToWide("\""), ASCIIToWide(" "));
    strVal = StripString(strVal);

    piCell.AddAttr("val", strVal);
    if (!strRef.empty())
        piCell.AddAttr("href", strRef);
}

void CPrettyWriter::AddDocument(CTextProcessor* pDoc, Stroka strUrl)
{
    m_Urls.insert(strUrl);

    TXmlNodePtr piPar("paragraph");
    for (size_t i = 0; i < pDoc->GetSentenceCount(); i++) {
        AddSentence(pDoc->GetSentenceProcessor(i), piPar, i);
        if (pDoc->GetSentence(i)->m_bPBreak) {
            m_piText.AddChild(piPar.Release());
            piPar = TXmlNodePtr("paragraph");
        }
    }

    AddInfo(pDoc, strUrl);

    m_iPrevSentCount += pDoc->GetSentenceCount();
}

void CPrettyWriter::AddWS(CWordSequence* pWS, CSentence* pSent, int iSent)
{
    (void)pSent; // unused?
    if (!pWS->IsValid())
        return;
    TXmlNodePtr piWS("ws");
    piWS.AddAttr("ws_text", pWS->GetCapitalizedLemma());

    TKeyWordType type = pWS->GetKWType();
    Stroka title = WideToUTF8(pWS->GetArticleTitle());
    Stroka s;
    if (type != NULL /*None_KW*/)
        s = Substitute("$0 [$1]", type->name(), title);
    else
        s = Substitute("[$0]", title);
    piWS.AddAttr("ws_type", s);

    piWS.AddAttr("href", Substitute("#$0", iSent + m_iPrevSentCount));

    m_piFPOs.AddChild(piWS);
}

void CPrettyWriter::AddInfo(CTextProcessor* pText, Stroka strUrl)
{
    m_piFPOs = TXmlNodePtr("FPO_objects");

    for (size_t i = 0; i < pText->GetSentenceCount(); i++) {
        CSentence* pSent = pText->GetSentence(i);
        for (size_t j = 0; j < pSent->GetWordSequences().size(); j++) {
            CWordSequence* pWS = pSent->GetWordSequences()[j].Get();
            TKeyWordType type = pWS->GetKWType();
            Wtroka title = pWS->GetArticleTitle();

            if (type == NULL && title.empty())
                continue;
            if (ParserOptions.m_KWDumpOptions.HasArticleTitleOrKW(title, type))
                AddWS(pWS, pSent, i);

            CFactsWS* pFactWS = dynamic_cast<CFactsWS*>(pWS);
            if (pFactWS != NULL)
                for (int k = 0; k < pFactWS->GetFactsCount(); ++k) {
                    const CFactFields& fact = pFactWS->GetFact(k);
                    AddFactTable(fact, i, strUrl);
                }
        }
    }

    m_piAfDoc.AddChild(m_piFPOs.Release());
}

yhash<Stroka, Stroka> GetKWTypeXmlNames()
{
    yhash<Stroka, Stroka> res;
    res["abbrev"] = "CompanyAbbrev";
    res["company_descr"] = "CompanyDescr";
    res["sub_company"] = "SubCompanyDescr";
    res["company_in_quotes"] = "Company";
    res["post"] = "Post";
    res["sub_post"] = "SubPost";
    res["fio"] = "Fio";
    res["date_chain"] = "Date";
    res["date"] = "Date";
    res["geo"] = "Geo";
    res["number"] = "Number";
    return res;
}

const Stroka* StrFromKwType(TKeyWordType kwType)
{
    /*Stroka str;
    switch( kwType )
    {
        case CompanyAbbrev_KW: str =  "CompanyAbbrev"; break;
        case CompanyDescr_KW: str = "CompanyDescr"; break;
        case SubCompanyDescr_KW: str =  "SubCompanyDescr"; break;
        case CompanyInQuotes_KW: str =  "Company"; break;
        case Post_KW: str =  "Post"; break;
        case SubPost_KW: str =  "SubPost"; break;
        case Fio_KW: str =  "Fio"; break;
        case DateChain_KW: str =  "Date";        break;
        case Date_KW: str =  "Date";         break;
        case Geo_KW: str = "Geo";           break;
        case Number_KW: str = "Number";         break;
        default: break; // empty string?
    }
    return str;
    */
    if (kwType != NULL) {
        static const yhash<Stroka, Stroka> xml_names = GetKWTypeXmlNames();
        yhash<Stroka, Stroka>::const_iterator it = xml_names.find(kwType->name());
        if (it != xml_names.end())
            return &it->second;
    }
    return NULL;

}

Stroka  CPrettyWriter::GetWordStrAttr(int  iW, CSentence* pSent)
{
    const CWord& sw = *pSent->getWordRus(iW);

    for (size_t i = 0; i < pSent->GetRusWords().GetMultiWordsCount(); i++) {
        const CWord& w = pSent->GetRusWords().GetMultiWord(i);

        if (!w.GetSourcePair().Includes(sw.GetSourcePair()))
            continue;

        for (CWord::SHomIt it = w.IterHomonyms(); it.Ok(); ++it) {
            TKeyWordType kwType = GlobalDictsHolder->GetKWType(*it, KW_DICT);
            const Stroka* str = StrFromKwType(kwType);
            if (str != NULL)
                return *str;
        }
    }

    for (CWord::SHomIt it = sw.IterHomonyms(); it.Ok(); ++it) {
        TKeyWordType kwType = GlobalDictsHolder->GetKWType(*it, KW_DICT);
        const Stroka* str = StrFromKwType(kwType);
        if (str != NULL)
            return *str;
    }
    return Stroka();
}

void CPrettyWriter::AddWordAttribute(TXmlNodePtrBase piWord, int  iW, CSentence* pSent)
{
    Stroka str = GetWordStrAttr(iW, pSent);
    if (!str.empty())
        piWord.AddAttr(str.c_str(), "");
}

TXmlNodePtrBase CPrettyWriter::AddSentence(CSentenceRusProcessor* pSentProc, TXmlNodePtrBase piPar, int iSentNum)
{
    TXmlNodePtrBase piSent = piPar.AddNewChild("sent");
    TStringStream ss;
    for (size_t i = 0; i < pSentProc->getWordsCount(); i++) {
        CWord* pWord = pSentProc->getWordRus(i);
        TXmlNodePtr piWord("word");
        piWord.AddAttr("word_str", pWord->GetOriginalText());
        AddWordAttribute(piWord, i, pSentProc);

        for (CWord::SHomIt it = pWord->IterHomonyms(); it.Ok(); ++it) {
            const CHomonym& pHom = *it;
            ss.clear();
            pHom.Print(ss, CODES_UTF8);
            piWord.AddNewChild("homonym").AddAttr("hom_str", ss);
        }
        piSent.AddChild(piWord.Release());
    }
    piSent.AddAttr("href_name", ToString(iSentNum + m_iPrevSentCount));
    return piSent;
}

void CPrettyWriter::AddVisitedPagesTable()
{
    Stroka sName = "Visited pages";

    TXmlNodePtrBase piTable = m_piAfDoc.AddNewChild("FactTable");
    piTable.AddAttr("name", sName);
    m_FactNameToXML[sName] = piTable;

    yset<Stroka>::const_iterator it = m_Urls.begin();
    for (; it != m_Urls.end(); ++it) {
        Stroka ss = *it;
        TXmlNodePtr piCell("FactRawCell");
        AddCellValue(piCell, CharToWide(ss), ss);

        TXmlNodePtr piRaw("FactRaw");
        piRaw.AddChild(piCell);
        piTable.AddChild(piRaw.Release());
    }
}

static const unsigned char afDumpXslArchive[] = {
    #include "afdump_xsl.inc"
};

class TAfDumpXslReader {
public:
    TAfDumpXslReader()
        : Reader(TBlob::NoCopy(afDumpXslArchive, sizeof(afDumpXslArchive))) {
    }

    TAutoPtr<TInputStream> Get() {
        return Reader.ObjectByKey("/AfDump.xsl");
    }

private:
    TArchiveReader Reader;
};

TAutoPtr<TInputStream> GetAfDumpXsl() {
    return Singleton<TAfDumpXslReader>()->Get();
}

void CPrettyWriter::SaveToFile(const Stroka& name)
{
    TXslTransform t(*GetAfDumpXsl().Get());

    xmlChar *buff;
    int buffsize;
    xmlDocDumpFormatMemory(m_piDoc.Get(), &buff, &buffsize, 1);

    TMemoryInput i((const char*)buff, buffsize);
    if ("stdout" == name)
        t.Transform(&i, &Cout);
    else if ("stderr" == name)
        t.Transform(&i, &Cerr);
    else {
        TBufferedFileOutput o(name);
        t.Transform(&i, &o);
    }

    xmlFree(buff);
}
