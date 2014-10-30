#include "factsproto.h"

#include "textprocessor.h"

#include <kernel/gazetteer/common/serialize.h>

#include <library/protobuf/json/proto2json.h>

#include <util/string/cast.h>


CFactsProtoWriter::CFactsProtoWriter(const CParserOptions::COutputOptions& parserOutputOptions, const Stroka& output, bool append, bool writeLeads, bool writeSimilarFacts)
    : CFactsWriterBase(parserOutputOptions, writeLeads, writeSimilarFacts)
    , OutputMode(PROTOBUF)
{
    if (output == "stdout" || output == "-")
        Output = &Cout;
    else {
        TFile f(output,  WrOnly | Seq | (append ? OpenAlways | ForAppend : CreateAlways));
        OutputFile.Reset(new TOFStream(f)); // @f will be copied and owned by OutputFile
        Output = OutputFile.Get();
    }
}


void CFactsProtoWriter::WriteFoundLinks(NFactex::TDocument& doc, const CTextProcessor& pText, const SKWDumpOptions& task)
{
    const yset<SArtPointer>& mwToFind = task.MultiWordsToFind;

    yvector<SMWAddressWithLink> mwFounds;
    pText.GetFoundMWInLinks(mwFounds, mwToFind);

    for (size_t j = 0; j < mwFounds.size(); ++j) {
        SMWAddressWithLink& mw = mwFounds[j];
        const CSentenceRusProcessor& pSent = *pText.GetSentenceProcessor(mw.m_iSentNum);
        doc.AddLink(EncodeText(GlobalDictsHolder->GetArticleTitle(pSent.GetRusWords()[mw], KW_DICT)));
    }
}

void CFactsProtoWriter::AddFacts(const CTextProcessor& pText, const Stroka& url, const Stroka& siteUrl,
                                 const Stroka& strArtInLink, const SKWDumpOptions* pTask)
{
    const CFactsCollection& factsCollection = pText.GetFactsCollection();
    if (factsCollection.GetFactsCount() == 0)
        return;

    NFactex::TDocument& doc = *Documents.Add();

    FactIdMap.clear();      // facts ids are not shared between distinct documents

    doc.SetUrl(url);
    doc.SetId(pText.GetAttributes().m_iDocID);
    doc.SetTitleSentenceCount(pText.GetAttributes().m_TitleSents.second - pText.GetAttributes().m_TitleSents.first + 1);

    if (!siteUrl.empty())
        doc.SetHost(siteUrl);
    if (pTask) {
        doc.SetUrlType(strArtInLink);
        WriteFoundLinks(doc, pText, *pTask);
    }

    if (pText.GetDateFromDateField().GetYear() != 1970 ||
        pText.GetDateFromDateField().GetMonth() != 1 ||
        pText.GetDateFromDateField().GetDay() != 1)

        doc.SetDate(pText.GetDateFromDateField().Format("%Y-%m-%d"));

    CLeadGenerator leadGenerator(pText, pText.GetAttributes().m_iDocID, url, CODES_UTF8);   // leads in protobuf facts always in utf8

    WriteFacts(doc, pText, leadGenerator);

    if (IsWriteSimilarFacts)
        WriteEqualFacts(doc, pText);

    if (IsWriteLeads)
        WriteLeads(doc, pText, leadGenerator);

    // in stream-mode flush converted document to @Output
    if (Output != NULL)
        Flush(Output);
}

void CFactsProtoWriter::WriteLeads(NFactex::TDocument& doc, const CTextProcessor& pText, CLeadGenerator& leadGenerator)
{
    yvector<const CLeadGenerator::CAttrWP*> sorted;
    for (size_t i = 0; i < pText.GetSentenceCount(); ++i) {
        const yvector<CLeadGenerator::SSentenceLead>& leads = leadGenerator.GetLeadsBySent(i);
        const CWordVector& rRusWords = pText.GetSentenceProcessor(i)->GetRusWords();
        const CWord& w1 = rRusWords.GetAnyWord(0);
        const CWord& w2 = rRusWords.GetAnyWord(rRusWords.OriginalWordsCount() - 1);

        for (size_t j = 0; j < leads.size(); ++j) {
            NFactex::TLead& leadProto = *doc.AddLead();
            leadProto.SetText(EncodeText(leads[j].m_PureText));
            leadProto.SetDocId(doc.GetId());
            leadProto.SetTextPos(w1.m_pos);
            leadProto.SetTextLen(w2.m_pos + w2.m_len - w1.m_pos);

            // sort spans by index (currently they are sorted by position)
            sorted.clear();
            sorted.reserve(leads[j].m_Periods.size());
            for (yset<CLeadGenerator::CAttrWP>::const_iterator it = leads[j].m_Periods.begin(); it != leads[j].m_Periods.end(); ++it)
                sorted.push_back(&(*it));
            ::Sort(sorted.begin(), sorted.end(), CLeadGenerator::CAttrWP::PtrIndexLess);

            for (size_t k = 0; k < sorted.size(); ++k) {
                const CLeadGenerator::CAttrWP& srcSpan = *sorted[k];
                NFactex::TLeadSpan& dstSpan = *leadProto.AddSpan();
                dstSpan.SetStartChar(srcSpan.m_StartChar);
                dstSpan.SetStopChar(srcSpan.m_StopChar);
                dstSpan.SetStartByte(srcSpan.m_StartByte);
                dstSpan.SetStopByte(srcSpan.m_StopByte);
                dstSpan.SetLemma(EncodeText(srcSpan.m_strCapitalizedName));
                dstSpan.SetTypeLetter(srcSpan.m_strTagName);
            }
        }
    }
}

void CFactsProtoWriter::WriteEqualFacts(NFactex::TDocument& doc, const CTextProcessor& pText)
{
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
            WriteEqualFactsPair(doc, pText, fact_addr1, fact_addr2, t,
                                equalFacts.m_strFieldName, equalFacts.m_strFieldName);
        }
    }

    ResetSimilarFacts();
    CFactsCollection::SetOfEqualFactPairsConstIt pairIt = facts.GetEqualFactPairsItBegin();
    for (; pairIt != facts.GetEqualFactPairsItEnd(); ++pairIt) {
        const SFactAddressAndFieldName& fact1 = pairIt->GetFact1();
        const SFactAddressAndFieldName& fact2 = pairIt->GetFact2();
        WriteEqualFactsPair(doc, pText, fact1.m_FactAddr, fact2.m_FactAddr, pairIt->m_EntryType,
                            fact1.m_strFieldName, fact2.m_strFieldName);
    }
}

void CFactsProtoWriter::WriteFacts(NFactex::TDocument& doc, const CTextProcessor& pText, CLeadGenerator& leadGenerator)
{
    const CFactsCollection& factsCollection = pText.GetFactsCollection();
    CFactsCollection::SetOfFactsConstIt factsIt = factsCollection.GetFactsItBegin();
    int iFactID = 0;
    for (; factsIt != factsCollection.GetFactsItEnd(); ++factsIt) {
        const SFactAddress& factAddress = *factsIt;
        NFactex::TFact* factProto = WriteFact(doc, factAddress, pText, leadGenerator);
        //может вернуть NULL (если это был, например, временный факт, или год у даты не влезает в sql)
        if (factProto != NULL) {
            int factIndex = iFactID++;
            FactIdMap[factAddress] = factIndex;
            factProto->SetIndex(factIndex);
        }
    }
}

NFactex::TFactField& CFactsProtoWriter::AddField(NFactex::TFact& fact, const Stroka& name, const Wtroka& value)
{
    NFactex::TFactField& field = *fact.AddField();
    field.SetName(name);
    field.SetValue(EncodeText(value));
    return field;
}

void CFactsProtoWriter::AddFioAttributes(NFactex::TFact& factProto, const CFioWS* pFioWS, const fact_field_descr_t& fieldDescr,
                                         const Stroka& strFactName)
{
    if (!CheckFieldValue(pFioWS, fieldDescr, strFactName))
        return;

    Wtroka s = pFioWS->m_Fio.m_strSurname;
    TMorph::ToUpper(s);
    AddField(factProto, fieldDescr.m_strFieldName + "_Surname", s);

    if (!pFioWS->m_Fio.m_strName.empty()) {
        s = pFioWS->m_Fio.m_strName;
        TMorph::ToUpper(s);
        AddField(factProto, fieldDescr.m_strFieldName + "_FirstName", s);
    }

    if (!pFioWS->m_Fio.m_strPatronomyc.empty()) {
        s = pFioWS->m_Fio.m_strPatronomyc;
        TMorph::ToUpper(s);
        AddField(factProto, fieldDescr.m_strFieldName + "_Patronymic", s);
    }

    if (pFioWS->m_Fio.m_bFoundSurname)
        AddField(factProto, fieldDescr.m_strFieldName + "_SurnameIsDictionary", CharToWide("1"));

    if (pFioWS->m_Fio.m_bSurnameLemma)
        AddField(factProto, fieldDescr.m_strFieldName + "_SurnameIsLemma", CharToWide("1"));
}

void CFactsProtoWriter::AddTextAttribute(NFactex::TFact& factProto, const CTextProcessor& pText, const CTextWS* pTextWS,
                                         const Wtroka& strVal, const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr,
                                         const Stroka& strFactName, const SFactAddress& factAddress)
{
    if (CheckFieldValue(pTextWS, fieldDescr, strFactName)) {
        NFactex::TFactField& fieldProto = AddField(factProto, fieldDescr.m_strFieldName, strVal);
        if (fieldDescr.m_bSaveTextInfo) //"info", написанное в '[' ']', в fact_types.cxx
            fieldProto.SetInfo(EncodeText(pTextWS->GetMainWordsDump()));

        for (yset<Stroka>::const_iterator it = fieldDescr.m_options.begin(); it != fieldDescr.m_options.end(); ++it)
            fieldProto.AddOption(*it);

        Wtroka sGeoArts;
        if (GetGeoArts(pText, pTextWS, pSent, strFactName, sGeoArts, factAddress))
            fieldProto.SetGeoArts(EncodeText(sGeoArts));
    }
}

void CFactsProtoWriter::AddBoolAttribute(NFactex::TFact& factProto, bool val, const fact_field_descr_t& fieldDescr)
{
    static const Wtroka kOne = CharToWide("1");
    static const Wtroka kZero = CharToWide("0");
    AddField(factProto, fieldDescr.m_strFieldName, val ? kOne : kZero);
}

void CFactsProtoWriter::AddDateAttribute(NFactex::TFact& factProto, const CDateWS* pDateWS, const fact_field_descr_t& fieldDescr)
{
    if (pDateWS->m_SpanInterps.empty())
        return;

    Stroka strDateBegin = pDateWS->m_SpanInterps.begin()->GetSQLFormat();
    Stroka strDateEnd = (pDateWS->m_SpanInterps.size() == 1) ? pDateWS->m_SpanInterps.begin()->GetSQLFormat()
                                                             : pDateWS->m_SpanInterps.rbegin()->GetSQLFormat();
    if (!strDateBegin.empty() && !strDateEnd.empty()) {
        //так как дата у нас может быть неточной ("октябрь 2000г."), то
        //представляется она всегда в виде интервала,
        //для этого прибавляем к имени поля из fact_type.cxx "Begin" и "End"
        AddField(factProto, fieldDescr.m_strFieldName + "Begin", CharToWide(strDateBegin));
        AddField(factProto, fieldDescr.m_strFieldName + "End", CharToWide(strDateEnd));
    }
}

NFactex::TFact* CFactsProtoWriter::WriteFact(NFactex::TDocument& doc, const SFactAddress& factAddress,
                                             const CTextProcessor& pText, CLeadGenerator& leadGenerator)
{
    const CFactFields& fact  = pText.GetFact(factAddress);
    const CDictsHolder* pDictsHolder = GlobalDictsHolder;
    const fact_type_t* pFactType = &pDictsHolder->RequireFactType(fact.GetFactName());

    int iFirstWord = -1;
    int iLastWord = -1;

    if (pFactType->m_bTemporary || !ParserOutputOptions.HasFactToShow(fact.GetFactName()))
        return NULL;

    NFactex::TFactGroup* factGroup = NULL;
    for (size_t i = 0; i < doc.FactGroupSize(); ++i)
        if (doc.GetFactGroup(i).GetType() == pFactType->m_strFactTypeName) {
            factGroup = doc.MutableFactGroup(i);
            break;
        }
    if (factGroup == NULL) {
        factGroup = doc.AddFactGroup();
        factGroup->SetType(pFactType->m_strFactTypeName);
    }

    NFactex::TFact& factProto = *factGroup->AddFact();

    const CSentenceRusProcessor* pSent = pText.GetSentenceProcessor(factAddress.m_iSentNum);
    for (size_t i= 0; i < pFactType->m_Fields.size(); ++i) {
        const fact_field_descr_t& fieldDescr = pFactType->m_Fields[i];
        switch (fieldDescr.m_Field_type) {
        case FioField:
            {
                const CFioWS* fioWS = fact.GetFioValue(fieldDescr.m_strFieldName);
                if (fioWS != NULL)
                    AddFioAttributes(factProto, fioWS , fieldDescr, pFactType->m_strFactTypeName);
                UpdateFirstLastWord(iFirstWord, iLastWord, fioWS);
                break;
            }
        case TextField:
            {
                const CTextWS* textWS = fact.GetTextValue(fieldDescr.m_strFieldName);
                UpdateFirstLastWord(iFirstWord, iLastWord, textWS);
                Wtroka strVal;
                if (textWS != NULL) {
                    strVal = StripString(GetWords(*textWS));
                    //если это обязательное поле со значением empty - не печатаем этот факт
                    if (fieldDescr.m_bNecessary && NStr::EqualCiRus(strVal, "empty") &&
                        !ParserOutputOptions.ShowFactsWithEmptyField()) {
                            factGroup->MutableFact()->RemoveLast();
                            return NULL;
                    }
                }
                AddTextAttribute(factProto, pText, textWS, strVal, pSent, fieldDescr, pFactType->m_strFactTypeName, factAddress);
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
                        AddDateAttribute(factProto, dateWS, fieldDescr);
                    //else if (fieldDescr.m_bNecessary) {
                    //    factGroup->MutableFact()->RemoveLast();
                    //    return false;
                    //}
                }
                break;
            }
        case BoolField:
            AddBoolAttribute(factProto, fact.GetBoolValue(fieldDescr.m_strFieldName), fieldDescr);
            break;

        default:
            break;
        }
    }

    NFactex::TFactAttr& factAttr = *factProto.MutableAttr();

    if (!pFactType->m_bNoLead) {
        //получаем id лида и строчку с аттрибутами, кторые надо подсвечивать именно для этого факта
        yvector<CLeadGenerator::CAttrWP> attrs;
        factAttr.SetLeadIndex(leadGenerator.AddFact(factAddress, attrs));
        for (size_t a = 0; a < attrs.size(); ++a)
            factAttr.AddLeadSpanIndex(attrs[a].m_Index);
    }

    if (pText.GetAttributes().m_TitleSents.first <= factAddress.m_iSentNum &&
        pText.GetAttributes().m_TitleSents.second >= factAddress.m_iSentNum)
        factAttr.SetInTitle(true);

    const CWordVector& rRusWords = pText.GetSentenceProcessor(factAddress.m_iSentNum)->GetRusWords();
    const CWordsPair& rWP = rRusWords.GetWord(factAddress).GetSourcePair();

    if (iFirstWord != -1 && iLastWord != -1) {
        const CWord& w1 = rRusWords.GetAnyWord(iFirstWord);
        const CWord& w2 = rRusWords.GetAnyWord(iLastWord);
        factAttr.SetTextPos(w1.m_pos);
        factAttr.SetTextLen(w2.m_pos + w2.m_len - w1.m_pos);
    }

    factAttr.SetSentenceNumber(factAddress.m_iSentNum);
    factAttr.SetFirstWord(rWP.FirstWord());
    factAttr.SetLastWord(rWP.LastWord());

    return &factProto;
}

void CFactsProtoWriter::WriteEqualFactsPair(NFactex::TDocument& doc, const CTextProcessor& text,
                                            const SFactAddress& factAddr1, const SFactAddress& factAddr2,
                                            EFactEntryEqualityType equalityType, Stroka fieldName1, Stroka fieldName2) {
    const CFactFields& fact1 = text.GetFact(factAddr1);
    const CFactFields& fact2 = text.GetFact(factAddr2);
    if (ParserOutputOptions.HasToPrintFactEquality(fact1.GetFactName()) && ParserOutputOptions.HasToPrintFactEquality(fact2.GetFactName())) {
        int id1 = 0, id2 = 0;
        // это вполне штатная ситуация, когда временный факт, типа PostCompany
        // не записывается в выходной XML, а, следовательно, и в FactIdMap
        if (FindFactId(factAddr1, id1) && FindFactId(factAddr2, id2))
            if (CheckSimilarFactsLimit())   // "топор" по количеству пар
                WriteEqualFactsPair(doc, id1, id2, equalityType, fieldName1, fieldName2);
    }
}

void CFactsProtoWriter::WriteEqualFactsPair(NFactex::TDocument& doc, int iFact1, int iFact2, EFactEntryEqualityType FactEntryEqualityType,
                                            Stroka strFieldName1, Stroka strFieldName2)
{
    NFactex::TSimilarFactPair& pair = *doc.AddEqualFact();
    pair.MutableFact1()->SetFactIndex(iFact1);
    pair.MutableFact2()->SetFactIndex(iFact2);
    pair.MutableFact1()->SetSimilarAttrs(strFieldName1);
    pair.MutableFact2()->SetSimilarAttrs(strFieldName2);
    pair.SetSimilarityType(static_cast<NFactex::TSimilarFactPair::ESimilarityType>(FactEntryEqualityType));
}

void CFactsProtoWriter::Flush(TOutputStream* out)
{
    if (OutputMode == PROTOBUF)
        SerializeAsProtobuf(out);
    else
        SerializeAsJson(out);

    // print protobuf formatted facts to stdout for debugging
    if (CGramInfo::s_bDebugProtobuf)
        for (int i = 0; i < Documents.size(); ++i)
            Cout << Documents.Get(i).Utf8DebugString() << Endl;

    Documents.Clear();
}

void CFactsProtoWriter::Flush(NJson::TJsonValue& out)
{
    SerializeAsJson(out);
    Documents.Clear();
}

void CFactsProtoWriter::SerializeAsProtobuf(TOutputStream* out)
{
    ::google::protobuf::io::TCopyingOutputStreamAdaptor adaptor(out);
    ::google::protobuf::io::CodedOutputStream encoder(&adaptor);

    for (int i = 0; i < Documents.size(); ++i)
        NGzt::TMessageSerializer::Save(&encoder, Documents.Get(i));
}

void CFactsProtoWriter::SerializeAsJson(TOutputStream* out)
{
    (*out) << "[";
    NProtobufJson::TProto2JsonConfig cfg;

    for (int i = 0; i < Documents.size(); ++i) {
        if (i > 0)
            (*out) << ",";
        NProtobufJson::Proto2Json(Documents.Get(i), *out, cfg);
    }

    (*out) << "]";
}

void CFactsProtoWriter::SerializeAsJson(NJson::TJsonValue& out)
{
    for (int i = 0; i < Documents.size(); ++i) {
        NJson::TJsonValue current;
        NProtobufJson::Proto2Json(Documents.Get(i), current);
        out.AppendValue(current);
    }
}



void CFactsProtoReader::TIter::Init(const TBlob& data)
{
    Ok_ = true;
    Data = data;
    Stream.Reset(Data.AsCharPtr(), Data.Size());
    operator++();
}

void CFactsProtoReader::TIter::operator++()
{
    if (Stream.Avail())
        NGzt::TMessageSerializer::Load(&Stream, Current);
    else
        Ok_ = false;
}
