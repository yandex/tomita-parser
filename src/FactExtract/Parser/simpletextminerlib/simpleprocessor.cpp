#include "simpleprocessor.h"

#include <locale.h>

#include <util/thread/queue.h>
#include <util/thread/tasks.h>
#include <util/system/info.h>


bool CSimpleProcessor::Init(const Stroka& dicPath, docLanguage lang, const Stroka& binPath, bool needAuxKwDict,
                            ECharset langDataEncoding, bool useThreadLocalMainLanguage)
{
    setlocale(LC_ALL,"rus");
    ::InitGlobalData(Singleton<CDictsHolder>(), lang, dicPath, binPath, needAuxKwDict,
                     langDataEncoding, useThreadLocalMainLanguage);
    return true;
}

void CSimpleProcessor::Init(docLanguage lang)
{
    ErrorStream = &DefaultErrorStream;
    m_pAfDoc.Reset(new CDocIE(lang));
    m_pAfDoc->SetErrorStream(ErrorStream);
}

void CSimpleProcessor::SetParserParamsFile(const Stroka& parserParamsFile)
{
    ParserOptionsPtr.Reset(new CParserOptions());
    ParserOptionsPtr->InitFromProtobuf(parserParamsFile);
    ParserOptionsPtr->ResolveOptionArticles();
}


CSimpleProcessor::CSimpleProcessor(docLanguage lang)
{
    Init(lang);
}

CSimpleProcessor::CSimpleProcessor(docLanguage lang, const Stroka& parserParamsFile)
{
    Init(lang);
    SetParserParamsFile(parserParamsFile);
}

CSimpleProcessor::CSimpleProcessor(docLanguage lang, const TSharedPtr<CParserOptions>& parserOptionsPtr)
{
    Init(lang);
    ParserOptionsPtr = parserOptionsPtr;
}

CSimpleProcessor::~CSimpleProcessor()
{
    if (m_pAfDoc.Get() != NULL)
        m_pAfDoc->FreeData();
}

CTextProcessor* CSimpleProcessor::GetTextProcessor()
{
    return dynamic_cast<CTextProcessor*>(m_pAfDoc->GetText());
}

bool CSimpleProcessor::ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs, ETypeOfDocument doctype)
{
    if(!ParserOptionsPtr.Get())
        ythrow yexception() << "Parser-params options are not initialized.";
    return ProcessString(s, attrs, ParserOptionsPtr, doctype);
}

bool CSimpleProcessor::ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs, TSharedPtr<CParserOptions> parserOptionsPtr, ETypeOfDocument doctype)
{
    try {
        parserOptionsPtr->ResolveOptionArticles();
        m_pAfDoc->SetDocType(doctype);
        return m_pAfDoc->Proceed(s, attrs, parserOptionsPtr.Get());
    } catch (...) {
        (*ErrorStream) << "Error in \"CSimpleProcessor::ProcessString\":\n\t" << CurrentExceptionMessage() << Endl;
        return false;
    }
}

bool CSimpleProcessor::ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs,
                                     yvector<TPair<Stroka,ymap<Stroka,Wtroka> > >& out_facts,
                                     yvector<TPair<int, int> >& facts_positions,
                                     ETypeOfDocument doctype)
{
    if(!ParserOptionsPtr.Get())
        ythrow yexception() << "Parser-params options are not initialized.";
    return ProcessString(s, attrs, out_facts, facts_positions, ParserOptionsPtr.Get(), doctype);
}

bool CSimpleProcessor::ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs,
                                     yvector<TPair<Stroka,ymap<Stroka,Wtroka> > >& out_facts,
                                     yvector<TPair<int, int> >& facts_positions, TSharedPtr<CParserOptions> parserOptionsPtr,
                                     ETypeOfDocument doctype)
{
    try {
        parserOptionsPtr->ResolveOptionArticles();
        m_pAfDoc->SetDocType(doctype);
        bool bRes = m_pAfDoc->Proceed(s, attrs, parserOptionsPtr.Get());
        CTextProcessor* pText = dynamic_cast<CTextProcessor*>(m_pAfDoc->GetText());
        pText->SerializeFacts(out_facts, facts_positions);
        return bRes;
    } catch (...) {
        return false;
    }
}

bool TSimpleTextMiner::DoInput(Wtroka& text, SDocumentAttribtes& docAttr, ETypeOfDocument& docType)
{
    try {
        return Input->NextDocument(text, docAttr, docType);

    } catch (...) {
        (*ErrorStream) << Substitute("Error on document input:\n$0", CurrentExceptionMessage()) << Endl;
        return false;
    }
}

bool TSimpleTextMiner::DoProcess(Wtroka& text, SDocumentAttribtes& docAttr, ETypeOfDocument& docType)
{
    try {
        return ProcessString(text, docAttr, docType);

    } catch (...) {
        (*ErrorStream) << Substitute("Error on document \"$0\" (processing):\n$1",
            docAttr.m_strUrl, CurrentExceptionMessage()) << Endl;
        return false;
    }
}

bool TSimpleTextMiner::DoOutput(SDocumentAttribtes& docAttr)
{
    CTextProcessor* pText = GetTextProcessor();
    YASSERT(pText != NULL);

    try {
        return Output->OutputFacts(*pText);

    } catch (...) {
        (*ErrorStream) << Substitute("Error on document \"$0\" (output):\n$1",
            docAttr.m_strUrl, CurrentExceptionMessage()) << Endl;
        return false;
    }
}

bool TSimpleTextMiner::Run()
{
    if (Input == NULL || Output == NULL)
        return false;

    Wtroka text;
    SDocumentAttribtes docAttr;
    ETypeOfDocument type;

    while (DoInput(text, docAttr, type)) {
        bool res = DoProcess(text, docAttr, type);
        if (res)
            res = DoOutput(docAttr);

        if (!res && Errors != NULL && dynamic_cast<TStringStream*>(ErrorStream) != NULL) {
            TStringStream* str = static_cast<TStringStream*>(ErrorStream);
            Errors->OutputError(str->Str());
            str->clear();
        }
    }

    return true;
}

TMtpTextMiner::TMtpTextMiner(docLanguage lang, ITextMinerInput* input, ITextMinerOutput* output, TSharedPtr<CParserOptions> parserOptionsPtr, size_t threadCount)
    : ThreadCount(threadCount ? threadCount : NSystemInfo::CachedNumberOfCpus())
{
    for (size_t i = 0; i < ThreadCount; ++i) {
        Workers.push_back(new TSimpleTextMiner(lang, input, output, &Errors));
        Workers.back()->ParserOptionsPtr = parserOptionsPtr;
        WorkerPtrs.push_back(Workers.back().Get());
    }
}

//void TMtpTextMiner::Init(const Stroka& dicPath, const Stroka& parserParamsFile,
//                            docLanguage lang, ECharset langDataEncoding)
//{
//    for (size_t i = 0; i < ThreadCount; ++i) {
//        Workers[i]->Init(dicPath, parserParamsFile,lang, langDataEncoding);
//    }
//}


void TMtpTextMiner::Run() {

    TMtpQueue queue;
    queue.Start(ThreadCount, 0);
    TMtpTask proc(&queue);

    TTaskToMultiThreadProcessing<TSimpleTextMiner> task;
    proc.Process(&task, ~WorkerPtrs, +WorkerPtrs);
}
