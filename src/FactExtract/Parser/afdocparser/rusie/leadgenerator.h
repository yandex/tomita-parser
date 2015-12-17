#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/rus/sentence.h>
#include "factfields.h"
#include "dictsholder.h"
#include <FactExtract/Parser/common/libxmlutil.h>

class CTextProcessor;

class CLeadGenerator
{

public:

    struct CAttrWP: public CWordsPair
    {
        CAttrWP()
            : m_Index(0)
            , m_StartChar(0)
            , m_StopChar(0)
            , m_StartByte(0)
            , m_StopByte(0)
        {
        }

        Stroka GetAttrName() const {
            return "n" + ToString(m_Index);
        }

        static bool PtrIndexLess(const CAttrWP* a1, const CAttrWP* a2) {
            return a1->m_Index < a2->m_Index;
        }

        //имена аттрибутов для подсвечивания отрезков
        Stroka m_strTagName;
        Wtroka m_strCapitalizedName;

        size_t m_Index;
        mutable size_t m_StartChar, m_StopChar;     // Edges of span respective to m_PureText of the lead
        mutable size_t m_StartByte, m_StopByte;     // Edges of span respective to encoded m_PureText (as utf-8) of the lead

    };

    static Stroka JoinAttrs(const yvector<CAttrWP>& attrs)
    {
        Stroka res;
        if (attrs.size() > 0)
            for (size_t i = 0; i < attrs.size(); ++i)
                res += attrs[i].GetAttrName() + ";";
        return res;
    }

    //найденные в данном предложении отрезки слов из фактов для подкрашивания
    struct SSentenceLead
    {
        SSentenceLead()
            : m_iLeadID(-1)
        {
        }

        bool OutOfSent(int i) const {
            return m_LeadInfo.m_iFirstSent != -1 && m_LeadInfo.m_iLastSent != -1 &&
                   (i < m_LeadInfo.m_iFirstSent || i > m_LeadInfo.m_iLastSent);
        }

        yset<CAttrWP> m_Periods;
        int m_iLeadID;
        SLeadInfo m_LeadInfo;
        Wtroka m_strLead;   // for XML leads

        Wtroka m_PureText;  // for protobuf leads
    };

public:
    CLeadGenerator(const CTextProcessor& text, int iBaseID, const Stroka& strUrl, ECharset encoding);

    //запоминает факт и выдает вектор аттрибутов, относящихся именно к этому факту.
    //возвращает номер лида
    int AddFact(const SFactAddress& factAddress, yvector<CAttrWP>& Attrs);

    //генерит все лиды для собранных фактов
    void GenerateLeads(int baseId, const char* url);

    //выдает лиды, построенные по этому предложению
    //их может быть несколько, если поля фактов пересекались
    const yvector<CLeadGenerator::SSentenceLead>& GetLeadsBySent(int iSent);

    static size_t s_iMaxLeadSentLength;

protected:
    void GenerateHeader();
    Stroka GetHeaderTagName(const CSentence* pSent);
    Wtroka GetLeadString();
    bool AddToSentencePeriods(const CFactFields& fact, const fact_type_t&  fact_type,
                              SSentenceLead& sentencePeriods, yvector<CAttrWP>& attrInfo);
    void GenerateLead(SSentenceLead& sentenceLead, int iSent);
    TXmlNodePtr GenerateLeadSent(SSentenceLead& sentenceLead, int iSent);
    Wtroka GenerateSpannedLead(SSentenceLead& sentenceLead, int iSent);

    void GeneratePureLead(SSentenceLead& sentenceLead, TXmlNodePtrBase piSent);
    Wtroka GeneratePureLeadText(SSentenceLead& sentenceLead, bool trim);

    Stroka EncodeText(const Wtroka& text) const {
        return WideToChar(text, Encoding);
    }

    const char* EncodingName() const {
        return NameByCharset(Encoding);
    }

protected:

    ECharset Encoding;

    TXmlDocPtr m_piDoc;
    TXmlNodePtrBase m_piBody;
    TXmlNodePtrBase m_piRoot;
    TXmlNodePtrBase m_piHeader;

    //у одного предложения может быть несколько лидов, если поля фактов из него пересекаются
    yvector<yvector<SSentenceLead> > m_SentencesPeriods;
    int m_iMaxLeadID;

    const CTextProcessor& m_Text;
    Stroka m_strHeader;
    int m_iDocID;
    Stroka m_strUrl;
};
