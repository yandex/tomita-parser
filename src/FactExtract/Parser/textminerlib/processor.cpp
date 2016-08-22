#include <locale.h>

#include "processor.h"
#include <FactExtract/Parser/common/smartfilefind.h>

#ifdef _win_
#include <fcntl.h>
#include <io.h>
#endif //_win_

CProcessor::CProcessor()
    : Retriever(NULL)
    , DocStream(NULL)
    , MapReduceMode(false)

    , Errors(&Cerr)

    , DocCount(0)
    , TotalVolume(0)
    , SentenceCount(0)
    , TouchedSentenceCount(0)
{
    ParserOptionsPtr.Reset(new CParserOptions());
}

void CProcessor::InitInterviewFile(Stroka strNameFile)
{
    if (strNameFile.empty())
        return;
    if (!PathHelper::Exists(strNameFile))
        ythrow yexception() << "Can't open name dic.";
    TBufferedFileInput file(strNameFile);

    Stroka str;
    while (file.ReadLine(str)) {
        TStringBuf line(str);
        line = StripString(line);
        if (line.empty())
            continue;

        TStringBuf url, fio;
        if (!line.SplitImpl('\t', url, fio))
            continue;

        url = StripString(url);
        fio = StripString(fio);
        InterviewUrl2Fio[ToString(url)] = CharToWide(ToString(fio), m_Parm.GetInputEncoding());
    }
}

bool CProcessor::Init(int argc, char* argv[])
{
    try {
        if (!m_Parm.AnalizeParameters(argc, argv) ||
            !m_Parm.CheckParameters() ||
            !m_Parm.InitParameters() ||
            !Factory.CreateStreamRetriever(m_Parm)) {

                WriteInformation("Bad parameters.\n" + m_Parm.m_strError);
                return false;
        }

        Retriever = &Factory.GetDocRetriever();
        DocStream = &Factory.GetDocStream();

        InitInterviewFile(m_Parm.GetInterviewUrl2FioFileName());

        ParserOptionsPtr.Reset(m_Parm.GetParserOptions());

        Stroka dicDir;
        if (!m_Parm.GetGramRoot().empty())
            dicDir = m_Parm.GetGramRoot();
        else {
            dicDir = m_Parm.GetDicDir();
            PathHelper::AppendSlash(dicDir);
        }

        Singleton<CDictsHolder>()->s_bForceRecompile = m_Parm.GetForceRecompile();
        Singleton<CDictsHolder>()->s_maxFactsCount = m_Parm.GetMaxFactsCountPerSentence();

        switch (m_Parm.GetBastardMode()) {
            case CCommonParm::EBastardMode::no:
                Singleton<CDictsHolder>()->s_BastardMode = EBastardMode::No;
                break;
      
            case CCommonParm::EBastardMode::outOfDict:
                Singleton<CDictsHolder>()->s_BastardMode = EBastardMode::OutOfDict;
                break;

            case CCommonParm::EBastardMode::always:
                Singleton<CDictsHolder>()->s_BastardMode = EBastardMode::Always;
                break;
        }

        CSimpleProcessor::Init(dicDir, m_Parm.GetLanguage(), m_Parm.GetBinaryDir(), m_Parm.NeedAuxKwDict(), m_Parm.GetLangDataEncoding());

        if (!m_Parm.GetDebugTreeFileName().empty()) {
            if ("stderr" == m_Parm.GetDebugTreeFileName())
                GetGlobalGramInfo()->s_PrintRulesStream = &Cerr;
            else if ("stdout" == m_Parm.GetDebugTreeFileName())
                GetGlobalGramInfo()->s_PrintRulesStream = &Cout;
            else {
                GetGlobalGramInfo()->s_PrintRulesStream = new TBufferedFileOutput(m_Parm.GetDebugTreeFileName());
                GetGlobalGramInfo()->s_PrintRulesStreamHolder.Reset(GetGlobalGramInfo()->s_PrintRulesStream);
            }
        }

        if (!m_Parm.GetDebugRulesFileName().empty()) {
            if ("stderr" == m_Parm.GetDebugRulesFileName())
                GetGlobalGramInfo()->s_PrintGLRLogStream = &Cerr;
            else if ("stdout" == m_Parm.GetDebugRulesFileName())
                GetGlobalGramInfo()->s_PrintGLRLogStream = &Cout;
            else {
                GetGlobalGramInfo()->s_PrintGLRLogStream = new TBufferedFileOutput(m_Parm.GetDebugRulesFileName());
            }
        }

        if (!m_Parm.GetPrettyOutputFileName().empty())
            PrettyXMLWriter.Reset(new CPrettySitWriter(m_Parm.GetOutputEncoding(), *(ParserOptionsPtr.Get())));

        TextMiner.Reset(new TMtpTextMiner(m_Parm.GetLanguage(), this, this, ParserOptionsPtr, m_Parm.GetJobCount()));
        TextMiner->SetErrorStream(Errors);

        InitOutput(m_Parm);

    } catch (...) {
        (*Errors) << "Error in CProcessor::Init: " << CurrentExceptionMessage() << Endl;
        return false;
    }

    return true;
}

void CProcessor::InitOutput(const CCommonParm& params)
{
    if ("xml" == params.GetOutputFormat()) {
        XmlWriter.Reset(new CFactsXMLWriter(ParserOptionsPtr->m_ParserOutputOptions, params.GetOutputFileName(), params.GetOutputEncoding(), "yarchive" == params.GetSourceType(), params.IsAppendFdo()));

        XmlWriter->SetErrorStream(Errors);
        XmlWriter->PutWriteLeads(m_Parm.IsWriteLeads());
        XmlWriter->PutWriteSimilarFacts(params.IsCollectEqualFacts());
    } else if ("text" == params.GetOutputFormat()) {
        PlainTextWriter.Reset(new CAfDocPlainTextWriter(params.GetOutputFileName(), params.GetOutputEncoding(), ParserOptionsPtr->m_ParserOutputOptions));
        PlainTextWriter->SetAppend(params.IsAppendFdo());
        PlainTextWriter->SetFileName(params.GetOutputFileName());
    } else if ("proto" == params.GetOutputFormat() || "json" == params.GetOutputFormat()) {
        ProtoWriter.Reset(new CFactsProtoWriter(ParserOptionsPtr->m_ParserOutputOptions, params.GetOutputFileName(), params.IsAppendFdo(), params.IsWriteLeads(), params.IsCollectEqualFacts()));   // stream mode
        if ("json" == params.GetOutputFormat())
            ProtoWriter->SetJsonMode();
    } else if ("mapreduce" == params.GetOutputFormat()) {
        #ifdef _win_
            setmode(fileno(stdout), O_BINARY);
        #endif
        XmlWriter.Reset(new CFactsXMLWriter(ParserOptionsPtr->m_ParserOutputOptions, &BufferStream, params.GetOutputEncoding(), "yarchive" == params.GetSourceType(), params.IsAppendFdo()));
    }

    if (ParserOptionsPtr->m_KWDumpOptions.m_strTextFileName.size() > 0 && "text" != params.GetOutputFormat()) {
        PlainTextWriter.Reset(new CAfDocPlainTextWriter(ParserOptionsPtr->m_KWDumpOptions.m_strTextFileName, params.GetOutputEncoding(), ParserOptionsPtr->m_ParserOutputOptions));
        PlainTextWriter->SetFileName(ParserOptionsPtr->m_KWDumpOptions.m_strTextFileName);
    }
}

void CProcessor::PrintSpeed(Stroka strUrl)
{
    if (DocCount % 50 == 0) {
        time_t curTime = time(&curTime);
        CTimeSpan cts(curTime - StartTime);
        double vm, tm;  //  Объем и время
        vm = (double) TotalVolume / 1048576.0;
        tm = (double) cts.GetTotalSeconds();
        double speed = (vm * 3600) / tm;
        Stroka intervalTime;
        intervalTime = Substitute("$0:$1:$2", cts.GetHours(), cts.GetMinutes(), cts.GetSeconds());

        double iTouchedSents = 0;
        if (SentenceCount != 0)
            iTouchedSents = 100 * TouchedSentenceCount / SentenceCount;

        Stroka suTime;
        suTime = Sprintf("Time:%s Doc:%lu Vol:%.2fMb Speed:%.0fMb/h (%s), Used sentences:%.2f%%",
            intervalTime.c_str(), DocCount, vm, speed, strUrl.c_str(), iTouchedSents);
        Clog << suTime << '\r';
    }
}

void CProcessor::WriteInformation(const Stroka& s)
{
    CTime t(time(NULL));
    Stroka msg;
    if ("yarchive" == m_Parm.GetSourceType())
        msg = Substitute("[$0] - $1  (Reloading base.)", t.Format("%d:%m:%y %H:%M:%S"), s);
    else
        msg = Substitute("[$0] - $1  (Processing files.)", t.Format("%d:%m:%y %H:%M:%S"), s);
    m_Parm.WriteToLog(msg);
}

Wtroka CProcessor::GetInterviewFio(Stroka strUrl) const
{
    ymap<Stroka, Wtroka>::const_iterator it = InterviewUrl2Fio.find(strUrl);
    if (it != InterviewUrl2Fio.end())
        return it->second;
    static const Stroka kHTTP = "http://";
    if (strUrl.has_prefix(kHTTP)) {
        strUrl = strUrl.substr(kHTTP.size());
        return GetInterviewFio(strUrl);
    }
    return Wtroka();
}

bool CProcessor::Run()
{
    try {
        StartTime = time(&StartTime);
        WriteInformation("Start.");

        TextMiner->Run();

        WriteInformation("End.");

        if (PrettyXMLWriter.Get() != NULL)
            PrettyXMLWriter->SaveToFile(m_Parm.GetPrettyOutputFileName());

        m_Parm.WriteToLog("\n");
        return true;

    } catch (...) {
        WriteInformation(Substitute("Fatal error: $0", CurrentExceptionMessage()));
        return false;
    }
}

bool CProcessor::NextDocument(Wtroka& text, SDocumentAttribtes& docAttr, ETypeOfDocument& docType) {

    TGuard<TMutex> guard(InputLock);

    if (Retriever == NULL)
        Retriever = &Factory.GetDocRetriever();

    if (DocStream == NULL)
        DocStream = &Factory.GetDocStream();

    Stroka strPath;
    while (Retriever->GetNextDocInfo(strPath, docAttr)) {
        DocBody.Reset();
        DocCount++;


        try {
            if (!DocStream->GetDocBodyByAdress(strPath, DocBody))
                continue;
        } catch (...) {
            Cerr << "Cannot read document from input: " << docAttr.DebugString() << Endl;
            continue;
        }

        TotalVolume += DocBody.GetBody().size();
        if (DocBody.GetCharset() >= 10 || !DocBody.GetCharset())
            continue;

        text = DocBody.GetBody();
        docType = (DocBody.GetCharset() == 4) ? DocumentHtml : DocumentText;

        docAttr.m_strInterviewFio = GetInterviewFio(docAttr.m_strUrl);
        return true;
    }

    return false;
}

bool CProcessor::OutputFacts(CTextProcessor& text)
{
    TGuard<TMutex> guard(OutputLock);

    SentenceCount += text.GetSentenceCount();
    TouchedSentenceCount += text.m_iTouchedSentencesCount;

    const SDocumentAttribtes& attr = text.GetAttributes();

    if (XmlWriter.Get() != NULL)
        XmlWriter->AddFacts(text, attr.m_strUrl);

    if (ProtoWriter.Get() != NULL)
        ProtoWriter->AddFacts(text, attr.m_strUrl);

    if (PlainTextWriter.Get() != NULL)
        PlainTextWriter->AddDocument(&text);

    if (MapReduceMode)
        OutputMapReduceFacts(attr.m_strUrl);

    if (PrettyXMLWriter.Get() != NULL)
        PrettyXMLWriter->AddDocument(&text, "");

    PrintSpeed(attr.m_strUrl);
    return true;
}

void CProcessor::OutputMapReduceFacts(const Stroka& url)
{
    ::Save(&Cout, url);
    if (!m_Parm.GetMapReduceSubkey().empty())
        ::Save(&Cout, m_Parm.GetMapReduceSubkey());
    TBuffer& buffer = BufferStream.Buffer();
    ::Save(&Cout, buffer); // stores size itself
    Cout.Flush();
    buffer.Clear();
}
