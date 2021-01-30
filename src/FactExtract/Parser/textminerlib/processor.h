#pragma once

#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/docreaders/streamretrieverfacrory.h>
#include <FactExtract/Parser/common/docreaders/docstreamdisc.h>
#include <FactExtract/Parser/common/docreaders/docbody.h>

#include <FactExtract/Parser/afdocparser/afdocrusparserielib/afdocrusie.h>
#include <FactExtract/Parser/afdocparser/rusie/factsproto.h>
#include <FactExtract/Parser/afdocparser/rusie/prettysitwriter.h>

#include <FactExtract/Parser/common/commonparm.h>

#include <FactExtract/Parser/simpletextminerlib/simpleprocessor.h>

class CProcessor: private ITextMinerInput, private ITextMinerOutput
{
    friend class TMtpTextMiner;
public:
    CProcessor();
    virtual ~CProcessor() {
    }
    bool Init(int argc, char* argv[]);
    bool Run();

    void WriteInformation(const Stroka& s);
    CCommonParm m_Parm;
    TOutputStream* m_OutStream;
    CStreamRetrieverFactory Factory;

protected:

private:
    void InitOutput(const CCommonParm& params);
    void InitInterviewFile(Stroka);
    Wtroka GetInterviewFio(Stroka strUrl) const;

    // implements ITextMinerInput
    virtual bool NextDocument(Wtroka& text, SDocumentAttribtes& docAttr, ETypeOfDocument& docType);

    // implements ITextMinerOutput
    virtual bool OutputFacts(CTextProcessor& text);

    void OutputMapReduceFacts(const Stroka& url);

    void PrintSpeed(Stroka strUrl);

private:
    TSharedPtr<CParserOptions> ParserOptionsPtr;
    ymap<Stroka, Wtroka> InterviewUrl2Fio;

    THolder<TMtpTextMiner> TextMiner;

    TMutex InputLock, OutputLock;

    CDocListRetrieverBase* Retriever;
    CDocStreamBase* DocStream;
    CDocBody DocBody;

    THolder<CFactsXMLWriter> XmlWriter;
    THolder<CFactsProtoWriter> ProtoWriter;
    THolder<CAfDocPlainTextWriter> PlainTextWriter;
    THolder<CPrettySitWriter> PrettyXMLWriter;

    TBufferStream BufferStream; // helps to store output in lenval format for MapReduce
    bool MapReduceMode;

    TOutputStream* Errors;

    time_t StartTime;
    size_t DocCount, TotalVolume;
    size_t SentenceCount, TouchedSentenceCount;
};
