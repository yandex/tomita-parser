#include <util/stream/file.h>

#include <FactExtract/Parser/afdocparser/rusie/factfields.h>
#include <FactExtract/Parser/afdocparser/rusie/dictsholder.h>

#include "afdocplaintextwriter.h"

CAfDocPlainTextWriter::CAfDocPlainTextWriter(const Stroka& strFileName, ECharset encoding, const CParserOptions::COutputOptions& outputOptions)
    : m_bPrintFileDelimiter(false)
    , Encoding(encoding)
    , m_strFileName(strFileName)
    , OutputOptions(outputOptions)
    , m_bAppend(false)
{
}

void CAfDocPlainTextWriter::SetAppend(bool bAppend)
{
    m_bAppend = bAppend;
}

void CAfDocPlainTextWriter::AddDocument(CTextProcessor* pText, TOutputStream* out)
{
  for (size_t i = 0; i < pText->GetSentenceCount(); i++) {
    CSentence* pSent = pText->GetSentence(i);
    Stroka result;
    for (size_t j = 0; j < pSent->GetWordSequences().size(); j++)
        result += AddWS(pSent->GetWordSequences()[j].Get(), pSent);

    // print sentence string
    Stroka SentStr;
    for (size_t j = 0; j < pSent->getWordsCount(); ++j) {
        SentStr += EncodeText(pSent->getWord(j)->GetOriginalText());
        SentStr += ' ';
    }
    SentStr += '\n';
    out->Write(~SentStr, +SentStr);

    out->Write(~result, +result);
  }
  if (m_bPrintFileDelimiter)
      out->Write("=====\n");
}

Stroka CAfDocPlainTextWriter::AddSigns(CFioWordSequence* pFioWS, CSentence* _pSent)
{
    if (!pFioWS->m_NameMembers[Surname].IsValid())
        return Stroka();
    Stroka sRes;
    SWordHomonymNum whSurname = pFioWS->m_NameMembers[Surname];

    const CHomonym& h = _pSent->m_Words[whSurname];
    if (h.HasGrammem(gSurname))
        sRes += "p ";
    else if (h.HasAnyOfPOS(TGramBitSet(gAdjective, gAdjNumeral)))
        sRes += "a ";
    else if (h.IsDictionary()) {
        sRes += "POS:";
        TStringStream s;
        h.PrintGrammems(h.Grammems.GetPOS(), s, Encoding);
        sRes += s;
        sRes += " ";
    } else
        sRes += "n ";

    if (pFioWS->m_NameMembers[FirstName].IsValid()) {
        if (pFioWS->m_NameMembers[Surname].m_WordNum < pFioWS->m_NameMembers[FirstName].m_WordNum)
            sRes += "l ";
        else
            sRes += "r ";
    }
    return sRes;
}

Stroka CAfDocPlainTextWriter::AddWS(CWordSequence* pWS, CSentence* _pSent)
{
    CSentenceRusProcessor* pSent = dynamic_cast<CSentenceRusProcessor*>(_pSent);
    const CDictsHolder* pDict = GlobalDictsHolder;

    if (pSent == NULL)
        ythrow yexception() << "Bas subclass of CSentence in \"CAfDocPlainTextWriter::AddWS\"";

    CFactsWS* pFactsWS = dynamic_cast<CFactsWS*>(pWS);
    if (pFactsWS == NULL)
        return Stroka();

    Stroka result;
    for (int j = 0; j < pFactsWS->GetFactsCount(); j++) {
        const CFactFields& fact = pFactsWS->GetFact(j);

        if (!OutputOptions.HasFactToShow(fact.GetFactName()))
            continue;

        const fact_type_t* pFactType = &pDict->RequireFactType(fact.GetFactName());

        if (pFactType == NULL)
            ythrow yexception() << "Unknown FactType " << fact.GetFactName();

        Stroka strFact = "\t" + fact.GetFactName() +"\n\t{" + "\n";
        bool bAddFact = true;
        for (size_t i = 0; i < pFactType->m_Fields.size(); i++) {
            const fact_field_descr_t& field = pFactType->m_Fields[i];
            if (!fact.HasValue(field.m_strFieldName))
                continue;

            strFact += "\t\t";
            strFact += field.m_strFieldName;
            strFact += " = ";
            switch (field.m_Field_type) {
                case FioField: strFact += EncodeText(fact.GetFioValue(field.m_strFieldName)->GetLemma()) +"\n"; break;
                case TextField: {
                    Stroka ss = EncodeText(fact.GetTextValue(field.m_strFieldName)->GetCapitalizedLemma());
                    //если это обязательное поле со значением empty - не печатаем этот факт
                    if (field.m_bNecessary && stroka(ss) == "empty" && !OutputOptions.ShowFactsWithEmptyField())
                        bAddFact = false;
                    strFact += ss +"\n";
                    break;
                }
                case BoolField: fact.GetBoolValue(field.m_strFieldName) ? strFact += "true\n" : strFact += "false\n"; break;
                case DateField: {
                    strFact += EncodeText(fact.GetDateValue(field.m_strFieldName)->ToString()) + "\n";
                    break;
                }
                case FactFieldTypesCount:
                    /* nothing to do? */
                    break;
            }
        }

        strFact += "\t}\n";
        if (bAddFact)
            result += strFact;
    };
    return result;

}

void CAfDocPlainTextWriter::AddDocument(CTextProcessor* pDoc)
{
    if (m_strFileName == "-")
        AddDocument(pDoc, &Cout);
    else {
        ui32 mode = WrOnly;

        if (m_bAppend) {
            if (!isexist(~m_strFileName))
                ythrow yexception() << "File \"" << m_strFileName << "\" doesn't exists while Append mode is requested" << Endl;
            mode |= OpenAlways;
        }
        else
            mode |= CreateAlways;

        TFile file(m_strFileName, mode);
        file.Seek(0, sEnd);
        TOFStream out(file);
        AddDocument(pDoc, &out);
    }
}

void CAfDocPlainTextWriter::SetFileName(const Stroka& s)
{
    if (m_strFileName.empty()) {
        m_strFileName = s;
        if (m_strFileName != "-")
            try {
                TBufferedFileOutput file(m_strFileName);
            } catch (yexception& e) {
                ythrow yexception() << e.what();
            }
    }
}
