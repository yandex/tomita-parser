#include "memoryreader.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#define TEXT_CHARSET 2

CMemoryReader::CMemoryReader()
    : m_Date(0)
    , m_currentDocumentNum(0)
{

}

CMemoryReader::~CMemoryReader(void)
{
}

void CMemoryReader::Reset()
{
    m_inputBuffer = NULL;
    m_inputBufferEnd = NULL;
    m_inputLengthsBuffer = NULL;
    m_inputLengthsBufferEnd = NULL;
    m_currentDocumentNum = 0;
}

void CMemoryReader::SetInputBuffer(char *inputBuffer) {
    m_inputBuffer = inputBuffer;
}

void CMemoryReader::SetInputBufferSize(size_t inputBufferSize) {
    m_inputBufferEnd = m_inputBuffer + inputBufferSize;
}

void CMemoryReader::SetInputLengthsBuffer(int *inputLengthsBuffer) {
    m_inputLengthsBuffer = inputLengthsBuffer;
}

void CMemoryReader::SetInputLengthsBufferSize(size_t inputLengthsBufferSize) {
    m_inputLengthsBufferEnd = m_inputLengthsBuffer + inputLengthsBufferSize;
}

bool CMemoryReader::GetNextDocInfo(Stroka& /*strFilePath*/, SDocumentAttribtes& attrs)
{
    attrs.Reset();
    if (m_inputBuffer < m_inputBufferEnd && m_inputLengthsBuffer < m_inputLengthsBufferEnd) {
        m_currentDocument = m_inputBuffer;
        m_currentDocumentLength = *m_inputLengthsBuffer;

        m_inputBuffer += m_currentDocumentLength;
        m_inputLengthsBuffer++;

        attrs.m_iDocID = m_currentDocumentNum++;

        char buf[8];
        if (snprintf(buf, sizeof(buf), "%d", attrs.m_iDocID) > 0) {
            attrs.m_strSource = buf;
            attrs.m_strUrl = buf;
        }

        if (m_Date != 0)
            attrs.m_Date = m_Date;
        else
            attrs.m_Date = time(0);

        return true;
    }

    return false;
}

bool CMemoryReader::GetDocBodyByAdress(const Stroka& /*strAddr*/, CDocBodyBase& body)
{
    return body.FillBody(*this, CODES_UTF8);
}

bool CMemoryReader::ReadInBuffer(ui8* p_buffer)
{
    memcpy(p_buffer, m_currentDocument, m_currentDocumentLength);
    return true;
}

long CMemoryReader::GetBufferLen()
{
    return m_currentDocumentLength;
}

int CMemoryReader::GetCharset()
{
    return TEXT_CHARSET;
}
