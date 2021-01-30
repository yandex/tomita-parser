#pragma once

#include "doclistretrieverbase.h"
#include "docstreambase.h"

#include <util/generic/stroka.h>

class CMemoryReader :
    public CDocListRetrieverBase,
    public CDocStreamBase
{

public:
    CMemoryReader();
    virtual ~CMemoryReader();

public:
    void Reset();
    void SetInputBuffer(char *inputBuffer);
    void SetInputBufferSize(size_t inputBufferSize);
    void SetInputLengthsBuffer(int *inputLengthsBuffer);
    void SetInputLengthsBufferSize(size_t inputLengthsBufferSize);

public:
    /*CDocListRetrieverBase methods*/
    virtual bool GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs);

public:
    /*CDocStreamBase methods*/
    virtual bool  GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body);
    virtual bool  AddAddressHint(const Stroka& /*strAddr*/)
    {
        return false;
    };

    time_t  m_Date;

protected:
    /*CDocStreamBase methods*/
    virtual bool ReadInBuffer(ui8* p_buffer);
    virtual long GetBufferLen();
    virtual int GetCharset();

private:
    int  *m_inputLengthsBuffer;
    int  *m_inputLengthsBufferEnd;
    char *m_inputBuffer;
    char *m_inputBufferEnd;

    char *m_currentDocument;
    int  m_currentDocumentLength;
    int m_currentDocumentNum;
};
