#pragma once

#include <library/json/json_value.h>
#include <FactExtract/Parser/afdocparser/common/afdocbase.h>
#include <util/system/mutex.h>
#include "factsxmlwriter.h"
#include "leadgenerator.h"
#include "factscollection.h"

#include <FactExtract/Parser/afdocparser/rusie/facts/facts.pb.h>

class CFactsProtoWriter: private CFactsWriterBase
{
public:

    // Collect-mode constructor
    CFactsProtoWriter(const CParserOptions::COutputOptions& parserOutputOptions, bool writeLeads = true, bool writeSimilarFacts = false)
        : CFactsWriterBase(parserOutputOptions, writeLeads, writeSimilarFacts)
        , Output(NULL)
        , OutputMode(PROTOBUF)
    {
    }

    // Stream-mode constructor
    CFactsProtoWriter(const CParserOptions::COutputOptions& parserOutputOptions, const Stroka& output, bool appendOutput = false,
                      bool writeLeads = true, bool writeSimilarFacts = false);

    void SetProtobufMode() {
        OutputMode = PROTOBUF;
    }

    void SetJsonMode() {
        OutputMode = JSON;      // convert to json
    }


    // Converts facts from @pText into protobuf objects and appends them to Documents.
    // In stream-mode calls Flush() and serialize Documents content to Output at once.
    // In collect-mode does not flush, so converted protobuf objects are collected in Documents
    // and could be processed or serialized manually.
    void AddFacts(const CTextProcessor& pText,
                  const Stroka& url = Stroka(), const Stroka& siteUrl = Stroka(),
                  const Stroka& strArtInLink = Stroka(), const SKWDumpOptions* pTask = NULL);

    size_t DocumentCount() const {
        return Documents.size();
    }

    const NFactex::TDocument& GetDocument(size_t index) const {
        return Documents.Get(index);
    }

    // Write accumulated content into @out as serialized proto-objects (size + objects itself),
    // or as JSON depending on OutputMode
    // Documents becomes empty. Useful only in collect-mode.
    void Flush(TOutputStream* out);
    void Flush(NJson::TJsonValue& out);

private:

    Stroka EncodeText(const Wtroka& text) const {
        return WideToUTF8(text);
    }

    void WriteEqualFactsPair(NFactex::TDocument& doc, int iFact1, int iFact2, EFactEntryEqualityType qqualityType,
                             Stroka fieldName1, Stroka fieldName2);
    void WriteEqualFactsPair(NFactex::TDocument& doc, const CTextProcessor& pText,
                             const SFactAddress& fact1, const SFactAddress& fact2,
                             EFactEntryEqualityType equalityType, Stroka fieldName1, Stroka fieldName2);

    NFactex::TFact* WriteFact(NFactex::TDocument& doc, const SFactAddress& factAddress, const CTextProcessor& pText,
                              CLeadGenerator& leadGenerator);

    void AddBoolAttribute(NFactex::TFact& factProto, bool val, const fact_field_descr_t& fieldDescr);

    void AddTextAttribute(NFactex::TFact& factProto, const CTextProcessor& pText, const CTextWS* pFioWS,
                          const Wtroka& strVal, const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr,
                          const Stroka& strFactName, const SFactAddress& factAddress);

    void AddFioAttributes(NFactex::TFact& factProto, const CFioWS* pFioWS, const fact_field_descr_t& fieldDescr,
                          const Stroka& strFactName);

    void AddDateAttribute(NFactex::TFact& factProto, const CDateWS* pDateWS, const fact_field_descr_t& fieldDescr);

    NFactex::TFactField& AddField(NFactex::TFact& fact, const Stroka& name, const Wtroka& value);

    void WriteFacts(NFactex::TDocument& doc, const CTextProcessor& pText, CLeadGenerator& leadGenerator);
    void WriteEqualFacts(NFactex::TDocument& doc, const CTextProcessor& pText);
    void WriteLeads(NFactex::TDocument& doc, const CTextProcessor& pText, CLeadGenerator& leadGenerator);
    void WriteFoundLinks(NFactex::TDocument& doc, const CTextProcessor& pText, const SKWDumpOptions& task);


    void SerializeAsProtobuf(TOutputStream* out);
    void SerializeAsJson(TOutputStream* out);
    void SerializeAsJson(NJson::TJsonValue& out);

private:
    ::google::protobuf::RepeatedPtrField<NFactex::TDocument> Documents;

    THolder<TOFStream> OutputFile;
    TOutputStream* Output;

    enum EOutputMode {
        PROTOBUF,
        JSON
    };
    EOutputMode OutputMode;
};


class CFactsProtoReader {
public:
    // Iterator over protobuf-serialized facts
    class TIter {
    public:
        TIter(TInputStream* input) {
            Init(TBlob::FromStream(*input));
        }

        TIter(const Stroka& input) {
            Init(TBlob::FromFile(input));
        }

        inline bool Ok() const {
            return Ok_;
        }

        void operator++();

        inline const NFactex::TDocument* operator->() const {
            return &Current;
        }

        inline const NFactex::TDocument& operator*() const {
            return Current;
        }

    private:
        void Init(const TBlob& data);

        bool Ok_;
        TBlob Data;
        TMemoryInput Stream;
        NFactex::TDocument Current;
    };
};
