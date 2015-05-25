#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/docreaders/streamretrieverfacrory.h>
#include <FactExtract/Parser/common/docreaders/docstreamdisc.h>
#include <FactExtract/Parser/common/docreaders/docbody.h>

#include <FactExtract/Parser/afdocparser/afdocrusparserielib/afdocrusie.h>
#include <FactExtract/Parser/afdocparser/rusie/factsxmlwriter.h>
#include <FactExtract/Parser/afdocparser/rusie/factsrdfwriter.h>
#include <FactExtract/Parser/afdocparser/rusie/afdocplaintextwriter.h>



class CSimpleProcessor {
    friend class TMtpTextMiner;
public:
    // global data initializer
    static bool Init(const Stroka& dicPath, docLanguage lang, const Stroka& binPath = "", bool needAuxKwDict = true,
                     ECharset langDataEncoding = CODES_WIN, bool useThreadLocalMainLanguage=false);

    // The single object is not thread-safe!
    // But you can create a pool of CSimpleProcessor and use them simultaniously.
    // Just don't forget to call Init() before to initialize common dictionaries.

    CSimpleProcessor(docLanguage lang, const Stroka& parserParamsFile);
    CSimpleProcessor(docLanguage lang);
    CSimpleProcessor(docLanguage lang, const TSharedPtr<CParserOptions>& parserOptionsPtr);
    ~CSimpleProcessor();

    void SetErrorStream(TOutputStream* errors) {
        ErrorStream = errors;
        m_pAfDoc->SetErrorStream(errors);
    }

    bool ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs,
                       yvector<TPair<Stroka, ymap<Stroka, Wtroka> > >& out_facts,
                       yvector<TPair<int, int> >& facts_positions,
                       ETypeOfDocument doctype = DocumentText);
    bool ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs, ETypeOfDocument doctype = DocumentText);

    bool ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs,
                       yvector<TPair<Stroka, ymap<Stroka, Wtroka> > >& out_facts,
                       yvector<TPair<int, int> >& facts_positions, TSharedPtr<CParserOptions> ParserOptionsPtr,
                       ETypeOfDocument doctype = DocumentText);
    bool ProcessString(const Wtroka& s, const SDocumentAttribtes& attrs, TSharedPtr<CParserOptions> ParserOptionsPtr,
                       ETypeOfDocument doctype = DocumentText);

    CTextProcessor* GetTextProcessor();
    void SetParserParamsFile(const Stroka& parserParamsFile);

protected:
    THolder<CDocIE> m_pAfDoc;
    TOutputStream* ErrorStream;
    TStringStream DefaultErrorStream;
    TSharedPtr<CParserOptions> ParserOptionsPtr;
    void Init(docLanguage lang);
};


// for multi-threaded processing

class ITextMinerInput {
public:
    // must be thread-safe
    virtual bool NextDocument(Wtroka&, SDocumentAttribtes&, ETypeOfDocument&) = 0;
};

class ITextMinerOutput {
public:
    // must be thread-safe
    virtual bool OutputFacts(CTextProcessor&) = 0;
};

class ITextMinerError {
public:
    // must be thread-safe
    virtual void OutputError(const Stroka& text) = 0;
};

// CSimpleProcessor with full processing cycle: input, processing, output
class TSimpleTextMiner: public CSimpleProcessor {
public:
    TSimpleTextMiner(docLanguage lang,
                     ITextMinerInput* input,
                     ITextMinerOutput* output,
                     ITextMinerError* errors = NULL)
        : CSimpleProcessor(lang)
        , Input(input)
        , Output(output)
        , Errors(errors)
    {
    }

    bool Run();

    // for TMtpTask
    inline void ProcessTask() {
        Run();
    }

private:
    bool DoInput(Wtroka& text, SDocumentAttribtes& docAttr, ETypeOfDocument& docType);
    bool DoProcess(Wtroka& text, SDocumentAttribtes& docAttr, ETypeOfDocument& docType);
    bool DoOutput(SDocumentAttribtes& docAttr);

    ITextMinerInput* Input;
    ITextMinerOutput* Output;
    ITextMinerError* Errors;
};



class TMtpTextMiner
{
public:
    // @threadCount == 0 corresponds to number of CPUs.
    TMtpTextMiner(docLanguage lang, ITextMinerInput* input, ITextMinerOutput* output,
                    TSharedPtr<CParserOptions>  optionsPtr, size_t threadCount = 0);

    void SetErrorStream(TOutputStream* errors) {
        Errors.ErrorStream = errors;
    }

    void Run();
    void Init(const Stroka& dicPath, const Stroka& parserParamsFile,
                            docLanguage lang, ECharset langDataEncoding);


private:
    struct TErrorCollector: public ITextMinerError {
        TOutputStream* ErrorStream;
        TMutex Lock;

        TErrorCollector()
            : ErrorStream(NULL)
        {
        }

        virtual ~TErrorCollector() {
        }

        virtual void OutputError(const Stroka& text) {
            TGuard<TMutex> guard(Lock);
            if (ErrorStream != NULL)
                (*ErrorStream) << text;
        }
    };


    size_t ThreadCount;

    yvector<TSharedPtr<TSimpleTextMiner> > Workers;
    yvector<void*> WorkerPtrs;
    TErrorCollector Errors;
};
