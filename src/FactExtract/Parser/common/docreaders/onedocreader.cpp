#include <util/string/vector.h>
#include "onedocreader.h"
#include <iostream>

using namespace std;

COneDocReader::COneDocReader(TInputStream& stream, ECharset encoding)
    : Encoding(encoding)
    , file(stream)
    , m_bDone(false)
{
}

COneDocReader::~COneDocReader(void)
{
}

bool COneDocReader::GetNextDocInfo(Stroka& /*strFilePath*/, SDocumentAttribtes& attrs)
{
    if (m_bDone) return false;

    try {
        attrs.Reset();

        Stroka str;
        m_CurrentLine = file.ReadAll();

        m_bDone = true;

        attrs.m_iDocID      =   0;
        attrs.m_strSource   =   "file";

        if (m_Date != 0)
            attrs.m_Date = m_Date;
        else
            attrs.m_Date = time(0);
    } catch (...) {
        cerr << "Error in \"COneDocReader::GetNextDocInfo\" proccessing DocN " << attrs.m_iDocID << "\n";
    }

    return true;
}

bool COneDocReader::GetDocBodyByAdress(const Stroka& /*strAddr*/, CDocBodyBase& body)
{
    return body.FillBody(*this, Encoding);
}

bool COneDocReader::ReadInBuffer(ui8* p_buffer)
{
    memcpy(p_buffer, m_CurrentLine.c_str(), m_CurrentLine.size());
    return true;
}

long COneDocReader::GetBufferLen()
{
    return m_CurrentLine.size();
}

int COneDocReader::GetCharset()
{
    return 2;
}
