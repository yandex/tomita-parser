#include <util/string/vector.h>
#include "stdindocperlinereader.h"
#include <iostream>

using namespace std;

CStdinDocPerLineReader::CStdinDocPerLineReader(TInputStream& stream, ECharset encoding)
    : Encoding(encoding)
    , Input(stream)
    , CurrentDocNumber(0)
    , LastDocNumber(Max<size_t>())
{
    m_CurrentLineNumber = 0;
}

CStdinDocPerLineReader::~CStdinDocPerLineReader(void)
{
}

bool CStdinDocPerLineReader::GetNextDocInfo(Stroka& /*strFilePath*/, SDocumentAttribtes& attrs)
{
    static const char   kFieldSeparator[]   =   "\t";
    try {
        attrs.Reset();

        do {
            if (!Input.ReadLine(m_CurrentLine))
                return false;

            m_CurrentLineNumber++;
        } while (m_CurrentLine.empty());

        ++CurrentDocNumber;
        if (CurrentDocNumber > LastDocNumber)
            return false;

        size_t      docId   =   m_CurrentLineNumber;
        VectorStrok fields;
        SplitStroku(&fields, m_CurrentLine, kFieldSeparator);
        if (1 != fields.size()) {
            try {
                docId           =   FromString<size_t>(fields[0]);
                m_CurrentLine   =   JoinStroku( fields.begin() + 1
                                              , fields.end()
                                              , kFieldSeparator );
            }
            catch (TFromStringException e) {
                //if first is not id => do nothing
            }
        }

        attrs.m_iDocID      =   docId;
        attrs.m_strSource   =   "stdin";

        if (m_Date != 0)
            attrs.m_Date = m_Date;
        else
            attrs.m_Date = time(0);
    } catch (...) {
        cerr << "Error in \"CStdinDocPerLineReader::GetNextDocInfo\" proccessing DocN " << attrs.m_iDocID << "\n";
    }

    return true;
}

bool CStdinDocPerLineReader::GetDocBodyByAdress(const Stroka& /*strAddr*/, CDocBodyBase& body)
{
    return body.FillBody(*this, Encoding);
}

bool CStdinDocPerLineReader::ReadInBuffer(ui8* p_buffer)
{
    memcpy(p_buffer, m_CurrentLine.c_str(), m_CurrentLine.size());
    return true;
}

long CStdinDocPerLineReader::GetBufferLen()
{
    return m_CurrentLine.size();
}

int CStdinDocPerLineReader::GetCharset()
{
    return 2;
}
